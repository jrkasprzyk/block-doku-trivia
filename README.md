# Trivia-Doku

A block-doku puzzle game with a trivia twist, built in C++ with raylib.

Place polyomino pieces on a 9×9 board. Clear full rows, columns, and 3×3 squares for points. Special **trivia blocks** trigger a question when their placement completes a clear — answer correctly for a bonus, answer wrong and a cleared cell comes back as an **unclearable stone**. Get 3 correct in a row to shatter all stones and trigger a comeback.

## Status

**Day 2.** Window opens, empty board renders, HUD shows. Piece tray below the board shows three randomly-chosen polyomino shapes each run (one may be a glowing trivia piece). No interaction yet.

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

From the project root:

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

(Adjust `C:/vcpkg/...` to wherever you installed vcpkg.)

## Run

```powershell
.\build\Release\trivia_doku.exe
```

You should see a dark window titled "TRIVIA-DOKU" with an empty 9×9 grid, a few stub cells for visual check, a small yellow "?" trivia block, and a score/streak HUD on the right. ESC to quit.

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

Questions live in `assets/trivia.json`. The file ships with 3 seed examples per category — **motorsport**, **music**, **film**, **general**, and **meta** (questions about the game itself). Fill in the `EXAMPLE — replace` entries and add as many more as you like. The schema is documented at the top of the file.

Scoring multipliers by difficulty: easy 1.5×, medium 2×, hard 3×.

## Roadmap

- [x] **Day 1** — window + empty board + HUD
- [x] **Day 2** — Piece definitions, piece tray, render a set of 3 offered pieces
- [ ] **Day 3** — Drag-and-drop placement with legality checks
- [ ] **Day 4** — Line/column/square detection and clearing
- [ ] **Day 5** — Game over detection
- [ ] **Day 6** — Trivia block flag + modal dialog + JSON loader
- [ ] **Day 7** — Stones, streak tracking, redemption mechanic
- [ ] **Day 8** — Help overlay (?), quips, polish

## Credits

Joseph Kasprzyk, with Claude as co-conspirator. Inspired by Block-Doku and Balatro, with a trivia twist.
