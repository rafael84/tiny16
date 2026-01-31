#!/bin/sh
set -eu

INPUT="$1"
EXT="${INPUT##*.}"

printf '\033[0;32m'

case "$EXT" in
    se)
        # Compile .se to .asm first, then assemble
        build/tiny16-sec "$INPUT" build/eval.asm
        build/tiny16-asm build/eval.asm build/eval.tiny16
        ;;
    asm)
        # Assemble directly
        build/tiny16-asm "$INPUT" build/eval.tiny16
        ;;
    *)
        printf '\033[0;31mError: Unknown file extension .%s (expected .se or .asm)\033[0m\n' "$EXT" >&2
        exit 1
        ;;
esac

#build/tiny16-emu build/eval.tiny16 -d -t -m "${2:-1000}"
build/tiny16-emu build/eval.tiny16

printf '\033[0m'
