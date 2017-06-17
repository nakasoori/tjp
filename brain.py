#!/usr/bin/python
import Queue	# thread safe
import select, socket, string, struct, sys, time

mega_to_node_map = {
    1: 1,
    2: 3,
    3: 2,
    4: 6,
    5: 5,
    6: 4,
    7: 3,
    }

# close all TCP connections and continue to run
def do_disconnect(ignored, neglected):
    global listener, sources #, writable, oops
    copy_of_sources = [] + sources	# array elements will be removed from sources itself
    for s in copy_of_sources:
        if s is not sys.stdin and s is not listener:
            disconnect(s, 'closing')
    # writable = []
    # oops = []

def do_list(ignored, neglected):
    global listener, sources
    for s in sources:
        if s is not sys.stdin:
            if s is listener:
                print 'listening on %s:%d' % s.getsockname()
            else:
                print 'connected to', remote_name[s]

def do_quit(ignored, neglected):
    global running
    running = False

# send a message to one remote or all
def do_send(socket, message):
    global message_queues, writing
    # print 'sending', repr(message)
    if socket and socket != 'send':
        list = [socket]
    else:
        list = message_queues
    for s in list:
        message_queues[s].put(message)
        if s not in writing:
            writing.append(s)

def do_show(ignored, neglected):
    # an arbitrary set of show parameters and colors
    colors = [2, 0, 4, 3]
    number_of_colors = len(colors)
    params = [3, 0, 0, number_of_colors, 1, 4, 1, 9, 2]

    do_send(None, struct.pack('>c%uB' % (9 + number_of_colors), 's', *(params + colors)))

next_timesync_sec = 0.0

# almost the same as do_send() above, but send the very latest
# timestamp to each remote or just the specified remote
def do_time(socket, neglected):
    global message_queues, next_timesync_sec, writing
    if socket == 'time':
        list = message_queues
        next_timesync_sec = time.time() + 31	# half a minute in the future
    else:
        list = [socket]
    for s in list:
        # send microseconds, which is all the precision we have
        # network byte order, a character and 64-bit unsigned integer
        message_queues[s].put(struct.pack('>cQ', 't', int(time.time() * 1000000.0)))
        if s not in writing:
            writing.append(s)

# a command and optional 8-bit unsigned integer
def do_simple(cmd, param):
    if param:
        do_send(None, struct.pack('>cB', cmd[0:1], ord(param) - ord('0')))
    else:
        do_send(None, cmd[0:1])

control_messages = {
#    'SetAllAudio':	do_unimplemented,
#    'SetAudioCh':	do_unimplemented,
#    'SetVolCh':		do_unimplemented,
#    'MuteAllAudio':	do_unimplemented,
    'SetAnimation':	(do_simple, 'program', None),
#    'SetDynAnimation':	do_unimplemented,
    'AllLEDoff':	(do_simple, 'program', '0'),
#    'CheckHandStat':	do_unimplemented,
#    'CheckAudioIn':	do_unimplemented,
    'disconnect':	(do_disconnect, None, None),
    'list':		(do_list, None, None),
   'node':		(do_simple, None, None),
    'quit':		(do_quit, None, None),
    'reconnect':	(do_simple, None, None),
    'send':		(do_send, None, None),
    'show':		(do_show, None, None),
    'time':		(do_time, None, None),
    }

# listen for TCP connections on the specified port
listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)	# disregard TIME_WAIT state
listener.setblocking(0)		# don't wait for anything
while listener:
    try:
        listener.bind(('', 3528))	# any local IPv4 address, port 3528
        break
    except:
        print sys.exc_value
        time.sleep(10)
listener.listen(6)		# maximum connection backlog

sources = [sys.stdin, listener]	# read from these
writing = []		# write to these sockets
message_queues = {}	# queues of outgoing messages
remote_name = {}	# socket remote name because getpeername() can fail

# print a message and clean up resources associated with an open TCP connection
def disconnect(socket, msg):
    print msg, remote_name[socket]
    if socket in writing:
        writing.remove(socket)
    sources.remove(socket)
    socket.close()	# shutdown() is too abrupt, do a graceful close()
    del message_queues[socket]
    del remote_name[socket]

do_list(None, None)
print sorted(control_messages.keys())
running = True
while running:
    try:
        if len(message_queues) == 0:
            timeout = None	# wait indefinitely when there are no remotes
        else:
            timeout = next_timesync_sec - time.time()
            if timeout <= 0:
                timeout = 0.01
        # could generate writing list here from nonempty message_queues
        readable, writable, oops = select.select(sources, writing, sources, timeout)

        for s in readable:
            if s is sys.stdin:
                message = s.readline().strip()
                try:
                    command, parameters = string.split(message, None, 1)	# one word separated by whitespace from the parameter(s)
                except ValueError:				# no whitespace
                    command = message
                    parameters = None
                try:
                    action = control_messages[command]
                    function = action[0]
                    if action[1]:
                        command = action[1]
                    if action[2]:
                        parameters = action[2]
                    function(command, parameters)
                except KeyError:
                    print sorted(control_messages.keys())	# unrecognized
            elif s is listener:
                remote, addr = s.accept()
                remote.setblocking(0)
                sources.append(remote)			# remember this connection
                message_queues[remote] = Queue.Queue()	# create outgoing FIFO queue
                remote_name[remote] = '%s:%d' % addr	# addr is the same as remote.getpeername()
                print 'connection from', remote_name[remote]
                do_time(remote, None);			# synchronize time immediately
            else:
                try:
                    message = s.recv(1024)
                except:
                    print sys.exc_value
                    message = None
                if message:
                    if message[0:1] == 'b':
                        if len(message) == 55:
                            do_send(None, message)	# relay to all nodes
                            print 'beat', repr(message), 'from', remote_name[s]
                    elif message[0:1] == 'm':
                        mega_number = ord(message[1:2])
                        try:
                            node_number = mega_to_node_map[mega_number]
                        except KeyError:
                            if mega_number >= 100:	# mock_mega
                                node_number = mega_number % 6 + 10
                            else:
                                node_number = None
                        print 'mega', mega_number, '( node ', repr(node_number), ') is at', remote_name[s]
                        if node_number:
                            do_send(s, struct.pack('>cB', 'n', node_number))
                    else:
                        print 'received', repr(message), 'from', remote_name[s]
                else:
                    disconnect(s, 'remote closed')

        for s in writable:
            try:
                next_msg = message_queues[s].get_nowait()
            except Queue.Empty:
                writing.remove(s)
            else:
                # print 'sending', repr(next_msg), 'to', remote_name[s]
                try:
                    sent = s.send(next_msg)
                except socket.error as err:
                    disconnect(s, err.strerror)
                else:
                    unsent = len(next_msg) - sent
                    if unsent != 0:
                        print 'failed to send %d bytes to %s' % (unsent, remote_name[s])
                        # Queue module can't push unsent data back to the front of the queue

        for s in oops:
            disconnect(s, sys.exc_value)

        if time.time() > next_timesync_sec:
            do_time('time', None)

    except KeyboardInterrupt:
        running = False

    except:
        print sys.exc_value
        do_disconnect(None, None)	# TODO: be more selective

for s in sources:
    s.close()
