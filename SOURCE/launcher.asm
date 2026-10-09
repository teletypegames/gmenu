; launcher.asm - command lines (RUN / SETUP / ARGS) and MENURUN.BAT generation

; [curgame], cmdbuf -> MENURUN.BAT, recent list, GMENU.SEL; exit with code 0
launch:
    call write_runner
    call add_play
    call add_recent
    call save_sel
    call restore_screen
    mov ax,4C00h
    int 21h

; si = game entry: run its RUN command; without RUN the only executable;
; otherwise (or if there is none) the detail view opens
play:
    mov [curgame],si
    mov bl,F_RUN
    call get_field
    cmp byte [si],0
    je .one
    mov di,cmdbuf
    call strcpy
    call add_args
    jmp launch
.one:
    mov al,[showall]
    push ax
    mov byte [showall],0
    call scan_files
    pop ax
    mov [showall],al
    cmp word [fcount],1
    jne .detail
    mov si,[fptrs]
    call file_cmd
    jmp launch
.detail:
    mov si,[curgame]
    jmp detail

; si = game entry: run its SETUP command (beep if it has none)
setup:
    mov [curgame],si
    mov bl,F_SETUP
    call get_field
    cmp byte [si],0
    jne .go
    call beep
    jmp game_loop
.go:
    mov di,cmdbuf
    call strcpy
    jmp launch

; si = file name in the game's folder -> cmdbuf = command line that runs it:
; the RUN file gets RUN's arguments and ARGS, the SETUP file SETUP's arguments
file_cmd:
    mov [runfile],si
    mov si,[curgame]
    mov bl,F_RUN
    call get_field
    mov di,[runfile]
    call tok_eq
    jne .setup
    mov di,cmdbuf
    call strcpy
    jmp add_args
.setup:
    mov si,[curgame]
    mov bl,F_SETUP
    call get_field
    mov di,[runfile]
    call tok_eq
    je .cp
    mov si,di
.cp:
    mov di,cmdbuf
    jmp strcpy

add_args:                   ; di = end of cmdbuf: appends ' ' + ARGS (if any)
    push di
    mov si,[curgame]
    mov bl,F_ARGS
    call get_field
    pop di
    cmp byte [si],0
    je .r
    mov al,' '
    stosb
    jmp strcpy
.r: ret

; si = command line, di = file name -> ZF=1 if the first word of si is di (case insensitive)
tok_eq:
    push si
    push di
.l: mov al,[si]
    cmp al,' '
    ja .c
    xor al,al               ; end of the word
.c: call upcase
    mov ah,al
    mov al,[di]
    call upcase
    cmp ah,al
    jne .r
    test ah,ah
    jz .r
    inc si
    inc di
    jmp .l
.r: pop di
    pop si
    ret

; -> ax = index of the RUN file in the file list (0 if it is not listed)
find_runfile:
    mov si,[curgame]
    mov bl,F_RUN
    call get_field
    xor bx,bx
.l: cmp bx,[fcount]
    jae .no
    push bx
    shl bx,1
    mov di,[fptrs+bx]
    pop bx
    call tok_eq
    je .y
    inc bx
    jmp .l
.no:
    xor bx,bx
.y: mov ax,bx
    ret

put_drive:                  ; al = drive letter
    stosb
    mov al,':'
    stosb
    mov si,crlf
    jmp strcpy

write_runner:               ; [curgame], cmdbuf = command line
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
    mov si,cmdbuf           ; a .BAT file is started with call
.f: lodsb
    cmp al,' '
    jbe .nocall
    cmp al,'.'
    jne .f
    mov al,[si]
    call upcase
    cmp al,'B'
    jne .nocall
    mov si,s_call
    call strcpy
.nocall:
    mov si,cmdbuf
    call strcpy
    mov si,crlf
    call strcpy
    mov al,[hdrive]         ; back to the drive and folder of the start
    call put_drive
    mov si,s_cdroot
    call strcpy
    mov si,startdir
    call strcpy
    mov si,crlf
    call strcpy
    mov cx,di
    sub cx,outbuf
    push cx
    mov ah,3Ch
    xor cx,cx
    mov dx,f_runner
    call prog_file
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
