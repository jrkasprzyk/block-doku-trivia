// Renderer.h — all raylib drawing lives here.
//
// Renderer is the ONLY file that includes raylib.h. Everything else in the
// game stays framework-agnostic. If we ever swap raylib for SDL (we won't,
// but if) this is the only file that needs rewriting.

#pragma once

#include "Board.h"
#include "Piece.h"
#include "TriviaBank.h"

#include <array>
#include <string>

namespace td {

struct Layout {
    int windowWidth  = 1200;
    int windowHeight = 880;
    int cellPixels   = 72;   // size of one board cell in pixels
    int boardOriginX = 80;   // top-left of the board in window coords
    int boardOriginY = 80;

    // Convert pixel position to board cell. Returns {-1,-1} if outside the board.
    Cell boardCellAt(int px, int py) const {
        const int col = (px - boardOriginX) / cellPixels;
        const int row = (py - boardOriginY) / cellPixels;
        if (px < boardOriginX || py < boardOriginY ||
            col < 0 || col >= kBoardSize || row < 0 || row >= kBoardSize)
            return {-1, -1};
        return {row, col};
    }

    bool isOverBoard(int px, int py) const {
        const Cell c = boardCellAt(px, py);
        return c.row != -1;
    }
};

class Renderer {
public:
    explicit Renderer(const Layout& layout);

    // Draw the board's current state: cells, grid lines, and sub-square borders.
    void drawBoard(const Board& board) const;

    // Draw the three offered pieces in the tray area below the board.
    // hiddenSlot (0-2) is grayed out when that piece is being dragged; pass -1 for none.
    void drawTray(const std::array<Piece, 3>& tray, int hiddenSlot = -1) const;

    // Score and streak panel on the right side of the window.
    void drawHud(int score, int streak) const;

    // Draw a piece ghost following the cursor at full board-cell scale, semi-transparent.
    void drawDragGhost(const Piece& piece, int mouseX, int mouseY) const;

    // Draw green/red placement preview on the board at (anchorRow, anchorCol).
    void drawPlacementPreview(const Board& board, const Piece& piece,
                              int anchorRow, int anchorCol) const;

    // Flash overlay for cells that were just cleared.
    // t is a 0→1 progress value (0 = animation done, 1 = just triggered).
    void drawClearFlash(const std::vector<Cell>& cells, float t) const;

    // Full-screen game-over overlay with final score and restart prompt.
    void drawGameOver(int finalScore) const;

    // Trivia modal overlay. Shows question, choices, and (after answering) feedback.
    // `selected` is the currently highlighted choice (0-3).
    // `answered` / `correct` control whether feedback is shown.
    // `quip` is the per-question flavour text (may be empty — falls back to generic).
    void drawTriviaModal(const Question& q, int selected,
                         bool answered, bool correct,
                         const std::string& quip) const;

private:
    Layout layout_;
};

} // namespace td
