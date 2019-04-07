#include <Ticker.h>
#include "FastLED.h"  // The FastLED library must be installed in addition if you got the 
                      // error message "..fatal error: FastLED.h: No such file or directory"
                      // Arduino IDE: Sketch / Include library / Manage libraries
                      //              Type "FastLED" in the "Filter your search..." field
                      //              Select the entry and click "Install"

#define BAUD_RATE 115200

// Onboard LEDs
#define ESP12LED    2 // GPIO2  - PIN17 - D4
#define NodeMCULED 16 // GPIO16 - PIN4  - D0

// SRF05
#define SRF05_TRIG_PIN 4
#define SRF05_ECHO_PIN 5

// WS2812B
#define NUM_LEDS 5
#define DATA_PIN 6    // GPIO12 - PIN6 - D6

Ticker ticker;

volatile int32_t  SRF05_triggered;
volatile int32_t  SRF05_start_ccount;
volatile uint32_t SRF05_distance_cm;
volatile uint32_t SRF05_distance_max = 300; // ignore everything more than 3m away
volatile int32_t  WS2812B_alert_cnt;

int32_t WS2812B_alert_state;

CRGB leds[NUM_LEDS];

//---------
// This function reads special 32bit register named CCOUNT that constantly counts clock ticks.
//---------
static inline int32_t asm_ccount(void) {
//---------
// ESP8266 can run both at 80Mhz and 160Mhz, with 80Mhz being default. The 80Mhz means that there
// is 80 000 000 clock ticks in second, 80 000 clock ticks on millisecond and 80 clock ticks on 
// microsecond. Everyone of those ticks increment's that CCOUNT by one.
// That 80Mhz clock also ticks at same speed despite how much load there is on system and the 80Mhz
// clock is exactly 80 million ticks on second always. Once we now know that there is 80 clock ticks
// on microsecond, that's all we neet. The easy facts are :
// * One tick is 1us / 80 = 0.0125us = 12.5ns
// * Four ticks is 4 x 12.5ns = 50ns = 0.05us
// * If CCOUNT has incremented by 80000, exactly 1 microsecond has passed.
//
// The problem with CCOUNT is that being 32-bit counter, with 80Mhz clock rate it overflows every
// 53.6 second. You need to take account that during measurement CCOUNT's value might have overflown
// and wrapped back to zero, so end value being smaller that CCOUNT you collected at beginning.
//
// See: http://sub.nanona.fi/esp8266/timing-and-ticks.html

    int32_t r;

    asm volatile ("rsr %0, ccount" : "=r"(r));
    return r;
}

//---------
// This function triggers a measurement on the HY-SRF05
//----------
void inline triggerSRF05(){
//----------
  //digitalWrite(ESP12LED,!(digitalRead(ESP12LED)));  //Toggle LED Pin
  digitalWrite(ESP12LED,LOW);
  digitalWrite(SRF05_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(SRF05_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SRF05_TRIG_PIN, LOW);
  SRF05_triggered = 1;
  digitalWrite(ESP12LED,HIGH);
}

//---------
// This function measurements echo on the HY-SRF05
//----------
void inline measureSRF05(){
  if(SRF05_triggered) {
    SRF05_start_ccount = asm_ccount();
    SRF05_triggered = 0;
    //digitalWrite(NodeMCULED,LOW);
  } else {
    int32_t etime = asm_ccount();
    int echo_ccount = ((uint32_t)(etime-SRF05_start_ccount));
    if(echo_ccount>1392000) { // 300cm  * 80 CCOUNT * 58us/cm
      SRF05_distance_cm = 0;
    } else {
      SRF05_distance_cm = ((uint32_t)(etime-SRF05_start_ccount)) / 80 / 58;
    }
    //digitalWrite(NodeMCULED,HIGH);
  }
}

//---------
// This function lights a bar in the given color.
//----------
void bar(CRGB x, int c) {
//----------
  for(int i=0;i<c;i++) {
    leds[i] = x;
  }
  for(int i=c;i<NUM_LEDS;i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

//---------
// This function is called once to initialize the program
//----------
void setup(){
//----------
  pinMode(ESP12LED, OUTPUT);
  pinMode(NodeMCULED, OUTPUT);

  digitalWrite(ESP12LED,HIGH);
  digitalWrite(NodeMCULED,HIGH);

  // Setup HY-SRF05
  pinMode(SRF05_TRIG_PIN, OUTPUT);
  pinMode(SRF05_ECHO_PIN, INPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

  Serial.begin(BAUD_RATE);

  attachInterrupt(digitalPinToInterrupt(SRF05_ECHO_PIN), measureSRF05, CHANGE);
  ticker.attach(0.1, triggerSRF05);
}

//---------
// This function contains the main loop which is executed continuously
//
// Note: CRGB::Green is Red and CRGB::Red is Green ... Don't know why.
//---------
void loop(){
//---------
  int distance_cm = SRF05_distance_cm;
  if(distance_cm) {
    //Serial.print("Nearest object within 3m: ");
    //Serial.println(SRF05_distance_cm);
    int bars;
    if(distance_cm<101) {
      bars = (100 - distance_cm) / 20;
      if(++bars > 4) { // last 20cm ... let it blink
        int32_t now = asm_ccount();
        if( ((uint32_t)(now-WS2812B_alert_cnt)) > 12000000) { // 12M CCOUNT = 150ms
          WS2812B_alert_state = !WS2812B_alert_state;
          WS2812B_alert_cnt = now;
        }
        if(WS2812B_alert_state) { bar(CRGB::Green, bars);
        } else {                  bar(CRGB::Black, 5);    }
      } else {
        bar(CRGB::Green, bars);
      }
    } else { // 100cm-300cm
      bars = (distance_cm-100)/40; // 2m/5leds=40cm
      bar(CRGB::Red, ++bars);
    }
  } else {
    bar(CRGB::Black, 5);
  }
}
