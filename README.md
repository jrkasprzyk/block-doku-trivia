# Trivia-Doku

A block-doku puzzle game with a trivia twist, built in C++ with raylib.

Place polyomino pieces on a 9×9 board. Clear full rows, columns, and 3×3 squares for points. Special **trivia blocks** trigger a question when their placement completes a clear — answer correctly for a bonus, answer wrong and a cleared cell comes back as an **unclearable stone**. Get 3 correct in a row to shatter all stones and trigger a comeback.

## Status

**Day 5.** The core puzzle loop is complete and playable end-to-end:

- **Board** — 9×9 grid with 3×3 sub-square highlights.
- **Pieces** — a tray of 3 random polyomino shapes; a fresh tray is dealt when all 3 are placed.
- **Drag-and-drop** — click a tray piece to lift it; a ghost follows the cursor. Hover over the board to snap to the grid with a green (legal) or red (illegal) preview; release to place or cancel.
- **Clearing** — any full row, column, or 3×3 square clears. Cleared cells flash yellow for ~0.3 s before disappearing. Score: 1 pt per cell placed + 10 pts per cleared cell.
- **Game over** — after each placement, the engine checks whether any remaining tray piece fits anywhere. If none can, a "GAME OVER" panel shows the final score. Press **R** to restart (resets board, tray, score, and streak). ESC quits.

Trivia blocks, stone penalties, and streak mechanics are stubbed but not yet wired to UI (Days 6–7).

## Prerequisites (Windows native)

1. **Visual Studio Build Tools** (MSVC compiler) — download from https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
   - During install, check "Desktop development with C++"
2. **CMake** (3.20+) — https://cmake.org/download/ — add to PATH during install
3. **Git** — https://git-scm.com/download/win
4. **vcpkg** — the Microsoft C++ package manager. Install it once, anywhere:
   ```powershell
   cd C:\
   git clone https://github.com/microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   ```

## Install dependencies

From the vcpkg directory:

```powershell
.\vcpkg install raylib:x64-windows nlohmann-json:x64-windows
```

Mental model: this is your `pip install raylib nlohmann-json`. First run takes a few minutes; subsequent runs are fast.

## Build

**Option A — Command Prompt or PowerShell (recommended):**

```powershell
# First time (configures CMake):
.\configure.bat

# Build:
.\build.bat
```

**Option B — Git Bash / MSYS2:**

```bash
# First time:
bash build.sh   # configure manually if needed, see configure.bat

# Build:
bash build.sh
```

**Option C — Developer Command Prompt** (vcvars64 already sourced):

```powershell
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

```powershell
.\build\trivia_doku.exe
```

A dark 900×700 window opens titled **TRIVIA-DOKU**. You'll see:

- A 9×9 grid (the board) with darker 3×3 sub-square borders.
- Three polyomino pieces in the tray below the board — click and drag one onto the board.
- A HUD on the right: score, streak counter, and keyboard hints.

When a piece completes a full row, column, or 3×3 square, those cells flash yellow before clearing. When the board fills up and no piece fits, a **GAME OVER** overlay shows your final score — press **R** to restart or **ESC** to quit.

## Project layout

```
trivia-doku/
├── src/
│   ├── main.cpp          # entry point + game loop
│   ├── Board.{h,cpp}     # 9x9 grid logic, line/column/square detection, stones
│   └── Renderer.{h,cpp}  # the ONLY file that includes raylib.h
├── assets/
│   └── trivia.json       # question bank — you own this, edit freely
├── CMakeLists.txt
└── README.md
```

**Architectural rule:** game logic never touches raylib. `Board` knows nothing about pixels. `Renderer` asks `Board` what's where and draws it. This keeps the logic unit-testable and the drawing swappable.

## Trivia bank

Questions live in `assets/trivia.json`. The bank currently has **56 questions** across six categories:

| Category | Count |
|---|---|
| motorsport | 8 |
| music | 8 |
| film | 8 |
| general | 12 |
| asia | 12 |
| meta *(questions about the game itself)* | 8 |

Add as many questions as you like — the schema is documented at the top of the file. The loader shuffles at startup and tracks what has been asked this session, so repeats are rare.

Scoring multipliers by difficulty: easy 1.5×, medium 2×, hard 3×.

## Roadmap

- [x] **Day 1** — window + empty board + HUD
- [x] **Day 2** — Piece definitions, piece tray, render a set of 3 offered pieces
- [x] **Day 3** — Drag-and-drop placement with legality checks
- [x] **Day 4** — Line/column/square clearing animations
- [x] **Day 5** — Game over detection
- [ ] **Day 6** — Trivia block flag + modal dialog + JSON loader
- [ ] **Day 7** — Stones, streak tracking, redemption mechanic
- [ ] **Day 8** — Help overlay (?), quips, polish

## Credits

Joseph Kasprzyk, with Claude as co-conspirator. Inspired by Block-Doku and Balatro, with a trivia twist.

## Developer: scripts package and editable install

The repository includes a small `scripts/` package that provides command-line helpers and utilities
for working with the trivia JSON (`assets/trivia.json`). To make the `scripts` package importable
and to install convenient console entry points, install the project into your Python virtual
environment in editable mode:

```powershell
# from the project root, using the repo's venv
& .venv\Scripts\Activate.ps1
pip install -e .
```

After installation you'll have console scripts available in the venv:

### Usage examples (console scripts and module run)

Examples below show both the installed console-script names (dashed and underscored aliases are available) and how to run the same tools directly with the venv Python using `-m` (no install required).

Activate the repo venv (Windows PowerShell):

```powershell
& .venv\Scripts\Activate.ps1
```

Add a question interactively (installed console script or module):

```powershell
add-trivia                 # dash-name
add_trivia                 # underscore alias
# or run the module directly with the venv Python:
& .venv\Scripts\python.exe -m scripts.add_trivia assets/trivia.json
```

Extract trivia examples (see available flags below):

```powershell
# list categories
extract-trivia --list-categories

# show prompt+answer fields from 'music', limit 5
extract-trivia -c music -F prompt,answer -n 5

# interactive picker
extract-trivia -i

# module form (no install)
& .venv\Scripts\python.exe -m scripts.extract_trivia --list-categories
```

Validate trivia JSON:

```powershell
validate-trivia             # uses assets/trivia.json by default
validate_trivia assets/trivia.json
```

Convert audio examples:

```powershell
# dry-run convert WAV -> OGG under assets
convert-audio --from-ext wav --to-ext ogg --dry-run

# convert a single file and overwrite destination
convert-audio --file assets/bg_wav/intro.wav --to-ext ogg --overwrite

# explicitly specify ffmpeg executable
convert-audio --file assets/bg_wav/intro.wav --to-ext ogg --ffmpeg C:\ffmpeg\bin\ffmpeg.exe
```

Quick CLI reference (major flags)

- `add-trivia [path]` — interactive authoring; optional path to trivia JSON (default: `assets/trivia.json`).
- `extract-trivia`:
   - `--file/-f PATH` — path to trivia JSON (default: `assets/trivia.json`)
   - `--list-categories` — list categories and counts
   - `--category/-c NAME` — category name or index
   - `--fields/-F F1,F2` — comma-separated fields to print
   - `--list-fields` — list available fields for the selected category
   - `--count` — print counts for selected category
   - `--limit/-n N` — limit results
   - `--json-lines` — output items as JSON Lines
   - `-i/--interactive` — interactive picker
- `validate-trivia [path]` — validate schema and content; optional path (default `assets/trivia.json`).
- `convert-audio`:
   - `--file` — convert a single file
   - `--src` — source directory to scan (default: `assets`)
   - `--from-ext` — source extension (default: `wav`)
   - `--to-ext` — destination extension (default: `ogg`)
   - `--quality` — Vorbis quality (0-10, default 5)
   - `--bitrate` — bitrate for MP3 (default `192k`)
   - `--overwrite`, `--dry-run`, `--ffmpeg` — overwrite existing, dry-run commands, explicit ffmpeg path

Notes:
- Console script names are registered with both dashed and underscored variants (e.g. `add-trivia` and `add_trivia`) so you can use whichever feels natural in your shell.
- For reproducible verification and CI use the venv Python: `& .venv\Scripts\python.exe -m scripts.<name>` (Windows PowerShell).

- `add-trivia` — interactive question authoring (maps to `scripts.add_trivia:main`)
- `extract-trivia` — extract/list trivia fields (maps to `scripts.extract_trivia:main`)
- `validate-trivia` — validate the trivia JSON (maps to `scripts.validate_trivia:main`)
- `convert-audio` — audio conversion helpers (maps to `scripts.convert_audio:main`)

Within Python code you can now import the shared helpers directly:

```python
from scripts.cli_helpers import load, save, pick, confirm
```

This repo previously included fallback imports to support running the scripts from the
repository without installation; those have been removed in favor of the package-style
imports above. Installing editable (`pip install -e .`) is the recommended workflow for
development.
