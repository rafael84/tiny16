#!/bin/sh
set -eu

printf '\033[0;32m'

bin/tiny16-asm "$1" bin/eval.tiny16
bin/tiny16-emu bin/eval.tiny16 -d -t -m "${2:-1000}"

printf '\033[0m'
