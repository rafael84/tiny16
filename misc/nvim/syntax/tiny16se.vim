" Vim syntax file
" Language: tiny16 SE (S-expression language)
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
syntax match tiny16seNumber "\<\d\+\>"
syntax match tiny16seNumber "\<0x[0-9a-fA-F]\+\>"

" Strings
syntax region tiny16seString start=/"/ skip=/\\"/ end=/"/ contains=tiny16seEscape
syntax match tiny16seEscape "\\[nrt\\\"0]" contained

" Parentheses (important for S-expressions)
syntax match tiny16seParen "[()]"

" Special Forms (language keywords)
syntax keyword tiny16seSpecial def defn let set if while do data db repeat include

" Arithmetic primitives
syntax keyword tiny16seBuiltin add sub neg inc dec

" Bitwise primitives
syntax keyword tiny16seBuiltin and or xor not shl shr

" Comparison primitives
syntax keyword tiny16seBuiltin eq ne lt gt le ge

" Logical primitives
syntax keyword tiny16seBuiltin lnot

" Operators (arithmetic, bitwise, comparison aliases)
" These are matched as single characters or sequences after open paren
syntax match tiny16seOperator "(\s*\zs+"
syntax match tiny16seOperator "(\s*\zs-"
syntax match tiny16seOperator "(\s*\zs\*"
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
syntax match tiny16seOperator "(\s*\zs!"

" Memory primitives
syntax keyword tiny16seBuiltin load store addr addr+ addr16
syntax keyword tiny16seBuiltin peek peek16 poke poke16
syntax match tiny16seBuiltin "\<peek\*\>"

" Compile-time helpers
syntax keyword tiny16seBuiltin hi lo

" Function definitions - highlight the function name
syntax match tiny16seFunction "(\s*defn\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Variable definitions - highlight the constant name
syntax match tiny16seConstant "(\s*def\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Data labels - highlight the data name
syntax match tiny16seDataLabel "(\s*data\s\+\zs[a-zA-Z_\-][a-zA-Z0-9_\-]*"

" Symbols (identifiers)
syntax match tiny16seSymbol "\<[a-zA-Z_\-][a-zA-Z0-9_\-]*\>"

" Boolean-like values (by convention)
syntax keyword tiny16seBoolean true false

" Highlighting links
highlight default link tiny16seComment Comment
highlight default link tiny16seTodo Todo
highlight default link tiny16seNumber Number
highlight default link tiny16seString String
highlight default link tiny16seEscape SpecialChar
highlight default link tiny16seParen Delimiter
highlight default link tiny16seSpecial Keyword
highlight default link tiny16seBuiltin Function
highlight default link tiny16seOperator Operator
highlight default link tiny16seFunction Function
highlight default link tiny16seConstant Constant
highlight default link tiny16seDataLabel Type
highlight default link tiny16seSymbol Identifier
highlight default link tiny16seBoolean Boolean
