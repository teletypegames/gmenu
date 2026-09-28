; GMENUPRG.COM - jatekmenu a \COLLECT mappahoz (DOS / DOSBox)
; Forditas: nasm -f bin -o GMENUPRG.COM GMENU.ASM
; Inditas: GMENU.BAT -> GMENUPRG.COM /B [jatekkonyvtar]  (alapertelmezes: \COLLECT)
; Kilepesi kod: 0 = \GMENU\MENURUN.BAT elkeszult (futtatni kell), 1 = kilepes, 2 = hiba

bits 16
org 100h

MAXG     equ 600            ; max jatek
ESZ      equ 64             ; bejegyzes merete: 13 byte mappanev + 51 byte cim
MAXF     equ 100            ; max futtathato fajl egy mappaban
LISTROWS equ 23

start:
    cld
    call parse_args
    cmp byte [argb],0       ; csak a GMENU.BAT-bol (/B kapcsoloval) indulhat
    jne .argok
    mov dx,msg_usebat
    mov ah,9
    int 21h
    mov ax,4C02h
    int 21h
.argok:
    mov ax,3
    int 10h
    mov ah,1
    mov cx,2000h            ; szoveges kurzor elrejtese
    int 10h
    mov ah,1Ah
    mov dx,dta
    int 21h
    xor ax,ax
    int 33h
    cmp ax,0FFFFh
    jne .nomouse
    mov byte [hasmouse],1
.nomouse:
    call scan_games
    cmp word [gcount],0
    jne .ok
    call restore_screen
    mov dx,msg_nogames
    mov ah,9
    int 21h
    mov di,colldir          ; konyvtar kiirasa
    call strend
    mov cx,di
    sub cx,colldir
    mov dx,colldir
    mov bx,1
    mov ah,40h
    int 21h
    mov dx,crlf_d
    mov ah,9
    int 21h
    mov ax,4C02h
    int 21h
.ok:
    mov bx,gptrs
    mov cx,[gcount]
    mov dx,13
    call sort_ptrs
    call load_sel
    call mouse_show

game_loop:
    mov byte [listmode],0
    mov ax,[vcount]
    mov [lcount],ax
    mov word [lptrs],vptrs
    mov word [lheader],hdr_games
    mov ax,[gsel]
    mov [lsel],ax
    mov ax,[gtop]
    mov [ltop],ax
    call run_list
    pushf
    mov bx,[lsel]
    mov [gsel],bx
    mov bx,[ltop]
    mov [gtop],bx
    popf
    jc quit
    mov bx,ax
    shl bx,1
    mov si,[vptrs+bx]
    mov [curgame],si
    call scan_files
    cmp word [fcount],0
    jne .files
    mov ah,2                ; nincs futtathato fajl: sipolas
    mov dl,7
    int 21h
    jmp game_loop
.files:
    call build_file_header
    mov byte [listmode],1
    mov ax,[fcount]
    mov [lcount],ax
    mov word [lptrs],fptrs
    mov word [lheader],hdrbuf
    mov word [lsel],0
    mov word [ltop],0
    call run_list
    jc game_loop
    mov bx,ax
    shl bx,1
    mov si,[fptrs+bx]
    call write_runner
    call save_sel
    call restore_screen
    mov ax,4C00h
    int 21h

quit:
    call save_sel
    call restore_screen
    mov ax,4C01h
    int 21h

; ---------------------------------------------------------------- kepernyo
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

; ---------------------------------------------------------------- jatekok beolvasasa
scan_games:
    mov di,pathbuf
    call put_colldir
    mov si,str_starf
    call strcpy
    mov dx,pathbuf
    mov cx,10h
    mov ah,4Eh
    int 21h
    jc .done
.loop:
    test byte [dta+15h],10h
    jz .next
    cmp byte [dta+1Eh],'.'
    je .next
    mov ax,[gcount]
    cmp ax,MAXG
    jae .done
    mov di,ax
    mov cl,6
    shl di,cl
    add di,entries
    mov bx,ax
    shl bx,1
    mov [gptrs+bx],di
    inc word [gcount]
    push di
    mov si,dta+1Eh
    mov cx,13
    rep movsb
    pop di
    call read_title
.next:
    mov ah,4Fh
    int 21h
    jnc .loop
.done:
    ret

; di = bejegyzes; a NAME.TXT elso sorat beirja a cimbe (alapertelmezes: mappanev)
read_title:
    push di
    mov bx,di
    mov di,pathbuf
    call put_colldir
    mov si,bx
    call strcpy
    mov si,str_nametxt
    call strcpy
    pop di
    call default_title
    mov ax,3D00h
    mov dx,pathbuf
    int 21h
    jc .ret
    mov bx,ax
    mov ah,3Fh
    mov cx,64
    mov dx,readbuf
    int 21h
    pushf
    push ax
    mov ah,3Eh
    int 21h
    pop cx
    popf
    jc .ret
    mov si,readbuf
    cmp cx,3                ; UTF-8 BOM atugrasa
    jb .nobom
    cmp byte [si],0EFh
    jne .nobom
    cmp byte [si+1],0BBh
    jne .nobom
    cmp byte [si+2],0BFh
    jne .nobom
    add si,3
    sub cx,3
.nobom:
    lea bx,[di+13]
    xor dx,dx
.cp:
    jcxz .end
    lodsb
    dec cx
    cmp al,13
    je .end
    cmp al,10
    je .end
    cmp al,1Ah
    je .end
    test al,al
    je .end
    cmp al,9
    jne .notab
    mov al,' '
.notab:
    cmp al,' '
    jne .store
    test dx,dx              ; vezeto szokozok kihagyasa
    jz .cp
.store:
    cmp dx,50
    jae .end
    mov [bx],al
    inc bx
    inc dx
    jmp .cp
.end:
    test dx,dx
    jz .empty
    cmp byte [bx-1],' '
    jne .term
    dec bx
    dec dx
    jmp .end
.term:
    mov byte [bx],0
.ret:
    ret
.empty:
    ; fallthrough: ures NAME.TXT -> mappanev

default_title:              ; di = bejegyzes
    push di
    mov si,di
    add di,13
    call strcpy
    pop di
    ret

; ---------------------------------------------------------------- fajlok beolvasasa
scan_files:
    mov word [fcount],0
    mov di,pathbuf
    call put_colldir
    mov si,[curgame]
    call strcpy
    mov si,str_star
    call strcpy
    mov dx,pathbuf
    xor cx,cx
    mov ah,4Eh
    int 21h
    jc .done
.loop:
    mov si,dta+1Eh
    call is_exec
    jne .next
    mov ax,[fcount]
    cmp ax,MAXF
    jae .done
    mov bx,14
    mul bx
    add ax,fnames
    mov di,ax
    mov bx,[fcount]
    shl bx,1
    mov [fptrs+bx],di
    inc word [fcount]
    mov si,dta+1Eh
    mov cx,13
    rep movsb
    mov byte [di],0
.next:
    mov ah,4Fh
    int 21h
    jnc .loop
.done:
    mov bx,fptrs
    mov cx,[fcount]
    xor dx,dx
    call sort_ptrs
    ret

; si = fajlnev; ZF=1 ha .COM / .EXE / .BAT
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

build_file_header:
    mov di,hdrbuf
    mov si,[curgame]
    push si
    add si,13
    call strcpy
    mov si,str_lpar
    call strcpy
    pop si
    call strcpy
    mov si,str_rpar
    call strcpy
    ret

; ---------------------------------------------------------------- lista UI
; be: lcount, lptrs, lsel, ltop, listmode, lheader
; ki: CF=0 es ax = kivalasztott index, vagy CF=1 (Esc)
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
    cmp al,8
    je .bksp
    test al,al
    jz .ext
    cmp al,0E0h
    je .ext
    cmp al,' '
    jb .wait
    cmp byte [listmode],0
    jne .jump
    call filter_add
    jmp .redraw
.jump:
    cmp al,' '
    je .wait
    call jump_letter
    jmp .moved
.bksp:
    cmp byte [listmode],0
    jne .wait
    call filter_back
    jmp .redraw
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
    jmp .wait
.up:
    test bx,bx
    jz .wait
    dec bx
    jmp .setsel
.down:
    inc bx
    cmp bx,[lcount]
    jae .wait
    jmp .setsel
.pgup:
    mov bx,[lsel]
    sub bx,LISTROWS
    jns .setsel
.home:
    xor bx,bx
    jmp .setsel
.pgdn:
    mov bx,[lsel]
    add bx,LISTROWS
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
    dec ax
    add ax,[ltop]
    cmp ax,[lcount]
    jae .wait
    mov [lsel],ax
    call draw_list
.enter:
    cmp word [lcount],0
    je .wait
    mov ax,[lsel]
    clc
    ret
.esc:
    cmp byte [listmode],0
    jne .esc2
    cmp byte [flen],0
    je .esc2
    call filter_clear       ; Esc elobb a keresest torli
    jmp .redraw
.esc2:
    stc
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

; ---------------------------------------------------------------- kereses
filter_add:                 ; al = karakter
    mov bl,[flen]
    cmp bl,30
    jae filter_ret
    xor bh,bh
    mov [fbuf+bx],al
    mov byte [fbuf+bx+1],0
    inc byte [flen]
    jmp refilter

filter_back:
    mov bl,[flen]
    test bl,bl
    jz filter_ret
    dec bl
    mov [flen],bl
    xor bh,bh
    mov byte [fbuf+bx],0
    jmp refilter

filter_clear:
    mov byte [flen],0
    mov byte [fbuf],0
    ; fallthrough

; szures ujra, a kijelolt jatek megtartasaval ha meg latszik
refilter:
    xor si,si
    cmp word [lcount],0
    je .n
    mov bx,[lsel]
    call get_item
.n: mov [keep],si
    call apply_filter
    mov ax,[vcount]
    mov [lcount],ax
    mov word [lsel],0
    mov word [ltop],0
    call find_keep
    mov ax,[lsel]           ; kijeloles kozepre
    sub ax,LISTROWS/2
    jns .t
    xor ax,ax
.t: mov [ltop],ax
    call clamp_sel
filter_ret:
    ret

; vptrs-ben keresi a [keep] bejegyzest -> lsel
find_keep:
    xor bx,bx
.f: cmp bx,[vcount]
    jae .r
    mov di,bx
    shl di,1
    mov ax,[vptrs+di]
    cmp ax,[keep]
    je .found
    inc bx
    jmp .f
.found:
    mov [lsel],bx
.r: ret

; gptrs -> vptrs a fbuf szerint (cim vagy mappanev tartalmazza)
apply_filter:
    xor dx,dx
    xor bx,bx
.l: cmp bx,[gcount]
    jae .done
    mov di,bx
    shl di,1
    mov si,[gptrs+di]
    call matches
    jne .next
    mov di,dx
    shl di,1
    mov [vptrs+di],si
    inc dx
.next:
    inc bx
    jmp .l
.done:
    mov [vcount],dx
    ret

matches:                    ; si = bejegyzes -> ZF=1 ha illeszkedik (si megmarad)
    cmp byte [flen],0
    je .r
    push si
    add si,13
    call instr
    pop si
    je .r
    push si
    call instr
    pop si
.r: ret

instr:                      ; si = szoveg, fbuf benne van-e (kis/nagybetu mindegy) -> ZF
    push bx
.s: cmp byte [si],0
    je .nf
    mov di,si
    mov bx,fbuf
.c: mov al,[bx]
    test al,al
    jz .found
    call upcase
    mov ah,al
    mov al,[di]
    call upcase
    cmp al,ah
    jne .adv
    inc bx
    inc di
    jmp .c
.adv:
    inc si
    jmp .s
.found:
    pop bx
    xor al,al
    ret
.nf:
    pop bx
    mov al,1
    test al,al
    ret

fix_top:
    mov ax,[lsel]
    cmp ax,[ltop]
    jae .a
    mov [ltop],ax
.a: mov bx,[ltop]
    add bx,LISTROWS
    cmp ax,bx
    jb .r
    sub ax,LISTROWS-1
    mov [ltop],ax
.r: ret

; billentyu: ax = kod; egerkattintas bal: ax=FF01h, [mrow]=sor; jobb: Esc
get_input:
%ifdef TEST
    mov si,[tkp]
    lodsw
    test ax,ax
    jz .tend
    mov [tkp],si
    ret
.tend:
    mov ax,011Bh
    ret
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
    mov cl,3
    shr dx,cl
    mov [mrow],dx
    mov ax,0FF01h
    ret

; bx = index -> si = elem mutato
get_item:
    shl bx,1
    add bx,[lptrs]
    mov si,[bx]
    ret

; al = karakter: ugras a kovetkezo ezzel kezdodo elemre
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
    cmp byte [listmode],0
    jne .f
    add si,13
.f: mov al,[si]
    call upcase
    cmp al,[jch]
    je .found
    loop .l
    ret
.found:
    mov [lsel],dx
    ret

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
    xor dx,dx
.row:
    mov ax,dx
    inc ax
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
    mov al,' '
    stosw
    cmp byte [listmode],0
    jne .file
    push si
    add si,13
    mov cx,62
    call put_str
    pop si
    lea bx,[bp+66*2]
    call fill_to
    mov ah,[rattr]
    cmp ah,30h
    je .d
    mov ah,17h
.d: mov cx,13
    call put_str
    jmp .pad
.file:
    mov cx,13
    call put_str
.pad:
    mov ah,[rattr]
.blank:
    lea bx,[bp+160]
    call fill_to
    inc dx
    cmp dx,LISTROWS
    jb .row
    mov di,24*160
    mov ax,7020h
    stosw
    mov si,footer_f
    cmp byte [listmode],0
    jne .ft
    mov si,footer_g
    cmp byte [flen],0
    je .ft
    mov ah,1Eh              ; kereses sor
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
.ft:
    mov cx,79
    call put_str
    mov bx,4000
    call fill_to
    pop es
    call mouse_show
    ret

; si = szoveg, ah = szin, cx = max hossz, es:di = kepernyo
put_str:
    jcxz .r
.l: lodsb
    test al,al
    jz .r
    stosw
    loop .l
.r: ret

; kitoltes szokozzel eddig: bx (di cel)
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

; ---------------------------------------------------------------- segedek
strcpy:                     ; si -> di, di a lezaro 0-ra mutat
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

; beszurasos rendezes: bx = mutatotomb, cx = darab, dx = kulcs offset
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

; ---------------------------------------------------------------- argumentumok
; GMENUPRG.COM /B [konyvtar]   pl. /B \COLLECT  vagy  /B D:\GAMES
parse_args:
    mov byte [argb],0
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
    jne .skip
    mov byte [argb],1
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
    ; colldir osszeallitasa: [X:] + '\' + utvonal, zaro '\' nelkul
    mov ah,19h
    int 21h
    add al,'A'
    mov [hdrive],al
    mov [gdrive],al
    mov si,argbuf
    mov di,colldir
    cmp byte [si],0
    je .r                   ; nincs megadva: marad az alapertelmezes
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
    mov si,colldir          ; cd-hez az utvonal meghajto nelkul
    cmp byte [si+1],':'
    jne .c
    add si,2
.c: mov [cdpath],si
    mov di,hdr_games        ; fejlec: "Games  (konyvtar)"
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

strend:                     ; di -> a lezaro 0
    cmp byte [di],0
    je .r
    inc di
    jmp strend
.r: ret

; ---------------------------------------------------------------- MENURUN.BAT
put_drive:                  ; al = meghajto betuje
    stosb
    mov al,':'
    stosb
    mov si,crlf
    jmp strcpy

write_runner:               ; si = fajlnev
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

; ---------------------------------------------------------------- utolso pozicio
; GMENU.SEL: utoljara kivalasztott mappanev (13) + kereses hossza (1) + kereses (32)
SELSZ equ 13+1+32

load_sel:
    mov ax,3D00h
    mov dx,f_sel
    int 21h
    jc .apply
    mov bx,ax
    mov ah,3Fh
    mov cx,SELSZ
    mov dx,selname
    int 21h
    mov ah,3Eh
    int 21h
    cmp byte [flen],30
    jbe .apply
    mov byte [flen],0
.apply:
    mov bl,[flen]
    xor bh,bh
    mov byte [fbuf+bx],0
    call apply_filter
    xor bx,bx               ; mappanev keresese a szurt listaban
.f: cmp bx,[vcount]
    jae .r
    push bx
    shl bx,1
    mov si,[vptrs+bx]
    mov di,selname
    call stricmp
    pop bx
    test ax,ax
    jz .found
    inc bx
    jmp .f
.found:
    mov [gsel],bx
    sub bx,LISTROWS/2
    jns .t
    xor bx,bx
.t: mov [gtop],bx
.r: ret

save_sel:
    mov byte [selname],0
    cmp word [vcount],0
    je .w
    mov bx,[gsel]
    shl bx,1
    mov si,[vptrs+bx]
    mov di,selname
    call strcpy
.w: mov ah,3Ch
    xor cx,cx
    mov dx,f_sel
    int 21h
    jc .r
    mov bx,ax
    mov ah,40h
    mov cx,SELSZ
    mov dx,selname
    int 21h
    mov ah,3Eh
    int 21h
.r: ret

; ---------------------------------------------------------------- adatok
colldir     db '\COLLECT',0
            times 64 db 0
str_starf   db '*.*',0
str_games   db 'Games',0
str_nametxt db '\NAME.TXT',0
str_star    db '\*.*',0
str_lpar    db '  (',0
str_rpar    db ')',0
f_runner    db '\GMENU\MENURUN.BAT',0
f_sel       db '\GMENU\GMENU.SEL',0
s_echo      db '@echo off',13,10,0
s_cd        db 'cd ',0
s_call      db 'call ',0
s_cdroot    db 'cd \'
crlf        db 13,10,0
footer_g    db 18h,19h,' PgUp/PgDn  Enter/click: open  Type: search  Esc: quit',0
footer_f    db 18h,19h,' PgUp/PgDn  Enter/click: run  A-Z: jump  Esc/right click: back',0
str_search  db 'Search: ',0
str_searchh db '   (Backspace: delete, Esc: clear search)',0
msg_usebat  db 'Start the menu with GMENU.BAT: \GMENU\GMENU',13,10,'$'
msg_nogames db 'No game folders found in: $'
crlf_d      db 13,10,'$'

gcount   dw 0
fcount   dw 0
gsel     dw 0
gtop     dw 0
vcount   dw 0
selname  times 13 db 0      ; selname, flen, fbuf egymas utan (GMENU.SEL)
flen     db 0
fbuf     times 32 db 0
%ifdef TEST
tkp      dw testkeys
testkeys dw TESTKEYS,0
%endif
hasmouse db 0
prevbtn  db 0
listmode db 0

section .bss
lcount  resw 1
lptrs   resw 1
lsel    resw 1
ltop    resw 1
lheader resw 1
mrow    resw 1
curgame resw 1
runfile resw 1
s_arr   resw 1
s_off   resw 1
s_n     resw 1
s_i     resw 1
s_key   resw 1
rattr   resb 1
jch     resb 1
dta     resb 128
pathbuf resb 128
readbuf resb 64
hdrbuf  resb 96
outbuf  resb 256
keep    resw 1
cdpath  resw 1
gdrive  resb 1
hdrive  resb 1
argb    resb 1
argbuf  resb 64
hdr_games resb 96
gptrs   resw MAXG
vptrs   resw MAXG
fptrs   resw MAXF
fnames  resb MAXF*14
entries resb MAXG*ESZ
