ARCH       = attiny85
F_CPU      = 16000000
PROGRAMMER = usbasp
FILENAME   = main
WS2812_LIB = light_ws2812
WS2812_PIN = 4
COMPILE    = avr-gcc -Wall -g0 -Os -I. -DF_CPU=$(F_CPU) -Dws2812_pin=$(WS2812_PIN) -mmcu=$(ARCH)
COMPILE   += -ffunction-sections -fdata-sections -fpack-struct
COMPILE   += -fno-move-loop-invariants -fno-tree-scev-cprop
COMPILE   += -fno-inline-small-functions -Wno-pointer-to-int-cast

.PHONY:	all clean install

all: clean build install

build: $(WS2812_LIB)
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf $(FILENAME).o $^.o
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

install: build
	@echo Uploading
	avrdude -p $(ARCH) -c $(PROGRAMMER) -U flash:w:$(FILENAME).hex:i

$(WS2812_LIB):
	@echo Building Library
	$(COMPILE) -o $@.o -c $@.c

clean:
	@echo Removing o/elf/hex
	rm --force *.o *.elf *.hex
