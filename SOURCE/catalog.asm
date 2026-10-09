; catalog.asm - game scanning, METADATA.INI parsing, field access

scan_games:
    call idx_open
    call .scan
    jmp idx_finish
.scan:
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

; di = entry: title and data from GMENU.IDX if the files did not change, otherwise
; from METADATA.INI, if missing: title from NAME.TXT
read_game:
    mov ax,di
    sub ax,entries
    mov cl,5                ; (offset / 64) * 2
    shr ax,cl
    add ax,A_SEGS
    mov [gslot],ax
    mov byte [gsrc],0
    call default_title
    mov si,str_metaini
    mov bx,stamp
    call file_stamp
    push di
    mov di,stamp+8
    mov si,zeros
    mov cx,4
    rep movsw
    pop di
    call idx_lookup
    jnc .keep
    push es                 ; not cached: no record, default title
    mov es,[auxseg]
    mov bx,[gslot]
    mov word [es:bx],0
    pop es
    mov byte [gsrc],0
    call default_title
    call parse_game
.keep:
    jmp keep_stamp

parse_game:
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
    mov byte [gsrc],1       ; title from NAME.TXT (or the folder name)
    mov si,str_nametxt
    mov bx,stamp+8
    call file_stamp
    mov si,str_nametxt
    call open_read
    jc .r
    mov si,ibuf
    call skip_bom
    jmp set_title
.r: ret

game_path:                  ; di = entry, si = '\X.Y' -> pathbuf = game directory\folder\X.Y
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
    ret

alloc_far:                  ; ax = bytes -> dx = segment of a new far block; CF=1 if no memory
    add ax,15
    mov cl,4
    shr ax,cl
    mov dx,[heapseg]
    mov cx,dx
    add cx,ax
    jc .r
    cmp [memtop],cx
    jb .r
    mov [heapseg],cx
.r: ret

; di = entry, si = filename ('\X.Y') -> ibuf (null-terminated), cx = length; CF=1 if missing
open_read:
    call game_path
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
    call utf8_437
    clc
.r: ret

; ibuf (null-terminated): UTF-8 -> code page 437 in place, cx = new length
; bytes that do not form a valid UTF-8 sequence are kept (text already in CP437);
; characters missing from CP437 become their base letter or '?', the BOM is dropped
utf8_437:
    push si
    push di
    mov si,ibuf
    mov di,si
.l: lodsb
    test al,al
    jz .end
    cmp al,0C2h             ; lead byte of a 2..4 byte sequence?
    jb .put
    cmp al,0F4h
    ja .put
    mov ah,[si]
    mov bl,ah
    and bl,0C0h
    cmp bl,80h              ; followed by a continuation byte?
    jne .put
    cmp al,0E0h
    jae .three
    mov dl,ah               ; 2 bytes: dx = (al & 1Fh) << 6 | (ah & 3Fh)
    and dl,3Fh
    xor dh,dh
    and ax,1Fh
    mov cl,6
    shl ax,cl
    or dx,ax
    inc si
    jmp .map
.three:
    cmp al,0F0h
    jae .other
    mov bl,[si+1]
    mov bh,bl
    and bh,0C0h
    cmp bh,80h
    jne .put
    add si,2                ; 3 bytes: dx = (al & 0Fh) << 12 | (ah & 3Fh) << 6 | (bl & 3Fh)
    mov cl,4
    shl al,cl
    mov dh,al
    and ah,3Fh
    mov dl,ah
    shr ah,1
    shr ah,1
    or dh,ah
    mov cl,6
    shl dl,cl
    and bl,3Fh
    or dl,bl
    cmp dx,0FEFFh           ; BOM
    je .l
.map:
    mov al,dl
    test dh,dh
    jnz .pairs
    cmp al,80h
    jb .put
    cmp al,0A0h
    jb .q
    mov bx,u2c_a0-0A0h      ; U+00A0..U+00FF: table
    xlat
    jmp .put
.pairs:
    mov bx,u2c_pairs
.p: mov ax,[bx]
    test ax,ax
    jz .q
    add bx,3
    cmp ax,dx
    jne .p
    mov al,[bx-1]
    jmp .put
.other:                     ; 4 bytes (outside the BMP): '?'
    inc si
    mov al,[si]
    and al,0C0h
    cmp al,80h
    je .other
.q: mov al,'?'
.put:
    stosb
    jmp .l
.end:
    mov [di],al
    mov cx,di
    sub cx,ibuf
    pop di
    pop si
    ret

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
; ibuf -> kv_off/kv_len (keys of key_tab); missing: length 0
; "KEY = value" lines, case insensitive; lines starting with ';' '#' '[' are skipped
parse_ini:
    push di
    xor ax,ax
    mov di,kv_len
    mov cx,NKEYS
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
    cmp dx,4                ; DESCRIPTION: collected in fldbuf
    jne .kv
    call desc_append
    jmp .line
.kv:
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

; bx = value, cx = length -> appended to the description in fldbuf (kv_off/kv_len[4]);
; "\n" = line break (0Ah), "\\" = backslash, each further DESCRIPTION= line starts a new line
desc_append:
    push si
    mov si,bx
    mov di,fldbuf
    mov [kv_off+8],di
    mov dx,[kv_len+8]
    add di,dx
    mov al,10
    test dx,dx
    jnz .put
.c: jcxz .done
    lodsb
    dec cx
    cmp al,'\'
    jne .put
    jcxz .put
    mov ah,[si]
    cmp ah,al               ; "\\" = a backslash
    je .esc
    or ah,20h
    cmp ah,'n'
    jne .put
    mov al,10
.esc:
    inc si
    dec cx
.put:
    cmp di,fldbuf+L_DESC
    jae .done
    stosb
    jmp .c
.done:
    sub di,fldbuf
    mov [kv_len+8],di
    pop si
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

; GENRE..ARGS -> new far record, its segment stored in the aux segment at [gslot]
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
    cmp bx,NKEYS*2
    jb .clip
    xor ax,ax
    mov bx,2
.sum:
    add ax,[kv_len+bx]
    add bx,2
    cmp bx,NKEYS*2
    jb .sum
    test ax,ax
    jz .r                   ; no data other than NAME
    add ax,NKEYS-1
    call alloc_far
    jc .r                   ; not enough memory: skip data
    push es
    mov es,[auxseg]
    mov bx,[gslot]
    mov [es:bx],dx
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
    cmp bx,NKEYS*2
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
    mov es,[cs:auxseg]
    mov ax,[es:di+A_SEGS]
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
