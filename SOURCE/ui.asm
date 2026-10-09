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
; out: [lact] = 0: selected (ax = index), 1: Esc, 2: other key (ax = code;
;      FF04h: double click in the game list)
run_list:
    call clamp_sel
.redraw:
    call draw_list
.wait:
    call get_input
    cmp ax,0FF01h
    je .mouse
    mov bx,[lsel]
    cmp ax,0FF02h
    je .wup
    cmp ax,0FF03h
    je .wdn
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
    cmp byte [listmode],1   ; detail view: Shift+PgUp/PgDn scrolls the description
    jne .ext2
    cmp ah,49h
    je .shift
    cmp ah,51h
    jne .ext2
.shift:
    push ax
    mov ah,2
    int 16h
    test al,3
    pop ax
    jnz .key
.ext2:
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
.wup:                       ; wheel: move without wrapping
    call wheel_left
    je .key
    sub bx,WSTEP
    jns .setsel
    jmp .home
.wdn:
    call wheel_left
    je .key
    add bx,WSTEP
    cmp bx,[lcount]
    jb .setsel
    jmp .endk
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
    cmp byte [listmode],0   ; game list: click selects, click on the selected game opens it,
    jne .msel               ; double click plays (ax = FF04h to the caller)
    cmp byte [mdbl],0
    jne .mplay
    cmp ax,[lsel]
    jne .mpick
    call wait_dbl
    jz .enter
    jmp .mplay2
.mpick:
    mov [lsel],ax
    jmp .moved
.mplay:
    mov [lsel],ax
.mplay2:
    mov ax,0FF04h
    jmp .key
.msel:
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

wheel_left:                 ; ZF=1 if the wheel turned over the detail view's left box
    cmp byte [listmode],1
    jne .r
    cmp word [mcol],FCOL-1
    jb .y
    or byte [listmode],0    ; ZF=0 (listmode = 1)
    ret
.y: cmp al,al
.r: ret

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
; key: ax = code; left mouse click: ax=FF01h, [mrow]/[mcol], [mdbl]; right: Esc
; wheel: ax=FF02h (up) / FF03h (down)
get_input:
%ifdef TEST
    mov si,[tkp]
    lodsw
    test ax,ax
    jz .tend
    mov [tkp],si
    ret
.tend:                      ; end of test: save screen, exit
    call mouse_hide         ; the mouse cursor is not part of the screen
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
%endif
.idle:
    mov ax,1680h            ; nothing to do: give the time slice to a multitasker
    int 2Fh
.l: mov ah,1
    int 16h
    jz .nokey
    xor ax,ax
    int 16h
    ret
.nokey:
    cmp byte [hasmouse],0
    je .idle
    mov ax,3
    int 33h
    mov [mwheel],bh
    push cx
    mov cl,3
    shr dx,cl
    mov [mrow],dx
    pop dx
    shr dx,cl
    mov [mcol],dx
    mov al,bl
    mov ah,[prevbtn]
    mov [prevbtn],al
    not ah
    and al,ah
    test al,1
    jnz .left
    test al,2
    jnz .right
    cmp byte [haswheel],0
    je .idle
    mov al,[mwheel]
    test al,al
    jz .idle
    mov ax,0FF03h
    jns .wr
    dec ax
.wr:
    ret
.right:
    mov ax,011Bh
    ret
.left:                      ; double click: same row, within DBLT ticks
    call ticks
    mov bx,ax
    sub ax,[lclick_t]
    mov byte [mdbl],0
    cmp ax,DBLT
    jae .first
    mov ax,[mrow]
    cmp ax,[lclick_r]
    jne .first
    mov byte [mdbl],1
    sub bx,DBLT             ; a third click starts a new pair
.first:
    mov [lclick_t],bx
    mov ax,[mrow]
    mov [lclick_r],ax
    mov ax,0FF01h
    ret

ticks:                      ; ax = BIOS timer ticks
    push ds
    mov ax,40h
    mov ds,ax
    mov ax,[6Ch]
    pop ds
    ret

; after a click on the selected game: ZF=0 if a second click follows within DBLT ticks
wait_dbl:
    mov ax,5                ; clear the press counter
    xor bx,bx
    int 33h
    call ticks
    mov [wd_t],ax
.l: mov ax,5
    xor bx,bx
    int 33h
    test bx,bx
    jnz .yes
    call ticks
    sub ax,[wd_t]
    cmp ax,DBLT
    jb .l
    cmp al,al
    ret
.yes:
    mov word [lclick_r],-1
    mov ax,3
    int 33h
    mov [prevbtn],bl
    or bh,1                 ; ZF=0
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
    call is_fav
    pushf
    mov ah,17h
    call row_attr
    COLSTR C_FOLDER,W_FOLDER
    mov ah,[rattr]
    popf
    jz .blank
    lea bx,[bp+79*2]        ; favorite: yellow '*' in the last column
    call fill_to
    mov ah,[rattr]
    and ah,0F0h
    or ah,0Eh
    mov al,'*'
    stosw
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
    mov cx,LBOXR-7
    mov ah,1Eh
    call box_title
    mov si,[curgame]
    call is_fav             ; favorite: " Title * "
    jz .nf
    mov al,'*'
    stosw
    mov al,' '
    stosw
.nf:
    mov dx,0800h
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
    mov dx,0703h
    mov si,lbl_played
    mov cx,10
    mov ah,1Bh
    call put_at
    call played_str
    mov dx,070Dh
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
    mov cx,(DROWS << 8) | (LBOXR-4)
    push word [dscroll]
    pop word [wr_skip]
    call wrap_text
    mov ax,[wr_tot]
    mov [dlines],ax
    mov ax,13h << 8 | 18h   ; scroll marks on the frame: up arrow / down arrow
    cmp word [dscroll],0
    je .nup
    mov dx,0800h+LBOXR-2
    call scr_at
    stosw
.nup:
    mov bx,[dlines]
    sub bx,[dscroll]
    cmp bx,DROWS
    jbe .ndn
    mov dx,1700h+LBOXR-2
    call scr_at
    mov al,19h
    stosw
.ndn:
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
    je .rf
    cmp byte [rexec],0
    je .rf
    mov al,10h
.rf:
    push ax                 ; the RUN file: yellow marker
    push di
    mov di,si
    push si
    mov si,[curgame]
    mov bl,F_RUN
    call get_field
    call tok_eq
    pop si
    pop di
    pop ax
    jne .m
    mov ax,1E10h
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
    mov dh,1
    mov dl,[pleft]
    mov ch,17h
    mov cl,[pright]
    call draw_box
    mov si,[ptitle]
    add dl,2
    mov cl,[pright]
    sub cl,[pleft]
    sub cl,5
    xor ch,ch
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
    mov dl,[pright]
    call scr_at
    mov bp,di
    mov dl,[pleft]
    inc dl
    call scr_at
    mov ah,1Fh
    cmp bx,[lsel]
    jne .ns
    mov ah,30h
.ns:
    call get_item
    mov al,' '
    stosw
    mov cl,[pright]
    sub cl,[pleft]
    sub cl,3
    xor ch,ch
    call put_str
    mov bx,bp
    call fill_to
    inc word [rowi]
    cmp word [rowi],PROWS
    jb .row
.r: ret

; F1: key reference in a box, closed by any key or click
show_help:
    call mouse_hide
    push es
    mov ax,0B800h
    mov es,ax
    mov ah,13h
    mov dx,0105h
    mov cx,174Ah
    call draw_box
    mov si,str_help
    mov dx,0107h
    mov cx,20
    mov ah,1Eh
    call box_title
    mov si,help_txt
    mov dx,0207h
.l: cmp byte [si],0
    je .w
    mov al,[si]             ; '#' = section title
    mov ah,1Fh
    cmp al,'#'
    jne .t
    inc si
    mov ah,1Eh
.t: push dx
    call scr_at
    push si
    mov cx,64
    call put_str
    pop si
.n: lodsb                   ; to the next line
    test al,al
    jnz .n
    pop dx
    inc dh
    jmp .l
.w: pop es
    call mouse_show
    call get_input
    ret

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
; 0Ah = line break; [wr_skip] = wrapped lines to skip (scrolling); out: [wr_tot] = all lines
wrap_text:
    mov [wr_w],cl
    mov cl,ch
    xor ch,ch
    mov [wr_rows],cx
    mov word [wr_tot],0
.line:
    cmp byte [si],' '
    jne .ns
    inc si
    jmp .line
.ns:
    cmp byte [si],0
    je .r
    xor bx,bx               ; bx = position in line
    xor cx,cx               ; cx = last space position (0 = none)
.sc:
    mov al,[si+bx]
    test al,al
    jz .fit
    cmp al,10               ; line break
    je .fit
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
    cmp al,10
    je .fit
    jcxz .fit               ; no space: hard break
    mov bx,cx
.fit:
    mov cx,[wr_tot]         ; visible line: wr_skip <= wr_tot < wr_skip + rows
    sub cx,[wr_skip]
    jb .adv
    cmp cx,[wr_rows]
    jae .adv
    push dx
    add dh,cl
    call scr_at
    pop dx
    push si
    mov cx,bx
    jcxz .pe
.pl:
    lodsb
    stosw
    loop .pl
.pe:
    pop si
.adv:
    add si,bx
    inc word [wr_tot]
    cmp byte [si],10
    jne .line
    inc si
    jmp .line
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
