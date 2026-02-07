" Vim syntax file
" Language: tiny16 SE (S-expression language) v2
" Maintainer: tiny16 project
" Latest Revision: 2026

if exists("b:current_syntax")
  finish
endif

let b:current_syntax = "tiny16se"

" Case sensitive (SE is case-sensitive unlike assembly)
syntax case match

" Comments
syntax match tiny16seComment ";.*$" contains=tiny16seTodo
syntax keyword tiny16seTodo TODO FIXME XXX NOTE HACK BUG contained

" Numbers
syntax match tiny16seNumber "\<\d\+\.\d\+\>"
syntax match tiny16seNumber "\<-\?\d\+\>"
syntax match tiny16seNumber "\<0x[0-9a-fA-F]\+\>"

" Strings
syntax region tiny16seString start=/"/ skip=/\\"/ end=/"/ contains=tiny16seEscape
syntax match tiny16seEscape "\\[nrt\\\"0]" contained

" Parentheses (important for S-expressions)
syntax match tiny16seParen "[()]"

" Keywords (start with :)
syntax match tiny16seKeyword "\<:[a-zA-Z_\-][a-zA-Z0-9_\-]*\>"

" Type hints (^u8, ^i8, ^u16, ^i16)
syntax match tiny16seTypeHint "\^u8\>"
syntax match tiny16seTypeHint "\^i8\>"
syntax match tiny16seTypeHint "\^u16\>"
syntax match tiny16seTypeHint "\^i16\>"

" Special literals
syntax keyword tiny16seNil nil
syntax keyword tiny16seBoolean true false

" Special Forms (language keywords)
syntax keyword tiny16seSpecial def defn defmacro defrecord var let fn
syntax match tiny16seSpecial "\<set!\>"

" Control flow
syntax keyword tiny16seControl if cond when unless while for do

" Module system
syntax keyword tiny16seModule ns require import

" Operators (arithmetic, bitwise, comparison)
syntax match tiny16seOperator "(\s*\zs+"
syntax match tiny16seOperator "(\s*\zs-"
syntax match tiny16seOperator "(\s*\zs\*"
syntax match tiny16seOperator "(\s*\zs/"
syntax match tiny16seOperator "(\s*\zs%"
syntax match tiny16seOperator "(\s*\zs&"
syntax match tiny16seOperator "(\s*\zs|"
syntax match tiny16seOperator "(\s*\zs\^"
syntax match tiny16seOperator "(\s*\zs\~"
syntax match tiny16seOperator "(\s*\zs<<"
syntax match tiny16seOperator "(\s*\zs>>"
syntax match tiny16seOperator "(\s*\zs="
syntax match tiny16seOperator "(\s*\zs!="
syntax match tiny16seOperator "(\s*\zs<="
syntax match tiny16seOperator "(\s*\zs>="
syntax match tiny16seOperator "(\s*\zs<\ze[^<=]"
syntax match tiny16seOperator "(\s*\zs>\ze[^>=]"

" Arithmetic builtins
syntax keyword tiny16seBuiltin inc dec

" Logic (short-circuit)
syntax keyword tiny16seBuiltin and or not

" Type predicates
syntax match tiny16seBuiltin "\<nil?\>"
syntax match tiny16seBuiltin "\<zero?\>"
syntax match tiny16seBuiltin "\<pos?\>"
syntax match tiny16seBuiltin "\<neg?\>"

" Memory access
syntax keyword tiny16seBuiltin load store hi lo

" Array and collection builtins
syntax keyword tiny16seBuiltin nth len array range

" Type casts
syntax keyword tiny16seCast u8 i8

" Inline assembly
syntax keyword tiny16seBuiltin asm

" Function definitions - highlight the function name
syntax match tiny16seFunction "(\s*defn\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-!?]*"

" Macro definitions - highlight the macro name
syntax match tiny16seFunction "(\s*defmacro\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-!?]*"

" Record definitions - highlight the record name
syntax match tiny16seType "(\s*defrecord\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Variable definitions - highlight the variable name
syntax match tiny16seVariable "(\s*var\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"
" Variable definitions with type hint - highlight the variable name
syntax match tiny16seVariable "(\s*var\s\+\^[iu]\d\+\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Constant definitions - highlight the constant name
syntax match tiny16seConstant "(\s*def\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Symbols (identifiers) - generic catch-all for identifiers
syntax match tiny16seSymbol "\<[a-zA-Z_\-][a-zA-Z0-9_\-!?]*\>"

" Namespaced symbols (e.g., apu/init, sprite/draw, apu/NOTE_C4)
" These must be defined AFTER generic symbol matching to take priority
" Match namespace prefix (the part before /)
syntax match tiny16seNsPrefix "\<[a-zA-Z_\-][a-zA-Z0-9_\-]*\ze/"
" Match the / separator in namespaced symbols
syntax match tiny16seNsSeparator "\<[a-zA-Z_\-][a-zA-Z0-9_\-]*\zs/\ze[a-zA-Z_\-]"
" Match the symbol after / in namespaced calls
syntax match tiny16seNsSymbol "/\zs[a-zA-Z_\-][a-zA-Z0-9_\-!?]*\>"

" Namespace name (after ns keyword)
syntax match tiny16seNamespace "(\s*ns\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Highlighting links
highlight default link tiny16seComment Comment
highlight default link tiny16seTodo Todo
highlight default link tiny16seNumber Number
highlight default link tiny16seString String
highlight default link tiny16seEscape SpecialChar
highlight default link tiny16seParen Delimiter
highlight default link tiny16seKeyword Constant
highlight default link tiny16seTypeHint Type
highlight default link tiny16seNil Constant
highlight default link tiny16seSpecial Keyword
highlight default link tiny16seControl Conditional
highlight default link tiny16seModule Include
highlight default link tiny16seNamespace Type
highlight default link tiny16seBuiltin Function
highlight default link tiny16seOperator Operator
highlight default link tiny16seFunction Function
highlight default link tiny16seType Type
highlight default link tiny16seVariable Identifier
highlight default link tiny16seConstant Constant
highlight default link tiny16seCast Type
highlight default link tiny16seSymbol Identifier
highlight default link tiny16seBoolean Boolean
" Namespaced symbol highlighting
highlight default link tiny16seNsPrefix Type
highlight default link tiny16seNsSeparator Delimiter
highlight default link tiny16seNsSymbol Identifier
