; cache.asm - GMENU.IDX: the data read from METADATA.INI / NAME.TXT, kept between runs
;
; A game's cached data is used when the date, time and size of its METADATA.INI
; (and of NAME.TXT, if the title came from there) are unchanged; otherwise the files
; are read again and the whole cache is rewritten after the scan.
; GMENU.IDX: 'GI', IDXVER, game directory (66), then per game in directory order:
;   folder (13), stamps (16: METADATA.INI, NAME.TXT: time, date, size), source (1:
;   1 = the title came from NAME.TXT / folder name), title length (1), title,
;   record length (2), far record

IDXVER equ 1
STSZ   equ 16               ; stamps per game
IHSZ   equ 13+STSZ+2        ; entry header

idx_open:                   ; before the scan: GMENU.IDX opened if valid -> [idxh] (0 = none)
    mov word [idxh],0
    mov byte [idxdirty],1
    mov ax,3D00h
    mov dx,f_idx
    call prog_file
    int 21h
    jc .r
    mov [idxh],ax
    mov byte [idxdirty],0
    mov dx,ihbuf
    mov cx,3+66
    call idx_read
    jc .r
    cmp word [ihbuf],'GI'
    jne .bad
    cmp byte [ihbuf+2],IDXVER
    jne .bad
    mov si,ihbuf+3
    mov di,colldir
    mov cx,66
    repe cmpsb
    je .r
.bad:
    call idx_drop
.r: ret

; dx = buffer, cx = count -> CF=1 if the read failed or was short (the cache is dropped)
idx_read:
    mov bx,[idxh]
    test bx,bx
    jz .fail
    mov ah,3Fh
    int 21h
    jc .fail
    cmp ax,cx
    je .r
.fail:
    call idx_drop
    stc
.r: ret

idx_drop:                   ; stop using the cache, it is rewritten after the scan
    mov byte [idxdirty],1
idx_close:
    mov bx,[idxh]
    test bx,bx
    jz .r
    mov ah,3Eh
    int 21h
    mov word [idxh],0
.r: ret

; di = entry, si = '\NAME.EXT' -> stamp (time, date, size: 8 bytes) at [bx]; zeros if missing
file_stamp:
    push di
    push bx
    call game_path
    mov ah,1Ah              ; own DTA: the folder scan's DTA must be kept
    mov dx,fdta
    int 21h
    mov dx,pathbuf
    mov cx,6
    mov ah,4Eh
    int 21h
    pop di
    mov si,fdta+16h
    mov cx,4
    jnc .cp
    mov si,zeros
.cp:
    rep movsw
    mov ah,1Ah
    mov dx,dta
    int 21h
    pop di
    ret

; di = entry -> CF=0 if its title and record came from GMENU.IDX ([stamp] has the
; METADATA.INI stamp; on a hit the NAME.TXT stamp is completed)
idx_lookup:
    cmp word [idxh],0
    je .miss
    push di
    mov dx,ihbuf
    mov cx,IHSZ
    call idx_read
    jc .missp
    mov si,ihbuf            ; same folder, same METADATA.INI
    call stricmp
    test ax,ax
    jne .bad
    mov si,ihbuf+13
    mov di,stamp
    mov cx,8
    repe cmpsb
    jne .bad
    pop di
    push di
    mov al,[ihbuf+13+STSZ]  ; source
    mov [gsrc],al
    test al,al
    jz .ttl
    mov si,str_nametxt      ; title from NAME.TXT: it must be unchanged too
    mov bx,stamp+8
    call file_stamp
    mov si,ihbuf+13+8
    mov di,stamp+8
    mov cx,8
    repe cmpsb
    jne .bad
    pop di
    push di
.ttl:
    mov cl,[ihbuf+IHSZ-1]
    xor ch,ch
    cmp cx,L_NAME
    ja .bad
    lea dx,[di+13]
    add di,cx
    mov byte [di+13],0
    call idx_read
    jc .missp
    mov dx,reclen
    mov cx,2
    call idx_read
    jc .missp
    mov ax,[reclen]
    test ax,ax
    jz .hit
    call alloc_far
    jc .bad
    mov [recseg],dx
    mov bx,[idxh]
    mov cx,[reclen]
    push ds
    mov ds,dx
    xor dx,dx
    mov ah,3Fh
    int 21h
    pop ds
    jc .bad
    cmp ax,[reclen]
    jne .bad
    push es
    mov es,[auxseg]
    mov di,[gslot]
    mov ax,[recseg]
    mov [es:di],ax
    pop es
.hit:
    pop di
    clc
    ret
.bad:
    call idx_drop
.missp:
    pop di
.miss:
    stc
    ret

; [gslot] game: stamps and source -> aux arrays (for the next GMENU.IDX)
keep_stamp:
    push di
    push es
    mov ax,[gslot]          ; game index * 2
    shr ax,1
    mov bx,ax
    mov cl,4
    shl ax,cl
    add ax,A_STAMP
    mov es,[auxseg]
    mov di,ax
    mov si,stamp
    mov cx,STSZ/2
    rep movsw
    cmp word [stamp+2],0    ; METADATA.INI date: 0 if there is none
    jne .ini
    or byte [es:bx+A_FLAGS],4
.ini:
    cmp byte [gsrc],0
    je .r
    or byte [es:bx+A_FLAGS],2
.r: pop es
    pop di
    ret

; after the scan: reads past the last entry (a removed game leaves extra data),
; rewrites GMENU.IDX if anything changed
idx_finish:
    cmp word [idxh],0
    je .w
    mov ah,3Fh
    mov bx,[idxh]
    mov cx,1
    mov dx,ihbuf
    int 21h
    jc .drop
    test ax,ax
    jz .close
.drop:
    mov byte [idxdirty],1
.close:
    call idx_close
.w: cmp byte [idxdirty],0
    je .r
    jmp idx_write
.r: ret

idx_write:
    mov ah,3Ch
    xor cx,cx
    mov dx,f_idx
    call prog_file
    int 21h
    jc .r
    mov [idxh],ax
    mov di,outbuf
    mov ax,'GI'
    stosw
    mov al,IDXVER
    stosb
    mov si,colldir
    mov cx,66
    rep movsb
    call .out
    xor bx,bx               ; bx = game index
.g: cmp bx,[gcount]
    jae .close
    push bx
    mov si,bx
    mov cl,6
    shl si,cl
    add si,entries
    mov di,outbuf
    push si
    mov cx,13
    rep movsb
    mov ax,bx               ; stamps
    mov cl,4
    shl ax,cl
    add ax,A_STAMP
    push ds
    mov ds,[auxseg]
    mov si,ax
    mov cx,STSZ/2
    rep movsw
    mov al,[bx+A_FLAGS]     ; source
    shr al,1
    and al,1
    stosb
    shl bx,1
    mov dx,[bx+A_SEGS]
    pop ds
    pop si
    add si,13               ; title
    push di
    inc di
    call strcpy
    pop bx
    mov ax,di
    sub ax,bx
    dec ax
    mov [bx],al
    xor ax,ax               ; record length
    test dx,dx
    jz .len
    push ds
    push di
    mov ds,dx
    xor di,di
    mov cx,NKEYS-1          ; fields: skip NKEYS-1 terminators
.f: cmp byte [di],0
    lea di,[di+1]
    jne .f
    loop .f
    mov ax,di
    pop di
    pop ds
.len:
    stosw
    mov [reclen],ax
    mov [recseg],dx
    call .out
    mov cx,[reclen]
    jcxz .n
    mov bx,[idxh]
    push ds
    mov ds,[recseg]
    xor dx,dx
    mov ah,40h
    int 21h
    pop ds
.n: pop bx
    inc bx
    jmp .g
.close:
    call idx_close
.r: ret
.out:                       ; outbuf..di -> file
    mov cx,di
    sub cx,outbuf
    mov bx,[idxh]
    mov dx,outbuf
    mov ah,40h
    int 21h
    ret
