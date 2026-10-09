; filter.asm - search, genre/year/players filters, filter picker

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
    or al,[favonly]
    ret

clear_filters:
    mov byte [fgenre],0
    mov byte [fyear],0
    mov byte [fplayers],0
    mov byte [favonly],0
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
; filters: favorites only, GENRE / YEAR / PLAYERS exact match; search: title, folder name or description
matches:
    cmp byte [favonly],0
    je .nofav
    call is_fav
    jnz .nofav
    or al,1                 ; not a favorite: ZF=0
    ret
.nofav:
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
    call instr_desc
.r: pop si
    ret

; si = entry, bl = field, di = filter -> ZF=1 if empty filter or matches (ax..dx clobbered)
; YEAR: "1990s" matches 1990..1999; PLAYERS: a number matches "a", "a-b" and "a+" ranges
match_field:
    cmp byte [di],0
    je .r
    push si
    call get_field
    cmp bl,F_PLAYERS
    je .pl
    cmp bl,F_YEAR
    jne .exact
    call is_decade
    jne .exact
    mov ax,[si]             ; first three digits equal, the fourth is a digit
    cmp ax,[di]
    jne .done
    mov al,[si+2]
    cmp al,[di+2]
    jne .done
    mov al,[si+3]
    sub al,'0'
    cmp al,10
    sbb al,al               ; digit: al = -1
    inc al
    jmp .done
.pl:
    mov si,di
    call parse_num          ; ax = players in the filter
    jc .exact
    push ax
    mov si,fldbuf
    call parse_range        ; ax..dx = range in the field
    pop cx
    jc .exact
    cmp cx,ax
    jb .no
    cmp cx,dx
    ja .no
    cmp al,al
    jmp .done
.no:
    or cl,1                 ; cx >= 1 here: ZF=0
    jmp .done
.exact:
    mov si,fldbuf
    call stricmp
    test ax,ax
.done:
    pop si
.r: ret

is_decade:                  ; di = filter -> ZF=1 if "NNN0s"
    cmp byte [di+5],0
    jne .r
    mov al,[di+4]
    or al,20h
    cmp al,'s'
.r: ret

; si = entry -> ZF=1 if fbuf is in its description (searched in the far record, not copied)
instr_desc:
    call fav_index
    shl bx,1
    push es
    mov es,[auxseg]
    mov ax,[es:bx+A_SEGS]
    pop es
    test ax,ax
    jz instr.nf0
    push ds
    mov ds,ax
    xor si,si
    mov cx,F_DESC           ; skip the fields before DESCRIPTION
.sk:
    lodsb
    test al,al
    jnz .sk
    loop .sk
    call instr
    pop ds
    ret

instr:                      ; ds:si = text, whether fbuf is contained (case insensitive) -> ZF
    push bx
.s: cmp byte [si],0
    je .nf
    mov di,si
    mov bx,fbuf
.c: mov al,[cs:bx]
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
.nf0:
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
    cmp bl,F_PLAYERS
    jne .collect
    call collect_players
    jmp .cur
.collect:
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
    call pool_add
.next:
    pop bx
    inc bx
    jmp .g
.sort:
    mov bx,pptrs+2
    mov cx,[pcount]
    dec cx
    mov dx,stricmp
    call sort_ptrs
    cmp byte [pfield],F_YEAR
    jne .cur
    call add_decades
.cur:
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
    mov byte [pleft],18h
    mov byte [pright],37h
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

pool_add:                   ; si = string -> new picker item (skipped when full)
    cmp word [pcount],MAXP
    jae .r
    mov di,[ppool]
    cmp di,pool+POOLSZ-L_GENRE-1
    ja .r
    mov bx,[pcount]
    shl bx,1
    mov [pptrs+bx],di
    inc word [pcount]
    call strcpy
    inc di
    mov [ppool],di
.r: ret

; picker items for PLAYERS: every player count 1..MAXPL covered by a game's range
collect_players:
    mov di,pseen
    mov cx,MAXPL+1
    xor al,al
    rep stosb
    xor bx,bx
.g: cmp bx,[gcount]
    jae .emit
    push bx
    shl bx,1
    mov si,[gptrs+bx]
    mov bl,F_PLAYERS
    call get_field
    call parse_range
    jc .n
    cmp dx,-1               ; "a+": only a is listed
    jne .cap
    mov dx,ax
.cap:
    cmp dx,MAXPL
    jbe .b
    mov dx,MAXPL
.b: cmp ax,dx
    ja .n
    mov bx,ax
    mov byte [pseen+bx],1
    inc ax
    jmp .b
.n: pop bx
    inc bx
    jmp .g
.emit:
    mov ax,1
.e: mov bx,ax
    cmp byte [pseen+bx],0
    je .nx
    push ax
    mov di,outbuf
    call num_str
    mov si,outbuf
    call pool_add
    pop ax
.nx:
    inc ax
    cmp ax,MAXPL
    jbe .e
    ret

; YEAR picker: the years are in pptrs[1..pcount-1], sorted -> decades ("1990s") are
; added and moved in front of the years
add_decades:
    mov ax,[pcount]
    mov [pyears],ax
    mov bx,1
.l: cmp bx,[pyears]
    jae .move
    push bx
    shl bx,1
    mov si,[pptrs+bx]
    call is_year4
    jne .n
    mov di,outbuf           ; "NNN0s"
    movsw
    movsb
    mov ax,'0s'
    stosw
    mov byte [di],0
    mov ax,[pcount]         ; the same decade as the previous one?
    cmp ax,[pyears]
    je .add
    mov bx,ax
    shl bx,1
    mov di,[pptrs+bx-2]
    mov si,outbuf
    call stricmp
    test ax,ax
    jz .n
.add:
    mov si,outbuf
    call pool_add
.n: pop bx
    inc bx
    jmp .l
.move:                      ; pptrs[j] -> pptrs[k], pptrs[k..j-1] one place up
    mov dx,1                ; k
    mov ax,[pyears]         ; j
.m: cmp ax,[pcount]
    jae .r
    mov si,ax
    shl si,1
    add si,pptrs
    mov bx,[si]
    mov cx,ax
    sub cx,dx
    mov di,si
    sub si,2
    std
    rep movsw
    cld
    mov [di],bx
    inc dx
    inc ax
    jmp .m
.r: ret

is_year4:                   ; si = text -> ZF=1 if it starts with 4 digits
    push si
    mov cx,4
.d: lodsb
    sub al,'0'
    cmp al,9
    ja .no
    loop .d
    pop si
    cmp al,al
    ret
.no:
    pop si
    ret
