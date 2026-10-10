; utf8.asm - UTF-8 text -> code page 437 (METADATA.INI, NAME.TXT); uses ibuf

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

; UTF-8 -> CP437: U+00A0..U+00FF (accented letters CP437 lacks: base letter)
u2c_a0      db ' ',0ADh,9Bh,9Ch,'?',9Dh,'|',15h,'"','c',0A6h,0AEh,0AAh,'-','r','-'
            db 0F8h,0F1h,0FDh,'3',27h,0E6h,14h,0FAh,',','1',0A7h,0AFh,0ACh,0ABh,'?',0A8h
            db 'AAAA',8Eh,8Fh,92h,80h,'E',90h,'EEIIII'
            db 'D',0A5h,'OOOO',99h,'xOUUU',9Ah,'YP',0E1h
            db 85h,0A0h,83h,'a',84h,86h,91h,87h,8Ah,82h,88h,89h,8Dh,0A1h,8Ch,8Bh
            db 'd',0A4h,95h,0A2h,93h,'o',94h,0F6h,'o',97h,0A3h,96h,81h,'yp',98h
; other code points: dw code point, db CP437 character; 0 = end
u2c_pairs   dw 150h
            db 99h                  ; O with double acute -> O umlaut
            dw 151h
            db 94h
            dw 170h
            db 9Ah                  ; U with double acute -> U umlaut
            dw 171h
            db 81h
            dw 2013h
            db '-'
            dw 2014h
            db '-'
            dw 2018h
            db 27h
            dw 2019h
            db 27h
            dw 201Ah
            db ','
            dw 201Ch
            db '"'
            dw 201Dh
            db '"'
            dw 201Eh
            db '"'
            dw 2022h
            db 7
            dw 2026h
            db '.'
            dw 0
