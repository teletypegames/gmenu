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
