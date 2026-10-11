#!/bin/sh
# Builds ../GMENUPRG.EXE and ../GMEDIT.EXE with Open Watcom v2, as 16-bit DOS programs.
# WATCOM points to the Open Watcom folder (default: ~/.local/opt/watcom).
# ./build.sh test builds the test versions (the keys come from TESTKEYS.TXT, see
# test/run.py) into build/.
set -e
cd "$(dirname "$0")"
. lib/tools/watcom.sh

OBJ=build
OUT=..
DEFS=
if [ "$1" = test ]; then
  OUT=$OBJ
  DEFS=-dTEST
fi
rm -rf "$OBJ"
mkdir -p "$OBJ"

# gmenu.ini: collect_dir = default game directory (used when GMENU.BAT gives none)
INI_FILE="../gmenu.ini"
COLLECT_DIR='\COLLECT'
if [ -f "$INI_FILE" ]; then
  v=$(sed -n 's/^collect_dir[[:space:]]*=[[:space:]]*//p' "$INI_FILE" | tr -d '\r' | head -n 1)
  [ -n "$v" ] && COLLECT_DIR=$v
fi
case $COLLECT_DIR in
  *\"*) echo "Error: collect_dir must not contain a quote" >&2; exit 1 ;;
esac
COLLECT_C=$(printf '%s' "$COLLECT_DIR" | sed 's/\\/\\\\/g')

LIB="lib/core/str.cpp lib/core/ini.cpp lib/core/text.cpp
     lib/platform/file_system.cpp lib/platform/dos/dos_text_output.cpp
     lib/platform/dos/dos_file_system.cpp lib/platform/dos/dos_screen.cpp lib/platform/dos/bios_keyboard.cpp
     lib/platform/dos/dos_speaker.cpp lib/ui/canvas.cpp lib/testing/scripted_keyboard.cpp lib/testing/screen_dump.cpp"
MENU="lib/ui/list_cursor.cpp
      lib/platform/dos/int33_mouse.cpp lib/platform/dos/bios_clock.cpp lib/platform/dos/dos_idle.cpp
      model/game.cpp model/game_library.cpp model/filter_criteria.cpp model/recent_list.cpp model/visible_games.cpp
      model/metadata.cpp model/locations.cpp model/launch_command.cpp model/pick_list.cpp model/file_list.cpp
      repository/files/index_file.cpp repository/files/folder_game_repository.cpp repository/files/fav_file_repository.cpp
      repository/files/sta_file_repository.cpp repository/files/sel_file_repository.cpp repository/files/bat_launcher_repository.cpp
      repository/files/folder_files_repository.cpp
      service/library_service.cpp service/launch_service.cpp service/picker_service.cpp
      controller/menu_input.cpp controller/list_runner.cpp controller/game_list_controller.cpp
      controller/detail_controller.cpp controller/picker_controller.cpp controller/help_controller.cpp
      view/frame.cpp view/game_list_view.cpp view/detail_view.cpp view/picker_view.cpp view/help_view.cpp
      app/gmenu.cpp"
EDIT="model/locations.cpp model/metadata_form.cpp model/executable_list.cpp
      repository/files/folder_metadata_repository.cpp service/metadata_edit_service.cpp
      controller/edit_form_controller.cpp view/edit_form_view.cpp app/gmedit.cpp"
LIB=$(echo $LIB)
MENU=$(echo $MENU)
EDIT=$(echo $EDIT)

CFLAGS="-0 -ml -os -s -ei -zm -we -zq -bt=dos -i=$WATCOM/h $DEFS"
compile wpp "$CFLAGS" $LIB $(echo $MENU $EDIT | tr ' ' '\n' | sort -u) main_gmedit.cpp
compile wpp "$CFLAGS -dCOLLECT=\"$COLLECT_C\"" main_gmenu.cpp
LINK="system dos option quiet option eliminate option stack=4096"
wlink $LINK name "$OUT/GMENUPRG.EXE" file "$(objs $LIB $MENU main_gmenu.cpp)"
wlink $LINK name "$OUT/GMEDIT.EXE" file "$(objs $LIB $EDIT main_gmedit.cpp)"
rm -f "$OBJ"/*.obj
ls -l "$OUT/GMENUPRG.EXE" "$OUT/GMEDIT.EXE"
