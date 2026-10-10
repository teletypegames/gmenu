; data.asm - string constants, initialized data, BSS

colldir     db COLLECT,0
            times colldir+66-$ db 0
str_starf   db '*.*',0
str_games   db 'Games',0
str_nametxt db '\NAME.TXT',0
str_metaini db '\METADATA.INI',0
str_star    db '\*.*',0
str_lpar    db '  (',0
str_rpar    db ')',0
f_runner    db 'GMENURUN.BAT',0  ; files in the program's folder (progdir)
f_sel       db 'GMENU.SEL',0
f_fav       db 'GMENU.FAV',0
f_sta       db 'GMENU.STA',0
f_idx       db 'GMENU.IDX',0
f_editor    db 'GMEDIT.COM',0
s_edarg     db ' /B ',0
zeros       dw 0,0,0,0
s_echo      db '@echo off',13,10,0
s_cd        db 'cd ',0
s_call      db 'call ',0
s_cdroot    db 'cd \',0
crlf        db 13,10,0
key_tab     db 'NAME',0,'GENRE',0,'YEAR',0,'PLAYERS',0,'DESCRIPTION',0
            db 'RUN',0,'SETUP',0,'ARGS',0,0
kv_lim      dw L_NAME,L_GENRE,L_YEAR,L_PLAYERS,L_DESC,L_CMD,L_CMD,L_CMD
footer_g    db '~Type~ Search ~Ctrl+Enter~ Play ~F1~ Help ~F2-4~ Filter ~F6/7~ Fav ~F9~ Sort ~F10~ Random',0
footer_f    db '~',18h,19h,'~ Move ~Enter~ Run ~Tab~ All files ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back',0
footer_fa   db '~',18h,19h,'~ Move ~Enter~ Run ~Tab~ Executables ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back',0
footer_p    db '~',18h,19h,'~ Move  ~Enter~ Select  ~A-Z~ Jump  ~Esc~ Cancel',0
str_help    db 'Keys',0
; help: 1,title,0 = section; 0 = empty row; key,0,description,0; 0FFh = end
help_txt    db 1,'Game list',0
            db 'Enter, click',0,'open the game page',0
            db 'Ctrl+Enter',0,'play (or double click)',0
            db 'Ctrl+S',0,'run the setup',0
            db 'Ctrl+E',0,'edit METADATA.INI',0
            db 'Typing',0,'search',0
            db 'F2  F3  F4',0,'filter: genre, year, players',0
            db 'F5',0,'clear the filters',0
            db 'F6  F7',0,'favorite, favorites only',0
            db 'Shift+F7',0,'no METADATA.INI only',0
            db 'F8',0,'recently played',0
            db 'F9  F10',0,'sort order, random game',0
            db 'Esc',0,'clear search, filters, quit',0
            db 0
            db 1,'Game page',0
            db 'Enter, click',0,'run the file',0
            db 'Tab',0,'all files / executables',0
            db 'A-Z  0-9',0,'jump to a file',0
            db 'Ctrl+PgUp/Dn',0,'scroll the description',0
            db 'F6',0,'favorite',0
            db 'Ctrl+E',0,'edit METADATA.INI',0
            db 'Esc  ',1Bh,0,'back to the list',0
            db 0FFh
help_keys   db 48h,-1,50h,1,49h,-HROWS,51h,HROWS,47h,-128,4Fh,127,0
footer_h    db '~',18h,19h,' PgUp PgDn~ Scroll  ~Esc~ Close',0
str_search  db 'Search: ',0
str_searchh db '   (Backspace: delete, Esc: clear search)',0
str_ctitle  db 'Title',0
str_cgenre  db 'Genre',0
str_cyear   db 'Year',0
str_cfolder db 'Folder',0
str_hgenre  db '  Genre: ',0
str_hyear   db '  Year: ',0
str_hplayers db '  Players: ',0
str_hfound  db '  Found: ',0
str_hof     db ' of ',0
str_hfav    db '  Favorites',0
str_hnoini  db '  No METADATA.INI',0
str_hsort   db '  Sort: ',0
sort_names  db 'title',0,'year',0,'genre',0,'most played',0
sort_cmps   dw cmp_title,cmp_year,cmp_genre,cmp_plays
sort_gaps   dw 701,301,132,57,23,10,4,1,0
str_precent db 'Recently played',0
str_pgenre  db 'Filter by genre',0
str_pyear   db 'Filter by year',0
str_pplayers db 'Filter by players',0
str_all     db '(all)',0
str_nomatch db 'No games match the search / filters.  (Esc: clear, F5: clear filters)',0
lbl_genre   db 'Genre',0
lbl_year    db 'Year',0
lbl_players db 'Players',0
lbl_folder  db 'Folder',0
lbl_played  db 'Played',0
str_never   db 'never',0
str_times   db 'x, last ',0
str_dash    db '-',0
str_nodesc  db 'No description.',0
str_exefiles db 'Executables',0
str_allfiles db 'All files',0
str_noexe   db '(no executables)',0
str_nofiles db '(no files)',0
str_tabhint db 'Tab: all files',0
msg_usebat  db 'Start the menu with GMENU.BAT (type GMENU).',13,10,'$'
msg_nomem   db 'Not enough memory.',13,10,'$'
msg_nogames db 'No game folders found in: $'
crlf_d      db 13,10,'$'
gcount   dw 0
fcount   dw 0
gsel     dw 0
gtop     dw 0
vcount   dw 0
selhdr   db 'GS',SELVER     ; selhdr..recent consecutive (GMENU.SEL)
selname  times 13 db 0
flen     db 0
fbuf     times 32 db 0
fgenre   times L_GENRE+1 db 0
fyear    times L_YEAR+1 db 0
fplayers times L_PLAYERS+1 db 0
showall  db 0
favonly  db 0
sortmode db 0
noini    db 0               ; only the games without METADATA.INI
recent   times NREC*13 db 0
%ifdef TEST
tkp      dw testkeys
testkeys dw TESTKEYS,0
%endif
hasmouse db 0
haswheel db 0
prevbtn  db 0
mwheel   db 0
listmode db 0
rseed    dw 0
mdbl     db 0               ; the last click was a double click
lclick_t dw 0               ; time and row of the previous click
lclick_r dw -1

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
s_cmp   resw 1
s_gap   resw 1
s_n     resw 1
s_i     resw 1
s_key   resw 1
sortf   resb 1
cmp_a   resw 1
cmp_b   resw 1
cmpbuf  resb L_GENRE+2
idxh    resw 1              ; GMENU.IDX handle (0 = not in use)
idxdirty resb 1             ; GMENU.IDX must be rewritten
ihbuf   resb 70
stamp   resb 16
gsrc    resb 1
reclen  resw 1
recseg  resw 1
fdta    resb 44
staseg  resw 1              ; GMENU.STA contents (far)
stan    resw 1              ; records in it
stah    resw 1
progdir resb 80             ; folder of GMENUPRG.COM, with a trailing '\'
fpath   resb 96             ; progdir + file name
startdir resb 66            ; current folder at start (restored after a game)
heapseg resw 1
auxseg  resw 1
memtop  resw 1
gslot   resw 1
pfield  resb 1
pdest   resw 1
ptitle  resw 1
pcount  resw 1
ppool   resw 1
pleft   resb 1              ; picker box columns
pright  resb 1
pyears  resw 1              ; year picker: end of the years
pseen   resb MAXPL+1        ; players picker: counts present
favh    resw 1
wr_w    resb 1
wr_rows resw 1
wr_tot  resw 1
wr_skip resw 1
htop    resw 1              ; help: first row shown, last possible first row
hmax    resw 1
dscroll resw 1              ; detail view: description lines scrolled off the top
dlines  resw 1              ; detail view: wrapped description lines
rattr   resb 1
rexec   resb 1
jch     resb 1
wd_t    resw 1
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
kv_off  resw NKEYS
kv_len  resw NKEYS
cmdbuf  resb 2*L_CMD+2      ; command line of the file to run
fldbuf  resb L_DESC+2
gptrs   resw MAXG
vptrs   resw MAXG
fptrs   resw MAXF           ; shared area: file list / picker (pptrs)
fnames  resb MAXF*FSZ       ; shared area: filenames / ibuf (read buffer) / pool (picker)
entries resb MAXG*ESZ
bss_end:

pptrs   equ fptrs
pool    equ fnames
ibuf    equ fnames
