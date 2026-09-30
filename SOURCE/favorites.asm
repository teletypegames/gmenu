; favorites.asm - favorite games (GMENU.FAV), recently played games (in GMENU.SEL)

; si = entry -> bx = entry index (other registers preserved)
fav_index:
    push cx
    mov bx,si
    sub bx,entries
    mov cl,6
    shr bx,cl
    pop cx
    ret

; si = entry -> ZF=0 if favorite (bx clobbered)
is_fav:
    call fav_index
    test byte [gflags+bx],1
    ret

; si = entry: toggle favorite, save GMENU.FAV
toggle_fav:
    call fav_index
    xor byte [gflags+bx],1
    ; fallthrough

; GMENU.FAV: folder names of the favorites, one per line
save_favs:
    mov ah,3Ch
    xor cx,cx
    mov dx,f_fav
    int 21h
    jc .r
    mov [favh],ax
    xor bx,bx
.l: cmp bx,[gcount]
    jae .close
    push bx
    shl bx,1
    mov si,[gptrs+bx]
    call is_fav
    jz .n
    mov di,si
    call strend
    mov cx,di
    sub cx,si
    mov dx,si
    mov bx,[favh]
    mov ah,40h
    int 21h
    mov cx,2
    mov dx,crlf
    mov ah,40h
    int 21h
.n: pop bx
    inc bx
    jmp .l
.close:
    mov bx,[favh]
    mov ah,3Eh
    int 21h
.r: ret

load_favs:
    mov di,gflags
    mov cx,MAXG
    xor al,al
    rep stosb
    mov ax,3D00h
    mov dx,f_fav
    int 21h
    jc .r
    mov bx,ax
    mov ah,3Fh
    mov cx,MAXF*FSZ-1
    mov dx,ibuf
    int 21h
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop cx
    popf
    jc .r
    mov bx,cx
    mov byte [ibuf+bx],0
    mov si,ibuf
.line:
    mov al,[si]
    test al,al
    jz .r
    cmp al,' '
    ja .name
    inc si
    jmp .line
.name:
    mov di,si
.e: inc si
    cmp byte [si],' '
    ja .e
    mov al,[si]             ; separator (0 = end of file)
    mov byte [si],0
    push ax
    push si
    call find_entry
    jc .nf
    call fav_index
    or byte [gflags+bx],1
.nf:
    pop si
    pop ax
    test al,al
    jz .r
    inc si
    jmp .line
.r: ret

; di = folder name -> si = entry, CF=1 if not found (di preserved)
find_entry:
    xor bx,bx
.l: cmp bx,[gcount]
    jae .no
    push bx
    shl bx,1
    mov si,[gptrs+bx]
    pop bx
    call stricmp
    test ax,ax
    jz .yes
    inc bx
    jmp .l
.no:
    stc
    ret
.yes:
    clc
    ret

; ---------------------------------------------------------------- recently played
; [curgame] -> to the front of the recent list (NREC folder names, 13 bytes each)
add_recent:
    mov di,[curgame]
    xor bx,bx
.f: cmp bx,NREC-1           ; not found: the last one drops off
    jae .shift
    mov ax,13
    mul bx
    add ax,recent
    mov si,ax
    call stricmp
    test ax,ax
    jz .shift
    inc bx
    jmp .f
.shift:                     ; recent[bx-1..0] -> recent[bx..1]
    test bx,bx
    jz .put
    lea ax,[bx-1]
    mov cx,13
    mul cx
    add ax,recent
    mov si,ax
    lea di,[si+13]
    mov cx,13
    rep movsb
    dec bx
    jmp .shift
.put:
    mov si,[curgame]
    mov di,recent
    jmp strcpy

; picker with the recently played games -> si = entry, CF=1 if cancelled / empty
pick_recent:
    mov word [pcount],0
    xor bx,bx
.l: cmp bx,NREC
    jae .done
    push bx
    mov ax,13
    mul bx
    add ax,recent
    mov di,ax
    cmp byte [di],0
    je .n
    call find_entry
    jc .n
    add si,13               ; list item: title
    mov bx,[pcount]
    shl bx,1
    mov [pptrs+bx],si
    inc word [pcount]
.n: pop bx
    inc bx
    jmp .l
.done:
    cmp word [pcount],0
    jne .run
    call beep
    stc
    ret
.run:
    mov byte [pleft],0Ah
    mov byte [pright],45h
    mov word [ptitle],str_precent
    mov byte [listmode],2
    mov ax,[pcount]
    mov [lcount],ax
    mov word [lptrs],pptrs
    mov word [lheader],str_precent
    mov word [lrow0],2
    mov word [lrows],PROWS
    mov word [lsel],0
    mov word [ltop],0
.again:
    call run_list
    cmp byte [lact],1
    je .esc
    ja .again
    mov bx,ax
    shl bx,1
    mov si,[pptrs+bx]
    sub si,13
    push si
    mov [keep],si           ; select it in the game list too, if visible
    call find_keep
    pop si
    clc
    ret
.esc:
    stc
    ret
