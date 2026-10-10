; common.asm - routines shared by GMENUPRG.COM and GMEDIT.COM: strings, the
; program's folder, screen drawing, executable names
; (each program defines progdir, fpath and startdir)

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

strend:                     ; di -> the terminating 0
    cmp byte [di],0
    je .r
    inc di
    jmp strend
.r: ret

; ---------------------------------------------------------------- INI lines
is_eol:                     ; al -> ZF=1 if end of line (0, CR, LF, EOF)
    test al,al
    je .r
    cmp al,13
    je .r
    cmp al,10
    je .r
    cmp al,1Ah
.r: ret

skip_blank:
    cmp byte [si],' '
    je .n
    cmp byte [si],9
    jne .r
.n: inc si
    jmp skip_blank
.r: ret

trim_back:                  ; bx = start, di = end -> di before trailing spaces
    cmp di,bx
    je .r
    cmp byte [di-1],' '
    je .d
    cmp byte [di-1],9
    jne .r
.d: dec di
    jmp trim_back
.r: ret

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

; ---------------------------------------------------------------- screen
put_footer:                 ; si = text, '~' toggles highlight color; out: ah = 70h
    mov ah,70h
.l: lodsb
    test al,al
    jz .r
    cmp al,'~'
    jne .s
    xor ah,04h
    jmp .l
.s: cmp di,4000
    jae .l
    stosw
    jmp .l
.r: mov ah,70h
    ret

; dh = row, dl = column -> di = screen offset
scr_at:
    push ax
    mov al,160
    mul dh
    mov di,ax
    mov al,dl
    xor ah,ah
    shl ax,1
    add di,ax
    pop ax
    ret

put_at:                     ; dh,dl = position, si = text, cx = max length, ah = color
    call scr_at
    jmp put_str

box_title:                  ; dh,dl = position, si = text, cx = max length, ah = color
    call scr_at
    mov al,' '
    stosw
    call put_str
    mov al,' '
    stosw
    ret

; double-line frame; dh,dl = top-left corner, ch,cl = bottom-right corner, ah = color
draw_box:
    push dx
    push bp
    call box_width
    mov al,0C9h
    mov bx,0CDBBh
    call .line
.mid:
    inc dh
    cmp dh,ch
    jae .bot
    mov al,0BAh
    mov bx,20BAh
    call .line
    jmp .mid
.bot:
    mov al,0C8h
    mov bx,0CDBCh
    call .line
    pop bp
    pop dx
    ret
.line:                      ; al = left edge, bh = fill, bl = right edge
    call scr_at
    stosw
    mov al,bh
    push cx
    mov cx,bp
    rep stosw
    pop cx
    mov al,bl
    stosw
    ret

box_width:                  ; bp = cl - dl - 1
    mov bl,cl
    sub bl,dl
    dec bl
    xor bh,bh
    mov bp,bx
    ret

hsep:                       ; separator line inside the frame; dh = row, dl..cl, ah = color
    push bp
    call box_width
    mov al,0C7h
    mov bx,0C4B6h
    call draw_box.line
    pop bp
    ret

; si = text, ah = color, cx = max length, es:di = screen
put_str:
    jcxz .r
.l: lodsb
    test al,al
    jz .r
    stosw
    loop .l
.r: ret

; fill with spaces up to: bx (di target)
fill_to:
    mov al,' '
.l: cmp di,bx
    jae .r
    stosw
    jmp .l
.r: ret

%ifdef TEST
; end of a test: the text screen -> SCREEN.BIN in the program's folder, exit code 3
test_screen:
    mov ah,3Ch
    xor cx,cx
    mov dx,f_screen
    call prog_file
    int 21h
    mov bx,ax
    push ds
    mov ax,0B800h
    mov ds,ax
    mov ah,40h
    mov cx,4000
    xor dx,dx
    int 21h
    pop ds
    mov ah,3Eh
    int 21h
    mov ax,4C03h
    int 21h

f_screen    db 'SCREEN.BIN',0
%endif

def_dir     db '\GMENU\',0     ; progdir if the environment has no program path
