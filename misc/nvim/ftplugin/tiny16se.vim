" Vim ftplugin file
" Language: tiny16 SE (S-expression language)
" Maintainer: tiny16 project

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

" Save compatibility options
let s:save_cpo = &cpo
set cpo&vim

" =============================================================================
" Indentation for S-expressions
" =============================================================================

" Use Lisp-style indentation
setlocal lisp

" Keywords that get special 2-space body indentation (defn-style)
" Format: first arg on same line, rest indented by 2
setlocal lispwords=def,defn,let,if,while,do,data,ns,require,import

" Basic indent settings
setlocal expandtab
setlocal shiftwidth=2
setlocal softtabstop=2
setlocal tabstop=2

" Auto-indent
setlocal autoindent
setlocal smartindent

" =============================================================================
" Comments
" =============================================================================

setlocal commentstring=;\ %s
setlocal comments=:;

" =============================================================================
" Formatting
" =============================================================================

" Don't wrap text automatically
setlocal textwidth=0

" Format options for code
setlocal formatoptions-=t  " Don't auto-wrap text
setlocal formatoptions+=c  " Auto-wrap comments
setlocal formatoptions+=r  " Insert comment leader after Enter
setlocal formatoptions+=q  " Allow formatting of comments with gq

" =============================================================================
" Matchit support for parentheses
" =============================================================================

if exists("loaded_matchit")
  let b:match_words = '(:)'
endif

" =============================================================================
" Cleanup
" =============================================================================

" Undo settings when filetype changes
let b:undo_ftplugin = "setlocal lisp< lispwords< expandtab< shiftwidth< softtabstop< tabstop<"
      \ . " autoindent< smartindent< commentstring< comments< textwidth< formatoptions<"

" Restore compatibility options
let &cpo = s:save_cpo
unlet s:save_cpo
