// main.cpp — entry point and main game loop.
//
// Day 3: drag-and-drop placement.
//   - Click a piece in the tray to pick it up.
//   - While dragging, the piece ghost follows the cursor at full board-cell scale.
//   - When the cursor is over the board the ghost snaps to the grid and a green/red
//     preview shows whether placement is legal.
//   - Release over a legal cell to place. Release anywhere else to cancel (piece
//     returns to its tray slot).
//   - After a placement, line/column/square clears are detected automatically.
//   - When all three tray slots are used up a fresh tray is dealt.
//
// Day 6+7: trivia modal, JSON loader, stones, streak, redemption.
//   - ~20 % of pieces are flagged as trivia (golden "?").
//   - Placing one that completes a clear pauses the game and shows a question.
//   - Correct answer: cleared cells score at a difficulty multiplier (1.5/2/3×);
//     streak increments. 3-in-a-row shatters all stones (+5 pts each) and resets.
//   - Wrong answer: normal score; streak resets; a stone appears at the trivia cell.
//   - Stones block line clears until shattered by a redemption streak.

#include "Board.h"
#include "Piece.h"
#include "Renderer.h"
#include "TriviaBank.h"
#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace {

// Game phase ——————————————————————————————————————————————————
enum class Phase { Playing, Trivia, GameOver };

// Clear-flash animation ———————————————————————————————————————
struct ClearAnim {
    std::vector<td::Cell> cells;
    int framesLeft = 0;
    static constexpr int kDuration = 18;   // ~0.3 s at 60 fps

    bool active() const { return framesLeft > 0; }
    float t()     const { return framesLeft / static_cast<float>(kDuration); }

    void trigger(std::vector<td::Cell> cleared) {
        cells      = std::move(cleared);
        framesLeft = kDuration;
    }
    void tick() { if (framesLeft > 0) --framesLeft; }
};

// Drag state —————————————————————————————————————————————————
struct DragState {
    bool  active    = false;
    int   traySlot  = -1;    // which of the 3 tray slots was picked up
    td::Piece piece;          // copy of the piece being dragged
};

// Trivia modal state ——————————————————————————————————————————
struct TriviaModal {
    const td::Question*   question      = nullptr;
    std::vector<td::Cell> clearedCells; // deferred scoring
    td::Cell              triviaOrigin  = {-1, -1}; // stone target on wrong answer
    int                   selected      = 0;         // highlighted choice 0-3
    bool                  answered      = false;
    bool                  correct       = false;
    int                   dismissFrames = 0;
    static constexpr int  kDismissDelay = 120;       // 2 s at 60 fps
};

// Compute the snap anchor on the board so the piece's bounding-box centre
// sits under the cursor pixels. This uses the same pixel-centering that
// Renderer::drawDragGhost uses, then rounds the resulting top-left cell to
// the nearest cell. This makes snapping consistent and permits dropping
// even when the pointer is slightly outside the board edge as long as the
// piece fits.
td::Cell snapAnchorFromMouse(const td::Piece& piece, int mouseX, int mouseY, const td::Layout& layout) {
    int maxR = 0, maxC = 0;
    for (const auto& c : piece.shape.cells) {
        maxR = std::max(maxR, c.row);
        maxC = std::max(maxC, c.col);
    }
    const int cell = layout.cellPixels;
    const int ox   = layout.boardOriginX;
    const int oy   = layout.boardOriginY;

    const int widthPx  = (maxC + 1) * cell;
    const int heightPx = (maxR + 1) * cell;

    const float originX = mouseX - widthPx / 2.0f;
    const float originY = mouseY - heightPx / 2.0f;

    const int anchorCol = static_cast<int>(std::round((originX - ox) / static_cast<float>(cell)));
    const int anchorRow = static_cast<int>(std::round((originY - oy) / static_cast<float>(cell)));
    return { anchorRow, anchorCol };
}

// Run the game-over check and transition if needed.
void checkGameOver(Phase& phase,
                   const td::Board& board,
                   const std::array<td::Piece, 3>& tray) {
    std::vector<std::vector<td::Cell>> shapes;
    for (const auto& p : tray) {
        if (!p.shape.cells.empty()) shapes.push_back(p.shape.cells);
    }
    if (!board.anyPlacementPossible(shapes)) {
        phase = Phase::GameOver;
    }
}

} // namespace

int main() {
    td::Layout layout;
    InitWindow(layout.windowWidth, layout.windowHeight, "Trivia-Doku");
    SetTargetFPS(60);

    td::Board    board;
    td::Renderer renderer(layout);
    td::TriviaBank triviaBank;
    triviaBank.loadFromFile("assets/trivia.json");

    // Optional: initialize audio and attempt to load conventionally-named assets
    // (no-op if files don't exist). User-provided assets should live under
    // `assets/` and follow names documented in /docs/audio-assets.md.
    renderer.initAudio();
    renderer.loadMusic("bg", "assets/music.ogg");
    renderer.loadSoundEffect("place",    "assets/sfx_place.wav");
    renderer.loadSoundEffect("clear",    "assets/sfx_clear.wav");
    renderer.loadSoundEffect("correct",  "assets/sfx_correct.wav");
    renderer.loadSoundEffect("wrong",    "assets/sfx_wrong.wav");
    renderer.loadSoundEffect("pickup",   "assets/sfx_pickup.wav");
    renderer.loadSoundEffect("cancel",   "assets/sfx_cancel.wav");
    renderer.loadSoundEffect("deal",     "assets/sfx_deal.wav");
    renderer.loadSoundEffect("shatter",  "assets/sfx_shatter.wav");
    renderer.loadSoundEffect("gameover", "assets/sfx_gameover.wav");
    renderer.loadSoundEffect("restart",  "assets/sfx_restart.wav");
    renderer.playMusicStream("bg");

    auto tray = td::makeTray();

    int score  = 0;
    int streak = 0;
    ClearAnim  clearAnim;
    TriviaModal triviaModal;
    Phase phase = Phase::Playing;

    // Tray geometry — must agree with Renderer::drawTray.
    const int trayY = layout.boardOriginY + td::kBoardSize * layout.cellPixels + 12;
    const int trayW = td::kBoardSize * layout.cellPixels;
    const int slotW = trayW / 3;

    DragState drag;

    while (!WindowShouldClose()) {

        // ── Input / Update ──────────────────────────────────────────────────
        const int mx = GetMouseX();
        const int my = GetMouseY();

        // Restart from game-over screen.
        if (phase == Phase::GameOver && IsKeyPressed(KEY_R)) {
            board        = td::Board{};
            tray         = td::makeTray();
            score        = 0;
            streak       = 0;
            clearAnim    = {};
            triviaModal  = {};
            drag         = {};
            phase        = Phase::Playing;
            renderer.playSoundEffect("restart");
        }

        // ── Trivia modal input ──────────────────────────────────────────────
        if (phase == Phase::Trivia) {
            if (!triviaModal.answered) {
                // Navigate choices.
                if (IsKeyPressed(KEY_UP))   triviaModal.selected = (triviaModal.selected + 3) % 4;
                if (IsKeyPressed(KEY_DOWN)) triviaModal.selected = (triviaModal.selected + 1) % 4;
                if (IsKeyPressed(KEY_ONE))   triviaModal.selected = 0;
                if (IsKeyPressed(KEY_TWO))   triviaModal.selected = 1;
                if (IsKeyPressed(KEY_THREE)) triviaModal.selected = 2;
                if (IsKeyPressed(KEY_FOUR))  triviaModal.selected = 3;

                // Submit answer.
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    triviaModal.answered = true;
                    triviaModal.correct  = (triviaModal.selected == triviaModal.question->answer);
                    triviaModal.dismissFrames = TriviaModal::kDismissDelay;

                    if (triviaModal.correct) {
                        const float mult = td::TriviaBank::multiplier(triviaModal.question->difficulty);
                        score += static_cast<int>(triviaModal.clearedCells.size() * 10 * mult);
                        streak++;
                        if (streak >= 3) {
                            const int shattered = board.shatterStones();
                            if (shattered > 0) renderer.playSoundEffect("shatter");
                            score  += shattered * 5;
                            streak  = 0;
                        }
                        renderer.playSoundEffect("correct");
                    } else {
                        score  += static_cast<int>(triviaModal.clearedCells.size()) * 10;
                        streak  = 0;
                        board.placeStone(triviaModal.triviaOrigin.row,
                                         triviaModal.triviaOrigin.col);
                        renderer.playSoundEffect("wrong");
                    }
                }
            } else {
                // After answer: any key skips the countdown.
                triviaModal.dismissFrames--;
                bool anyKey = false;
                // Poll a range of common keys for "any key".
                for (int k = 32; k < 350; ++k) {
                    if (IsKeyPressed(k)) { anyKey = true; break; }
                }
                if (triviaModal.dismissFrames <= 0 || anyKey) {
                    triviaModal = {};
                    phase = Phase::Playing;
                    checkGameOver(phase, board, tray);
                    if (phase == Phase::GameOver) renderer.playSoundEffect("gameover");
                }
            }
        }

        // ── Gameplay input (only when Playing) ─────────────────────────────

        // Pick up a piece from the tray.
        if (phase == Phase::Playing && !drag.active && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const int relX = mx - layout.boardOriginX;
            if (my >= trayY && my < layout.windowHeight - 8 &&
                relX >= 0 && relX < trayW) {
                const int slot = relX / slotW;
                if (slot >= 0 && slot < 3 && !tray[slot].shape.cells.empty()) {
                    drag.active   = true;
                    drag.traySlot = slot;
                    drag.piece    = tray[slot];
                    tray[slot].shape.cells.clear(); // hide from tray while dragging
                    renderer.playSoundEffect("pickup");
                }
            }
        }

        // Drop or cancel.
        if (phase == Phase::Playing && drag.active && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            bool placed = false;

            // Compute anchor based on the mouse pixel position so snapping matches
            // the drag ghost and drops are allowed even if the pointer is slightly
            // outside the board as long as the piece would fit.
            const td::Cell anchor = snapAnchorFromMouse(drag.piece, mx, my, layout);
            if (board.canPlace(drag.piece.shape.cells, anchor.row, anchor.col)) {
                board.place(drag.piece.shape.cells, anchor.row, anchor.col,
                            drag.piece.isTrivia);
                renderer.playSoundEffect("place");

                const auto result = board.detectAndClear();

                // 1 pt per cell placed — always immediate.
                score += static_cast<int>(drag.piece.shape.cells.size());

                if (result.triggeredTrivia) {
                    // Defer cleared-cell scoring until after the question is answered.
                    triviaModal.question     = &triviaBank.pick();
                    triviaModal.clearedCells = result.clearedCells;
                    triviaModal.triviaOrigin = result.triviaCells[0];
                    triviaModal.selected     = 0;
                    triviaModal.answered     = false;
                    triviaModal.dismissFrames = 0;
                    // Flash still fires so the clear is satisfying.
                    if (!result.clearedCells.empty())
                        clearAnim.trigger(result.clearedCells);
                    if (!result.clearedCells.empty()) renderer.playSoundEffect("clear");
                    phase = Phase::Trivia;
                } else {
                    // Normal clear: score immediately.
                    score += static_cast<int>(result.clearedCells.size()) * 10;
                    if (!result.clearedCells.empty()) {
                        clearAnim.trigger(result.clearedCells);
                        renderer.playSoundEffect("clear");
                    }
                }

                placed = true;
            }

            if (!placed) {
                // Return the piece to its original slot.
                tray[drag.traySlot] = drag.piece;
                renderer.playSoundEffect("cancel");
            } else {
                // Refill the tray when every slot is exhausted.
                bool allEmpty = true;
                for (const auto& p : tray) {
                    if (!p.shape.cells.empty()) { allEmpty = false; break; }
                }
                if (allEmpty) {
                    tray = td::makeTray();
                    renderer.playSoundEffect("deal");
                }

                // Game-over check only when not paused for a question.
                if (phase == Phase::Playing) {
                    checkGameOver(phase, board, tray);
                    if (phase == Phase::GameOver) renderer.playSoundEffect("gameover");
                }
            }

            drag = {};
        }

        clearAnim.tick();
        // Keep streaming music alive.
        renderer.updateAudio();

        // ── Draw ────────────────────────────────────────────────────────────
        BeginDrawing();
            ClearBackground({ 24, 28, 38, 255 });

            // Placement preview sits between the board background and the board cells
            // so that existing filled cells are still visible through the overlay.
            if (drag.active) {
                const td::Cell anchor = snapAnchorFromMouse(drag.piece, mx, my, layout);
                renderer.drawPlacementPreview(board, drag.piece, anchor.row, anchor.col);
            }

            renderer.drawBoard(board);

            // Clear flash draws on top of the (now-empty) cells.
            if (clearAnim.active())
                renderer.drawClearFlash(clearAnim.cells, clearAnim.t());

            renderer.drawTray(tray, drag.active ? drag.traySlot : -1);
            renderer.drawHud(score, streak);

            // Ghost on top of everything so it's always readable.
            if (drag.active) {
                renderer.drawDragGhost(drag.piece, mx, my);
            }

            // Modals on top of the entire scene.
            if (phase == Phase::Trivia && triviaModal.question) {
                const std::string quip = triviaModal.answered
                    ? (triviaModal.correct ? triviaModal.question->quipCorrect
                                           : triviaModal.question->quipWrong)
                    : "";
                renderer.drawTriviaModal(*triviaModal.question, triviaModal.selected,
                                         triviaModal.answered, triviaModal.correct, quip);
            }
            if (phase == Phase::GameOver) {
                renderer.drawGameOver(score);
            }
        EndDrawing();
    }

    renderer.shutdownAudio();
    CloseWindow();
    return 0;
}
