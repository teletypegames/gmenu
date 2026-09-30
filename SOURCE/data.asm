; data.asm - string constants, initialized data, BSS

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
