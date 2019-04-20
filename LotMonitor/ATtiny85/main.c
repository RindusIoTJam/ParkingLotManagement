#ifndef F_CPU
#define F_CPU 16000000UL  // 16MHz for the Attiny85
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812.h"  // https://github.com/cpldcpu/light_ws2812
#include "main.h"

#define ONBOARD_LED   PB1
#define SRF05_1WIRE   PB3
//      WS2819_DATA   PB4 // See Makefile WS2812_PIN

#define INTERRUPT_DIV 10

#define WS2819_STRIPE_LEN 5
//#define WS2819_BRIGHTNESS 25  // percent [optional, comment for full brightness]

struct cRGB led[WS2819_STRIPE_LEN+1];      // one more to prevent array overflow

volatile int32_t ticks = 0;                // every 58 usec
volatile int32_t SRF05_trigger_tick = 0;
volatile int32_t SRF05_echo_tick = 0;
volatile int16_t SRF05_echo_length = 0;
volatile int32_t last_25cm_flicker = 0;

//---------
// This function lights a bar in the given RGB and length.
//----------
void inline setLEDS(uint8_t r, uint8_t g, uint8_t b, uint8_t c) {
//----------
#ifdef WS2819_BRIGHTNESS
  uint8_t factor = 100/WS2819_BRIGHTNESS;
#endif
  for(int i=0;i<c;i++) {
#ifndef WS2819_BRIGHTNESS
    led[i].r=r;led[i].g=g;led[i].b=b;
#endif
#ifdef WS2819_BRIGHTNESS
    led[i].r=r/factor;led[i].g=g/factor;led[i].b=b/factor;
#endif
  }
  for(int i=c;i<WS2819_STRIPE_LEN;i++) {
    led[i].r=0;led[i].g=0;led[i].b=0;
  }
  ws2812_setleds(led,WS2819_STRIPE_LEN);
}

//---------
// This function turns all LEDs off
//----------
void allOFF() {
//----------
  allColor(color_black);
}

//---------
// This function lights all LEDs to a given color
//----------
void inline allColor(struct cRGB color) {
//----------
  setLEDS(color.r,color.g,color.b, WS2819_STRIPE_LEN);
}

//---------
// This function lights a bar in the given color and length
//----------
void inline barColor(struct cRGB color, int bars) {
//----------
  setLEDS(color.r,color.g,color.b, bars);
}

//---------
// This function creates a beginning of the SRF05 trigger pulse every
// 100 milliseconds and toggles the flicker-flag for the <26cm alarm
//----------
ISR(TIMER1_COMPA_vect) {
//----------
  GIMSK &= ~(1 << PCIE);                   // General Interrupt Mask Register,
                                           //   Pin Change Interrupt DISable
  DDRB  |= (1 << SRF05_1WIRE);             // set data direction register to
                                           //   OUT for HY-SRF05 to trigger port
  PORTB |= (1 << SRF05_1WIRE);             // Start HY-SRF05 trigger
  SRF05_trigger_tick = ticks;              // Snapshot trigger ticks
  last_25cm_flicker = !last_25cm_flicker;  // See "distance_cm < 26" in main
                                           //   while(1) loop
}

//---------
// This function increments ticks every 58 microseconds and creates
// a ending of the SRF05 trigger pulse after 1 tick (= 58 microseconds)
//----------
ISR(TIMER0_COMPA_vect) {
//----------
  ++ticks;
  if (SRF05_trigger_tick) {
    SRF05_trigger_tick = 0;                // Reset trigger snapshot
    PORTB &= ~(1 << SRF05_1WIRE);          // End HY-SRF05 trigger, and then...
    DDRB  &= ~(1 << SRF05_1WIRE);          // Set data direction register to IN
                                           //   for HY-SRF05 (echo)
    MCUCR |= (1 << ISC01) | (1 << ISC00);  // The rising edge of INT0 generates
                                           //   an interrupt request, so next
                                           //   INT0 must be ECHO start.
    PCMSK |= (1 << SRF05_1WIRE);           // Pin Change Mask Register, ENable
                                           //   for ECHO.
    GIMSK |= (1 << PCIE);                  // General Interrupt Mask Register,
                                           //   Pin Change Interrupt Enable
  }
}

//---------
// This function measures the SRF05 echo length direct in distance in cm
//----------
ISR(PCINT0_vect){
  if (!SRF05_echo_tick) {
    SRF05_echo_tick = ticks;       // Snapshot ECHO start in ticks
    MCUCR &= ~(1 << ISC00);        // The falling edge of INT0 generates an
                                   // interrupt request, so next INT0 must be
                                   // ECHO end.
    PORTB |= (1 << ONBOARD_LED);   // Turn on led (just for fun)
  } else {
    SRF05_echo_length = ticks - SRF05_echo_tick;
    if (SRF05_echo_length > 300) { // Ignores distance greater than 3m
      SRF05_echo_length = 0;
    }
    GIMSK &= ~(1 << PCIE);         // General Interrupt Mask Register,
                                   //   Pin Change Interrupt DISable
    PCMSK &= ~(1 << SRF05_1WIRE);  // Pin Change Mask Register, DISable for ECHO
    PORTB &= ~(1 << ONBOARD_LED);  // Turn off led
    SRF05_echo_tick = 0;
  }
}

//---------
// Setup the necessarities Interrupt and Timer-wise
//----------
void setupInterrupts(void) {
  /*
   * Setup timer0 to fire every 58 microsecond
   */
  TCCR0A |= (1 << WGM01);   // set timer counter mode to CTC
  TCCR0B |= (1 << CS01);    // set prescaler to 64 (CLK=16MHz/8/116=17.2kHz, 58us)
  OCR0A   = 116;            // set Timer's counter max value
  TIMSK  |= (1 << OCIE0A);  // enable Timer CTC interrupt

  /*
   * Setup timer1 to fire every 0.1 second
   */
  TCNT1   = 0;              // Clear registers
  TCCR1   = 0;
  TCCR1  |= (1 << CTC1);    // set timer counter mode to CTC
  OCR1C   = 195;            // set Timer's counter max value
  OCR1A   = OCR1C;
  // set prescaler to 8192 (CLK=16MHz/8192/195=10.01Hz, 0.099s)
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11);
  TIMSK |= (1 << OCIE1A);   // enable Timer CTC interrupt
}

//---------
// This function is main entry point
//----------
int main(void) {
//----------
  // Setup Data-Direction-Register
  DDRB   |= (1 << ONBOARD_LED);   // set data direction register for ONBOARD_LED to output

  setupInterrupts();
  sei();                          //enable global interrupt

  while(1)
  {
    int distance_cm = SRF05_echo_length;     // snapshot b/c volatile may change
    int bars;

    if ((distance_cm < 1) | (distance_cm > 300)) { // Switch stripe off if
      allOFF();                                    //   invalid (0) distance or
    } else {                                       //   greater than 3m.
      if (distance_cm < 26 ) {                     // less than 26cm, flicker
        if ( last_25cm_flicker ) {                 //   all leds like crazy
          allOFF();                                //   based on volatile
        } else {                                   //   last_25cm_flicker.
          barColor(color_red, WS2819_STRIPE_LEN);  //   See ISR(TIM0_COMPA_vect)
        }
      } else {                                     // Distance 26cm-3m:
        if (distance_cm < 101) {                   //  more red leds if closer
          bars = (100 - distance_cm) / 15;         //   than 1m till 26cm. One
          barColor(color_red, ++bars);             //   additional led per 15cm
        } else {                                   //  less green leds if closer
          bars = (distance_cm-100)/40;             //   than 3m till 1m. One led
          barColor(color_green, ++bars);           //   less per 40cm.
        }
      }
    }
  }
}
