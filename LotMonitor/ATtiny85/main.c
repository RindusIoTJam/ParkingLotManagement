#ifndef F_CPU
#define F_CPU 16000000UL  // 16MHz for the Attiny85
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812.h"  // https://github.com/cpldcpu/light_ws2812
#include "main.h"

#define ONBOARD_LED   PB1
#define SRF05_ECHO    PB2
#define SRF05_TRIGGER PB3
//      WS2819_DATA   PB4 // See Makefile WS2812_PIN

#define INTERRUPT_DIV 10

#define WS2819_STRIPE_LEN 5
#define WS2819_BRIGHTNESS 25  // percent [optional, comment for full brightness]

struct cRGB led[WS2819_STRIPE_LEN+1];      // one more to prevent array overflow

volatile int32_t interrupt_cnt     = INTERRUPT_DIV;
volatile int32_t SRF05_distance_cm = 0;
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
void inline allOFF() {
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
// This function lights a bar in the given color and length.
//----------
void inline barColor(struct cRGB color, int bars) {
//----------
  setLEDS(color.r,color.g,color.b, bars);
}


//---------
// This function is executed 100 times per sec by timer interupt (See TCCR0B)
//----------
ISR(TIM0_COMPA_vect) {
//----------

  if (++interrupt_cnt > INTERRUPT_DIV)         // skip some interrupts b/c SRF05
  {                                            //   handles max. 20 trigger/sec.
    interrupt_cnt = 0;                         //   INTERRUPT_DIV=10 does 10/sec

    last_25cm_flicker = !last_25cm_flicker;    // See "distance_cm < 26" in main
                                               //   while(1) loop
    PORTB |= (1 << SRF05_TRIGGER);             // Trigger SRF05 Measurement
    _delay_us(10);
    PORTB &= ~(1 << SRF05_TRIGGER);

    uint32_t echo_time_us = 0;
    uint32_t echo_wait_us = 300;               // timeout to wait for echo in us

    while ( !(PINB & (1 << SRF05_ECHO)) & (echo_wait_us > 0) ) {  // Wait 4 echo
      _delay_us(1);                                               //  max. 300us
      --echo_wait_us;
    }

    if (echo_wait_us) {                            // we got an echo
      PORTB |= (1 << ONBOARD_LED);                 // turn on led (just for fun)

      while ( (PINB & (1 << SRF05_ECHO)) ) {       // count echo HIGH time
        _delay_us(1);
        ++echo_time_us;
      }
    } else {
      allColor(color_blue);                       // no echo, sensor gone?
    }

    if (echo_time_us > 17450) {          // 17450 / 58 = 300.86cm, to be ignored
      SRF05_distance_cm = 0;
    } else {
      SRF05_distance_cm = echo_time_us / 58;
    }

    PORTB &= ~(1 << ONBOARD_LED);        // turn off led
  }
}

//---------
// This function is main entry point
//----------
int main(void) {
//----------

  // Setup Data-Direction-Register
  DDRB   &= ~(1 << SRF05_ECHO);   // set data direction register for HY-SRF05 echo port
  PORTB  &= ~(1 << SRF05_ECHO);   // No internal pullup

  DDRB   |= (1 << ONBOARD_LED);   // set data direction register for ONBOARD_LED as output
  DDRB   |= (1 << SRF05_TRIGGER); // set data direction register for HY-SRF05 trigger port

  TCCR0A |= _BV(WGM01);           // set timer counter mode to CTC
  TCCR0B |= _BV(CS02)|_BV(CS00);  // set prescaler to 1024 (CLK=16MHz/1024/160=97.68Hz, 0.010s)
  OCR0A   = 160;                  // set Timer's counter max value
  TIMSK  |= _BV(OCIE0A);          // enable Timer CTC interrupt

  sei();                          //enable global interrupt

  while(1)
  {
    int distance_cm = SRF05_distance_cm;     // snapshot b/c volatile may change
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
