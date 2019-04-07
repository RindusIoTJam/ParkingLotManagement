#ifndef F_CPU
#define F_CPU 16000000UL  // 16MHz for the Attiny85
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812.h"  // https://github.com/cpldcpu/light_ws2812
#include "main.h"

#define WS2819_STRIPE_LEN 5
#define WS2819_BRIGHTNESS 1 // percent

struct cRGB led[WS2819_STRIPE_LEN];

// Set all stripe LED to RGB
void inline all(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t factor = 100/WS2819_BRIGHTNESS;
  for(int c=0;c<WS2819_STRIPE_LEN;c++) {
    led[c].r=r/factor;led[c].g=g/factor;led[c].b=b/factor;
  }
}

void inline allOFF() {
  allColor(color_black);
}

void inline allColor(struct cRGB color) {
  allRGB(color.r,color.g,color.b);
}

void inline allRGB(uint8_t r, uint8_t g, uint8_t b) {
  all(r,g,b);
  ws2812_setleds(led,WS2819_STRIPE_LEN);
}

int main(void)
{
  while(1)
  {
    allColor(color_red);
    _delay_ms(1);

    allOFF();
    _delay_ms(100);

    allColor(color_blue);
    _delay_ms(1);

    allOFF();
    _delay_ms(898);
  }
}
