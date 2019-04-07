DEVICE     = attiny85
CLOCK      = 16000000
PROGRAMMER = usbasp
FILENAME   = main
WS2812_LIB = light_ws2812
WS2812_PIN = 4
COMPILE    = avr-gcc -Wall -g0 -Os -I. -DF_CPU=$(CLOCK) -Dws2812_pin=$(WS2812_PIN) -mmcu=$(DEVICE)
COMPILE   += -ffunction-sections -fdata-sections -fpack-struct
COMPILE   += -fno-move-loop-invariants -fno-tree-scev-cprop
COMPILE   += -fno-inline-small-functions -Wno-pointer-to-int-cast

all: clean build upload

build: $(WS2812_LIB)
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf $(FILENAME).o $^.o
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

upload: build
	@echo Uploading
	avrdude -p $(DEVICE) -c $(PROGRAMMER) -U flash:w:$(FILENAME).hex:i

$(WS2812_LIB):
	@echo Building Library
	$(COMPILE) -o $@.o -c $@.c

.PHONY:	clean

clean:
	@echo Removing o/elf/hex
	rm --force *.o *.elf *.hex
