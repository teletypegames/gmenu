GMENU – a small game menu for DOS / DOSBox.

**Install:** unzip `GMENU-<version>.zip` to the DOS drive (for example `C:\APPS\GMENU`), set the game directory in `GMENU.BAT`, and start the menu with `GMENU` (without `.BAT`). To update an installed menu, copy `GMENUPRG.EXE`, `GMEDIT.EXE` and the new `GMENU.BAT` into its folder (keep your game directory in `GMENU.BAT`). See `README.md` for `METADATA.INI` and all the keys (F1 in the menu).

**New in 2.2.1:**

- `GMENU.BAT` starts the menu when it is started with the `.BAT` extension too (for example from Total Commander or Windows 98): it then runs the menu from `C:\APPS\GMENU`, the default folder (where DOSINST installs it). To update, copy the new `GMENU.BAT` (keep your game directory in it)

**New in 2.1.0:**

- The code is rebuilt in layers (model, repositories, services, controllers, views) on doslib, the library GMENU shares with DOSINST; the menu and GMEDIT work as in 2.0.0, and their files are the same. To update, copy `GMENUPRG.EXE` and `GMEDIT.EXE` (`GMENU.BAT` did not change)
- `GMENU.IDX` and `GMENU.STA` no longer get random bytes after the folder names
- Unit tests on the host and a layer check run in the pipeline before the DOSBox tests

**New in 2.0.0:**

- GMENU is rewritten in C++ (Open Watcom), like DOSINST; the menu works as in 1.1.1, and its files (`GMENU.SEL`, `.FAV`, `.STA`, `.IDX`) are kept
- The programs are `GMENUPRG.EXE` and `GMEDIT.EXE` (they were `.COM`): an installed menu needs the new `GMENU.BAT` too, which starts `GMENUPRG.EXE`. DOSINST updates all of them and removes the old `.COM` files

**New in 1.1.1:**

- GMENU is also in the [DOSINST](https://git.teletypegames.org/dos/dosinst) program store (`ftp://ftp.teletypegames.org`): a DOS machine running DOSINST can install and update it from there. The menu itself is the same as in 1.1.0

**New in 1.1.0:**

- `GMEDIT.COM`: creates or edits a game's `METADATA.INI` with a form (Ctrl+E in the list or on the game page). A new file starts with the folder's `NAME.TXT`, its executable and setup program; editing rewrites only the changed values, comments and the other lines are kept
- Shift+F7: only the games without `METADATA.INI`
- F1: the keys in a colored table that scrolls
- `GMENU.SEL` has a new format: the last position and filters are reset once after the update

**All features:**


- Game list with title, genre, year and folder from `METADATA.INI` / `NAME.TXT`; search in titles, folders and descriptions
- Filters by genre, year or decade (`1990s`) and number of players (`1-4` matches 2); favorites, recently played games
- Sort by title, year, genre or most played (F9), random game (F10)
- Ctrl+Enter / double click plays: `RUN=` (with `ARGS=`), or the only executable; Ctrl+S runs `SETUP=`
- Detail page: data, play count and last played date, scrollable multi-line description, file list
- UTF-8 `METADATA.INI` / `NAME.TXT` converted to code page 437
- `GMENU.IDX` cache: quick start with large collections
- The `GMENU` folder can be anywhere; after a game the menu returns to the folder it was started from
- `METADATA.INI` editor (`GMEDIT.EXE`), games without `METADATA.INI` (Shift+F7)
