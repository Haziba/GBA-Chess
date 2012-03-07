# Makefile
#	arm-eabi-g++	: gcc C++ compiler
#		useful option
#		CFLAG
#			-c		: compile
#			-O3		: auto-optimization level3
#		MODEL
# 			-mthumb	: compile to THUMB code
#			-mthumb-interwork -specs=gba_mb.specs : tell GCC that we're using GBA

# Makefile section
#	target : dependancy
#					update target base on the dependencies (read more on GCC Makefile)
#	clean:
#					project clean-up section  

CC = arm-eabi-g++
OBJCOPY = arm-eabi-objcopy
CFLAGS = -g -O3 -Wall
MODEL = -mthumb -mthumb-interwork -specs=gba_mb.specs

TARGET = main

all: $(TARGET).gba

%.gba: %.elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@

%.elf: %.o gba.o font.o
	$(CC) $(MODEL) -o $@ $^ -lm

%.o: %.cpp gba.h font.h
	$(CC) $(CFLAGS) $(MODEL) -c $<

run: run-$(TARGET)

run-%: %.gba
	vba $*.gba
	
clean:
	rm -f *.gba *.elf *.o
