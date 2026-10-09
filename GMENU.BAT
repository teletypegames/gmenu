@echo off
rem Game folder: change \COLLECT below (e.g. D:\GAMES). Add /A to list all files by default.
rem Start the menu as GMENU or C:\GMENU\GMENU (without .BAT): %0 locates GMENUPRG.COM
rem and GMENURUN.BAT, so the GMENU folder can be anywhere.
:loop
%0PRG.COM /B \COLLECT
if errorlevel 1 goto end
call %0RUN.BAT
goto loop
:end
