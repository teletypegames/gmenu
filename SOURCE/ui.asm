; ui.asm - screen, mouse, generic list widget, input, drawing

restore_screen:
    cmp byte [hasmouse],0
    je .n
    xor ax,ax
    int 33h
.n: mov ax,3
    int 10h
    ret

mouse_show:
    cmp byte [hasmouse],0
    je .r
    mov ax,1
    int 33h
.r: ret

mouse_hide:
    cmp byte [hasmouse],0
    je .r
    mov ax,2
    int 33h
.r: ret

; ---------------------------------------------------------------- list UI
; in: lcount, lptrs, lsel, ltop, lrow0, lrows, listmode, lheader
; out: [lact] = 0: selected (ax = index), 1: Esc, 2: other key (ax = code)
run_list:
    call clamp_sel
.redraw:
    call draw_list
.wait:
    call get_input
    cmp ax,0FF01h
    je .mouse
    cmp al,27
    je .esc
    cmp al,13
    je .enter
    test al,al
    jz .ext
    cmp al,0E0h
    je .ext
    cmp al,' '
    jbe .key
    cmp byte [listmode],0
    je .key                 ; game list: search (handled by caller)
    call jump_letter
    jmp .moved
.ext:
    mov bx,[lsel]
    cmp ah,48h
    je .up
    cmp ah,50h
    je .down
    cmp ah,49h
    je .pgup
    cmp ah,51h
    je .pgdn
    cmp ah,47h
    je .home
    cmp ah,4Fh
    je .endk
    cmp ah,4Bh
    je .esc
    cmp ah,4Dh
    je .enter
.key:
    mov byte [lact],2
    ret
.up:
    test bx,bx
    jz .endk                ; top: wrap to the last item
    dec bx
    jmp .setsel
.down:
    inc bx
    cmp bx,[lcount]
    jae .home               ; bottom: wrap to the first item
    jmp .setsel
.pgup:
    mov bx,[lsel]
    sub bx,[lrows]
    jns .setsel
.home:
    xor bx,bx
    jmp .setsel
.pgdn:
    mov bx,[lsel]
    add bx,[lrows]
    cmp bx,[lcount]
    jb .setsel
.endk:
    mov bx,[lcount]
    dec bx
.setsel:
    cmp word [lcount],0
    je .wait
    mov [lsel],bx
.moved:
    call fix_top
    jmp .redraw
.mouse:
    mov ax,[mrow]
    test ax,ax
    jz .pgup
    cmp ax,24
    jae .pgdn
    sub ax,[lrow0]
    jb .wait
    cmp ax,[lrows]
    jae .wait
    cmp byte [listmode],1
    jne .mok
    cmp word [mcol],FCOL-1  ; detail view: only the right-side list
    jb .wait
.mok:
    add ax,[ltop]
    cmp ax,[lcount]
    jae .wait
    mov [lsel],ax
    call draw_list
.enter:
    cmp word [lcount],0
    je .wait
    mov ax,[lsel]
    mov byte [lact],0
    ret
.esc:
    mov byte [lact],1
    ret

clamp_sel:
    mov ax,[lsel]
    mov bx,[lcount]
    test bx,bx
    jnz .a
    xor ax,ax
    jmp .s
.a: cmp ax,bx
    jb .s
    lea ax,[bx-1]
.s: mov [lsel],ax
    jmp fix_top

fix_top:
    mov ax,[lsel]
    cmp ax,[ltop]
    jae .a
    mov [ltop],ax
.a: mov bx,[ltop]
    add bx,[lrows]
    cmp ax,bx
    jb .r
    sub ax,[lrows]
    inc ax
    mov [ltop],ax
.r: ret

; ---------------------------------------------------------------- input
; key: ax = code; left mouse click: ax=FF01h, [mrow]/[mcol]; right: Esc
get_input:
%ifdef TEST
    mov si,[tkp]
    lodsw
    test ax,ax
    jz .tend
    mov [tkp],si
    ret
.tend:                      ; end of test: save screen, exit
    mov ah,3Ch
    xor cx,cx
    mov dx,f_screen
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
%endif
.l: mov ah,1
    int 16h
    jz .nokey
    xor ax,ax
    int 16h
    ret
.nokey:
    cmp byte [hasmouse],0
    je .l
    mov ax,3
    int 33h
    mov al,bl
    mov ah,[prevbtn]
    mov [prevbtn],al
    not ah
    and al,ah
    test al,1
    jnz .left
    test al,2
    jz .l
    mov ax,011Bh
    ret
.left:
    mov bx,cx
    mov cl,3
    shr dx,cl
    mov [mrow],dx
    shr bx,cl
    mov [mcol],bx
    mov ax,0FF01h
    ret

; bx = index -> si = item pointer
get_item:
    shl bx,1
    add bx,[lptrs]
    mov si,[bx]
    ret

; al = character: jump to the next item starting with this letter
jump_letter:
    call upcase
    mov [jch],al
    mov cx,[lcount]
    mov dx,[lsel]
.l: inc dx
    cmp dx,[lcount]
    jb .c
    xor dx,dx
.c: mov bx,dx
    call get_item
    mov al,[si]
    call upcase
    cmp al,[jch]
    je .found
    loop .l
    ret
.found:
    mov [lsel],dx
    ret

; ---------------------------------------------------------------- drawing
; fill_to up to a given column, then text (si) with max length; ah = color, bp = row start
%macro COLSTR 2
    lea bx,[bp+(%1)*2]
    call fill_to
    mov cx,%2
    call put_str
%endmacro

draw_list:
    call mouse_hide
    push es
    mov ax,0B800h
    mov es,ax
    xor di,di
    mov ax,7020h
    stosw
    mov si,[lheader]
    mov cx,64
    call put_str
    mov bx,68*2
    call fill_to
    mov ax,[lsel]
    cmp word [lcount],0
    je .hz
    inc ax
.hz:
    call put_num
    mov ax,7000h+'/'
    stosw
    mov ax,[lcount]
    call put_num
    mov ah,70h
    mov bx,160
    call fill_to
    mov al,[listmode]
    cmp al,1
    je .det
    ja .pick
    call draw_games
    jmp .foot
.det:
    call draw_detail
    jmp .foot
.pick:
    call draw_picker
.foot:
    call draw_footer
    pop es
    call mouse_show
    ret

; game list table: Title / Genre / Year / Folder
draw_games:
    mov di,160
    mov bp,di
    mov ah,1Eh
    mov si,str_ctitle
    COLSTR C_TITLE,W_TITLE
    mov si,str_cgenre
    COLSTR C_GENRE,W_GENRE
    mov si,str_cyear
    COLSTR C_YEAR,W_YEAR
    mov si,str_cfolder
    COLSTR C_FOLDER,W_FOLDER
    lea bx,[bp+160]
    call fill_to
    xor dx,dx
.row:
    mov ax,dx
    add ax,[lrow0]
    mov bx,160
    push dx
    mul bx
    pop dx
    mov di,ax
    mov bp,ax
    mov bx,[ltop]
    add bx,dx
    mov ah,1Fh
    cmp bx,[lcount]
    jae .blank
    cmp bx,[lsel]
    jne .ns
    mov ah,30h
.ns:
    mov [rattr],ah
    call get_item
    push si
    add si,13
    COLSTR C_TITLE,W_TITLE
    pop si
    push si
    mov bl,F_GENRE
    call get_field
    mov ah,1Bh
    call row_attr
    COLSTR C_GENRE,W_GENRE
    pop si
    push si
    mov bl,F_YEAR
    call get_field
    mov ah,1Eh
    call row_attr
    COLSTR C_YEAR,W_YEAR
    pop si
    mov ah,17h
    call row_attr
    COLSTR C_FOLDER,W_FOLDER
    mov ah,[rattr]
.blank:
    lea bx,[bp+160]
    call fill_to
    inc dx
    cmp dx,[lrows]
    jb .row
    cmp word [lcount],0
    jne .r
    mov dx,0303h
    mov si,str_nomatch
    mov cx,70
    mov ah,17h
    call put_at
.r: ret

row_attr:                   ; ah = column color, selected row uses selection color
    cmp byte [rattr],30h
    jne .r
    mov ah,30h
.r: ret

; detail view: data on the left, file list on the right
draw_detail:
    mov ah,13h
    mov dx,0100h
    mov cx,1700h+LBOXR
    call draw_box
    mov si,[curgame]
    add si,13
    mov dx,0102h
    mov cx,LBOXR-5
    mov ah,1Eh
    call box_title
    mov dx,0700h
    mov cl,LBOXR
    mov ah,13h
    call hsep
    mov dh,3
    mov bl,F_GENRE
    mov si,lbl_genre
    call info_line
    mov dh,4
    mov bl,F_YEAR
    mov si,lbl_year
    call info_line
    mov dh,5
    mov bl,F_PLAYERS
    mov si,lbl_players
    call info_line
    mov dx,0603h
    mov si,lbl_folder
    mov cx,10
    mov ah,1Bh
    call put_at
    mov dl,13
    mov si,dirbuf
    mov cx,LBOXR-14
    mov ah,1Fh
    call put_at
    mov si,[curgame]        ; description, with line wrapping
    mov bl,F_DESC
    call get_field
    mov ah,17h
    cmp byte [si],0
    jne .desc
    mov si,str_nodesc
    mov ah,18h
.desc:
    mov dx,0903h
    mov cx,(14 << 8) | (LBOXR-4)
    call wrap_text
    mov ah,13h              ; right box: files
    mov dx,0100h+LBOXR+1
    mov cx,1700h+79
    call draw_box
    mov si,str_exefiles
    cmp byte [showall],0
    je .t
    mov si,str_allfiles
.t: mov dx,0100h+LBOXR+3
    mov cx,18
    mov ah,1Eh
    call box_title
    cmp word [lcount],0
    jne .list
    mov si,str_nofiles
    cmp byte [showall],0
    jne .e
    mov si,str_noexe
.e: mov dx,0200h+FCOL+1
    mov cx,20
    mov ah,17h
    call put_at
    cmp byte [showall],0
    jne .r
    mov dh,4
    mov si,str_tabhint
    mov cx,20
    call put_at
.r: ret
.list:
    mov word [rowi],0
.row:
    mov bx,[ltop]
    add bx,[rowi]
    cmp bx,[lcount]
    jae .r
    mov dh,[rowi]
    add dh,2
    mov dl,79
    call scr_at
    mov bp,di
    mov dl,FCOL
    call scr_at
    mov ah,1Fh
    cmp bx,[lsel]
    jne .ns
    mov ah,30h
.ns:
    mov [rattr],ah
    call get_item
    mov byte [rexec],1
    cmp byte [showall],0
    je .mk
    push si
    call is_exec
    pop si
    je .mk
    mov byte [rexec],0
.mk:
    mov ax,1A20h            ; marker before executables in all-files view
    cmp byte [showall],0
    je .m
    cmp byte [rexec],0
    je .m
    mov al,10h
.m: call row_attr
    stosw
    mov ah,[rattr]
    cmp byte [rexec],0
    jne .n2
    mov ah,17h
    call row_attr
.n2:
    mov cx,12
    call put_str
    mov bx,bp
    call fill_to
    inc word [rowi]
    cmp word [rowi],FROWS
    jb .row
    ret

info_line:                  ; dh = row, bl = field, si = label
    mov dl,3
    mov cx,10
    mov ah,1Bh
    call put_at
    mov si,[curgame]
    call get_field
    mov ah,1Fh
    cmp byte [si],0
    jne .v
    mov si,str_dash
    mov ah,17h
.v: mov dl,13
    mov cx,LBOXR-14
    jmp put_at

; filter picker: a centered box with the values
draw_picker:
    mov di,160
    mov ax,1720h
    mov cx,23*80
    rep stosw
    mov ah,13h
    mov dx,0118h
    mov cx,1737h
    call draw_box
    mov si,[ptitle]
    mov dx,011Ah
    mov cx,26
    mov ah,1Eh
    call box_title
    mov word [rowi],0
.row:
    mov bx,[ltop]
    add bx,[rowi]
    cmp bx,[lcount]
    jae .r
    mov dh,[rowi]
    add dh,2
    mov dl,37h
    call scr_at
    mov bp,di
    mov dl,19h
    call scr_at
    mov ah,1Fh
    cmp bx,[lsel]
    jne .ns
    mov ah,30h
.ns:
    call get_item
    mov al,' '
    stosw
    mov cx,28
    call put_str
    mov bx,bp
    call fill_to
    inc word [rowi]
    cmp word [rowi],PROWS
    jb .row
.r: ret

draw_footer:
    mov di,24*160
    mov ax,7020h
    stosw
    mov al,[listmode]
    mov si,footer_f
    cmp byte [showall],0
    je .f
    mov si,footer_fa
.f: cmp al,1
    je .ft
    mov si,footer_p
    cmp al,2
    je .ft
    mov si,footer_g
    cmp byte [flen],0
    je .ft
    mov ah,1Eh              ; search line
    mov si,str_search
    mov cx,20
    call put_str
    mov ah,1Fh
    mov si,fbuf
    mov cx,30
    call put_str
    mov ax,1F5Fh            ; '_'
    stosw
    mov ah,1Eh
    mov si,str_searchh
    mov cx,79
    call put_str
    jmp .fill
.ft:
    call put_footer
.fill:
    mov bx,4000
    jmp fill_to

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

; text with word wrap; si = text, dh,dl = position, ch = rows, cl = width, ah = color
; if it doesn't fit, the last row ends with "..."
wrap_text:
    mov [wr_w],cl
    mov [wr_n],ch
    mov byte [wr_ell],0
.row:
    cmp byte [wr_n],0
    je .r
.sk:
    cmp byte [si],' '
    jne .ns
    inc si
    jmp .sk
.ns:
    cmp byte [si],0
    je .r
    xor bx,bx               ; bx = position in line
    xor cx,cx               ; cx = last space position (0 = none)
.sc:
    mov al,[si+bx]
    test al,al
    jz .fit
    cmp bl,[wr_w]
    jae .over
    cmp al,' '
    jne .nsp
    mov cx,bx
.nsp:
    inc bx
    jmp .sc
.over:
    cmp al,' '
    je .fit
    jcxz .fit               ; no space: hard break
    mov bx,cx
.fit:
    cmp byte [wr_n],1
    jne .print
    cmp byte [si+bx],0
    je .print
    mov cl,[wr_w]           ; last row, but more text remains
    xor ch,ch
    sub cx,3
    mov byte [wr_ell],1
    cmp bx,cx
    jbe .print
    mov bx,cx
.print:
    call scr_at
    mov cx,bx
    jcxz .pe
.pl:
    lodsb
    stosw
    loop .pl
.pe:
    cmp byte [wr_ell],0
    je .nx
    mov al,'.'
    stosw
    stosw
    stosw
    ret
.nx:
    inc dh
    dec byte [wr_n]
    jmp .row
.r: ret

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

put_num:
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
    mov ah,70h
    stosw
    loop .o
    ret
