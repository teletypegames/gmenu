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
    jmp game_loop

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

%include "browser.asm"
%include "catalog.asm"
%include "filter.asm"
%include "ui.asm"
%include "launcher.asm"
%include "session.asm"
%include "helpers.asm"
%include "data.asm"
