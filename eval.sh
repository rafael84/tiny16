#!/bin/sh
set -eu
bin/tiny16-asm "$1" bin/eval.tiny16
bin/tiny16-emu bin/eval.tiny16 -d -t -m "${2:-100}"
