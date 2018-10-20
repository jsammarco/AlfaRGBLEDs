#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include "FastLED.h"
#include <SoftwareSerial.h>
SoftwareSerial BTserial(2, 3); // RX | TX

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    6
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    49
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  60
char c = ' ';
char r = ' ';
char mode = 'W';
char last_mode = 'W';
char MODE_WHITE = 'W';
char MODE_DEMO = 'D';
char MODE_KNIGHT = 'K';
char MODE_KNIGHT2 = '2';
char MODE_FIRE = 'F';
char HELP = 'H';

bool gReverseDirection = false;
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  25

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 30
 
// BTconnected will = false when not connected and true when connected
boolean BTconnected = false;
 
// connect the STATE pin to Arduino pin D4
const byte BTpin = 4;

void setup() {
  // set the BTpin for input
  pinMode(BTpin, INPUT);
  Serial.begin(9600);
  Serial.println("Arduino is ready");
  Serial.println("Connect the HC-05 to an Android device to continue");
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(13, OUTPUT);
  //for( int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(13, HIGH);
  //  delay(10);
  //  digitalWrite(13, LOW);
  //  delay(10);
  //}
  BTserial.begin(9600);
  delay(10);
  helpMsg();
}

void helpMsg() {
  BTserial.write("Welcome to ConsultingJoe's Alfa Romeo RGB LED System\n\n*SELECT A COMMAND*\nW = White\nD = Demo Reel\nK = Knight Rider/Cylon\nF = Fire\n\n");
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void checkNewMode() {
 //if (BTserial.available()){
      last_mode = mode;
      c = BTserial.read();
      Serial.write(c);
      if(c == MODE_WHITE){
        mode = MODE_WHITE;
        BTserial.write("White\n");
      }else if((c == MODE_KNIGHT || c == MODE_KNIGHT2) && last_mode != MODE_KNIGHT){
        mode = MODE_KNIGHT;
        BTserial.write("Knight Rider 1 (-)\n");
      }else if(c == MODE_KNIGHT && last_mode == MODE_KNIGHT){
        mode = MODE_KNIGHT2;
        BTserial.write("Knight Rider 2 (V)\n");
      }else if(c == MODE_DEMO){
        mode = MODE_DEMO;
        BTserial.write("Demo\n");
      }else if(c == MODE_FIRE){
        mode = MODE_FIRE;
        BTserial.write("Fire\n");
      }else if(c == HELP){
        helpMsg();
      }
 //} 
}

void loop()
{
  static uint8_t hue = 0;
  //if ( digitalRead(BTpin)==HIGH)  { BTconnected = true;};
  // Keep reading from the HC-05 and send to Arduino Serial Monitor
  checkNewMode();
  if(mode == MODE_DEMO){
    gPatterns[gCurrentPatternNumber]();
    checkNewMode();
    FastLED.show();
    FastLED.delay(8);
    //EVERY_N_MILLISECONDS( 3 ) {
      gHue++;
    //} // slowly cycle the "base color" through the rainbow
    EVERY_N_SECONDS( 5 ) { nextPattern(); } // change patterns periodically
  }else if(mode == MODE_WHITE){
    fill_solid(leds, NUM_LEDS, CRGB::White);
    checkNewMode();
    FastLED.show();
  }else if(mode == MODE_KNIGHT){
    knightRider();
    checkNewMode();
    FastLED.show();  
    FastLED.delay(2);
  }else if(mode == MODE_KNIGHT2){
    knightRider2();
    checkNewMode();
    FastLED.show();  
    FastLED.delay(2);
  }else if(mode == MODE_FIRE){
    checkNewMode();
    EVERY_N_MILLISECONDS( 5 ) {
      Fire2012();
    }
    FastLED.show();
  }

  // Keep reading from Arduino Serial Monitor input field and send to HC-05
//  if (Serial.available())
//  {
//      r =  Serial.read();
//      BTserial.write(r);  
//  }
  

}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void knightRider()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(26,0,NUM_LEDS);
  leds[pos] += CRGB::Red;
}

void knightRider2()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(26,0,NUM_LEDS/2+1);
  leds[pos] += CRGB::Red;
  leds[NUM_LEDS-pos-1] += CRGB::Red;
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    if (BTserial.available()){
      c = BTserial.read();
      Serial.write(c);
    }
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
    if (BTserial.available()){
      c = BTserial.read();
      Serial.write(c);
    }
  }
}

void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS/2+1; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
      leds[NUM_LEDS-pixelnumber-1] = color;
      
    }
}
