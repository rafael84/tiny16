Tiny16 SE Tutorial
==================

Step-by-step tiny16se (S-expression) examples that compile to tiny16 assembly.

QUICK START:
  cd sec/tutorial/
  ../../build/tiny16-sec 01_expressions.se 01_expressions.asm
  ../../build/tiny16-asm 01_expressions.asm 01_expressions.tiny16
  ../../build/tiny16-emu 01_expressions.tiny16 -m 200 -d

The compiler always calls (main) as the program entry point.

LEARNING PATH
=============

01  Expressions & Constants      def, arithmetic, main entry
02  Variables & Mutation         let, set, inc/dec
03  Control Flow                 if, while, comparisons
04  Functions & Composition       defn, arguments, nested calls
05  Data & Memory                data, db, repeat, peek/poke

RESOURCES:
  ../../specs/tiny16-se.txt
  ../../examples/se/*.se

Start with 01 and work your way up!
