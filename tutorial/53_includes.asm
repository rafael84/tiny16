; Tutorial 53: Includes
; Level: 8 - Assembler Power Features
;
; Goal: Reuse constants and macros from another file with .include
; Expected result: R0 = 16
;
; Hint: The include path is relative to this file

.include "includes/constants.inc"

section .code

loadi r0, 4
ADD_BASE r0
halt
