" Filetype detection for tiny16 assembly and SE files
" Add to your nvim config or copy to ~/.config/nvim/ftdetect/

" Detect tiny16 assembly files
" Uses pattern matching on stdlib includes or section directives
autocmd BufRead,BufNewFile *.asm
    \ if getline(1) =~# '\.include.*tiny16\|stdlib\|section\s\+\.code' ||
    \    getline(1) =~# '^;\s*tiny16' ||
    \    search('\.include.*tiny16\.inc', 'nw') ||
    \    search('\<LOADI\>\|MOVSPR\>\|MOVRSP\>', 'nw')
    \ | setfiletype tiny16asm
    \ | endif

" Detect tiny16 SE files (.se extension)
autocmd BufRead,BufNewFile *.se setfiletype tiny16se

" Also detect by modeline: vim: ft=tiny16asm or vim: ft=tiny16se
