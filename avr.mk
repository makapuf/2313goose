ifndef PROJECT
    $(error PROJECT is undefined)
endif 

MMU=attiny2313
cpu_speed=8000000UL
CFLAGS= -Wall -std=c99 -Os -DF_CPU=$(cpu_speed) -fdata-sections -ffunction-sections
LDFLAGS= -Wl,--gc-sections

OBJECTS := $(C_FILES:.c=.o)

all: $(PROJECT).hex

$(PROJECT).elf: $(OBJECTS)
	avr-gcc -mmcu=$(MMU) $(LDFLAGS) $(OBJECTS) -o $@
	avr-size $@

%.hex: %.elf
	avr-objcopy -O ihex $< $@

%.o: %.c
	avr-gcc $(CFLAGS) -mmcu=$(MMU) -c $< 

clean: 
	rm -f *.o $(PROJECT) *.hex *.bin *.elf

upload: $(PROJECT).hex
	avrdude -p $(MMU) -c avrisp -P /dev/ttyUSB0 -b 9600 -e -U flash:w:$<
