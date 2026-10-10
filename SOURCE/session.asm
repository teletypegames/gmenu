; session.asm - save/load last position and filters (GMENU.SEL)

; GMENU.SEL: 'GS' + version (1) + folder name (13) + search length (1) + search (32)
;            + filters: genre (25), year (11), players (11) + all-files flag (1)
;            + favorites-only flag (1) + sort order (1) + without-METADATA.INI-only flag (1)
;            + recently played folder names (NREC * 13)
; A file with another magic, version or size is ignored (format of an older build).
SELVER equ 4
SELSZ equ 13+1+32+(L_GENRE+1)+(L_YEAR+1)+(L_PLAYERS+1)+1+1+1+1+NREC*13

load_sel:
    mov ax,3D00h
    mov dx,f_sel
    call prog_file
    int 21h
    jc .apply
    mov bx,ax
    mov ah,3Fh
    mov cx,SELSZ+4          ; one more byte than expected: detects a longer file
    mov dx,ibuf
    int 21h
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop ax
    popf
    jc .apply
    cmp ax,SELSZ+3
    jne .apply
    cmp word [ibuf],'GS'
    jne .apply
    cmp byte [ibuf+2],SELVER
    jne .apply
    mov si,ibuf+3
    mov di,selname
    mov cx,SELSZ
    rep movsb
    cmp byte [flen],30
    jbe .v
    mov byte [flen],0
.v: mov byte [selname+12],0
    mov byte [fgenre+L_GENRE],0
    mov byte [fyear+L_YEAR],0
    mov byte [fplayers+L_PLAYERS],0
    and byte [showall],1
    and byte [favonly],1
    and byte [noini],1
    mov di,recent+12
    mov cx,NREC
.rt:
    mov byte [di],0
    add di,13
    loop .rt
    cmp byte [sortmode],NSORT
    jb .apply
    mov byte [sortmode],0
.apply:
    call sort_games
    mov bl,[flen]
    xor bh,bh
    mov byte [fbuf+bx],0
    call apply_filter
    xor bx,bx               ; search for folder name in the filtered list
.f: cmp bx,[vcount]
    jae .r
    push bx
    shl bx,1
    mov si,[vptrs+bx]
    mov di,selname
    call stricmp
    pop bx
    test ax,ax
    jz .found
    inc bx
    jmp .f
.found:
    mov [gsel],bx
    sub bx,GROWS/2
    jns .t
    xor bx,bx
.t: mov [gtop],bx
.r: ret

save_sel:
    mov byte [selname],0
    cmp word [vcount],0
    je .w
    mov bx,[gsel]
    shl bx,1
    mov si,[vptrs+bx]
    mov di,selname
    call strcpy
.w: mov ah,3Ch
    xor cx,cx
    mov dx,f_sel
    call prog_file
    int 21h
    jc .r
    mov bx,ax
    mov ah,40h
    mov cx,SELSZ+3
    mov dx,selhdr
    int 21h
    mov ah,3Eh
    int 21h
.r: ret
