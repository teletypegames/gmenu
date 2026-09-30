# GMENU – game menu for DOS / DOSBox

GMENU is a small DOS menu program (about 6 KB). It lists the game folders in a directory (`\COLLECT` by default) in a table with the title, genre, year and folder of each game. The data comes from the folder's `METADATA.INI` (or, if there is none, from the first line of `NAME.TXT`). Enter opens a detail page with all the game's data and the list of its `.COM`, `.EXE` and `.BAT` files, which you can start. It works with the keyboard and the mouse (including the wheel), it can search in the titles and descriptions, it can filter by genre, year and number of players, and it remembers your favorite and recently played games.

![GMENU screenshot](screenshot.png)

## Files

| File | Purpose |
|---|---|
| `GMENU.BAT` | Starts the menu. The game directory is set here. |
| `GMENUPRG.COM` | The compiled menu program. |
| `SOURCE/GMENU.ASM` | Source code (NASM, 16-bit x86). |
| `SOURCE/build.sh` | Build script (macOS / Linux). |
| `gmenu.ini` | Configuration (game directory, build settings). |
| `MENURUN.BAT` | *Created at runtime:* starts the selected game. |
| `GMENU.SEL` | *Created at runtime:* the last selected game, the search text, the filters, the Tab setting and the recently played games. |
| `GMENU.FAV` | *Created at runtime:* the favorite games (one folder name per line). |

## Building

The build needs [NASM](https://www.nasm.us/):

```sh
brew install nasm        # macOS
sudo apt install nasm    # Debian / Ubuntu
```

Then run this from the `SOURCE` folder:

```sh
cd SOURCE
./build.sh
```

It produces `GMENUPRG.COM` in the parent folder. To build by hand: `nasm -f bin -o ../GMENUPRG.COM GMENU.ASM` (from `SOURCE/`).

## Installation

1. Copy the `GMENU` folder to the root of the DOS C: drive, so you get `C:\GMENU\`. The menu needs only `GMENU.BAT` and `GMENUPRG.COM`.
2. Put the games in one common directory. Give each game its own folder, and put a `METADATA.INI` (or at least a `NAME.TXT`) in each folder:
   ```
   C:\COLLECT\
     DOOM\
       METADATA.INI  <- title, genre, year, players, description
       DOOM.EXE
     PRINCE\
       NAME.TXT      <- first line is the displayed name, e.g. "Prince of Persia"
       PRINCE.EXE
   ```
   `METADATA.INI` looks like this (every line is optional, the keys are not case sensitive, the `[GAME]` line and lines starting with `;` are ignored):
   ```ini
   [GAME]
   NAME=Doom
   GENRE=Shooter
   YEAR=1993
   PLAYERS=1-4
   DESCRIPTION=The whole description in one line...
   ```
   If `METADATA.INI` has no `NAME`, the title comes from `NAME.TXT`. If there is neither, the menu shows the folder name. Missing fields are shown as `-`.
3. *(Optional)* In the `[autoexec]` section of your DOSBox config, add the menu to the PATH and start it at boot:
   ```
   [autoexec]
   mount c /Users/.../Dosbox
   c:
   path Z:\;C:\GMENU
   gmenu
   ```

### Setting the game directory

In `GMENU.BAT`, the last argument on the `GMENUPRG.COM` line is the game directory:

```bat
\GMENU\GMENUPRG.COM /B \COLLECT
```

Add `/A` (for example `/B /A \COLLECT`) to show all files of a game on the detail page by default, not only the executables. (Tab switches between the two at any time.)

You can change it, for example to `\MYGAMES` or `D:\GAMES`. If you give another drive, the menu switches to that drive before it starts the game and switches back afterwards. If you leave the directory out, the menu uses `\COLLECT`. Keep the `/B` switch (see below).

## Usage

Start the menu:

```
\GMENU\GMENU
```

(or just `gmenu` if `C:\GMENU` is in the PATH)

### Game list

| Key / mouse | Action |
|---|---|
| ↑ ↓, PgUp/PgDn, Home/End | Move in the list (↑ on the first game jumps to the last one, ↓ on the last to the first) |
| Mouse wheel | Move 3 games up / down |
| Enter, → or left click | Open the game's detail page |
| Typing | Search: the list narrows to the matches as you type (it searches the title, the folder name and the description, and it ignores case) |
| Backspace | Delete one character from the search |
| F2 / F3 / F4 | Filter by genre / year / number of players (pick a value from a list, `(all)` removes the filter) |
| F5 | Clear all filters (including favorites only) |
| F6 | Mark / unmark the selected game as a favorite (yellow `*` at the end of the row) |
| F7 | Show only the favorites / all games |
| F8 | Recently played games (the last 10, newest first); Enter opens the game's detail page |
| Esc or right click | The first press clears the search, the next one the filters, the last one quits |
| Click on the header / footer | Page up / down |

When a search or a filter is active, the header shows how many games match, for example `Found: 42 of 318`. The number in the top right corner is the position in the list.

### Detail page (inside a game)

On the left: title, genre, year, players, folder and the description. On the right: the executable files, or all files after Tab (executables are marked with ►).

| Key / mouse | Action |
|---|---|
| ↑ ↓, PgUp/PgDn, Home/End | Move in the file list (wraps around at the ends) |
| Mouse wheel | Move 3 files up / down |
| Enter, → or left click | Run the file (only `.COM`, `.EXE`, `.BAT`) |
| Tab | Show all files / only the executables |
| Letter / digit | Jump to the next file that starts with it |
| F6 | Mark / unmark the game as a favorite |
| Esc, ← or right click | Back to the game list |

When you quit a game, the menu starts again on the same game, with the same search and filters.

The mouse wheel needs a driver with the CuteMouse WheelAPI (for example DOSBox Staging's built-in mouse driver or CuteMouse 2.x). Without it, the menu works as before, only the wheel does nothing.

## How it works

`GMENUPRG.COM` is not in memory while a game runs, so the game gets all the free memory:

1. `GMENU.BAT` runs `GMENUPRG.COM /B <directory>`.
2. When you choose a file, the program writes `\GMENU\MENURUN.BAT` and exits with code 0. When you quit, the exit code is 1, and on an error it is 2. `MENURUN.BAT` looks like this:
   ```bat
   @echo off
   C:
   cd \COLLECT\DOOM
   DOOM.EXE          (for a .BAT file: call XY.BAT)
   C:
   cd \
   ```
3. `GMENU.BAT` runs it (with `call`) and then starts the menu again.

The `/B` switch tells the program that `GMENU.BAT` started it. Without the switch, the program does not open the menu. It only prints that you must use `GMENU.BAT`. If it ran on its own, it would write `MENURUN.BAT`, but nothing would start the game.

## Limitations

- At most 600 game folders and 600 files per folder. The recent list keeps the last 10 games.
- Titles can be up to 50 characters long, genres 24, year and players 10, descriptions 2000 (the detail page shows about 700 characters, then `...`). `METADATA.INI` is read up to 4 KB.
- The screen uses code page 437, so accented (UTF-8) characters in `METADATA.INI` / `NAME.TXT` may display incorrectly.

## License

MIT – see [LICENSE.md](LICENSE.md).
