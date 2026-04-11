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

From the project root:

```powershell
.\scripts\build.ps1
```

(Adjust `C:/vcpkg/...` to wherever you installed vcpkg.)

## Run

```powershell
.\build\Release\trivia_doku.exe
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
