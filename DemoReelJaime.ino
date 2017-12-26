#include "FastLED.h"
#include <SPI.h>

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

#define DATA_PIN    4
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
// 98 on long side
// 32 on short side
// 130 on side and a half
// 97 on second long side

#define NUM_LEDS_SIDE_1 98
#define NUM_LEDS_SIDE_2 32
#define NUM_LEDS_SIDE_3 97
#define NUM_LEDS_SIDE_4 32

#define NUM_LEDS    (98+32+97+32)
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          64
#define FRAMES_PER_SECOND  120



#include<Wire.h>
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
void setupJAccel(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  //Serial.begin(9600);
}
void JReadAccel(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void setup() {
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

   setupJAccel();
}
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { /*rainbow, rainbowWithGlitter, confetti, sinelon, */ juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void set_balance_pos(int led, int start, int width) {
  CRGB color = CRGB::Yellow; // CHSV(hue++,255,255);
    for(int i=-5;i<=5;++i) {
      int a = led + i;
      if(a < start || a >= start + width) {
         continue;
      }
      leds[a] = color;
    }

    for(int i=0;i<10;++i) {
      leds[start + i] = CRGB::Red;
      leds[start+width-1-i] = CRGB::Blue;
    }
}

void balance_loop() { 
  static uint8_t hue = 0;

  int led = 0;
  int direction = 1;
  int target = NUM_LEDS_SIDE_1 / 2;

  long center_millis = 0;
  
  while(true) {
    set_balance_pos(led,0,NUM_LEDS_SIDE_1);
    set_balance_pos(led+NUM_LEDS_SIDE_1+NUM_LEDS_SIDE_2,NUM_LEDS_SIDE_1+NUM_LEDS_SIDE_2,NUM_LEDS_SIDE_3);

    //lsm.readAccel();
    JReadAccel();
    
    target = NUM_LEDS_SIDE_1/2 + (int)((float)AcY / 10000.0f * (float)NUM_LEDS_SIDE_1);

    if(led < target) {
      direction = 1;
    } else if(led > target) {
      direction = -1;
    } else {
      direction = 0;
    }
    
    led = led + direction;
    if(led >= NUM_LEDS_SIDE_1) {
      led = NUM_LEDS_SIDE_1 - 1;
    }
    if(led < 0) {
      led = 0;
    }

    // if we are in the center
    const int center = NUM_LEDS_SIDE_1/2;
    const int center_width = 10;
    bool in_center = led > center - center_width && led < center + center_width;
    if(!in_center) {
      center_millis = 0;
    } else if(center_millis == 0) {
      center_millis = millis();
    }
    if(center_millis != 0 && millis() - center_millis > 3000) {
      pattern_loop();
      center_millis = 0;
    }
    
    FastLED.show();
    fadeall();
   // readSensor();
  }
}
  
void loop()
{
  balance_loop();
}

void pattern_loop()
{
  while(true) {  
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
  
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 
  
    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    EVERY_N_SECONDS( 10 ) { nextPattern(); break; } // change patterns periodically
  }
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

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
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
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

