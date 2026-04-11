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

#include "Board.h"
#include "Piece.h"
#include "Renderer.h"
#include <raylib.h>

#include <algorithm>

namespace {

// Drag state —————————————————————————————————————————————————
struct DragState {
    bool  active    = false;
    int   traySlot  = -1;    // which of the 3 tray slots was picked up
    td::Piece piece;          // copy of the piece being dragged
};

// Compute the snap anchor on the board so the piece's bounding-box centre
// sits under the cursor cell.
td::Cell snapAnchor(const td::Piece& piece, td::Cell hovered) {
    int maxR = 0, maxC = 0;
    for (const auto& c : piece.shape.cells) {
        maxR = std::max(maxR, c.row);
        maxC = std::max(maxC, c.col);
    }
    return { hovered.row - maxR / 2, hovered.col - maxC / 2 };
}

} // namespace

int main() {
    td::Layout layout;
    InitWindow(layout.windowWidth, layout.windowHeight, "Trivia-Doku");
    SetTargetFPS(60);

    td::Board    board;
    td::Renderer renderer(layout);

    auto tray = td::makeTray();

    int score  = 0;
    int streak = 0;

    // Tray geometry — must agree with Renderer::drawTray.
    const int trayY = layout.boardOriginY + td::kBoardSize * layout.cellPixels + 12;
    const int trayW = td::kBoardSize * layout.cellPixels;
    const int slotW = trayW / 3;

    DragState drag;

    while (!WindowShouldClose()) {

        // ── Input / Update ──────────────────────────────────────────────────
        const int mx = GetMouseX();
        const int my = GetMouseY();

        // Pick up a piece from the tray.
        if (!drag.active && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const int relX = mx - layout.boardOriginX;
            if (my >= trayY && my < layout.windowHeight - 8 &&
                relX >= 0 && relX < trayW) {
                const int slot = relX / slotW;
                if (slot >= 0 && slot < 3 && !tray[slot].shape.cells.empty()) {
                    drag.active   = true;
                    drag.traySlot = slot;
                    drag.piece    = tray[slot];
                    tray[slot].shape.cells.clear(); // hide from tray while dragging
                }
            }
        }

        // Drop or cancel.
        if (drag.active && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            bool placed = false;

            if (layout.isOverBoard(mx, my)) {
                const td::Cell hovered = layout.boardCellAt(mx, my);
                const td::Cell anchor  = snapAnchor(drag.piece, hovered);

                if (board.canPlace(drag.piece.shape.cells, anchor.row, anchor.col)) {
                    board.place(drag.piece.shape.cells, anchor.row, anchor.col,
                                drag.piece.isTrivia);

                    const auto result = board.detectAndClear();

                    // 1 pt per cell placed + 10 pt per cleared cell
                    score += static_cast<int>(drag.piece.shape.cells.size());
                    score += static_cast<int>(result.clearedCells.size()) * 10;

                    // Trivia modal will land in Day 6; note the trigger for now.
                    // if (result.triggeredTrivia) { ... }

                    placed = true;
                }
            }

            if (!placed) {
                // Return the piece to its original slot.
                tray[drag.traySlot] = drag.piece;
            } else {
                // Refill the tray when every slot is exhausted.
                bool allEmpty = true;
                for (const auto& p : tray) {
                    if (!p.shape.cells.empty()) { allEmpty = false; break; }
                }
                if (allEmpty) tray = td::makeTray();
            }

            drag = {};
        }

        // ── Draw ────────────────────────────────────────────────────────────
        BeginDrawing();
            ClearBackground({ 24, 28, 38, 255 });

            // Placement preview sits between the board background and the board cells
            // so that existing filled cells are still visible through the overlay.
            if (drag.active && layout.isOverBoard(mx, my)) {
                const td::Cell hovered = layout.boardCellAt(mx, my);
                const td::Cell anchor  = snapAnchor(drag.piece, hovered);
                renderer.drawPlacementPreview(board, drag.piece, anchor.row, anchor.col);
            }

            renderer.drawBoard(board);
            renderer.drawTray(tray, drag.active ? drag.traySlot : -1);
            renderer.drawHud(score, streak);

            // Ghost on top of everything so it's always readable.
            if (drag.active) {
                renderer.drawDragGhost(drag.piece, mx, my);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
