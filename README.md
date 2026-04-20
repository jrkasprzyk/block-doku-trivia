# Trivia-Doku

A block-doku puzzle game with a trivia twist, built in C++ with raylib.

Place polyomino pieces on a 9×9 board. Clear full rows, columns, and 3×3 squares for points. Special **trivia blocks** trigger a question when their placement completes a clear — answer correctly for a bonus, answer wrong and a cleared cell comes back as an **unclearable stone**. Get 3 correct in a row to shatter all stones and trigger a comeback.

## Features

- **Board** — 9×9 grid with 3×3 sub-square highlights.
- **Pieces** — a tray of 3 random polyomino shapes; a fresh tray is dealt when all 3 are placed.
- **Drag-and-drop** — click a tray piece to lift it; a ghost follows the cursor. Hover over the board to snap to the grid with a green (legal) or red (illegal) preview; release to place or cancel.
- **Gamepad support** — D-pad/stick to move cursor, LB/RB to cycle tray, A to pick up and place.
- **Clearing** — any full row, column, or 3×3 square clears. Cleared cells flash before disappearing. Score: 1 pt per cell placed + 10 pts per cleared cell.
- **Trivia** — ~20% of pieces are golden trivia blocks. Placing one that completes a clear pauses the game and shows a question.
- **Stones** — a wrong answer places a stone on the board that blocks future clears until shattered.
- **Streak** — 3 correct answers in a row shatters all stones and resets the streak.
- **Scoring multipliers** — easy 1.5×, medium 2×, hard 3×.
- **Music + SFX** — background playlist from `assets/bg_music/`, sound effects for all major events.
- **Fullscreen** — F11 toggles borderless fullscreen; maximizing the window auto-enters it.
- **Game over** — when no tray piece fits anywhere, a panel shows your final score. Press **R** to restart.

## Prerequisites (Windows native)

1. **Visual Studio Build Tools** (MSVC compiler) — https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
   - During install, check "Desktop development with C++"
2. **CMake** (3.20+) — https://cmake.org/download/ — add to PATH during install
3. **Git** — https://git-scm.com/download/win
4. **vcpkg** — install once, anywhere:
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

## Build

**Option A — PowerShell (recommended):**

```powershell
# First time (configure CMake):
.\configure.bat

# Build:
.\build.bat
```

**Option B — Git Bash / MSYS2:**

```bash
bash build.sh
```

**Option C — Developer Command Prompt** (vcvars64 already sourced):

```powershell
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

At configure time, CMake downloads the trivia JSON from jsdelivr and bundles it with the build. If the network is unavailable it falls back to the local copy in `assets/`.

## Run

```powershell
.\build\trivia_doku.exe
```

## Project layout

```
trivia-doku/
├── src/
│   ├── main.cpp              # entry point + game loop
│   ├── Board.{h,cpp}         # 9×9 grid logic, clearing, stones
│   ├── Piece.{h,cpp}         # polyomino shapes and tray generation
│   ├── Renderer.{h,cpp}      # the ONLY file that includes raylib.h
│   └── TriviaBank.{h,cpp}    # JSON loader and shuffled-deck question picker
├── assets/
│   ├── bg_music/             # background music playlist (.ogg)
│   ├── sfx/                  # sound effects (.wav)
│   └── jrb_industries_trivia.0.1.300.json  # local fallback trivia file
├── scripts/
│   └── convert_audio.py      # batch WAV→OGG conversion helper
├── CMakeLists.txt
└── README.md
```

**Architectural rule:** game logic never touches raylib. `Board` knows nothing about pixels. `Renderer` asks `Board` what's where and draws it.

## Trivia bank

The question bank is maintained in the [promptukit](https://github.com/jrkasprzyk/promptukit) repository and downloaded automatically at build time from jsdelivr. To update to a newer version, bump `TRIVIA_VERSION` in `CMakeLists.txt` and re-run cmake.

The bank currently has **294 questions** across ten categories:

| Category | Count |
|---|---|
| general | 84 |
| pop | 60 |
| film-and-tv | 27 |
| books | 24 |
| music | 22 |
| science and math | 19 |
| motorsport | 17 |
| linguistics | 18 |
| asia | 13 |
| meta *(questions about the game itself)* | 10 |

## Scripts

The `scripts/` package provides one utility kept in this repo:

- **`convert-audio`** — batch-convert audio assets between formats using ffmpeg.

```powershell
& .venv\Scripts\Activate.ps1
pip install -e .

# dry-run WAV → OGG
convert-audio --from-ext wav --to-ext ogg --dry-run

# convert a single file
convert-audio --file assets/bg_wav/intro.wav --to-ext ogg --overwrite
```

## Credits

Joseph Kasprzyk, with Claude as co-conspirator. Inspired by Block-Doku and Balatro, with a trivia twist.
