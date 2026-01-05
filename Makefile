.PHONY: all asm tests examples

CC = gcc
CFLAGS += $(shell cat compile_flags.txt)

all: tests asm emulator examples

bin:
	mkdir -p $@

tests: bin vm/*.c vm/*.h tests/*.c | bin
	$(CC) $(CFLAGS) -o bin/tiny16-tests tests/tiny16.c
	bin/tiny16-tests

asm: bin vm/*.c vm/*.h asm/*.h asm/*.c | bin
	$(CC) $(CFLAGS) -o bin/tiny16-asm asm/tiny16.c

emulator: bin vm/*.c vm/*.h emulator/*.c | bin
	$(CC) $(CFLAGS) -o bin/tiny16-emu emulator/tiny16.c

EXAMPLES := $(wildcard examples/*.asm)
examples: asm
	@for F in $(EXAMPLES); do \
		bin/tiny16-asm $$F bin/$$(basename $$F .asm).tiny16; \
		done

compile_commands.json:
	bear -- $(MAKE)
