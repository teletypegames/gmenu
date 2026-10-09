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

; shell sort: bx = pointer array, cx = count, dx = compare routine
; (si, di = items -> ax <0 / 0 / >0; must keep bx, cx, si, di)
sort_ptrs:
    mov [s_arr],bx
    mov [s_n],cx
    mov [s_cmp],dx
    mov si,sort_gaps
.gap:
    lodsw
    test ax,ax
    jz .done
    cmp ax,[s_n]
    jae .gap
    push si
    mov [s_i],ax
    shl ax,1
    mov [s_gap],ax          ; gap in bytes
.outer:
    mov ax,[s_i]
    cmp ax,[s_n]
    jae .next
    mov bx,ax
    shl bx,1
    add bx,[s_arr]          ; bx = &a[j]
    mov ax,[bx]
    mov [s_key],ax
    mov cx,[s_gap]
.inner:
    mov ax,bx
    sub ax,[s_arr]
    cmp ax,cx
    jb .place               ; j < gap
    mov di,bx
    sub di,cx
    mov si,[di]             ; a[j-gap] > key: move it up
    mov di,[s_key]
    call [s_cmp]
    test ax,ax
    jle .place
    mov di,bx
    sub di,cx
    mov ax,[di]
    mov [bx],ax
    mov bx,di
    jmp .inner
.place:
    mov ax,[s_key]
    mov [bx],ax
    inc word [s_i]
    jmp .outer
.next:
    pop si
    jmp .gap
.done:
    ret

cmp_title:                  ; si, di = game entries: by title, then by folder
    add si,13
    add di,13
    call stricmp
    sub si,13
    sub di,13
    test ax,ax
    jnz .r
    call stricmp
.r: ret

cmp_year:
    mov byte [sortf],F_YEAR
    jmp cmp_field
cmp_genre:
    mov byte [sortf],F_GENRE
cmp_field:                  ; si, di = game entries: by field [sortf] (empty last), then title
    push bx
    mov [cmp_a],si
    mov [cmp_b],di
    mov bl,[sortf]
    mov si,di
    call get_field
    mov di,cmpbuf
    call strcpy
    mov si,[cmp_a]
    call get_field
    mov di,cmpbuf
    mov al,[si]
    mov ah,[di]
    test al,al
    jz .aempty
    test ah,ah
    jz .first
    call stricmp
    test ax,ax
    jnz .r
.title:
    mov si,[cmp_a]
    mov di,[cmp_b]
    pop bx
    jmp cmp_title
.aempty:
    test ah,ah
    jz .title
    mov ax,1
    jmp .r
.first:
    mov ax,-1
.r: mov si,[cmp_a]
    mov di,[cmp_b]
    pop bx
    ret

; gptrs sorted by [sortmode]
sort_games:
    mov bl,[sortmode]
    xor bh,bh
    shl bx,1
    mov dx,[sort_cmps+bx]
    mov bx,gptrs
    mov cx,[gcount]
    jmp sort_ptrs

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

; folder of this program -> progdir: the path after the environment strings, up to
; the last '\'; the current folder -> startdir
get_progdir:
    mov ah,47h
    xor dl,dl
    mov si,startdir
    int 21h
    jnc .env
    mov byte [startdir],0
.env:
    mov di,progdir
    mov bx,di               ; after the last '\'
    push ds
    mov ds,[2Ch]
    xor si,si
.e: lodsb                   ; skip the strings up to the empty one
    test al,al
    jnz .e
    cmp byte [si],0
    jne .e
    inc si
    lodsw                   ; strings after it (1: program path)
    test ax,ax
    jz .def
.c: lodsb
    stosb
    test al,al
    jz .end
    cmp al,'\'
    jne .n
    mov bx,di
.n: cmp di,progdir+80-14
    jb .c
.def:
    pop ds
    mov si,def_dir
    mov di,progdir
    jmp strcpy
.end:
    pop ds
    cmp bx,progdir
    je .def
    mov byte [bx],0
    ret

prog_file:                  ; dx = file name -> dx = progdir + name (other registers kept)
    push si
    push di
    push ax
    push es
    push ds
    pop es
    mov si,progdir
    mov di,fpath
    call strcpy
    mov si,dx
    call strcpy
    mov dx,fpath
    pop es
    pop ax
    pop di
    pop si
    ret

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

num_str:                    ; ax -> decimal digits at di, di points to the terminating 0
    mov bx,10
    xor cx,cx
.d: xor dx,dx
    div bx
    push dx
    inc cx
    test ax,ax
    jnz .d
.o: pop ax
    add al,'0'
    stosb
    loop .o
    mov byte [di],0
    ret

parse_num:                  ; si = text -> ax = decimal number, si after it; CF=1 if no digit
    xor ax,ax
    xor cx,cx
.l: mov bl,[si]
    sub bl,'0'
    cmp bl,9
    ja .e
    xor bh,bh
    shl ax,1                ; ax = ax * 10 + digit
    mov dx,ax
    shl ax,1
    shl ax,1
    add ax,dx
    add ax,bx
    inc si
    inc cx
    jmp .l
.e: cmp cx,1
    ret

; si = "a", "a-b" or "a+" -> ax = a, dx = b ("a+": FFFFh); CF=1 if it is not a number
parse_range:
    call parse_num
    jc .r
    mov dx,ax
    call skip_blank
    cmp byte [si],'+'
    je .plus
    cmp byte [si],'-'
    jne .ok
    inc si
    call skip_blank
    push ax
    call parse_num
    mov dx,ax
    pop ax
    jnc .ok
    mov dx,ax
.ok:
    clc
.r: ret
.plus:
    mov dx,-1
    jmp .ok
