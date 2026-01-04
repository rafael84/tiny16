.PHONY: all asm tests edit-cpu edit-asm examples

CFLAGS += $(shell cat compile_flags.txt)

all: bin tests asm emulator

bin:
	mkdir -p $@

tests: src/*.c src/*.h tests/*.c
	gcc $(CFLAGS) -o bin/tiny16-tests tests/tiny16.c

asm: src/*.c src/*.h asm/*.c
	gcc $(CFLAGS) -o bin/tiny16-asm asm/tiny16.c

emulator: src/*.c src/*.h emulator/*.c
	gcc $(CFLAGS) -o bin/tiny16-emu emulator/tiny16.c

edit-cpu:
	nvim src/cpu.c '+SpecOpen tiny16-spec.txt'

edit-asm:
	nvim asm/tiny16.c '+SpecOpen examples/test_03_alu.asm'

edit-emu:
	nvim emulator/tiny16.c '+SpecOpen examples/demo.asm'

EXAMPLES := $(wildcard examples/*.asm)
BINS     := $(patsubst examples/%.asm,bin/%.tiny16,$(EXAMPLES))

examples: asm
	@for F in $(EXAMPLES); do \
		bin/tiny16-asm $$F bin/$$(basename $$F .asm).tiny16; \
	done
