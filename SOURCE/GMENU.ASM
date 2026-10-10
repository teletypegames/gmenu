%ifdef MAPFILE
[map symbols MAPFILE]
%endif
%ifndef COLLECT
%define COLLECT '\COLLECT'  ; default game directory (build.sh: gmenu.ini collect_dir)
%endif

; GMENUPRG.COM - game menu for the \COLLECT folder (DOS / DOSBox)
; Build: nasm -f bin -o GMENUPRG.COM GMENU.ASM
; Launch: GMENU.BAT -> GMENUPRG.COM /B [/A] [game_directory]  (default: \COLLECT)
;         /A = all files are shown in the game's folder, not just executables
; Exit code: 0 = GMENURUN.BAT created next to the program (must be run), 1 = quit, 2 = error
; The program's files (GMENURUN.BAT, GMENU.SEL/.FAV/.STA/.IDX) are in its own folder.
;
; Per folder: METADATA.INI (NAME, GENRE, YEAR, PLAYERS, DESCRIPTION, RUN, SETUP, ARGS),
; if missing: NAME.TXT. The fields after NAME reside in a far (above 64 KB) segment:
; per game a paragraph-aligned record: genre,0,year,0,players,0,desc,0,run,0,setup,0,args,0

bits 16
org 100h

MAXG     equ 600            ; max games
ESZ      equ 64             ; entry size: 13-byte folder name + 51-byte title
MAXF     equ 600            ; max files per folder
FSZ      equ 13             ; filename size (8.3 + 0)
GROWS    equ 22             ; game list rows (2..23)
FROWS    equ 21             ; file list rows in detail view (2..22)
PROWS    equ 21             ; picker list rows (2..22)
DROWS    equ 14             ; description rows in detail view (9..22)
HROWS    equ 21             ; help rows (2..22)
HKW      equ 16             ; help: key cap width
MAXP     equ 250            ; max values in the picker
NREC     equ 10             ; recently played games
MAXPL    equ 16             ; players filter: highest player count listed
NSORT    equ 4              ; sort orders (F9): title, year, genre, most played
WSTEP    equ 3              ; mouse wheel: items per notch
POOLSZ   equ 6000
IBUFSZ   equ 4096           ; METADATA.INI / NAME.TXT read buffer

F_GENRE   equ 0             ; field order in the far record
F_YEAR    equ 1
F_PLAYERS equ 2
F_DESC    equ 3
F_RUN     equ 4             ; RUN / SETUP: command line (file [arguments]), ARGS: for RUN
F_SETUP   equ 5
F_ARGS    equ 6
NKEYS     equ 8             ; METADATA.INI keys: NAME + the far record fields
%include "limits.inc"     ; max field lengths (L_NAME ...)
DBLT      equ 9             ; double click: max ticks (1/18.2 s) between the clicks

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

; Memory after the program's 64 KB segment: stack, per game arrays (aux), far records
STACKSZ  equ 800h           ; stack size (bytes)
A_SEGS   equ 0              ; aux segment: far record segment per game (word)
A_FLAGS  equ A_SEGS+MAXG*2  ;   flags per game (byte): bit 0 = favorite, 1 = title not from
                            ;   METADATA.INI, 2 = no METADATA.INI
A_STAMP  equ A_FLAGS+MAXG   ;   file stamps per game (STSZ bytes, see cache.asm)
A_PLAYS  equ A_STAMP+MAXG*16 ;  starts per game (word)
A_LAST   equ A_PLAYS+MAXG*2 ;   date of the last start (word, DOS format)
AUXSZ    equ A_LAST+MAXG*2

start:
    cld
    mov ax,cs
    add ax,1000h
    mov bx,[2]              ; PSP: top of the memory DOS gave us
    mov [memtop],bx
    sub bx,ax
    jb .nomem
    cmp bx,STACKSZ/16+(AUXSZ+15)/16+100h
    jae .memok
.nomem:
    mov dx,msg_nomem
    mov ah,9
    int 21h
    mov ax,4C02h
    int 21h
.memok:
    cli                     ; own stack segment, so the BSS can fill the 64 KB
    mov ss,ax
    mov sp,STACKSZ
    sti
    add ax,STACKSZ/16
    mov [auxseg],ax
    mov es,ax               ; clear the aux arrays
    xor di,di
    mov cx,AUXSZ
    xor al,al
    rep stosb
    push ds
    pop es
    mov ax,[auxseg]
    add ax,(AUXSZ+15)/16    ; far records after the aux arrays
    mov [heapseg],ax
    call get_progdir
    call parse_args
    cmp byte [argb],0       ; can only be launched from GMENU.BAT (with /B switch)
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
    mov ax,11h              ; CuteMouse WheelAPI: enables wheel data in BH of fn 3
    int 33h
    cmp ax,574Dh
    jne .nomouse
    test cl,1
    jz .nomouse
    mov byte [haswheel],1
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
    call load_favs
    call load_stats
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
%include "cache.asm"
%include "filter.asm"
%include "favorites.asm"
%include "stats.asm"
%include "ui.asm"
%include "launcher.asm"
%include "session.asm"
%include "helpers.asm"
%include "common.asm"
%include "utf8.asm"
%include "data.asm"
