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
#define ANIMATION_TIME 15000 // 15 seconds per animation    //
                                                            //
// Node-specific                                            //
#define NODE_ID 0 // Change in code running on each node    //
                                                            //
// Due controlled versus pi controlled animation choices    //
#define PI_CONTROLLED                                       //
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

// LEDs ----------------------------------------------------------------------//
// Physical constants                                                         //
#define NUM_NODES 6  // fixme: for SN RG; 6 for final structure               //
#define RINGS_PER_NODE 12                                                     //
#define STRIPS_PER_NODE 4                                                     //
#define LEDS_PER_STRIP 1260                                                   //
                                                                              //
#define LEDS_PER_NODE (LEDS_PER_STRIP * STRIPS_PER_NODE)                      //
#define NUM_LEDS (LEDS_PER_NODE * NUM_NODES)                                  //
#define NUM_RINGS (RINGS_PER_NODE * NUM_NODES)                                //
#define LEDS_PER_RING (NUM_LEDS / NUM_RINGS)                                  //
#define HALF_RING (LEDS_PER_RING/2)                                           //
                                                                              //
// Globals                                                                    //
bool new_animation_triggered;                                                 //
word current_animation, loop_count;                                           //
unsigned long current_time, animation_saved_time, refresh_saved_time;         //
                                                                              //
// LED array and additional ways to reference it                              //
CRGB leds_raw[NUM_RINGS][LEDS_PER_RING];                                      //
CRGBSet leds_all(*leds_raw, NUM_LEDS);                                        //
CRGBSet leds_node_all(leds_raw[NODE_ID * RINGS_PER_NODE], LEDS_PER_NODE);     //
                                                                              //
// Would love to initialize these elegantly, not sure it's possible           //
// on Arduino since objects must be initialized on declaration?               //
CRGBSet leds_node[RINGS_PER_NODE] = {                                         //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE], LEDS_PER_RING),                   //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+1], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+2], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+3], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+4], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+5], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+6], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+7], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+8], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+9], LEDS_PER_RING),                 //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+10], LEDS_PER_RING),                //
  CRGBSet(leds_raw[NODE_ID*RINGS_PER_NODE+11], LEDS_PER_RING)                 //
};                                                                            //
                                                                              //
CRGBSet leds[NUM_RINGS] = {                                                   //
  CRGBSet(leds_raw[0], LEDS_PER_RING), CRGBSet(leds_raw[1], LEDS_PER_RING),   //
  CRGBSet(leds_raw[2], LEDS_PER_RING), CRGBSet(leds_raw[3], LEDS_PER_RING),   //
  CRGBSet(leds_raw[4], LEDS_PER_RING), CRGBSet(leds_raw[5], LEDS_PER_RING),   //
  CRGBSet(leds_raw[6], LEDS_PER_RING), CRGBSet(leds_raw[7], LEDS_PER_RING),   //
  CRGBSet(leds_raw[8], LEDS_PER_RING), CRGBSet(leds_raw[9], LEDS_PER_RING),   //
  CRGBSet(leds_raw[10], LEDS_PER_RING), CRGBSet(leds_raw[11], LEDS_PER_RING), //
  CRGBSet(leds_raw[12], LEDS_PER_RING), CRGBSet(leds_raw[13], LEDS_PER_RING), // 
  CRGBSet(leds_raw[14], LEDS_PER_RING), CRGBSet(leds_raw[15], LEDS_PER_RING), //
  CRGBSet(leds_raw[16], LEDS_PER_RING), CRGBSet(leds_raw[17], LEDS_PER_RING), // 
  CRGBSet(leds_raw[18], LEDS_PER_RING), CRGBSet(leds_raw[19], LEDS_PER_RING), //
  CRGBSet(leds_raw[20], LEDS_PER_RING), CRGBSet(leds_raw[21], LEDS_PER_RING), // 
  CRGBSet(leds_raw[22], LEDS_PER_RING), CRGBSet(leds_raw[23], LEDS_PER_RING), //
  CRGBSet(leds_raw[24], LEDS_PER_RING), CRGBSet(leds_raw[25], LEDS_PER_RING), // 
  CRGBSet(leds_raw[26], LEDS_PER_RING), CRGBSet(leds_raw[27], LEDS_PER_RING), //
  CRGBSet(leds_raw[28], LEDS_PER_RING), CRGBSet(leds_raw[29], LEDS_PER_RING), // 
  CRGBSet(leds_raw[30], LEDS_PER_RING), CRGBSet(leds_raw[31], LEDS_PER_RING), //
  CRGBSet(leds_raw[32], LEDS_PER_RING), CRGBSet(leds_raw[33], LEDS_PER_RING), // 
  CRGBSet(leds_raw[34], LEDS_PER_RING), CRGBSet(leds_raw[35], LEDS_PER_RING), //
  CRGBSet(leds_raw[36], LEDS_PER_RING), CRGBSet(leds_raw[37], LEDS_PER_RING), // 
  CRGBSet(leds_raw[38], LEDS_PER_RING), CRGBSet(leds_raw[39], LEDS_PER_RING), //
  CRGBSet(leds_raw[40], LEDS_PER_RING), CRGBSet(leds_raw[41], LEDS_PER_RING), // 
  CRGBSet(leds_raw[42], LEDS_PER_RING), CRGBSet(leds_raw[43], LEDS_PER_RING), //
  CRGBSet(leds_raw[44], LEDS_PER_RING), CRGBSet(leds_raw[45], LEDS_PER_RING), // 
  CRGBSet(leds_raw[46], LEDS_PER_RING), CRGBSet(leds_raw[47], LEDS_PER_RING), //
  CRGBSet(leds_raw[48], LEDS_PER_RING), CRGBSet(leds_raw[49], LEDS_PER_RING), // 
  CRGBSet(leds_raw[50], LEDS_PER_RING), CRGBSet(leds_raw[51], LEDS_PER_RING), //
  CRGBSet(leds_raw[52], LEDS_PER_RING), CRGBSet(leds_raw[53], LEDS_PER_RING), // 
  CRGBSet(leds_raw[54], LEDS_PER_RING), CRGBSet(leds_raw[55], LEDS_PER_RING), //
  CRGBSet(leds_raw[56], LEDS_PER_RING), CRGBSet(leds_raw[57], LEDS_PER_RING), // 
  CRGBSet(leds_raw[58], LEDS_PER_RING), CRGBSet(leds_raw[59], LEDS_PER_RING), //
  CRGBSet(leds_raw[60], LEDS_PER_RING), CRGBSet(leds_raw[61], LEDS_PER_RING), // 
  CRGBSet(leds_raw[62], LEDS_PER_RING), CRGBSet(leds_raw[63], LEDS_PER_RING), //
  CRGBSet(leds_raw[64], LEDS_PER_RING), CRGBSet(leds_raw[65], LEDS_PER_RING), // 
  CRGBSet(leds_raw[66], LEDS_PER_RING), CRGBSet(leds_raw[67], LEDS_PER_RING), //
  CRGBSet(leds_raw[68], LEDS_PER_RING), CRGBSet(leds_raw[69], LEDS_PER_RING), // 
  CRGBSet(leds_raw[70], LEDS_PER_RING), CRGBSet(leds_raw[71], LEDS_PER_RING) //
};                                                                            //
//----------------------------------------------------------------------------//


//  Show parameters coming from the pi ----------------------------------------//
#define NUM_PARAMETERS 9                                                       //
#define NUM_COLORS_PER_PALETTE 7                                               //
                                                                               //
//  Indices into show_parameters[] which holds information from the pi         //
#define ANIMATION_INDEX 0   // which animation to play                         //
#define BEAT_EFFECT_INDEX 1   // how to respond to beat                        //
#define PALETTE_INDEX 2   // which color palette to use                        //
#define NUM_COLORS_INDEX 3   // how many colors to use out of this palette     //
#define COLOR_THICKNESS_INDEX 4   // how many consecutive lit LEDs in a row    //
#define BLACK_THICKNESS_INDEX 5   // how many dark LEDs between lit ones       //
#define INTRA_RING_MOTION_INDEX 6   // 0 none, 1 CW, 2 CCW, 3 split            //
#define INTRA_RING_SPEED_INDEX 7   // fixme: still need to decide on units     //
#define COLOR_CHANGE_STYLE_INDEX 8                                             //
                // 0 none, 1 cycle thru selected, 2 cycle thru palette         //
                                                                               //
//  Evolving parameters defining the show                                      //
int show_parameters[NUM_PARAMETERS];                                           //
int show_colors[NUM_COLORS_PER_PALETTE];                                       //
//-----------------------------------------------------------------------------//


// Color palette choices ------------------------------------------------------//
// Eventually this may be stored in the database if space issues arise
const CRGB icy_bright[NUM_COLORS_PER_PALETTE] = 
    {CRGB(255,255,255), CRGB(254,207,241),                // light
    CRGB(255,108,189), CRGB(0,172,238), CRGB(44,133,215),    //  medium
    CRGB(114,78,184), CRGB(227,0,141)};                   // dark

const CRGB watermelon[NUM_COLORS_PER_PALETTE] = 
    {CRGB(47,192,9), CRGB(70,190,31),                     // light
    CRGB(47,192,9), CRGB(72,160,50), CRGB(148,33,137),    //  medium
    CRGB(120,86,103), CRGB(14,139,0)};                 // dark

const CRGB fruit_loop[NUM_COLORS_PER_PALETTE] = 
{CRGB(255,247,0), CRGB(255,127,14),                   // light
    CRGB(188,0,208), CRGB(255,65,65), CRGB(255,73,0),     //  medium
    CRGB(178,6,88), CRGB(162,80,204)};                // dark
    
// const *CRGB palette[3] = { icy_bright, watermelon, fruit_loop};

                                                                              //
//-----------------------------------------------------------------------------//


//  Sparkle layer -------------------------------------------------------------//
#define MAX_SPARKLE_INTENSITY 250 // fixme: these should be color dependent to avoid color drift at high intensity
#define MIN_SPARKLE_INTENSITY 50
#define NUM_SPARKLE_FNS 10


//  Sparkle layer variables
int sparkle_count = 0;
CRGB sparkle[NUM_RINGS][LEDS_PER_RING];  // Sparkle LED layer as a 2D array.
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
//    sparkle_fn[8] = sparkle_torus_knot;



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
  LEDS.addLeds<WS2811_PORTD, 8>(leds_node_all, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);              //
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
  #ifdef PI_CONTROLLED  //  Get updated parameters from the pi                            // 
    // fixme Jeff: this is where the parameters from the pi should be read in
    update_parameters();                                                                  //
  #else                                                                                   //
    update_current_animation();  //  Select animation                                     //                    //
  #endif                                                                                  //
                                                                                          //
  //  Draw animation                                                                      //
  draw_current_animation();                                                               //
                                                                                          //
  //  Create sparkle layer (phase 2)                                                      //
  // sparkle_fn[show_parameters[SPARKLE_INDEX]];                                          //
                                                                                          //
  //  Overlay sparkle layer atop background (phase 2)                                     //
  // overlay();                                                                           // 
                                                                                          //                                                                           //
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

// Updates show parameters coming from the pi
void update_parameters() {
    // Will update arrays show_parameters[] and show_colors[]
    // I don't know how this will work yet - have to learn from Jeff how these will be passed
}
 
// Selects the animation
void update_current_animation() {
  //  If an animation has played long enough randomly choose another animation. 

  //  You can also force an animation by uncommenting the last line of code 
  //  nested in this for loop.
  
  //  Use this global to: SS_PIN_RESET anything that has to do with an animation.
  new_animation_triggered = current_time - animation_saved_time >= ANIMATION_TIME;
  
  if (new_animation_triggered)
  {
    animation_saved_time = current_time;
    loop_count = 0;
    
    //  Change the second number as more shows are added.
    current_animation = random8(1,4);  // current_animation is a random number from 1 to (2nd number - 1)
    //  uncomment next line if you want to force an animation
    // current_animation = 3;

    #ifdef DEBUG
      Serial.print("New show started: ");
      Serial.println(current_animation);
    #endif
  }
  
  #ifdef DEBUG_TIMING
    unsigned long now = millis();
    serial_val[1] = now - last_debug_time;
    last_debug_time = now;
  #endif
}

// Draws the current animation
void draw_current_animation() {
  //  If pixel refresh time has expired update the LED pattern
  if (current_time - refresh_saved_time > REFRESH_TIME)
  {
    refresh_saved_time = current_time;
  
    // This is where you would add a new animation.
    switch (current_animation)  {   
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

// Used to verify multiple views of LEDs all reference the raw leds_raw[][] array correctly.
void testLEDs() {
  /*
    CRGB leds_raw[NUM_RINGS][LEDS_PER_RING];                                  //
    CRGBSet leds_all(*leds, NUM_LEDS);                                    //
    CRGBSet leds_node(leds_raw[NODE_ID * RINGS_PER_NODE], LEDS_PER_NODE);     //
    CRGBSet* leds;// this might not work                               //
 */

  Serial.println("Testing LEDs");

  leds[20](0, 10) = CRGB(1,2,3);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds_raw[20][i] == CRGB(1,2,3) ? "Match1, " : "oops1, ");
  Serial.println();

  leds_all(LEDS_PER_RING, LEDS_PER_RING+10) = CRGB(4,5,6);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds_raw[1][i] == CRGB(4,5,6) ? "Match2, " : "oops2, ");    
  Serial.println();

  leds_node[2](0, 10) = CRGB(7,8,9);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds_raw[NODE_ID*RINGS_PER_NODE+2][i] == CRGB(7,8,9) ? "Match3, " : "oops3, ");
  Serial.println();

  leds_node_all(3*LEDS_PER_RING, 3*LEDS_PER_RING+10) = CRGB(2,3,4);
  for(int i = 0; i <= 10; i++)
    Serial.print(leds_raw[NODE_ID*RINGS_PER_NODE+3][i] == CRGB(2,3,4) ? "Match4, " : "oops4, ");
  Serial.println();

  Serial.flush();
}

