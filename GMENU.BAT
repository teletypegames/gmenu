@echo off
rem Game folder: change \COLLECT below (e.g. D:\GAMES)
:loop
\GMENU\GMENUPRG.COM /B \COLLECT
if errorlevel 1 goto end
call \GMENU\MENURUN.BAT
goto loop
:end
