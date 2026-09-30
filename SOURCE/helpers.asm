; helpers.asm - string utilities, sorting, argument parsing

strcpy:                     ; si -> di, di points to the terminating 0
    lodsb
    stosb
    test al,al
    jnz strcpy
    dec di
    ret

upcase:
    cmp al,'a'
    jb .r
    cmp al,'z'
    ja .r
    sub al,32
.r: ret

stricmp:                    ; si vs di -> ax <0 / 0 / >0
    push si
    push di
.l: mov al,[si]
    inc si
    call upcase
    mov ah,al
    mov al,[di]
    inc di
    call upcase
    cmp ah,al
    jne .diff
    test ah,ah
    jnz .l
    xor ax,ax
    jmp .r
.diff:
    mov ax,1
    ja .r
    mov ax,-1
.r: pop di
    pop si
    ret

; insertion sort: bx = pointer array, cx = count, dx = key offset
sort_ptrs:
    mov [s_arr],bx
    mov [s_off],dx
    mov [s_n],cx
    mov word [s_i],1
.outer:
    mov ax,[s_i]
    cmp ax,[s_n]
    jae .done
    mov si,ax
    shl si,1
    add si,[s_arr]
    mov ax,[si]
    mov [s_key],ax
.inner:
    cmp si,[s_arr]
    je .place
    mov di,[si-2]
    add di,[s_off]
    push si
    mov si,[s_key]
    add si,[s_off]
    call stricmp
    pop si
    test ax,ax
    jns .place
    mov ax,[si-2]
    mov [si],ax
    sub si,2
    jmp .inner
.place:
    mov ax,[s_key]
    mov [si],ax
    inc word [s_i]
    jmp .outer
.done:
    ret

; ---------------------------------------------------------------- arguments
; GMENUPRG.COM /B [/A] [directory]   e.g. /B \COLLECT  or  /B /A D:\GAMES
parse_args:
    mov byte [argb],0
    mov byte [arga],0
    mov byte [argbuf],0
    mov bl,[80h]
    xor bh,bh
    mov byte [81h+bx],0
    mov si,81h
.next:
    lodsb
    test al,al
    jz .done
    cmp al,' '
    jbe .next
    cmp al,'/'
    jne .path
    lodsb
    call upcase
    cmp al,'B'
    jne .noB
    mov byte [argb],1
.noB:
    cmp al,'A'
    jne .skip
    mov byte [arga],1
.skip:
    lodsb
    cmp al,' '
    ja .skip
    dec si
    jmp .next
.path:
    dec si
    mov di,argbuf
.cp:
    lodsb
    cmp al,' '
    jbe .etok
    cmp di,argbuf+60
    jae .cp
    call upcase
    stosb
    jmp .cp
.etok:
    dec si
    mov byte [di],0
    jmp .next
.done:
    ; build colldir: [X:] + '\' + path, without trailing '\'
    mov ah,19h
    int 21h
    add al,'A'
    mov [hdrive],al
    mov [gdrive],al
    mov si,argbuf
    mov di,colldir
    cmp byte [si],0
    je .r                   ; not specified: keep the default
    cmp byte [si+1],':'
    jne .nodrv
    lodsb
    mov [gdrive],al
    stosb
    movsb
.nodrv:
    cmp byte [si],'\'
    je .abs
    mov al,'\'
    stosb
.abs:
    call strcpy
.trim:
    cmp di,colldir
    je .r
    cmp byte [di-1],'\'
    jne .r
    dec di
    mov byte [di],0
    jmp .trim
.r:
    mov si,colldir          ; path without drive letter for cd
    cmp byte [si+1],':'
    jne .c
    add si,2
.c: mov [cdpath],si
    mov di,hdr_games        ; header: "Games  (directory)"
    mov si,str_games
    call strcpy
    mov si,str_lpar
    call strcpy
    mov si,colldir
    call strcpy
    mov si,str_rpar
    jmp strcpy

put_colldir:                ; di <- colldir + '\'
    mov si,colldir
    call strcpy
    mov al,'\'
    stosb
    mov byte [di],0
    ret

strend:                     ; di -> the terminating 0
    cmp byte [di],0
    je .r
    inc di
    jmp strend
.r: ret
