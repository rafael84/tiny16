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

" Custom indentation function for SE language
" Handles 'let' bindings specially: aligns bindings with first binding
function! GetTiny16SEIndent()
  let lnum = v:lnum

  if lnum == 1
    return 0
  endif

  " Use searchpairpos to find the innermost unclosed ( that contains us
  " Save cursor position
  let save_cursor = getpos('.')

  " Move to start of current line
  call cursor(lnum, 1)

  " Find matching unclosed paren - search backwards
  let [paren_line, paren_col] = searchpairpos('(', '', ')', 'bnW')

  " Restore cursor
  call setpos('.', save_cursor)

  if paren_line == 0
    return 0
  endif

  " Check if this paren is the binding list of a let
  " i.e., the line has pattern (let ( before this position
  let paren_line_text = getline(paren_line)
  let before_paren = strpart(paren_line_text, 0, paren_col - 1)

  if before_paren =~# '(let\s*$'
    " We're inside the binding list of a let!
    " Find the column of the first binding symbol (right after the opening paren)
    let first_binding_col = match(paren_line_text, '(let\s*(\zs\S')
    if first_binding_col >= 0
      return first_binding_col
    endif
  endif

  " Check if we're deeper inside - maybe in a nested form within let bindings
  " Go up one more level
  call cursor(paren_line, paren_col)
  let [outer_line, outer_col] = searchpairpos('(', '', ')', 'bnW')
  call setpos('.', save_cursor)

  if outer_line > 0
    let outer_line_text = getline(outer_line)
    let before_outer = strpart(outer_line_text, 0, outer_col - 1)

    if before_outer =~# '(let\s*$'
      " The outer paren is a let's binding list
      " We're in a nested expression within the bindings
      " Use default lisp indent (will align with the expression start)
      return lispindent(lnum)
    endif
  endif

  " Default: use built-in lisp indentation
  return lispindent(lnum)
endfunction

" Use custom indentation (don't set 'lisp' as it overrides indentexpr)
setlocal indentexpr=GetTiny16SEIndent()
setlocal indentkeys=!^F,o,O,)
setlocal lispwords=def,defn,let,if,while,do,data,ns,require,import

" Basic indent settings
setlocal expandtab
setlocal shiftwidth=2
setlocal softtabstop=2
setlocal tabstop=2

" Auto-indent
setlocal autoindent

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
let b:undo_ftplugin = "setlocal indentexpr< indentkeys< lispwords< expandtab< shiftwidth< softtabstop< tabstop<"
      \ . " autoindent< commentstring< comments< textwidth< formatoptions<"

" Restore compatibility options
let &cpo = s:save_cpo
unlet s:save_cpo
