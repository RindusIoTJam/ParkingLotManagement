#ifndef F_CPU
#define F_CPU 16000000UL  // 16MHz for the Attiny85
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#ifndef AVR_STUDIO
#include "light_ws2812.h"  // https://github.com/cpldcpu/light_ws2812
#endif // AVR_STUDIO
#include "main.h"

#define ONBOARD_LED   PB1
#define SRF05_ECHO    PB2
#define SRF05_TRIGGER PB3
//      WS2819_DATA   PB4 // See Makefile WS2812_PIN


#define INTERRUPT_DIV 10  
#define ECHO_TIMEOUT 517 // 30000us / 58us 
#define TRIGGER_TIMER_FREQ 160 // 100Hz
#define ECHO_TIMER_FREQ 116 // 58us interruption

#define WS2819_STRIPE_LEN 5
#define WS2819_BRIGHTNESS 100 // percent [optional, comment for full brightness]


     // one more to prevent array overflow
;
volatile int32_t last_25cm_flicker = 0;
volatile struct US_variables SRF05_values;


struct cRGB led[WS2819_STRIPE_LEN+1]; 
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
void inline allOFF(uint8_t update_available) {
//----------
  if (update_available == False) //WS2812 library is dis-activating interrupt when change the color, creating error in the US measurements
  {
	allColor(color_black);
  }
  
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
void inline barColor(struct cRGB color, int bars,uint8_t update_available) {
//----------
	if (update_available == False)
	{
	  setLEDS(color.r,color.g,color.b, bars);
	}
}

//---------
// This function is executed 100 times per sec by timer interrupt (See TCCR0B)
//----------
ISR(TIM0_COMPA_vect) {
//----------
	
	switch(SRF05_values.state)
	{
		case trigger:
			
			if(SRF05_values.trigger_elapsed >= INTERRUPT_DIV )  // software divisor to trigger measure every 10 ms
			{
				SRF05_values.updating_measures = True;
				PORTB |= (1 << SRF05_TRIGGER);
				set_timer0_preescaler_and_compare_registers(TIM0_PREESCALER_8,ECHO_TIMER_FREQ); //set prescaler to 8 (CLK=16MHz/8/116=97.17241Hz, 58us)
				SRF05_values.state = waiting_for_echo;
			}
			else
			{
				SRF05_values.updating_measures = False;
				SRF05_values.trigger_elapsed++ ;
			}
		break;
	
		case waiting_for_echo:
			PORTB &= ~(1 << SRF05_TRIGGER);  //deactivate trigger pin and wait for echo signal. Maximum waiting time 30 ms
			if(!(PINB & (1 << SRF05_ECHO)))
			{
				 if(SRF05_values.echo_elapsed >= ECHO_TIMEOUT)
				 {
					 
					 SRF05_values.state = time_out;
				 }
				 else
				 {
					 SRF05_values.echo_elapsed++;
				 }
			}
			else
			{
				SRF05_values.state = echo_received;
			}
		break;
	
		case echo_received:
			PINB |= (1 << ONBOARD_LED);  //With logic analyzer you can check that the measurement are corrects connecting in PB1
			SRF05_values.echo_total_time++;
			
			if(SRF05_values.echo_total_time < ECHO_TIMEOUT)
			{
				if(!(PINB & (1 << SRF05_ECHO)))
				{
					SRF05_values.echo_buffered_total_time = SRF05_values.echo_total_time;
					SRF05_values.echo_elapsed = 0;
					SRF05_values.trigger_elapsed = 0;
					SRF05_values.echo_total_time = 0;
					set_timer0_preescaler_and_compare_registers(TIM0_PREESCALER_1024,TRIGGER_TIMER_FREQ); //set prescaler to 1024 (CLK=16MHz/1024/160=97.68Hz, 0.010s)
					SRF05_values.state = trigger;
				}
			}
			else
			{
				SRF05_values.state = time_out;
			}
			
	
		break;
	
		case time_out:
	
		SRF05_values.echo_elapsed = 0;
		SRF05_values.trigger_elapsed = 0;
		SRF05_values.echo_total_time = 0;
		SRF05_values.echo_buffered_total_time = 0xffff;
		set_timer0_preescaler_and_compare_registers(TIM0_PREESCALER_1024,TRIGGER_TIMER_FREQ); //set prescaler to 1024 (CLK=16MHz/1024/160=97.68Hz, 0.010s)
		SRF05_values.state = trigger;
	
	
		break;
	}

}

//---------
// This function is main entry point
//----------
int main(void) {
//----------
	
	SRF05_values.echo_elapsed = 0;
	SRF05_values.state = trigger;
	SRF05_values.trigger_elapsed = 0;
	SRF05_values.echo_total_time = 0;
	SRF05_values.echo_buffered_total_time = 0xffff;

  // Setup Data-Direction-Register
  DDRB   &= ~(1 << SRF05_ECHO);   // set data direction register for HY-SRF05 echo port
  PORTB  &= ~(1 << SRF05_ECHO);   // No internal pullup

  DDRB   |= (1 << ONBOARD_LED);   // set data direction register for ONBOARD_LED as output
  PORTB &= ~(1 << ONBOARD_LED);
  DDRB   |= (1 << SRF05_TRIGGER); // set data direction register for HY-SRF05 trigger port
  TCCR0A |= _BV(WGM01);           // set timer counter mode to CTC
  TIMSK  |= _BV(OCIE0A);          // enable Timer CTC interrupt
  
  set_timer0_preescaler_and_compare_registers(TIM0_PREESCALER_1024,TRIGGER_TIMER_FREQ); //set prescaler to 1024 (CLK=16MHz/1024/160=97.68Hz, 0.010s)
  
  sei();                          //enable global interrupt

  while(1)
  {
    uint16_t distance_cm = SRF05_values.echo_buffered_total_time;     // snapshot b/c volatile may change
    int bars;

    if ((distance_cm < 1) | (distance_cm > 300)) { // Switch stripe off if
      allOFF(SRF05_values.updating_measures);                                    //   invalid (0) distance or
    } else {                                       //   greater than 3m.
      if (distance_cm < 26 ) {                     // less than 26cm, flicker
		_delay_ms(100);								//will be optimized
        if ( last_25cm_flicker ) {                 //   all leds like crazy
			last_25cm_flicker   = 0;
          allOFF(SRF05_values.updating_measures);                                //   based on volatile
        } else {   
			last_25cm_flicker   = 1   ;                          //   last_25cm_flicker.
          barColor(color_red, WS2819_STRIPE_LEN,SRF05_values.updating_measures);  //   See ISR(TIM0_COMPA_vect)
        }
      } else {                                     // Distance 26cm-3m:
        if (distance_cm < 101) {                   //  more red leds if closer
          bars = (100 - distance_cm) / 15;         //   than 1m till 26cm. One
          barColor(color_red, ++bars,SRF05_values.updating_measures);             //   additional led per 15cm
        } else {                                   //  less green leds if closer
          bars = (distance_cm-100)/40;             //   than 3m till 1m. One led
          barColor(color_green, ++bars,SRF05_values.updating_measures);           //   less per 40cm.
        }
      }
    }
  }
}


void set_timer0_preescaler_and_compare_registers(uint8_t preescaler,uint8_t compare)
{
	TCCR0B &= TIM0_NO_CLOCK;
	TCNT0  = 0	;
	OCR0A   = compare;		
	TCCR0B |= preescaler;	
}