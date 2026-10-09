; stats.asm - play statistics (GMENU.STA): how many times and when a game was started
;
; GMENU.STA: records of folder (13), starts (2), date of the last start (2, DOS format);
; the records of folders missing from the game directory are kept

STREC  equ 17
MAXSTA equ 1000

load_stats:
    mov word [stan],0
    mov ax,STREC*MAXSTA     ; the file is kept in a far block
    call alloc_far
    jc .r
    mov [staseg],dx
    mov ax,3D00h
    mov dx,f_sta
    call prog_file
    int 21h
    jc .r
    mov bx,ax
    mov cx,STREC*MAXSTA
    push ds
    mov ds,[staseg]
    xor dx,dx
    mov ah,3Fh
    int 21h
    pop ds
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop ax
    popf
    jc .r
    xor dx,dx
    mov cx,STREC
    div cx
    mov [stan],ax
    mov cx,ax
    jcxz .r
    xor bx,bx               ; bx = record offset
.l: push cx
    push bx
    push ds
    mov si,bx
    mov ds,[staseg]
    mov di,ihbuf
    mov cx,STREC
    rep movsb
    pop ds
    mov byte [ihbuf+12],0
    mov di,ihbuf
    call find_entry
    jc .n
    call fav_index
    shl bx,1
    mov ax,[ihbuf+13]
    mov dx,[ihbuf+15]
    push es
    mov es,[auxseg]
    mov [es:bx+A_PLAYS],ax
    mov [es:bx+A_LAST],dx
    pop es
    pop bx
    push bx
    push es
    mov es,[staseg]
    mov byte [es:bx],0      ; known game: saved from the aux arrays
    pop es
.n: pop bx
    pop cx
    add bx,STREC
    loop .l
.r: ret

save_stats:
    mov ah,3Ch
    xor cx,cx
    mov dx,f_sta
    call prog_file
    int 21h
    jc .r
    mov [stah],ax
    xor bx,bx               ; bx = game index
.g: cmp bx,[gcount]
    jae .old
    push bx
    shl bx,1
    push es
    mov es,[auxseg]
    mov ax,[es:bx+A_PLAYS]
    mov dx,[es:bx+A_LAST]
    pop es
    test ax,ax
    jz .n
    mov [ihbuf+13],ax
    mov [ihbuf+15],dx
    mov cl,5
    shl bx,cl
    lea si,[bx+entries]
    mov di,ihbuf
    mov cx,13
    rep movsb
    mov dx,ihbuf
    call .write
.n: pop bx
    inc bx
    jmp .g
.old:                       ; records of other folders
    mov cx,[stan]
    jcxz .close
    xor dx,dx
.o: push cx
    push dx
    push ds
    mov ds,[staseg]
    mov bx,dx
    cmp byte [bx],0
    je .skip
    call .write
.skip:
    pop ds
    pop dx
    pop cx
    add dx,STREC
    loop .o
.close:
    mov bx,[cs:stah]
    mov ah,3Eh
    int 21h
.r: ret
.write:                     ; ds:dx -> one record
    mov bx,[cs:stah]
    mov cx,STREC
    mov ah,40h
    int 21h
    ret

; [curgame]: one more start, today
add_play:
    mov si,[curgame]
    call fav_index
    shl bx,1
    mov ah,2Ah              ; date: cx = year, dh = month, dl = day
    int 21h
    sub cx,1980
    mov al,dh
    xor ah,ah
    push cx
    mov cl,5
    shl ax,cl
    pop cx
    or al,dl
    mov dx,cx
    mov cl,9
    shl dx,cl
    or ax,dx
    push es
    mov es,[auxseg]
    inc word [es:bx+A_PLAYS]
    mov [es:bx+A_LAST],ax
    pop es
    jmp save_stats

; si = entry -> ax = starts, dx = date of the last start (bx clobbered)
get_plays:
    call fav_index
    shl bx,1
    push es
    mov es,[auxseg]
    mov ax,[es:bx+A_PLAYS]
    mov dx,[es:bx+A_LAST]
    pop es
    ret

; [curgame] -> si = "5x, last 2026-10-09" or "never" (es, di preserved)
played_str:
    push es
    push di
    push ds
    pop es
    mov si,[curgame]
    call get_plays
    mov si,str_never
    test ax,ax
    jz .r
    mov di,outbuf
    push dx
    call num_str
    mov si,str_times
    call strcpy
    pop dx
    mov ax,dx
    mov cl,9
    shr ax,cl
    add ax,1980
    push dx
    call num_str
    pop dx
    mov al,'-'
    stosb
    mov ax,dx
    mov cl,5
    shr ax,cl
    and al,0Fh
    call .num2
    mov al,'-'
    stosb
    mov al,dl
    and al,1Fh
    call .num2
    mov byte [di],0
    mov si,outbuf
.r: pop di
    pop es
    ret
.num2:                      ; al < 100 -> two digits
    aam
    add ax,'00'
    xchg al,ah
    stosw
    ret

; sort: most starts first, then the latest start, then title
cmp_plays:
    push bx
    push si
    call get_plays
    mov [cmp_a],ax
    mov [cmp_b],dx
    mov si,di
    call get_plays
    pop si
    pop bx
    cmp [cmp_a],ax
    ja .first
    jb .second
    cmp [cmp_b],dx
    ja .first
    jb .second
    jmp cmp_title
.first:
    mov ax,-1
    ret
.second:
    mov ax,1
    ret
