#include <FastLED.h>
typedef void (*sparkle_f_ptr)();

// Config --------------------------------------------------//
// Debugging options; comment/uncomment to test diff things //
#define DEBUG                                               //
//#define DEBUG_SPECTRUM                                    //
//#define DEBUG_TIMING                                      //
//#define DEBUG_LED_ARRAYS                                  //
                                                            //
// Timing settings                                          //
#define REFRESH_TIME 50 // 20 frames per second             //
#define SHOW_TIME 15000 // 15 seconds per show              //
                                                            //
// Node-specific                                            //
#define NODE_ID 0 // Change in code running on each node    //
//----------------------------------------------------------//

// Debugging globals -------------//
#ifdef DEBUG                      //
  word serial_val[20];            //
  unsigned long last_debug_time;  //
#endif                            //
//--------------------------------//

// Spectrum Shield ---------------------------------------------------------//
// Pin connections                                                          //
#define SS_PIN_STROBE 4                                                     //
#define SS_PIN_RESET 5                                                      //
#define SS_PIN_DC_ONE A0                                                    //
#define SS_PIN_DC_TWO A1                                                    //
#define NUM_CHANNELS 7                                                      //
                                                                            //
// Globals                                                                  //
int frequencies_one[NUM_CHANNELS];                                          //
int frequencies_two[NUM_CHANNELS];                                          //
int frequencies_avg[NUM_CHANNELS];                                          //
bool is_beat = false;                                                       //
uint8_t low_band_emphasis = 0; // 0 or 1                                    //
uint8_t mid_band_emphasis = 0; // 0, 1, or 2                                //
uint8_t high_band_emphasis = 0; // 0 or 1                                   //
//--------------------------------------------------------------------------//

// LEDs --------------------------------------------------------------//
// Physical constants                                                 //
#define NUM_NODES 6                                                   //
#define RINGS_PER_NODE 12                                             //
#define STRIPS_PER_NODE 4                                             //
#define LEDS_PER_STRIP 1260                                           //
                                                                      //
#define LEDS_PER_NODE (LEDS_PER_STRIP * STRIPS_PER_NODE)              //
#define NUM_LEDS (LEDS_PER_NODE * NUM_NODES)                          //
#define NUM_RINGS (RINGS_PER_NODE * NUM_NODES)                        //
#define STRIPS_PER_RING (STRIPS_PER_NODE / RINGS_PER_NODE)            //
#define LEDS_PER_RING (NUM_LEDS / NUM_RINGS)                          //
#define HALF_RING (LEDS_PER_RING/2)                                   //
                                                                      //
// Globals                                                            //
bool new_show_triggered;                                              //
word current_show, loop_count;                                        //
unsigned long current_time, show_saved_time, refresh_saved_time;      //
                                                                      //
// LED array and additional ways to reference it                      //
CRGB leds[NUM_RINGS][LEDS_PER_RING];                                  //
CRGBSet leds_all(*leds, NUM_LEDS);                                    //
CRGBSet leds_node(leds[NODE_ID * RINGS_PER_NODE], LEDS_PER_NODE);     //
                                                                      //
// Would love to initialize this in setup(), not sure it's possible   //
// on Arduino since objects must be initialized on declaration?       //
CRGBSet leds_2d[NUM_RINGS] = {                                        //
  CRGBSet(leds[0], LEDS_PER_RING), CRGBSet(leds[1], LEDS_PER_RING),   //
  CRGBSet(leds[2], LEDS_PER_RING), CRGBSet(leds[3], LEDS_PER_RING),   //
  CRGBSet(leds[4], LEDS_PER_RING), CRGBSet(leds[5], LEDS_PER_RING),   //
  CRGBSet(leds[6], LEDS_PER_RING), CRGBSet(leds[7], LEDS_PER_RING),   //
  CRGBSet(leds[8], LEDS_PER_RING), CRGBSet(leds[9], LEDS_PER_RING),   //
  CRGBSet(leds[10], LEDS_PER_RING), CRGBSet(leds[11], LEDS_PER_RING), //
  CRGBSet(leds[12], LEDS_PER_RING), CRGBSet(leds[13], LEDS_PER_RING), // 
  CRGBSet(leds[14], LEDS_PER_RING), CRGBSet(leds[15], LEDS_PER_RING), //
  CRGBSet(leds[16], LEDS_PER_RING), CRGBSet(leds[17], LEDS_PER_RING), // 
  CRGBSet(leds[18], LEDS_PER_RING), CRGBSet(leds[19], LEDS_PER_RING), //
  CRGBSet(leds[20], LEDS_PER_RING), CRGBSet(leds[21], LEDS_PER_RING), // 
  CRGBSet(leds[22], LEDS_PER_RING), CRGBSet(leds[23], LEDS_PER_RING), //
  CRGBSet(leds[24], LEDS_PER_RING), CRGBSet(leds[25], LEDS_PER_RING), // 
  CRGBSet(leds[26], LEDS_PER_RING), CRGBSet(leds[27], LEDS_PER_RING), //
  CRGBSet(leds[28], LEDS_PER_RING), CRGBSet(leds[29], LEDS_PER_RING), // 
  CRGBSet(leds[30], LEDS_PER_RING), CRGBSet(leds[31], LEDS_PER_RING), //
  CRGBSet(leds[32], LEDS_PER_RING), CRGBSet(leds[33], LEDS_PER_RING), // 
  CRGBSet(leds[34], LEDS_PER_RING), CRGBSet(leds[35], LEDS_PER_RING), //
  CRGBSet(leds[36], LEDS_PER_RING), CRGBSet(leds[37], LEDS_PER_RING), // 
  CRGBSet(leds[38], LEDS_PER_RING), CRGBSet(leds[39], LEDS_PER_RING), //
  CRGBSet(leds[40], LEDS_PER_RING), CRGBSet(leds[41], LEDS_PER_RING), // 
  CRGBSet(leds[42], LEDS_PER_RING), CRGBSet(leds[43], LEDS_PER_RING), //
  CRGBSet(leds[44], LEDS_PER_RING), CRGBSet(leds[45], LEDS_PER_RING), // 
  CRGBSet(leds[46], LEDS_PER_RING), CRGBSet(leds[47], LEDS_PER_RING), //
  CRGBSet(leds[48], LEDS_PER_RING), CRGBSet(leds[49], LEDS_PER_RING), // 
  CRGBSet(leds[50], LEDS_PER_RING), CRGBSet(leds[51], LEDS_PER_RING), //
  CRGBSet(leds[52], LEDS_PER_RING), CRGBSet(leds[53], LEDS_PER_RING), // 
  CRGBSet(leds[54], LEDS_PER_RING), CRGBSet(leds[55], LEDS_PER_RING), //
  CRGBSet(leds[56], LEDS_PER_RING), CRGBSet(leds[57], LEDS_PER_RING), // 
  CRGBSet(leds[58], LEDS_PER_RING), CRGBSet(leds[59], LEDS_PER_RING), //
  CRGBSet(leds[60], LEDS_PER_RING), CRGBSet(leds[61], LEDS_PER_RING), // 
  CRGBSet(leds[62], LEDS_PER_RING), CRGBSet(leds[63], LEDS_PER_RING), //
  CRGBSet(leds[64], LEDS_PER_RING), CRGBSet(leds[65], LEDS_PER_RING), // 
  CRGBSet(leds[66], LEDS_PER_RING), CRGBSet(leds[67], LEDS_PER_RING), //
  CRGBSet(leds[68], LEDS_PER_RING), CRGBSet(leds[69], LEDS_PER_RING), // 
  CRGBSet(leds[70], LEDS_PER_RING), CRGBSet(leds[71], LEDS_PER_RING), //
};                                                                    //
//--------------------------------------------------------------------//



// -------------------------- put your animation constants / macros here --------------------------------------------------

// Sparkle constants
#define MAX_SPARKLE_INTENSITY 250 // fixme: these should be color dependent to avoid color drift at high intensity
#define MIN_SPARKLE_INTENSITY 50
#define NUM_SPARKLE_FNS 10


// Diane: Trying the same with a 2D array. We'll eventually have 4 long strips, and 3 rings worth of LEDs per strip.
CRGB sparkle[NUM_RINGS][LEDS_PER_RING];


//---------------------------------------- Declare your variables here ------------------------------------------------------------


// Diane's additions
int sparkle_count = 0;
boolean is_set[STRIPS_PER_NODE][LEDS_PER_STRIP];
boolean increasing[STRIPS_PER_NODE][LEDS_PER_STRIP];

void sparkle_twinkle_init();
void sparkle_twinkle();

// array of sparkle setup functions
   sparkle_f_ptr sparkle_init[] = { sparkle_twinkle_init };
//      void (*sparkle_init_fn[NUM_SPARKLE_FNS])();
//      sparkle_init_fn[0] = sparkle_twinkle_init;
//      sparkle_init_fn[1] = sparkle_rain_init;
//    sparkle_init_fn[2] = sparkle_glitter_init;
//    sparkle_init_fn[3] = sparkle_wind_init;
//    sparkle_init_fn[4] = sparkle_wiggle_wind_init;
//    sparkle_init_fn[5] = sparkle_warp_speed_init;
//    sparkle_init_fn[6] = comet_init;
//    sparkle_init_fn[7] = sparkle_3_circle_init;
//    sparkle_init_fn[8] = sparkle_torus_knot_init;

// array of sparkle functions to make it easier to choose one randomly
   sparkle_f_ptr sparkle_fn[] = { sparkle_twinkle };
//   sparkle_fn[0] = sparkle_twinkle;
//    sparkle_fn[1] = sparkle_rain;
//    sparkle_fn[2] = sparkle_glitter;
//    sparkle_fn[3] = sparkle_wind;
//    sparkle_fn[4] = sparkle_wiggle_wind;
//    sparkle_fn[5] = sparkle_warp_speed;
//    sparkle_fn[6] = comet;
//    sparkle_fn[7] = sparkle_3_circle;
//    sparkle_fn[8] = sparkle_torus_knot();



// the setup function runs once when you press reset or power the board ------------------------------------//
void setup() {                                                                                              //
  // Setup Serial port                                                                                      //
  #ifdef DEBUG                                                                                              //
    Serial.begin(9600);                                                                                     //
    Serial.println("Serial port initialized.");                                                             //
    Serial.flush();                                                                                         //
  #endif                                                                                                    //
                                                                                                            //
  // Initialize digital pin LED_BUILTIN as an output                                                        //
  pinMode(LED_BUILTIN, OUTPUT);                                                                             //
                                                                                                            //
  // Setup LED output ports.                                                                                //
  // This needs to be declared as 8 strips even though we only use 4                                        //
  LEDS.addLeds<WS2811_PORTD, 8>(leds_node, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);                  //
                                                                                                            //
  // Set spectrum shield pin configurations                                                                 //
  pinMode(SS_PIN_STROBE, OUTPUT);                                                                           //
  pinMode(SS_PIN_RESET, OUTPUT);                                                                            //
  pinMode(SS_PIN_DC_ONE, INPUT);                                                                            //
  pinMode(SS_PIN_DC_TWO, INPUT);                                                                            //
  digitalWrite(SS_PIN_STROBE, HIGH);                                                                        //
  digitalWrite(SS_PIN_RESET, HIGH);                                                                         //
                                                                                                            //
  // Initialize spectrum analyzers                                                                          //
  digitalWrite(SS_PIN_STROBE, LOW);                                                                         //
  delay(1);                                                                                                 //
  digitalWrite(SS_PIN_RESET, HIGH);                                                                         //
  delay(1);                                                                                                 //
  digitalWrite(SS_PIN_STROBE, HIGH);                                                                        //
  delay(1);                                                                                                 //
  digitalWrite(SS_PIN_STROBE, LOW);                                                                         //
  delay(1);                                                                                                 //
  digitalWrite(SS_PIN_RESET, LOW);                                                                          //                                                                                                   //
                                                                                                            //
  //  Clear all LEDs                                                                                        //
  LEDS.clear();                                                                                             //
}                                                                                                           //
//----------------------------------------------------------------------------------------------------------//

// the loop function runs over and over again forever ------------------------------------//
void loop() {                                                                             //
  #ifdef DEBUG_LED_ARRAYS                                                                 //
    testLEDs();                                                                           //
    delay(10000);                                                                         //
    return;                                                                               //
  #endif                                                                                  //
                                                                                          //
  //  how many milliseconds have passed since DUE was started                             //
  current_time = millis();                                                                //
                                                                                          //  
  //  Reads spectrum analyzer                                                             //
  read_frequencies();                                                                     //
                                                                                          //
  //  Select animation                                                                    //
  update_current_show();                                                                  //
                                                                                          //
  //  Draw animation                                                                      //
  draw_current_show();                                                                    //
                                                                                          //
  // Write LEDs                                                                           //
  LEDS.show();                                                                            //
  #ifdef DEBUG_TIMING                                                                     //
    unsigned long now = millis();                                                         //
    serial_val[3] = now - last_debug_time;                                                //
    last_debug_time = now;                                                                //
  #endif                                                                                  //
                                                                                          //
  // Serial output for debugging                                                          //
  #ifdef DEBUG                                                                            //
    write_to_serial();                                                                    //
  #endif                                                                                  //
                                                                                          //
  loop_count++;                                                                           //
}                                                                                         //
//----------------------------------------------------------------------------------------//

// Selects the animation
void update_current_show() {
  //  If a show has played long enough randomly choose another show
  //  You can also force a show by uncommenting the last line of code 
  //  nested in this for loop.
  
  //  Use this global to: SS_PIN_RESET anything that has to do with a show.
  new_show_triggered = current_time - show_saved_time >= SHOW_TIME;
  
  if (new_show_triggered)
  {
    show_saved_time = current_time;
    loop_count = 0;
    
    //  Change the second number as more shows are added.
    current_show = random8(1,4);  // current_show is a random number from 1 to (2nd number - 1)
    //  uncomment next line if you want to force a show
    //current_show = 3;

    #ifdef DEBUG
      Serial.print("New show started: ");
      Serial.println(current_show);
    #endif
  }
  
  #ifdef DEBUG_TIMING
    unsigned long now = millis();
    serial_val[1] = now - last_debug_time;
    last_debug_time = now;
  #endif
}

// Draws the current animation
void draw_current_show() {
  //  If pixel refresh time has expired update the LED pattern
  if (current_time - refresh_saved_time > REFRESH_TIME)
  {
    refresh_saved_time = current_time;
  
    // This is where you would add a new show.
    switch (current_show)  {   
      case 1:
        run_dot();
        break;
  
      case 2:
        run_dot_green();
        break;
    
      case 3:
        pulse();
        break; 
  
      default:
        run_dot();
    }
  }

  #ifdef DEBUG_TIMING
    unsigned long now = millis();
    serial_val[2] = now - last_debug_time;
    last_debug_time = now;
  #endif
}

// For debugging only, writes all saved variables
void write_to_serial() {
  int num_serial_vals = 0;
  
  #ifdef DEBUG_SPECTRUM
    Serial.print("Spectrum:");
    num_serial_vals = 14;
  #endif
  #ifdef DEBUG_TIMING
    Serial.print("Timing:");
    num_serial_vals = 4;
  #endif
  
  for(int i = 0; i < num_serial_vals; i++) {
    Serial.print("\t");
    Serial.print(serial_val[i]);
  }

  if(num_serial_vals > 0)
  {
    Serial.println();
    Serial.flush(); // Guarantees buffer will write if future code hangs
  }
}

// Used to verify multiple views of LEDs all reference the raw leds[][] array correctly.
void testLEDs() {
  /*
    CRGB leds[NUM_RINGS][LEDS_PER_RING];                                  //
    CRGBSet leds_all(*leds, NUM_LEDS);                                    //
    CRGBSet leds_node(leds[NODE_ID * RINGS_PER_NODE], LEDS_PER_NODE);     //
    CRGBSet* leds_2d;// this might not work                               //
 */

  Serial.println("Testing LEDs");

  leds_2d[20](0, 10) = CRGB(1,2,3);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds[20][i] == CRGB(1,2,3) ? "Match1, " : "oops1, ");
  Serial.println();

  leds_all(LEDS_PER_RING, LEDS_PER_RING+10) = CRGB(4,5,6);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds[1][i] == CRGB(4,5,6) ? "Match2, " : "oops2, ");    
  Serial.println();

  leds_node(2*LEDS_PER_RING, 2*LEDS_PER_RING+10) = CRGB(7,8,9);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds[2][i] == CRGB(7,8,9) ? "Match3, " : "oops3, ");
  Serial.println();
  Serial.flush();
}
