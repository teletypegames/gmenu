#!/usr/bin/env python3
"""Screen tests for GMENUPRG.COM in DOSBox.

Each case builds the program with -DTEST and a list of keystrokes (TESTKEYS),
runs it in DOSBox on a copy of test/collect, and compares the result with
test/expected/<case>.txt: the text screen the program saved (SCREEN.BIN) when
the keystrokes ran out, or GMENURUN.BAT if the program started a game.

usage: run.py [--update] [case ...]
  --update   write the results to expected/ instead of comparing
DOSBOX=<path> selects the emulator (default: dosbox-staging / dosbox in PATH,
or DOSBox Staging.app on macOS).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.dirname(HERE)
CP437_LOW = " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼"
NAMED_KEYS = {
    "Enter": 0x1C0D, "Esc": 0x011B, "Tab": 0x0F09, "Bksp": 0x0E08,
    "Up": 0x4800, "Down": 0x5000, "Left": 0x4B00, "Right": 0x4D00,
    "PgUp": 0x4900, "PgDn": 0x5100, "Home": 0x4700, "End": 0x4F00,
    "CtrlEnter": 0x1C0A, "CtrlS": 0x1F13, "CtrlPgUp": 0x8400, "CtrlPgDn": 0x7600,
    "WheelUp": 0xFF02, "WheelDn": 0xFF03,
    **{f"F{n}": (0x3A + n) << 8 for n in range(1, 11)},
}


def find_dosbox():
    if os.environ.get("DOSBOX"):
        return os.environ["DOSBOX"]
    for name in ("dosbox-staging", "dosbox"):
        path = shutil.which(name)
        if path:
            return path
    app = "/Applications/DOSBox Staging.app/Contents/MacOS/dosbox"
    if os.path.exists(app):
        return app
    sys.exit("DOSBox not found (set DOSBOX=<path>)")


def parse_keys(text):
    """'Down Down Enter "abc" 0x3B00' -> [0x5000, 0x5000, 0x1C0D, 0x61, 0x62, 0x63, 0x3B00]"""
    keys = []
    for tok in re.findall(r'"[^"]*"|\S+', text):
        if tok.startswith('"'):
            keys += [ord(c) for c in tok[1:-1]]
        elif tok in NAMED_KEYS:
            keys.append(NAMED_KEYS[tok])
        else:
            keys.append(int(tok, 0))
    return keys


def load_cases():
    """cases.txt: name | keys of run 1 ; keys of run 2 ... | program arguments (optional)"""
    cases = []
    with open(os.path.join(HERE, "cases.txt"), encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [p.strip() for p in line.split("|")]
            name, runs = parts[0], [parse_keys(r) for r in parts[1].split(";")]
            args = parts[2] if len(parts) > 2 and parts[2] else r"/B \COLLECT"
            cases.append((name, runs, args))
    return cases


def render(screen):
    rows = []
    for r in range(25):
        chars = screen[r * 160:(r + 1) * 160:2]
        rows.append("".join(CP437_LOW[c] if c < 32 else bytes([c]).decode("cp437")
                            for c in chars).rstrip())
    return "\n".join(rows).rstrip("\n") + "\n"


def normalize(text):
    return re.sub(r"\d{4}-\d\d-\d\d", "YYYY-MM-DD", text)


def build(keys, out):
    words = ",".join(f"0{k:04X}h" for k in keys) or "0"
    subprocess.run(["sh", os.path.join(SOURCE, "build.sh"), "-DTEST", f"-DTESTKEYS={words}"],
                   env={**os.environ, "OUT": out}, check=True, stdout=subprocess.DEVNULL)


def run_dosbox(dosbox, cdrive, args):
    version = subprocess.run([dosbox, "--version"], capture_output=True, text=True).stdout
    cmds = [f"mount c {cdrive}", "c:", rf"\GMENU\GMENUPRG.COM {args}", "exit"]
    cmd = [dosbox]
    if "staging" in version.lower():
        cmd += ["--noprimaryconf", "--nolocalconf"]
    else:
        conf = os.path.join(cdrive, "..", "test.conf")
        with open(conf, "w") as f:
            f.write("[sdl]\nautolock=false\n[cpu]\ncycles=max\n")
        cmd += ["-conf", conf]
    for c in cmds:
        cmd += ["-c", c]
    env = {**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"}
    subprocess.run(cmd, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                   timeout=60)


def run_case(dosbox, tmp, name, runs, args):
    cdrive = os.path.join(tmp, name, "c")
    gmenu = os.path.join(cdrive, "GMENU")
    os.makedirs(gmenu)
    shutil.copytree(os.path.join(HERE, "collect"), os.path.join(cdrive, "COLLECT"))
    result = ""
    for keys in runs:               # the runs share GMENU.SEL / .FAV / .STA / .IDX
        for f in ("SCREEN.BIN", "GMENURUN.BAT"):
            if os.path.exists(os.path.join(gmenu, f)):
                os.remove(os.path.join(gmenu, f))
        build(keys, os.path.join(gmenu, "GMENUPRG.COM"))
        run_dosbox(dosbox, cdrive, args)
        screen = os.path.join(gmenu, "SCREEN.BIN")
        runner = os.path.join(gmenu, "GMENURUN.BAT")
        if os.path.exists(screen):
            with open(screen, "rb") as f:
                result = render(f.read())
        elif os.path.exists(runner):
            with open(runner, "rb") as f:
                result = "GMENURUN.BAT:\n" + f.read().decode("cp437").replace("\r\n", "\n")
        else:
            result = "(no SCREEN.BIN or GMENURUN.BAT)\n"
    return normalize(result)


def main():
    argv = sys.argv[1:]
    update = "--update" in argv
    selected = [a for a in argv if a != "--update"]
    dosbox = find_dosbox()
    cases = [c for c in load_cases() if not selected or c[0] in selected]
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, runs, args in cases:
            got = run_case(dosbox, tmp, name, runs, args)
            path = os.path.join(HERE, "expected", name + ".txt")
            if update:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(got)
                print(f"updated  {name}")
                continue
            want = open(path, encoding="utf-8").read() if os.path.exists(path) else None
            if got == want:
                print(f"ok       {name}")
            else:
                failed += 1
                print(f"FAIL     {name}")
                with open(os.path.join(HERE, "expected", name + ".got"), "w",
                          encoding="utf-8") as f:
                    f.write(got)
    if not update:
        print(f"{len(cases) - failed}/{len(cases)} passed")
        if failed:
            print("(the results of the failed cases are in expected/*.got)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
