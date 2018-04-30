" Enables/Disables syntax highlighting when the terminal has colors.
" Also switches on hightlighting the last used search pattern.
syntax on
set hlsearch
set incsearch

set tabstop=4
set shiftwidth=4
set expandtab

:hi Comment ctermfg=Lightblue
:hi Search cterm=NONE ctermfg=Black ctermbg=Yellow

" expand vim color pallete when accessed through putty
set t_Co=256
