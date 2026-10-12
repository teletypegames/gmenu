@echo off
rem Game folder: change \COLLECT below (e.g. D:\GAMES). Add /A to list all files by default.
rem Started as GMENU or C:\APPS\GMENU\GMENU (without .BAT), %0 locates GMENUPRG.EXE and
rem GMENURUN.BAT, so the GMENU folder can be anywhere. Started with .BAT (e.g. from a file
rem manager), they are taken from C:\APPS\GMENU: change it below if GMENU is elsewhere.
set GMENU=%0
if not exist %0PRG.EXE set GMENU=C:\APPS\GMENU\GMENU
:loop
%GMENU%PRG.EXE /B \COLLECT
if errorlevel 1 goto end
call %GMENU%RUN.BAT
goto loop
:end
set GMENU=
