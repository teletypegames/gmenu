; launcher.asm - MENURUN.BAT generation for running selected games

put_drive:                  ; al = drive letter
    stosb
    mov al,':'
    stosb
    mov si,crlf
    jmp strcpy

write_runner:               ; si = filename
    mov [runfile],si
    mov di,outbuf
    mov si,s_echo
    call strcpy
    mov al,[gdrive]
    call put_drive
    mov si,s_cd
    call strcpy
    mov si,[cdpath]
    call strcpy
    mov al,'\'
    stosb
    mov si,[curgame]
    call strcpy
    mov si,crlf
    call strcpy
    mov si,[runfile]
.f: lodsb
    cmp al,'.'
    jne .f
    mov al,[si]
    call upcase
    cmp al,'B'
    jne .nocall
    mov si,s_call
    call strcpy
.nocall:
    mov si,[runfile]
    call strcpy
    mov si,crlf
    call strcpy
    mov al,[hdrive]
    call put_drive
    mov si,s_cdroot
    call strcpy
    mov cx,di
    sub cx,outbuf
    push cx
    mov ah,3Ch
    xor cx,cx
    mov dx,f_runner
    int 21h
    pop cx
    jc .r
    mov bx,ax
    mov ah,40h
    mov dx,outbuf
    int 21h
    mov ah,3Eh
    int 21h
.r: ret
