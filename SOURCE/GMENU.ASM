; GMENUPRG.COM - game menu for the \COLLECT folder (DOS / DOSBox)
; Build: nasm -f bin -o GMENUPRG.COM GMENU.ASM
; Launch: GMENU.BAT -> GMENUPRG.COM /B [/A] [game_directory]  (default: \COLLECT)
;         /A = all files are shown in the game's folder, not just executables
; Exit code: 0 = \GMENU\MENURUN.BAT created (must be run), 1 = quit, 2 = error
;
; Per folder: METADATA.INI (NAME, GENRE, YEAR, PLAYERS, DESCRIPTION), if missing: NAME.TXT
; The GENRE/YEAR/PLAYERS/DESCRIPTION reside in a far (above 64 KB) segment:
; per game a paragraph-aligned record: genre,0,year,0,players,0,desc,0

bits 16
org 100h

MAXG     equ 600            ; max games
ESZ      equ 64             ; entry size: 13-byte folder name + 51-byte title
MAXF     equ 600            ; max files per folder
FSZ      equ 13             ; filename size (8.3 + 0)
GROWS    equ 22             ; game list rows (2..23)
FROWS    equ 21             ; file list rows in detail view (2..22)
PROWS    equ 21             ; picker list rows (2..22)
MAXP     equ 250            ; max values in the picker
POOLSZ   equ 6000
IBUFSZ   equ 4096           ; METADATA.INI / NAME.TXT read buffer

F_GENRE   equ 0             ; field order in the far record
F_YEAR    equ 1
F_PLAYERS equ 2
F_DESC    equ 3
L_NAME    equ 50            ; max field lengths
L_GENRE   equ 24
L_YEAR    equ 10
L_PLAYERS equ 10
L_DESC    equ 2000

C_TITLE  equ 1              ; game list columns
W_TITLE  equ 44
C_GENRE  equ 46
W_GENRE  equ 15
C_YEAR   equ 62
W_YEAR   equ 4
C_FOLDER equ 67
W_FOLDER equ 12

LBOXR    equ 55             ; detail view: right edge of left box
FCOL     equ 57             ; file list first column (inside right box)

%if MAXF*FSZ < IBUFSZ || MAXF*FSZ < POOLSZ || MAXF < MAXP
%error "the fnames/fptrs area is too small for ibuf/pool/pptrs"
%endif

start:
    cld
    call parse_args
    cmp byte [argb],0       ; can only be launched from GMENU.BAT (with /B switch)
    jne .argok
    mov dx,msg_usebat
    mov ah,9
    int 21h
    mov ax,4C02h
    int 21h
.argok:
    mov ax,cs               ; far records after the program's 64 KB
    add ax,1000h
    mov [heapseg],ax
    mov ax,[2]              ; PSP: top of available memory
    mov [memtop],ax
    mov ax,3
    int 10h
    mov ah,1
    mov cx,2000h            ; hide text cursor
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
    mov di,colldir          ; print directory
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
    cmp byte [arga],0
    je .noa
    mov byte [showall],1
.noa:
    call mouse_show

game_loop:
    mov byte [listmode],0
    mov ax,[vcount]
    mov [lcount],ax
    mov word [lptrs],vptrs
    mov word [lrow0],2
    mov word [lrows],GROWS
    call build_game_header
    mov word [lheader],hdrbuf
    mov ax,[gsel]
    mov [lsel],ax
    mov ax,[gtop]
    mov [ltop],ax
    call run_list
    mov bx,[lsel]
    mov [gsel],bx
    mov bx,[ltop]
    mov [gtop],bx
    cmp byte [lact],1
    je .esc
    ja .key
    mov bx,ax
    shl bx,1
    mov si,[vptrs+bx]
    jmp detail
.esc:
    cmp byte [flen],0       ; Esc: clears search first, then filters
    je .esc2
    call filter_clear
    jmp game_loop
.esc2:
    call filters_active
    jz quit
    call clear_filters
    jmp game_loop
.key:
    test al,al
    jz .ext
    cmp al,0E0h
    je .ext
    cmp al,8
    je .bksp
    cmp al,' '
    jb game_loop
    call filter_add
    jmp game_loop
.bksp:
    call filter_back
    jmp game_loop
.ext:
    mov bl,F_GENRE
    mov di,fgenre
    mov dx,str_pgenre
    cmp ah,3Ch              ; F2
    je .pick
    mov bl,F_YEAR
    mov di,fyear
    mov dx,str_pyear
    cmp ah,3Dh              ; F3
    je .pick
    mov bl,F_PLAYERS
    mov di,fplayers
    mov dx,str_pplayers
    cmp ah,3Eh              ; F4
    je .pick
    cmp ah,3Fh              ; F5: clear filters
    jne game_loop
    call clear_filters
    jmp game_loop
.pick:
    call pick_filter
    jmp game_loop

; ---------------------------------------------------------------- detail view
detail:                     ; si = game entry
    mov [curgame],si
    call build_file_header
.rescan:
    call scan_files
    mov word [fsel],0
    mov word [ftop],0
.loop:
    mov byte [listmode],1
    mov ax,[fcount]
    mov [lcount],ax
    mov word [lptrs],fptrs
    mov word [lheader],hdrbuf
    mov word [lrow0],2
    mov word [lrows],FROWS
    mov ax,[fsel]
    mov [lsel],ax
    mov ax,[ftop]
    mov [ltop],ax
    call run_list
    mov bx,[lsel]
    mov [fsel],bx
    mov bx,[ltop]
    mov [ftop],bx
    cmp byte [lact],1
    je game_loop
    ja .key
    mov bx,ax
    shl bx,1
    mov si,[fptrs+bx]
    push si
    call is_exec
    pop si
    je .run
    call beep               ; not an executable file
    jmp .loop
.run:
    call write_runner
    call save_sel
    call restore_screen
    mov ax,4C00h
    int 21h
.key:
    cmp ah,0Fh              ; Tab: executables / all files
    jne .loop
    xor byte [showall],1
    jmp .rescan

quit:
    call save_sel
    call restore_screen
    mov ax,4C01h
    int 21h

beep:
    mov ah,2
    mov dl,7
    int 21h
    ret

; ---------------------------------------------------------------- screen
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

; ---------------------------------------------------------------- scanning games
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
    call read_game
.next:
    mov ah,4Fh
    int 21h
    jnc .loop
.done:
    ret

; di = entry: title and data from METADATA.INI, if missing: title from NAME.TXT
read_game:
    mov ax,di
    sub ax,entries
    mov cl,5                ; (offset / 64) * 2
    shr ax,cl
    add ax,gsegs
    mov [gslot],ax
    mov bx,ax
    mov word [bx],0
    call default_title
    mov si,str_metaini
    call open_read
    jc .nametxt
    push di
    call parse_ini
    call store_record
    pop di
    mov cx,[kv_len]         ; NAME
    jcxz .nametxt
    mov si,[kv_off]
    jmp set_title
.nametxt:
    mov si,str_nametxt
    call open_read
    jc .r
    mov si,ibuf
    call skip_bom
    jmp set_title
.r: ret

; di = entry, si = filename ('\X.Y') -> ibuf (null-terminated), cx = length; CF=1 if missing
open_read:
    push di
    mov bx,di
    push si
    mov di,pathbuf
    call put_colldir
    mov si,bx
    call strcpy
    pop si
    call strcpy
    pop di
    mov ax,3D00h
    mov dx,pathbuf
    int 21h
    jc .r
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
    jc .r
    mov bx,cx
    mov byte [ibuf+bx],0
.r: ret

skip_bom:                   ; si = text, cx = length: skip UTF-8 BOM
    cmp byte [si],0EFh
    jne .r
    cmp byte [si+1],0BBh
    jne .r
    cmp byte [si+2],0BFh
    jne .r
    add si,3
    sub cx,3
.r: ret

; di = entry, si = text, cx = length: the first line becomes the title (max 50 chars,
; without leading/trailing spaces); if empty, the title remains the folder name
set_title:
    lea bx,[di+13]
    xor dx,dx
.cp:
    jcxz .end
    lodsb
    dec cx
    call is_eol
    je .end
    cmp al,9
    jne .notab
    mov al,' '
.notab:
    cmp al,' '
    jne .store
    test dx,dx              ; skip leading spaces
    jz .cp
.store:
    cmp dx,L_NAME
    jae .end
    mov [bx],al
    inc bx
    inc dx
    jmp .cp
.end:
    test dx,dx
    jz default_title
    cmp byte [bx-1],' '
    jne .term
    dec bx
    dec dx
    jmp .end
.term:
    mov byte [bx],0
    ret

default_title:              ; di = entry
    push di
    mov si,di
    add di,13
    call strcpy
    pop di
    ret

; ---------------------------------------------------------------- METADATA.INI
; ibuf -> kv_off/kv_len (NAME, GENRE, YEAR, PLAYERS, DESCRIPTION); missing: length 0
; "KEY = value" lines, case insensitive; lines starting with ';' '#' '[' are skipped
parse_ini:
    push di
    xor ax,ax
    mov di,kv_len
    mov cx,5
    rep stosw
    mov si,ibuf
    call skip_bom
.line:
    call skip_blank
    mov al,[si]
    call is_eol
    jne .text
    test al,al
    jz .done
    cmp al,1Ah
    je .done
    inc si
    jmp .line
.text:
    cmp al,';'
    je .skipl
    cmp al,'#'
    je .skipl
    cmp al,'['
    je .skipl
    mov bx,si               ; start of key
.key:
    mov al,[si]
    call is_eol
    je .line                ; no '=' in the line
    cmp al,'='
    je .gotkey
    inc si
    jmp .key
.gotkey:
    mov di,si
    call trim_back
    call find_key
    mov dx,ax               ; dx = key index or -1
    inc si
    call skip_blank
    mov bx,si               ; start of value
.val:
    mov al,[si]
    call is_eol
    je .vend
    inc si
    jmp .val
.vend:
    mov di,si
    call trim_back
    mov cx,di
    sub cx,bx
    cmp cx,2                ; strip surrounding "quotes"
    jb .store
    cmp byte [bx],'"'
    jne .store
    cmp byte [di-1],'"'
    jne .store
    inc bx
    sub cx,2
.store:
    test dx,dx
    js .line
    mov di,dx
    shl di,1
    mov [kv_off+di],bx
    mov [kv_len+di],cx
    jmp .line
.skipl:
    mov al,[si]
    call is_eol
    je .line
    inc si
    jmp .skipl
.done:
    pop di
    ret

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

find_key:                   ; bx..di = key -> ax = index in key_tab, or -1
    push si
    push dx
    mov si,key_tab
    xor dx,dx
.k: cmp byte [si],0
    je .none
    push bx
.c: cmp bx,di
    je .kend
    mov al,[bx]
    call upcase
    cmp al,[si]
    jne .ne
    inc bx
    inc si
    jmp .c
.kend:
    cmp byte [si],0
    jne .ne
    pop bx
    mov ax,dx
    jmp .r
.ne:
    pop bx
.sk:
    lodsb
    test al,al
    jnz .sk
    inc dx
    jmp .k
.none:
    mov ax,-1
.r: pop dx
    pop si
    ret

; GENRE..DESCRIPTION -> new far record, its segment stored in [gslot]
store_record:
    xor bx,bx               ; clip lengths
.clip:
    mov ax,[kv_len+bx]
    cmp ax,[kv_lim+bx]
    jbe .cok
    mov ax,[kv_lim+bx]
    mov [kv_len+bx],ax
.cok:
    add bx,2
    cmp bx,10
    jb .clip
    mov ax,[kv_len+2]
    add ax,[kv_len+4]
    add ax,[kv_len+6]
    add ax,[kv_len+8]
    test ax,ax
    jz .r                   ; no data other than NAME
    add ax,4+15
    mov cl,4
    shr ax,cl               ; paragraphs
    mov dx,[heapseg]
    mov cx,dx
    add cx,ax
    jc .r
    cmp cx,[memtop]
    ja .r                   ; not enough memory: skip data
    mov [heapseg],cx
    mov bx,[gslot]
    mov [bx],dx
    push es
    mov es,dx
    xor di,di
    mov bx,2
.fld:
    mov si,[kv_off+bx]
    mov cx,[kv_len+bx]
    jcxz .z
.c: lodsb
    cmp al,9
    jne .s
    mov al,' '
.s: stosb
    loop .c
.z: xor al,al
    stosb
    add bx,2
    cmp bx,10
    jb .fld
    pop es
.r: ret

; si = entry, bl = field (F_*) -> si = fldbuf (field value, empty if none)
; preserves all other registers
get_field:
    push ax
    push bx
    push cx
    push di
    push es
    push ds
    mov ax,si
    sub ax,entries
    mov cl,5
    shr ax,cl
    mov di,ax
    mov ax,[gsegs+di]
    push cs
    pop es
    mov di,fldbuf
    test ax,ax
    jz .end
    mov ds,ax
    xor si,si
.skip:
    test bl,bl
    jz .copy
.s2:
    lodsb
    test al,al
    jnz .s2
    dec bl
    jmp .skip
.copy:
    lodsb
    stosb
    test al,al
    jnz .copy
    dec di
.end:
    mov byte [es:di],0
    pop ds
    pop es
    pop di
    pop cx
    pop bx
    pop ax
    mov si,fldbuf
    ret

; ---------------------------------------------------------------- scanning files
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
    cmp byte [showall],0
    jne .add
    mov si,dta+1Eh
    call is_exec
    jne .next
.add:
    mov ax,[fcount]
    cmp ax,MAXF
    jae .done
    mov bx,FSZ
    mul bx
    add ax,fnames
    mov di,ax
    mov bx,[fcount]
    shl bx,1
    mov [fptrs+bx],di
    inc word [fcount]
    mov si,dta+1Eh
    mov cx,FSZ
    rep movsb
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

build_file_header:          ; header: "Title  (FOLDER)", dirbuf: full folder path
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
    mov di,dirbuf
    call put_colldir
    mov si,[curgame]
    jmp strcpy

build_game_header:          ; header: "Games  (\COLLECT)" + active filters
    mov di,hdrbuf
    mov si,hdr_games
    call strcpy
    mov bx,fgenre
    mov si,str_hgenre
    call .add
    mov bx,fyear
    mov si,str_hyear
    call .add
    mov bx,fplayers
    mov si,str_hplayers
.add:
    cmp byte [bx],0
    je .r
    call strcpy
    mov si,bx
    call strcpy
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

; ---------------------------------------------------------------- search and filters
filter_add:                 ; al = character
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

filters_active:             ; ZF=1 if no active filters
    mov al,[fgenre]
    or al,[fyear]
    or al,[fplayers]
    ret

clear_filters:
    mov byte [fgenre],0
    mov byte [fyear],0
    mov byte [fplayers],0
    jmp refilter

filter_clear:
    mov byte [flen],0
    mov byte [fbuf],0
    ; fallthrough

; re-filter, keeping the selected game if still visible (gsel, gtop)
refilter:
    xor si,si
    cmp word [vcount],0
    je .n
    mov bx,[gsel]
    shl bx,1
    mov si,[vptrs+bx]
.n: mov [keep],si
    call apply_filter
    mov word [gsel],0
    call find_keep
    mov ax,[gsel]           ; center the selection
    sub ax,GROWS/2
    jns .t
    xor ax,ax
.t: mov [gtop],ax
filter_ret:
    ret

; searches for the [keep] entry in vptrs -> gsel
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
    mov [gsel],bx
.r: ret

; gptrs -> vptrs according to filters and search
apply_filter:
    xor dx,dx
    xor bx,bx
.l: cmp bx,[gcount]
    jae .done
    mov di,bx
    shl di,1
    mov si,[gptrs+di]
    push bx
    push dx
    call matches
    pop dx
    pop bx
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

; si = entry -> ZF=1 if it matches (si preserved)
; filters: GENRE / YEAR / PLAYERS exact match; search: title, folder name or description
matches:
    push si
    mov bl,F_GENRE
    mov di,fgenre
    call match_field
    jne .r
    mov bl,F_YEAR
    mov di,fyear
    call match_field
    jne .r
    mov bl,F_PLAYERS
    mov di,fplayers
    call match_field
    jne .r
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
    je .r
    mov bl,F_DESC
    call get_field
    call instr
.r: pop si
    ret

match_field:                ; si = entry, bl = field, di = filter -> ZF=1 if empty filter or matches
    cmp byte [di],0
    je .r
    push si
    call get_field
    call stricmp
    pop si
    test ax,ax
.r: ret

instr:                      ; si = text, whether fbuf is contained (case insensitive) -> ZF
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

; pick a filter from existing values
; bl = field, di = filter variable, dx = title
pick_filter:
    mov [pfield],bl
    mov [pdest],di
    mov [ptitle],dx
    mov word [pptrs],str_all
    mov word [pcount],1
    mov word [ppool],pool
    xor bx,bx
.g: cmp bx,[gcount]
    jae .sort
    push bx
    shl bx,1
    mov si,[gptrs+bx]
    mov bl,[pfield]
    call get_field
    cmp byte [si],0
    je .next
    mov dx,1                ; already exists?
.chk:
    cmp dx,[pcount]
    jae .add
    mov bx,dx
    shl bx,1
    mov di,[pptrs+bx]
    call stricmp
    test ax,ax
    jz .next
    inc dx
    jmp .chk
.add:
    cmp word [pcount],MAXP
    jae .next
    mov di,[ppool]
    cmp di,pool+POOLSZ-L_GENRE-1
    ja .next
    mov bx,[pcount]
    shl bx,1
    mov [pptrs+bx],di
    inc word [pcount]
    call strcpy
    inc di
    mov [ppool],di
.next:
    pop bx
    inc bx
    jmp .g
.sort:
    mov bx,pptrs+2
    mov cx,[pcount]
    dec cx
    xor dx,dx
    call sort_ptrs
    mov word [lsel],0       ; select the current value
    mov si,[pdest]
    cmp byte [si],0
    je .run
    mov dx,1
.f: cmp dx,[pcount]
    jae .run
    mov bx,dx
    shl bx,1
    mov di,[pptrs+bx]
    call stricmp
    test ax,ax
    jz .fnd
    inc dx
    jmp .f
.fnd:
    mov [lsel],dx
.run:
    mov byte [listmode],2
    mov ax,[pcount]
    mov [lcount],ax
    mov word [lptrs],pptrs
    mov ax,[ptitle]
    mov [lheader],ax
    mov word [lrow0],2
    mov word [lrows],PROWS
    mov word [ltop],0
.again:
    call run_list
    cmp byte [lact],1
    je .r
    ja .again
    mov di,[pdest]
    mov byte [di],0
    test ax,ax
    jz .set                 ; "(all)"
    mov bx,ax
    shl bx,1
    mov si,[pptrs+bx]
    call strcpy
.set:
    jmp refilter
.r: ret

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

; ---------------------------------------------------------------- helpers
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

; ---------------------------------------------------------------- MENURUN.BAT
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

; ---------------------------------------------------------------- last position
; GMENU.SEL: folder name (13) + search length (1) + search (32)
;            + filters: genre (25), year (11), players (11) + all-files flag (1)
SELSZ equ 13+1+32+(L_GENRE+1)+(L_YEAR+1)+(L_PLAYERS+1)+1

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
    jbe .v
    mov byte [flen],0
.v: mov byte [selname+12],0
    mov byte [fgenre+L_GENRE],0
    mov byte [fyear+L_YEAR],0
    mov byte [fplayers+L_PLAYERS],0
    and byte [showall],1
.apply:
    mov bl,[flen]
    xor bh,bh
    mov byte [fbuf+bx],0
    call apply_filter
    xor bx,bx               ; search for folder name in the filtered list
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
    sub bx,GROWS/2
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

; ---------------------------------------------------------------- data
colldir     db '\COLLECT',0
            times 64 db 0
str_starf   db '*.*',0
str_games   db 'Games',0
str_nametxt db '\NAME.TXT',0
str_metaini db '\METADATA.INI',0
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
key_tab     db 'NAME',0,'GENRE',0,'YEAR',0,'PLAYERS',0,'DESCRIPTION',0,0
kv_lim      dw L_NAME,L_GENRE,L_YEAR,L_PLAYERS,L_DESC
footer_g    db '~',18h,19h,'~ Move ~Enter~ Details ~Type~ Search ~F2~ Genre ~F3~ Year ~F4~ Players ~F5~ Clear ~Esc~ Quit',0
footer_f    db '~',18h,19h,'~ Move  ~Enter~ Run  ~Tab~ All files  ~A-Z~ Jump  ~Esc~ Back',0
footer_fa   db '~',18h,19h,'~ Move  ~Enter~ Run  ~Tab~ Executables only  ~A-Z~ Jump  ~Esc~ Back',0
footer_p    db '~',18h,19h,'~ Move  ~Enter~ Select  ~A-Z~ Jump  ~Esc~ Cancel',0
str_search  db 'Search: ',0
str_searchh db '   (Backspace: delete, Esc: clear search)',0
str_ctitle  db 'Title',0
str_cgenre  db 'Genre',0
str_cyear   db 'Year',0
str_cfolder db 'Folder',0
str_hgenre  db '  Genre: ',0
str_hyear   db '  Year: ',0
str_hplayers db '  Players: ',0
str_pgenre  db 'Filter by genre',0
str_pyear   db 'Filter by year',0
str_pplayers db 'Filter by players',0
str_all     db '(all)',0
str_nomatch db 'No games match the search / filters.  (Esc: clear, F5: clear filters)',0
lbl_genre   db 'Genre',0
lbl_year    db 'Year',0
lbl_players db 'Players',0
lbl_folder  db 'Folder',0
str_dash    db '-',0
str_nodesc  db 'No description.',0
str_exefiles db 'Executables',0
str_allfiles db 'All files',0
str_noexe   db '(no executables)',0
str_nofiles db '(no files)',0
str_tabhint db 'Tab: all files',0
msg_usebat  db 'Start the menu with GMENU.BAT: \GMENU\GMENU',13,10,'$'
msg_nogames db 'No game folders found in: $'
crlf_d      db 13,10,'$'
%ifdef TEST
f_screen    db '\GMENU\SCREEN.BIN',0
%endif

gcount   dw 0
fcount   dw 0
gsel     dw 0
gtop     dw 0
vcount   dw 0
selname  times 13 db 0      ; selname..showall consecutive (GMENU.SEL)
flen     db 0
fbuf     times 32 db 0
fgenre   times L_GENRE+1 db 0
fyear    times L_YEAR+1 db 0
fplayers times L_PLAYERS+1 db 0
showall  db 0
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
lrow0   resw 1
lrows   resw 1
lheader resw 1
lact    resb 1
mrow    resw 1
mcol    resw 1
curgame resw 1
runfile resw 1
fsel    resw 1
ftop    resw 1
rowi    resw 1
s_arr   resw 1
s_off   resw 1
s_n     resw 1
s_i     resw 1
s_key   resw 1
heapseg resw 1
memtop  resw 1
gslot   resw 1
pfield  resb 1
pdest   resw 1
ptitle  resw 1
pcount  resw 1
ppool   resw 1
wr_w    resb 1
wr_n    resb 1
wr_ell  resb 1
rattr   resb 1
rexec   resb 1
jch     resb 1
dta     resb 128
pathbuf resb 128
hdrbuf  resb 200
dirbuf  resb 80
outbuf  resb 256
keep    resw 1
cdpath  resw 1
gdrive  resb 1
hdrive  resb 1
argb    resb 1
arga    resb 1
argbuf  resb 64
hdr_games resb 96
kv_off  resw 5
kv_len  resw 5
fldbuf  resb L_DESC+2
gptrs   resw MAXG
vptrs   resw MAXG
gsegs   resw MAXG
fptrs   resw MAXF           ; shared area: file list / picker (pptrs)
fnames  resb MAXF*FSZ       ; shared area: filenames / ibuf (read buffer) / pool (picker)
entries resb MAXG*ESZ
bss_end:

pptrs   equ fptrs
pool    equ fnames
ibuf    equ fnames
