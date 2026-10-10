; browser.asm - game list navigation, detail view, file scanning

game_loop:
    mov byte [listmode],0
    mov ax,[vcount]
    mov [lcount],ax
    mov word [lptrs],vptrs
    mov word [lrow0],2
    mov word [lrows],GROWS
    call build_game_header
    mov word [lheader],hdrbuf
    mov ax,[gsel]
    mov [lsel],ax
    mov ax,[gtop]
    mov [ltop],ax
    call run_list
    mov bx,[lsel]
    mov [gsel],bx
    mov bx,[ltop]
    mov [gtop],bx
    cmp byte [lact],1
    je .esc
    ja .key
    mov bx,ax
    shl bx,1
    mov si,[vptrs+bx]
    jmp detail
.esc:
    cmp byte [flen],0       ; Esc: clears search first, then filters
    je .esc2
    call filter_clear
    jmp game_loop
.esc2:
    call filters_active
    jz quit
    call clear_filters
    jmp game_loop
.key:
    cmp ax,0FF04h           ; double click: play
    je .play
    test al,al
    jz .ext
    cmp al,0E0h
    je .ext
    cmp al,10               ; Ctrl+Enter: play
    je .play
    cmp al,13h              ; Ctrl+S: setup
    je .setup
    cmp al,5                ; Ctrl+E: create METADATA.INI
    je .edit
    cmp al,8
    je .bksp
    cmp al,' '
    jb game_loop
    call filter_add
    jmp game_loop
.bksp:
    call filter_back
    jmp game_loop
.ext:
    cmp ah,3Bh              ; F1: help
    je .help
    mov bl,F_GENRE
    mov di,fgenre
    mov dx,str_pgenre
    cmp ah,3Ch              ; F2
    je .pick
    mov bl,F_YEAR
    mov di,fyear
    mov dx,str_pyear
    cmp ah,3Dh              ; F3
    je .pick
    mov bl,F_PLAYERS
    mov di,fplayers
    mov dx,str_pplayers
    cmp ah,3Eh              ; F4
    je .pick
    cmp ah,40h              ; F6: toggle favorite
    je .fav
    cmp ah,41h              ; F7: favorites only
    je .favonly
    cmp ah,5Ah              ; Shift+F7: games without METADATA.INI only
    je .noini
    cmp ah,42h              ; F8: recently played
    je .recent
    cmp ah,43h              ; F9: sort order
    je .sort
    cmp ah,44h              ; F10: random game
    je .random
    cmp ah,3Fh              ; F5: clear filters
    jne game_loop
    call clear_filters
    jmp game_loop
.pick:
    call pick_filter
    jmp game_loop
.help:
    call show_help
    jmp game_loop
.play:
    call cur_game
    jc game_loop
    jmp play
.setup:
    call cur_game
    jc game_loop
    jmp setup
.fav:
    call cur_game
    jc game_loop
    call toggle_fav
    push word [gsel]        ; keep the position (the game may drop out of the list)
    call apply_filter
    pop word [gsel]
    jmp game_loop
.favonly:
    xor byte [favonly],1
    call refilter
    jmp game_loop
.noini:
    xor byte [noini],1
    call refilter
    jmp game_loop
.edit:
    call cur_game
    jc game_loop
    call edit_ini
    jmp game_loop
.sort:
    mov al,[sortmode]
    inc ax
    cmp al,NSORT
    jb .sset
    xor al,al
.sset:
    mov [sortmode],al
    call sort_games
    call refilter
    jmp game_loop
.random:
    call random_game
    jmp game_loop
.recent:
    call pick_recent
    jc game_loop
    jmp detail

random_game:                ; selects a random game of the list (not the current one)
    mov cx,[vcount]
    cmp cx,1
    jbe .r
.again:
    call ticks              ; LCG, stirred with the timer
    mov bx,ax
    mov ax,[rseed]
    mov dx,25173
    mul dx
    add ax,13849
    add ax,bx
    mov [rseed],ax
    mul cx                  ; dx = ax * vcount / 65536
    cmp dx,[gsel]
    je .again
    mov [gsel],dx
    sub dx,GROWS/2          ; center it
    jns .t
    xor dx,dx
.t: mov [gtop],dx
.r: ret

cur_game:                   ; -> si = selected game entry; CF=1 if the list is empty
    cmp word [vcount],0
    stc
    je .r
    mov bx,[gsel]
    shl bx,1
    mov si,[vptrs+bx]
    clc
.r: ret

; ---------------------------------------------------------------- detail view
detail:                     ; si = game entry
    mov [curgame],si
    mov word [dscroll],0
    call build_file_header
.rescan:
    call scan_files
    call find_runfile       ; the RUN file is selected first
    mov [fsel],ax
    mov word [ftop],0
.loop:
    mov byte [listmode],1
    mov ax,[fcount]
    mov [lcount],ax
    mov word [lptrs],fptrs
    mov word [lheader],hdrbuf
    mov word [lrow0],2
    mov word [lrows],FROWS
    mov ax,[fsel]
    mov [lsel],ax
    mov ax,[ftop]
    mov [ltop],ax
    call run_list
    mov bx,[lsel]
    mov [fsel],bx
    mov bx,[ltop]
    mov [ftop],bx
    cmp byte [lact],1
    je game_loop
    ja .key
    mov bx,ax
    shl bx,1
    mov si,[fptrs+bx]
    push si
    call is_exec
    pop si
    je .run
    call beep               ; not an executable file
    jmp .loop
.run:
    call file_cmd
    jmp launch
.key:
    cmp ah,0Fh              ; Tab: executables / all files
    je .tab
    mov bx,-WSTEP           ; description scrolling: wheel over the left box,
    cmp ax,0FF02h           ; Ctrl+PgUp/PgDn, Shift+PgUp/PgDn
    je .scroll
    mov bx,WSTEP
    cmp ax,0FF03h
    je .scroll
    cmp al,5                ; Ctrl+E: create METADATA.INI
    je .edit
    test al,al
    jz .ext
    cmp al,0E0h
    jne .loop
.ext:
    cmp ah,40h              ; F6: toggle favorite
    je .fav
    cmp ah,3Bh              ; F1: help
    jne .nohelp
    call show_help
    jmp .loop
.nohelp:
    mov bx,-(DROWS-1)
    cmp ah,84h
    je .scroll
    cmp ah,49h
    je .scroll
    mov bx,DROWS-1
    cmp ah,76h
    je .scroll
    cmp ah,51h
    jne .loop
.scroll:
    add bx,[dscroll]        ; clamp to 0..dlines-DROWS
    mov ax,[dlines]
    sub ax,DROWS
    jns .max
    xor ax,ax
.max:
    cmp bx,ax
    jle .min
    mov bx,ax
.min:
    test bx,bx
    jns .set
    xor bx,bx
.set:
    mov [dscroll],bx
    jmp .loop
.tab:
    xor byte [showall],1
    jmp .rescan
.edit:
    mov si,[curgame]
    call edit_ini
    jmp .loop
.fav:
    mov si,[curgame]
    call toggle_fav
    call apply_filter       ; the game list may change (favorites only)
    jmp .loop

; ---------------------------------------------------------------- scanning files
scan_files:
    mov word [fcount],0
    mov di,pathbuf
    call put_colldir
    mov si,[curgame]
    call strcpy
    mov si,str_star
    call strcpy
    mov dx,pathbuf
    xor cx,cx
    mov ah,4Eh
    int 21h
    jc .done
.loop:
    cmp byte [showall],0
    jne .add
    mov si,dta+1Eh
    call is_exec
    jne .next
.add:
    mov ax,[fcount]
    cmp ax,MAXF
    jae .done
    mov bx,FSZ
    mul bx
    add ax,fnames
    mov di,ax
    mov bx,[fcount]
    shl bx,1
    mov [fptrs+bx],di
    inc word [fcount]
    mov si,dta+1Eh
    mov cx,FSZ
    rep movsb
.next:
    mov ah,4Fh
    int 21h
    jnc .loop
.done:
    mov bx,fptrs
    mov cx,[fcount]
    mov dx,stricmp
    call sort_ptrs
    ret

build_file_header:          ; header: "Title  (FOLDER)", dirbuf: full folder path
    mov di,hdrbuf
    mov si,[curgame]
    push si
    add si,13
    call strcpy
    mov si,str_lpar
    call strcpy
    pop si
    call strcpy
    mov si,str_rpar
    call strcpy
    mov di,dirbuf
    call put_colldir
    mov si,[curgame]
    jmp strcpy

build_game_header:          ; header: "Games  (\COLLECT)" + match count + active filters
    mov di,hdrbuf
    mov si,hdr_games
    call strcpy
    call filters_active
    jnz .cnt
    cmp byte [flen],0
    je .fo
.cnt:
    mov si,str_hfound
    call strcpy
    mov ax,[vcount]
    call num_str
    mov si,str_hof
    call strcpy
    mov ax,[gcount]
    call num_str
.fo:
    mov cl,[sortmode]       ; sort order other than title
    test cl,cl
    jz .fv
    mov si,str_hsort
    call strcpy
    mov si,sort_names
.sn:
    lodsb                   ; skip cl names
    test al,al
    jnz .sn
    dec cl
    jnz .sn
    call strcpy
.fv:
    cmp byte [favonly],0
    je .fl
    mov si,str_hfav
    call strcpy
.fl:
    cmp byte [noini],0
    je .fi
    mov si,str_hnoini
    call strcpy
.fi:
    mov bx,fgenre
    mov si,str_hgenre
    call .add
    mov bx,fyear
    mov si,str_hyear
    call .add
    mov bx,fplayers
    mov si,str_hplayers
.add:
    cmp byte [bx],0
    je .r
    call strcpy
    mov si,bx
    call strcpy
.r: ret
