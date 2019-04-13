# LotMonitor

## ATtiny C with AVR-GCC

WS2812 controlled by cpldcpu/light_ws2812 (https://github.com/cpldcpu/light_ws2812)

make should do the job if gcc-avr, avr-libc and avrdude is installed as well if the
attiny is connected by an usbasp programmer.

```bash
$ sudo apt-get install binutils gcc-avr avr-libc uisp avrdude flex byacc bison
$ make
```
