" Vim syntax file
" Language: tiny16 Assembly
" Maintainer: tiny16 project
" Latest Revision: 2026

if exists("b:current_syntax")
  finish
endif

let b:current_syntax = "tiny16asm"

" Case insensitive matching
syntax case ignore

" Comments
syntax match tiny16asmComment ";.*$" contains=tiny16asmTodo
syntax keyword tiny16asmTodo TODO FIXME XXX NOTE HACK BUG contained

" Numbers
syntax match tiny16asmNumber "\<\d\+\>"
syntax match tiny16asmNumber "\<0x[0-9a-fA-F]\+\>"
syntax match tiny16asmNumber "\<0b[01]\+\>"

" Strings
syntax region tiny16asmString start=/"/ skip=/\\"/ end=/"/ contains=tiny16asmEscape
syntax match tiny16asmEscape "\\[nrt\\\"0]" contained

" Registers
syntax match tiny16asmRegister "\<R[0-7]\>"
syntax match tiny16asmRegister "\<SP\>"
syntax match tiny16asmRegister "\<PC\>"
syntax match tiny16asmRegister "\<FP\>"
" Register pairs
syntax match tiny16asmRegPair "\<R[0-7]:R[0-7]\>"

" Labels (identifiers followed by colon)
syntax match tiny16asmLabel "^\s*[a-zA-Z_][a-zA-Z0-9_]*:"
syntax match tiny16asmLabel "@[a-zA-Z_][a-zA-Z0-9_]*:" " Local labels in macros

" Constants (NAME = value)
syntax match tiny16asmConstant "^\s*[A-Z_][A-Z0-9_]*\s*=" contains=tiny16asmNumber

" Instructions - Data Movement
syntax keyword tiny16asmInstr LOADI LOAD STORE MOV

" Instructions - Arithmetic
syntax keyword tiny16asmInstr ADD SUB ADC SBC INC DEC

" Instructions - Bitwise
syntax keyword tiny16asmInstr AND OR XOR SHL SHR NOT

" Instructions - Comparison
syntax keyword tiny16asmInstr CMP CMPI

" Instructions - Control Flow
syntax keyword tiny16asmInstr JMP JZ JNZ JC JNC CALL RET HALT

" Instructions - Stack
syntax keyword tiny16asmInstr PUSH POP MOVSPR MOVRSP

" Directives (with dot prefix)
syntax match tiny16asmDirective "\.\<macro\>"
syntax match tiny16asmDirective "\.\<endmacro\>"
syntax match tiny16asmDirective "\.\<include\>"

" Directives (without dot prefix)
syntax keyword tiny16asmDirective section ORG TIMES DB

" Section names
syntax match tiny16asmSection "\.\<code\>"
syntax match tiny16asmSection "\.\<data\>"

" Operators in expressions
syntax match tiny16asmOperator "[+\-*/%&|^~]"
syntax match tiny16asmOperator "<<"
syntax match tiny16asmOperator ">>"

" Memory addressing brackets
syntax match tiny16asmBracket "[\[\]]"
syntax match tiny16asmBracket "[+\-]" contained

" Macro parameters (when inside macro definitions)
syntax match tiny16asmMacroParam "@[a-zA-Z_][a-zA-Z0-9_]*"

" Standard library macros (common ones from stdlib)
syntax keyword tiny16asmMacro SETADDR LOAD16 STORE16 STORE16I
syntax keyword tiny16asmMacro PUSH2 POP2 PUSH4 POP4
syntax keyword tiny16asmMacro MUL2 MUL4 DIV2 DIV4
syntax keyword tiny16asmMacro CLEAR SUBI ADDI
syntax keyword tiny16asmMacro APU_INIT APU_CH0_NOTE APU_CH0_OFF
syntax keyword tiny16asmMacro OAM_WRITE_SPRITE READ_FRAME_COUNT WAIT_VSYNC

" PPU/APU related constants (commonly used)
syntax keyword tiny16asmConstDef PPU_CTRL_HI PPU_CTRL_LO FRAME_COUNT_HI FRAME_COUNT_LO

" Highlighting links
highlight default link tiny16asmComment Comment
highlight default link tiny16asmTodo Todo
highlight default link tiny16asmNumber Number
highlight default link tiny16asmString String
highlight default link tiny16asmEscape SpecialChar
highlight default link tiny16asmRegister Identifier
highlight default link tiny16asmRegPair Identifier
highlight default link tiny16asmLabel Function
highlight default link tiny16asmConstant Constant
highlight default link tiny16asmInstr Statement
highlight default link tiny16asmDirective PreProc
highlight default link tiny16asmSection Type
highlight default link tiny16asmOperator Operator
highlight default link tiny16asmBracket Delimiter
highlight default link tiny16asmMacroParam Special
highlight default link tiny16asmMacro Macro
highlight default link tiny16asmConstDef Constant
