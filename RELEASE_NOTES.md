GMENU – a small game menu for DOS / DOSBox.

**Install:** unzip `GMENU-<version>.zip` to the DOS drive (for example `C:\GMENU`), set the game directory in `GMENU.BAT`, and start the menu with `GMENU` (without `.BAT`). To update an installed menu, replace its `GMENUPRG.COM`. See `README.md` for `METADATA.INI` and all the keys (F1 in the menu).

- Game list with title, genre, year and folder from `METADATA.INI` / `NAME.TXT`; search in titles, folders and descriptions
- Filters by genre, year or decade (`1990s`) and number of players (`1-4` matches 2); favorites, recently played games
- Sort by title, year, genre or most played (F9), random game (F10)
- Ctrl+Enter / double click plays: `RUN=` (with `ARGS=`), or the only executable; Ctrl+S runs `SETUP=`
- Detail page: data, play count and last played date, scrollable multi-line description, file list
- UTF-8 `METADATA.INI` / `NAME.TXT` converted to code page 437
- `GMENU.IDX` cache: quick start with large collections
- The `GMENU` folder can be anywhere; after a game the menu returns to the folder it was started from
