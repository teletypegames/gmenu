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
    test al,al
    jz .ext
    cmp al,0E0h
    je .ext
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
    cmp ah,42h              ; F8: recently played
    je .recent
    cmp ah,3Fh              ; F5: clear filters
    jne game_loop
    call clear_filters
    jmp game_loop
.pick:
    call pick_filter
    jmp game_loop
.fav:
    cmp word [vcount],0
    je game_loop
    mov bx,[gsel]
    shl bx,1
    mov si,[vptrs+bx]
    call toggle_fav
    push word [gsel]        ; keep the position (the game may drop out of the list)
    call apply_filter
    pop word [gsel]
    jmp game_loop
.favonly:
    xor byte [favonly],1
    call refilter
    jmp game_loop
.recent:
    call pick_recent
    jc game_loop
    jmp detail

; ---------------------------------------------------------------- detail view
detail:                     ; si = game entry
    mov [curgame],si
    call build_file_header
.rescan:
    call scan_files
    mov word [fsel],0
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
    call write_runner
    call add_recent
    call save_sel
    call restore_screen
    mov ax,4C00h
    int 21h
.key:
    cmp ah,40h              ; F6: toggle favorite
    je .fav
    cmp ah,0Fh              ; Tab: executables / all files
    jne .loop
    xor byte [showall],1
    jmp .rescan
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
    xor dx,dx
    call sort_ptrs
    ret

; si = filename; ZF=1 if .COM / .EXE / .BAT
is_exec:
.f: lodsb
    test al,al
    jz .no
    cmp al,'.'
    jne .f
    mov al,[si]
    call upcase
    mov bl,al
    mov al,[si+1]
    call upcase
    mov bh,al
    mov al,[si+2]
    call upcase
    mov dl,al
    cmp byte [si+3],0
    jne .no
    cmp bx,'CO'
    jne .t2
    cmp dl,'M'
    ret
.t2:
    cmp bx,'EX'
    jne .t3
    cmp dl,'E'
    ret
.t3:
    cmp bx,'BA'
    jne .no
    cmp dl,'T'
    ret
.no:
    or al,1
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
    cmp byte [favonly],0
    je .fl
    mov si,str_hfav
    call strcpy
.fl:
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
