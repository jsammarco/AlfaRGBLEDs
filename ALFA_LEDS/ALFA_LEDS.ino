/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN    23
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    49
CRGB leds[NUM_LEDS];

//#define BRIGHTNESS          96
int brightness = 100;
#define FRAMES_PER_SECOND  60

bool gReverseDirection = false;
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  25

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 30

char mode = 'W';
char last_mode = 'W';
char MODE_WHITE = 'W';
char MODE_DEMO = 'D';
char MODE_KNIGHT = 'K';
char MODE_KNIGHT2 = '2';
char MODE_FIRE = 'F';
char HELP = 'H';

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


void respond(std::string myVal) {

    if(myVal.at(0) == 'B' && myVal.length() > 1 && myVal.length() < 8){
      String myBrightness = myVal.c_str();
      myBrightness.trim();
      myBrightness.remove(0,1);
      brightness = myBrightness.toInt();
      brightness = max(0, min(100, brightness));
      String res = "Brightness: " + String(brightness) + "%\n";
      Serial.print(res);
      pTxCharacteristic->setValue(res.c_str());
      pTxCharacteristic->notify();
      FastLED.setBrightness(brightness);
    }else if(myVal[0] == MODE_WHITE){
      mode = MODE_WHITE;
      pTxCharacteristic->setValue("White\n");
      pTxCharacteristic->notify();
    }else if(myVal[0] == MODE_KNIGHT){
      mode = MODE_KNIGHT;
      pTxCharacteristic->setValue("Knight Rider 1 (-)\n");
      pTxCharacteristic->notify();
    }else if(myVal[0] == MODE_KNIGHT2){
      mode = MODE_KNIGHT2;
      pTxCharacteristic->setValue("Knight Rider 2 (V)\n");
      pTxCharacteristic->notify();
    }else if(myVal[0] == MODE_DEMO){
      mode = MODE_DEMO;
      pTxCharacteristic->setValue("Demo\n");
      pTxCharacteristic->notify();
    }else if(myVal[0] == MODE_FIRE){
      mode = MODE_FIRE;
      pTxCharacteristic->setValue("Fire\n");
      pTxCharacteristic->notify();
    }else if(myVal[0] == HELP){
      helpMsg();
    }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
        if (deviceConnected) {
          respond(rxValue);
        }
      }
    }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Welcome to AlfaLEDs");
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  FastLED.setBrightness(brightness);
  
  // Create the BLE Device
  BLEDevice::init("AlfaLEDs");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(pService->getUUID());
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };


void loop() {

  if (deviceConnected) {
//    pTxCharacteristic->setValue(".");
//    pTxCharacteristic->notify();
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

    if(mode == MODE_DEMO){
      gPatterns[gCurrentPatternNumber]();
      FastLED.show();
      delay(8); //FastLED.delay(8);
      //EVERY_N_MILLISECONDS( 3 ) {
        gHue++;
      //} // slowly cycle the "base color" through the rainbow
      EVERY_N_SECONDS( 5 ) { nextPattern(); } // change patterns periodically
    }else if(mode == MODE_WHITE){
      fill_solid(leds, NUM_LEDS, CRGB::White);
      FastLED.show();
    }else if(mode == MODE_KNIGHT){
      knightRider();
      FastLED.show();  
      delay(2); //FastLED.delay(2);
    }else if(mode == MODE_KNIGHT2){
      knightRider2();
      FastLED.show();  
      delay(2); //FastLED.delay(2);
    }else if(mode == MODE_FIRE){
      //EVERY_N_MILLISECONDS( 5 ) {
        Fire2012();
      //}
      FastLED.show();
    }
}

void helpMsg() {  
  pTxCharacteristic->setValue("Welcome to ConsultingJoe's Alfa Romeo RGB LED System\n\n*SELECT A COMMAND*\nW = White\nD = Demo Reel\nK = Knight Rider\n2 = Knight Rider (V)/Cylon\nF = Fire\n\n");
  pTxCharacteristic->notify();
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
  int pos = beatsin16(26,0,NUM_LEDS-1);
  leds[pos] += CRGB::Red;
}

void knightRider2()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(26,0,NUM_LEDS/2);
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
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+6,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 1));
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
    for( int j = 0; j < NUM_LEDS/2; j++) {
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