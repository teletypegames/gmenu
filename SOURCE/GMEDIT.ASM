; GMEDIT.COM - creates or edits the METADATA.INI of a game folder (DOS / DOSBox)
; Build: build.sh (by hand: nasm -f bin -o ../GMEDIT.COM GMEDIT.ASM)
; Launch: GMEDIT.COM [/B] <game folder>    e.g. GMEDIT C:\COLLECT\DOOM
;         GMENUPRG.COM starts it through GMENURUN.BAT (Ctrl+E) with /B: an error message
;         then waits for a key, otherwise the menu would clear it at once
; Exit code: 0 = METADATA.INI written, 1 = cancelled or nothing changed, 2 = error
;
; A new file's form starts with what the folder tells: NAME from NAME.TXT (or the folder
; name), RUN if the folder has one executable besides the setup programs, SETUP from the
; first SETUP* / INSTALL* executable. An existing file is changed line by line: only the
; lines of the changed values are rewritten (a value's first line; its further lines, e.g.
; more DESCRIPTION= lines, are dropped), the other lines (comments, other keys, the
; values as they were, a BOM) are kept. A new value is written in UTF-8; an emptied value
; loses its line. The file is written as METADATA.$$$, then renamed.

bits 16
org 100h

%include "limits.inc"
NFLD     equ 8              ; form fields
FSIZE    equ 16             ; field table entry
IBUFSZ   equ 4096           ; NAME.TXT read buffer, a value converted to CP437
RAWSZ    equ 8192           ; largest METADATA.INI that can be edited
MAXX     equ 100            ; executables kept
XCOLS    equ 5              ; executables shown: columns x rows, column width
XROWS    equ 5
XW       equ 14
VCOL     equ 16             ; first column of the values
F_DESC   equ 4              ; field index of DESCRIPTION

; field table entry: INI key, label, label position, value buffer, max length,
; value position (row << 8 | column), shown width, hint
FK  equ 0
FL  equ 2
FLP equ 4
FB  equ 6
FM  equ 8
FP  equ 10
FW  equ 12
FH  equ 14
%macro FIELD 8
    dw %1,%2,%3,%4,%5,%6,%7,%8
%endmacro

start:
    cld
    mov di,bss_start        ; a .COM file's BSS is not cleared by DOS
    mov cx,bss_end-bss_start
    xor al,al
    rep stosb
    call get_progdir
    call parse_arg
    mov dx,msg_usage
    jc fail
    mov ah,1Ah              ; own DTA: the default one is the command line (parsed already)
    mov dx,dta
    int 21h
    mov dx,gdir             ; the folder must exist
    mov cx,10h
    mov ah,4Eh
    int 21h
    jc .nodir
    test byte [dta+15h],10h
    jnz .dir
.nodir:
    mov dx,msg_nodir
    jmp fail
.dir:
    call load_ini           ; an existing METADATA.INI -> the fields
    mov si,fbufs            ; the values as they were: only the changed ones are written
    mov di,orig
    mov cx,ORIGOFF
    rep movsb
    call scan_exes
    cmp byte [isold],0
    jne .form
    call prefill            ; a new file: starting values from the folder
.form:
    mov ax,3
    int 10h
    xor ax,ax
    call set_field
    jmp edit

fail:                       ; dx = message ('$'), exit code 2 (started by the menu: after a key)
    mov ah,9
    int 21h
    cmp byte [argb],0
    je .x
    mov dx,msg_key
    mov ah,9
    int 21h
    call get_key
.x: mov ax,4C02h
    int 21h

cancel:
    mov al,1
exit:                       ; al = exit code
    push ax
    mov ax,3
    int 10h
    pop ax
    mov ah,4Ch
    int 21h

beep:
    mov ah,2
    mov dl,7
    int 21h
    ret

; command line: /B -> [argb] = 1; the first argument that is not a /switch -> gdir
; (upper case, without a trailing '\'); CF=1 if there is none
parse_arg:
    mov bl,[80h]
    xor bh,bh
    mov byte [81h+bx],0
    mov si,81h
.next:
    lodsb
    test al,al
    jz .none
    cmp al,' '
    jbe .next
    cmp al,'/'
    jne .path
    lodsb
    call upcase
    cmp al,'B'
    jne .sw
    mov byte [argb],1
.sw:
    dec si
.skip:
    lodsb
    cmp al,' '
    ja .skip
    dec si
    jmp .next
.path:
    dec si
    mov di,gdir
.cp:
    lodsb
    cmp al,' '
    jbe .trim
    cmp di,gdir+64
    jae .cp
    call upcase
    stosb
    jmp .cp
.trim:
    mov byte [di],0
    cmp di,gdir+1
    jbe .ok
    dec di
    cmp byte [di],'\'
    je .trim
.ok:
    clc
    ret
.none:
    stc
    ret

folder_file:                ; si = '\NAME.EXT' -> pathbuf = game folder + it
    mov di,pathbuf
folder_to:                  ; si = '\NAME.EXT', di = buffer -> game folder + it
    push si
    mov si,gdir
    call strcpy
    pop si
    jmp strcpy

; ---------------------------------------------------------------- METADATA.INI
; an existing METADATA.INI -> rawbuf, [isold] = 1, its values -> the fields
load_ini:
    mov si,str_metaini
    call folder_file
    mov ax,3D00h
    mov dx,pathbuf
    int 21h
    jc .r
    mov bx,ax
    mov ah,3Fh
    mov cx,RAWSZ
    mov dx,rawbuf
    int 21h
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop cx
    popf
    mov dx,msg_read
    jc fail
    mov dx,msg_big
    cmp cx,RAWSZ
    jae fail
    mov bx,cx
    mov byte [rawbuf+bx],0
    mov byte [isold],1
    call raw_start
.l: call scan_line
    jc .r
    mov bx,[lfld]
    test bx,bx
    js .l
    call load_value
    jmp .l
.r: ret

raw_start:                  ; -> si = rawbuf, after the UTF-8 BOM
    mov si,rawbuf
    cmp word [si],0BBEFh
    jne .r
    cmp byte [si+2],0BFh
    jne .r
    add si,3
.r: ret

; si = a line in rawbuf -> [lbeg]..[lend] = the line (without CR / LF), [lfld] = the field
; of a "KEY = value" line (-1: another line), [vbeg] / [vlen] = its value (trimmed, without
; surrounding quotes), si = the next line; CF=1 at the end of the file
scan_line:
    push di
    mov [lbeg],si
    mov al,[si]
    test al,al
    jz .eof
    cmp al,1Ah
    je .eof
    mov di,si
.e: mov al,[di]
    call is_eol
    je .le
    inc di
    jmp .e
.le:
    mov [lend],di
    cmp al,13               ; CR, LF or CR LF
    jne .lf
    inc di
    mov al,[di]
.lf:
    cmp al,10
    jne .nx
    inc di
.nx:
    push di
    mov word [lfld],-1
    call skip_blank
    mov al,[si]
    cmp al,';'
    je .done
    cmp al,'#'
    je .done
    cmp al,'['
    je .done
    mov bx,si               ; the key: bx..di
.k: cmp si,[lend]
    je .done
    cmp byte [si],'='
    je .key
    inc si
    jmp .k
.key:
    mov di,si
    call trim_back
    call find_field
    mov [lfld],ax
    inc si
    call skip_blank
    mov bx,si
    mov di,[lend]
    call trim_back
    mov cx,di
    sub cx,bx
    cmp cx,2                ; strip surrounding "quotes"
    jb .v
    cmp byte [bx],'"'
    jne .v
    cmp byte [di-1],'"'
    jne .v
    inc bx
    sub cx,2
.v: mov [vbeg],bx
    mov [vlen],cx
.done:
    pop si
    pop di
    clc
    ret
.eof:
    pop di
    stc
    ret

find_field:                 ; bx..di = key -> ax = field index (case insensitive), or -1
    push si
    push bp
    mov bp,fields
    xor dx,dx
.f: mov si,[bp+FK]
    push bx
.c: cmp bx,di
    je .end
    mov al,[bx]
    call upcase
    cmp al,[si]
    jne .no
    inc bx
    inc si
    jmp .c
.end:
    cmp byte [si],0
    jne .no
    pop bx
    mov ax,dx
    jmp .r
.no:
    pop bx
    add bp,FSIZE
    inc dx
    cmp dx,NFLD
    jb .f
    mov ax,-1
.r: pop bp
    pop si
    ret

; bx = field index, [vbeg] / [vlen] = value -> the field (in CP437, clipped); a further
; DESCRIPTION line is appended after \n (as the menu reads it: not after an empty start)
load_value:
    push si
    push bx
    mov si,[vbeg]
    mov cx,[vlen]
    cmp cx,IBUFSZ-1
    jbe .c
    mov cx,IBUFSZ-1
.c: mov di,ibuf
    rep movsb
    mov byte [di],0
    call utf8_437
    pop bx
    push bx
    mov cl,4
    shl bx,cl
    add bx,fields
    mov di,[bx+FB]
    mov dx,di
    add dx,[bx+FM]          ; dx = the end of the room
    cmp bx,fields+F_DESC*FSIZE
    jne .cp
    call strend
    cmp di,[bx+FB]
    je .cp
    lea cx,[di+2]
    cmp cx,dx
    ja .z
    mov ax,'\n'
    stosw
.cp:
    mov si,ibuf
.l: cmp di,dx
    jae .z
    lodsb
    test al,al
    jz .z
    stosb
    jmp .l
.z: mov byte [di],0
    pop bx
    pop si
    ret

; ---------------------------------------------------------------- starting values
scan_exes:
    mov si,str_starf        ; the executables, sorted
    call folder_file
    mov dx,pathbuf
    xor cx,cx
    mov ah,4Eh
    int 21h
    jc .name
.f: mov si,dta+1Eh
    call is_exec
    jne .n
    call add_exe
.n: mov ah,4Fh
    int 21h
    jnc .f
.name:
    ret

prefill:
    mov si,str_nametxt      ; NAME: the first line of NAME.TXT
    call folder_file
    mov ax,3D00h
    mov dx,pathbuf
    int 21h
    jc .folder
    mov bx,ax
    mov ah,3Fh
    mov cx,IBUFSZ-1
    mov dx,ibuf
    int 21h
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop cx
    popf
    jc .folder
    mov bx,cx
    mov byte [ibuf+bx],0
    call utf8_437
    mov si,ibuf
    call skip_bom
    call first_line
    cmp byte [b_name],0
    jne .run
.folder:                    ; no NAME.TXT (or it is empty): the folder name
    mov si,gdir
    mov bx,si
.fl: lodsb
    test al,al
    jz .fc
    cmp al,'\'
    je .sep
    cmp al,':'
    jne .fl
.sep:
    mov bx,si
    jmp .fl
.fc:
    mov si,bx
    mov di,b_name
    call strcpy
.run:                       ; RUN: the only executable that is not a setup program,
    xor dx,dx               ; SETUP: the first setup program
    mov si,xnames
    mov cx,[xcount]
    jcxz .r
.x: call is_setup
    jne .play
    cmp byte [b_setup],0
    jne .nx
    push si
    mov di,b_setup
    call strcpy
    pop si
    jmp .nx
.play:
    inc dx
    mov bx,si
.nx:
    add si,13
    loop .x
    cmp dx,1
    jne .r
    mov si,bx
    mov di,b_run
    call strcpy
.r: ret

; si = text, cx = length -> b_name = its first line (tabs as spaces, without leading and
; trailing spaces, max L_NAME characters)
first_line:
    mov di,b_name
    xor dx,dx
.c: jcxz .end
    lodsb
    dec cx
    test al,al
    jz .end
    cmp al,13
    je .end
    cmp al,10
    je .end
    cmp al,1Ah
    je .end
    cmp al,9
    jne .nt
    mov al,' '
.nt:
    cmp al,' '
    jne .s
    test dx,dx              ; skip leading spaces
    jz .c
.s: cmp dx,L_NAME
    jae .end
    stosb
    inc dx
    jmp .c
.end:
    test dx,dx
    jz .z
    cmp byte [di-1],' '
    jne .z
    dec di
    dec dx
    jmp .end
.z: mov byte [di],0
    ret

; si = file name -> ZF=1 if it is a setup program (SETUP* or INSTALL*); si, bx..dx kept
is_setup:
    mov di,str_setup
    call prefix
    je .r
    mov di,str_install
    call prefix
.r: ret

prefix:                     ; si = name, di = prefix -> ZF=1 if the name starts with it
    push si
.l: mov al,[di]
    test al,al
    jz .r
    inc di
    cmp al,[si]
    jne .r
    inc si
    jmp .l
.r: pop si
    ret

; the name in the DTA (an executable) -> xnames, sorted (skipped when the list is full)
add_exe:
    mov si,dta+1Eh          ; the name, padded with zeros -> xnew
    mov di,xnew
    mov cx,13
.cp:
    lodsb
    stosb
    test al,al
    loopnz .cp
    rep stosb
    mov ax,[xcount]
    cmp ax,MAXX
    jae .r
    mov bx,xnames           ; bx = its place: before the first name greater than it
    mov dx,ax
.f: test dx,dx
    jz .ins
    mov si,xnew
    mov di,bx
    mov cx,13
    repe cmpsb
    jb .ins
    add bx,13
    dec dx
    jmp .f
.ins:
    mov cx,13               ; the names from its place move one place up
    mul cx
    add ax,xnames
    mov cx,ax
    sub cx,bx
    mov si,ax
    dec si
    lea di,[si+13]
    std
    rep movsb
    cld
    mov si,xnew
    mov di,bx
    mov cx,13
    rep movsb
    inc word [xcount]
.r: ret

; ---------------------------------------------------------------- editing
set_field:                  ; ax = field index: the cursor goes to the end of its value
    mov [fcur],ax
    mov cl,4
    shl ax,cl
    add ax,fields
    mov [fptr],ax
    mov bx,ax
    mov di,[bx+FB]
    push di
    call strend
    pop ax
    sub di,ax
    mov [ecur],di
    mov word [escr],0
    ret

edit:
    call draw
    call get_key
    mov word [msg],0
    mov bx,[fptr]
    mov si,[bx+FB]          ; si = value, di = cursor
    mov di,[ecur]
    cmp al,27
    je cancel
    cmp al,9
    je .next
    cmp al,13
    je .next
    cmp al,8
    je .bksp
    cmp al,19h              ; Ctrl+Y: clear the value
    je .clear
    test al,al
    jz .ext
    cmp al,0E0h             ; E0h with a scan code: grey key; without: the character
    jne .char
    test ah,ah
    jnz .ext
.char:
    cmp al,' '
    jb edit
    cmp al,7Fh
    je edit
    call ins_char
    jmp edit
.ext:
    cmp ah,0Fh              ; Shift+Tab
    je .prev
    cmp ah,48h
    je .prev
    cmp ah,50h
    je .next
    cmp ah,4Bh
    je .left
    cmp ah,4Dh
    je .right
    cmp ah,47h
    je .home
    cmp ah,4Fh
    je .end
    cmp ah,53h
    je .del
    cmp ah,3Ch              ; F2: save
    jne edit
    jmp save
.prev:
    mov ax,[fcur]
    dec ax
    jns .set
    mov ax,NFLD-1
    jmp .set
.next:
    mov ax,[fcur]
    inc ax
    cmp ax,NFLD
    jb .set
    xor ax,ax
.set:
    call set_field
    jmp edit
.left:
    test di,di
    jz edit
    dec word [ecur]
    jmp edit
.right:
    add si,di
    cmp byte [si],0
    je edit
    inc word [ecur]
    jmp edit
.home:
    mov word [ecur],0
    jmp edit
.end:
    mov ax,[fcur]
    jmp .set
.bksp:
    test di,di
    jz edit
    dec di
    mov [ecur],di
.del:                       ; the character at si + di is removed
    add si,di
    cmp byte [si],0
    je edit
    mov di,si
    inc si
    call strcpy
    jmp edit
.clear:
    mov byte [si],0
    mov word [ecur],0
    jmp edit

ins_char:                   ; al = character -> inserted at the cursor (beep if the value is full)
    mov bx,[fptr]
    mov si,[bx+FB]
    mov di,si
    call strend
    mov cx,di
    sub cx,si
    cmp cx,[bx+FM]
    jae beep
    mov bx,si
    add bx,[ecur]           ; bx = cursor
    mov cx,di               ; the characters from the cursor and the 0 move up by one
    sub cx,bx
    inc cx
    mov si,di
    inc di
    std
    rep movsb
    cld
    mov [bx],al
    inc word [ecur]
    ret

get_key:                    ; ax = BIOS key code
%ifdef TEST
    mov si,[tkp]
    lodsw
    test ax,ax
    jnz .k
    jmp test_screen
.k: mov [tkp],si
    ret
%endif
    xor ax,ax
    int 16h
    ret

; ---------------------------------------------------------------- saving
save:
    mov bp,fields           ; state: 0 = unchanged, 1 = changed, 2 = written
    xor bx,bx
.st:
    mov si,[bp+FB]
    lea di,[si+ORIGOFF]
.cm:
    lodsb
    scasb
    jne .chg
    test al,al
    jnz .cm
    jmp .ss
.chg:
    mov al,1
.ss:
    mov [state+bx],al
    add bp,FSIZE
    inc bx
    cmp bx,NFLD
    jb .st
    mov di,outbuf
    cmp byte [isold],0
    jne .old
    mov si,s_game
    call strcpy
    jmp .rest
.old:
    push di                 ; nothing changed: nothing to write
    mov di,state
    mov cx,NFLD
    mov al,1
    repne scasb
    pop di
    jne cancel
    call raw_start
    mov cx,si               ; the BOM is kept
    sub cx,rawbuf
    push si
    mov si,rawbuf
    rep movsb
    pop si
.l: call scan_line
    jc .rest
    mov bx,[lfld]
    test bx,bx
    js .keep
    mov al,[state+bx]
    test al,al
    jz .keep
    dec al
    jnz .l                  ; written already: a further line of the key is dropped
    mov byte [state+bx],2
    call put_field
    jmp .l
.keep:
    push si
    mov si,[lbeg]
    mov cx,[lend]
    sub cx,si
    rep movsb
    pop si
    mov ax,0A0Dh
    stosw
    jmp .l
.rest:                      ; the changed values the file had no line for
    xor bx,bx
.r: cmp byte [state+bx],1
    jne .rn
    call put_field
.rn:
    inc bx
    cmp bx,NFLD
    jb .r
    push di
    mov si,str_tmp
    call folder_file
    mov si,str_metaini
    mov di,pathini
    call folder_to
    pop cx
    sub cx,outbuf
    push cx
    mov ah,3Ch              ; METADATA.$$$
    xor cx,cx
    mov dx,pathbuf
    int 21h
    pop cx
    jc .err
    mov bx,ax
    mov ah,40h
    mov dx,outbuf
    int 21h
    jc .werr
    cmp ax,cx
    jne .werr
    mov ah,3Eh
    int 21h
    jc .del
    mov ah,41h              ; METADATA.$$$ -> METADATA.INI
    mov dx,pathini
    int 21h
    mov ah,56h
    mov dx,pathbuf
    mov di,pathini
    int 21h
    jc .err
    xor al,al
    jmp exit
.werr:
    mov ah,3Eh
    int 21h
.del:                       ; a partly written file is deleted
    mov ah,41h
    mov dx,pathbuf
    int 21h
.err:
    mov word [msg],msg_write
    call beep
    jmp edit

put_field:                  ; bx = field index -> di: "KEY=value" line (none if the value is blank)
    push si
    push bx
    mov cl,4
    shl bx,cl
    add bx,fields
    mov si,[bx+FB]
    call is_blank
    je .r
    push si
    mov si,[bx+FK]
    call strcpy
    mov al,'='
    stosb
    pop si
    call put_utf8
    mov ax,0A0Dh
    stosw
.r: pop bx
    pop si
    ret

is_blank:                   ; si = text -> ZF=1 if it is empty or spaces only
    push si
.l: lodsb
    cmp al,' '
    je .l
    test al,al
    pop si
    ret

; si = text in code page 437 -> di = the text in UTF-8 (without the terminating 0)
put_utf8:
    push bx
.l: lodsb
    test al,al
    jz .r
    jns .s
    mov bl,al
    xor bh,bh
    shl bx,1
    mov ax,[c2u-100h+bx]
    mov dx,ax
    cmp ax,800h
    jae .three
    mov cl,6                ; 110xxxxx 10xxxxxx
    shr ax,cl
    or al,0C0h
    jmp .last
.three:
    mov cl,12               ; 1110xxxx 10xxxxxx 10xxxxxx
    shr ax,cl
    or al,0E0h
    stosb
    mov ax,dx
    mov cl,6
    shr ax,cl
    and al,3Fh
    or al,80h
.last:
    stosb
    mov al,dl
    and al,3Fh
    or al,80h
.s: stosb
    jmp .l
.r: pop bx
    ret

; ---------------------------------------------------------------- drawing
draw:
    push es
    mov ax,0B800h
    mov es,ax
    xor di,di               ; header: the folder
    mov ah,70h
    mov si,str_head
    cmp byte [isold],0
    je .hd
    mov si,str_heade
.hd:
    mov cx,20
    call put_str
    mov si,gdir
    mov cx,60
    call put_str
    mov bx,160
    call fill_to
    mov ah,13h
    mov dx,0100h
    mov cx,174Fh
    call draw_box
    mov si,str_form
    mov dx,0102h
    mov cx,30
    mov ah,1Eh
    call box_title
    mov bx,fields
    xor cx,cx
.f: call draw_field
    add bx,FSIZE
    inc cx
    cmp cx,NFLD
    jb .f
    mov ah,13h              ; the executables of the folder
    mov dx,0F00h
    mov cl,79
    call hsep
    mov si,str_exes
    mov dx,0F02h
    mov cx,20
    mov ah,1Eh
    call box_title
    cmp word [xcount],0
    jne .xl
    mov si,str_noexe
    mov dx,1003h
    mov cx,20
    mov ah,17h
    call put_at
.xl:
    mov si,xnames
    xor bx,bx
.x: cmp bx,[xcount]
    jae .hint
    cmp bx,XCOLS*XROWS
    jae .hint
    mov ax,bx               ; row 16 + index / XCOLS, column 3 + index % XCOLS * XW
    mov dl,XCOLS
    div dl
    mov dh,al
    add dh,10h
    mov cl,ah
    mov al,XW
    mul cl
    add al,3
    mov dl,al
    push si
    mov cx,12
    mov ah,1Fh
    call put_at
    pop si
    add si,13
    inc bx
    jmp .x
.hint:
    mov ah,13h              ; the hint of the field, or the error
    mov dx,1500h
    mov cl,79
    call hsep
    mov bx,[fptr]
    mov si,[bx+FH]
    mov ah,17h
    cmp word [msg],0
    je .h
    mov si,[msg]
    mov ah,1Eh
.h: mov dx,1603h
    mov cx,74
    call put_at
    mov di,24*160           ; footer
    mov ax,7020h
    stosw
    mov si,footer
    call put_footer
    mov bx,4000
    call fill_to
    mov bx,[fptr]           ; the cursor
    mov dx,[bx+FP]
    mov ax,[ecur]
    sub ax,[escr]
    add dl,al
    mov ah,2
    xor bh,bh
    int 10h
    pop es
    ret

; bx = field entry, cx = its index: label and value (the active one scrolled to the cursor)
draw_field:
    push cx
    mov dx,[bx+FLP]
    mov si,[bx+FL]
    mov cx,VCOL-4
    mov ah,1Bh
    call put_at
    pop cx
    mov dx,[bx+FP]
    call scr_at
    mov si,[bx+FB]
    mov bp,[bx+FW]
    mov ah,07h
    cmp cx,[fcur]
    jne .v
    mov ax,[ecur]           ; escr: the cursor must be within the width
    cmp ax,[escr]
    jae .a
    mov [escr],ax
.a: sub ax,[escr]
    cmp ax,bp
    jb .s
    mov ax,[ecur]
    sub ax,bp
    inc ax
    mov [escr],ax
.s: add si,[escr]
    mov ah,70h
.v: push cx
    mov cx,bp
.c: lodsb
    test al,al
    jz .pad
    stosw
    loop .c
    pop cx
    ret
.pad:
    mov al,' '
    rep stosw
    pop cx
    ret

%include "common.asm"
%include "utf8.asm"

; ---------------------------------------------------------------- data
str_metaini db '\METADATA.INI',0
str_nametxt db '\NAME.TXT',0
str_starf   db '\*.*',0
str_setup   db 'SETUP',0
str_install db 'INSTALL',0
s_game      db '[GAME]',13,10,0
str_head    db ' New METADATA.INI: ',0
str_heade   db ' Edit METADATA.INI: ',0
str_tmp     db '\METADATA.$$$',0
str_form    db 'Game data',0
str_exes    db 'Executables',0
str_noexe   db '(no executables)',0
footer      db '~Tab~/~',18h,19h,'~ Field  ~F2~ Save  ~Esc~ Cancel  ~Ctrl+Y~ Clear',0
msg_usage   db 'Usage: GMEDIT <game folder>   (creates or edits its METADATA.INI)',13,10,'$'
msg_nodir   db 'GMEDIT: the game folder was not found.',13,10,'$'
msg_read    db 'GMEDIT: METADATA.INI could not be read.',13,10,'$'
msg_big     db 'GMEDIT: METADATA.INI is too big to edit (max 8 KB).',13,10,'$'
msg_key     db 'Press a key.$'
msg_write   db 'METADATA.INI could not be written (the disk is full or write-protected)',0

k_name      db 'NAME',0
k_genre     db 'GENRE',0
k_year      db 'YEAR',0
k_players   db 'PLAYERS',0
k_desc      db 'DESCRIPTION',0
k_run       db 'RUN',0
k_args      db 'ARGS',0
k_setup     db 'SETUP',0
l_name      db 'Name',0
l_genre     db 'Genre',0
l_year      db 'Year',0
l_players   db 'Players',0
l_desc      db 'Description',0
l_run       db 'Run',0
l_args      db 'Args',0
l_setup     db 'Setup',0
h_name      db 'The title in the game list',0
h_genre     db 'For example: Action, Adventure, Racing, Sports, Strategy',0
h_year      db 'The year of the release, for example: 1993',0
h_players   db 'Number of players: a number (2), a range (1-4) or open-ended (2+)',0
h_desc      db 'The text of the game page; \n starts a new line',0
h_run       db 'Ctrl+Enter plays it: a file of the folder, with its arguments',0
h_args      db 'Arguments added to RUN, also when it is started from the file list',0
h_setup     db 'Ctrl+S runs it: the setup program of the game, with its arguments',0

fields:
    FIELD k_name,    l_name,    0303h, b_name,    L_NAME,    0300h+VCOL, L_NAME,    h_name
    FIELD k_genre,   l_genre,   0403h, b_genre,   L_GENRE,   0400h+VCOL, L_GENRE,   h_genre
    FIELD k_year,    l_year,    0503h, b_year,    L_YEAR,    0500h+VCOL, L_YEAR,    h_year
    FIELD k_players, l_players, 0603h, b_players, L_PLAYERS, 0600h+VCOL, L_PLAYERS, h_players
    FIELD k_desc,    l_desc,    0803h, b_desc,    L_DESC,    0903h,      74,        h_desc
    FIELD k_run,     l_run,     0B03h, b_run,     L_CMD,     0B00h+VCOL, L_CMD,     h_run
    FIELD k_args,    l_args,    0C03h, b_args,    L_CMD,     0C00h+VCOL, L_CMD,     h_args
    FIELD k_setup,   l_setup,   0D03h, b_setup,   L_CMD,     0D00h+VCOL, L_CMD,     h_setup

; code page 437 80h..FFh -> Unicode
c2u         dw 0C7h,0FCh,0E9h,0E2h,0E4h,0E0h,0E5h,0E7h
            dw 0EAh,0EBh,0E8h,0EFh,0EEh,0ECh,0C4h,0C5h
            dw 0C9h,0E6h,0C6h,0F4h,0F6h,0F2h,0FBh,0F9h
            dw 0FFh,0D6h,0DCh,0A2h,0A3h,0A5h,20A7h,192h
            dw 0E1h,0EDh,0F3h,0FAh,0F1h,0D1h,0AAh,0BAh
            dw 0BFh,2310h,0ACh,0BDh,0BCh,0A1h,0ABh,0BBh
            dw 2591h,2592h,2593h,2502h,2524h,2561h,2562h,2556h
            dw 2555h,2563h,2551h,2557h,255Dh,255Ch,255Bh,2510h
            dw 2514h,2534h,252Ch,251Ch,2500h,253Ch,255Eh,255Fh
            dw 255Ah,2554h,2569h,2566h,2560h,2550h,256Ch,2567h
            dw 2568h,2564h,2565h,2559h,2558h,2552h,2553h,256Bh
            dw 256Ah,2518h,250Ch,2588h,2584h,258Ch,2590h,2580h
            dw 3B1h,0DFh,393h,3C0h,3A3h,3C3h,0B5h,3C4h
            dw 3A6h,398h,3A9h,3B4h,221Eh,3C6h,3B5h,2229h
            dw 2261h,0B1h,2265h,2264h,2320h,2321h,0F7h,2248h
            dw 0B0h,2219h,0B7h,221Ah,207Fh,0B2h,25A0h,0A0h

%ifdef TEST
tkp         dw testkeys
testkeys    dw TESTKEYS,0
%endif

section .bss
bss_start:
isold       resb 1          ; 1: METADATA.INI exists (rawbuf)
argb        resb 1          ; /B: started by the menu
lbeg        resw 1          ; scan_line: the line, its field and value
lend        resw 1
lfld        resw 1
vbeg        resw 1
vlen        resw 1
state       resb NFLD       ; save: per field 0 = unchanged, 1 = changed, 2 = written
fcur        resw 1          ; active field: index, table entry
fptr        resw 1
ecur        resw 1          ; cursor in its value, first character shown
escr        resw 1
msg         resw 1          ; error message shown instead of the hint (0 = none)
xcount      resw 1
xnew        resb 13
xnames      resb MAXX*13    ; executables (13 bytes each, zero padded)
gdir        resb 66         ; the game folder
pathbuf     resb 96
pathini     resb 96
progdir     resb 80         ; folder of GMEDIT.COM, with a trailing '\' (common.asm)
fpath       resb 96
startdir    resb 66
dta         resb 128
fbufs:                      ; the values (orig: the same as loaded)
b_name      resb L_NAME+1
b_genre     resb L_GENRE+1
b_year      resb L_YEAR+1
b_players   resb L_PLAYERS+1
b_desc      resb L_DESC+1
b_run       resb L_CMD+1
b_args      resb L_CMD+1
b_setup     resb L_CMD+1
orig        resb orig-fbufs
ORIGOFF     equ orig-fbufs
ibuf        resb IBUFSZ
rawbuf      resb RAWSZ+1
outbuf      resb 2*RAWSZ+L_DESC*3+1024  ; kept lines (LF -> CR LF) and the new ones
bss_end:
