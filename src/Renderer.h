// Renderer.h — all raylib drawing lives here.
//
// Renderer is the ONLY file that includes raylib.h. Everything else in the
// game stays framework-agnostic. If we ever swap raylib for SDL (we won't,
// but if) this is the only file that needs rewriting.

#pragma once

#include "Board.h"
#include "Piece.h"

#include <array>

namespace td {

struct Layout {
    int windowWidth  = 900;
    int windowHeight = 700;
    int cellPixels   = 56;   // size of one board cell in pixels
    int boardOriginX = 60;   // top-left of the board in window coords
    int boardOriginY = 60;
};

class Renderer {
public:
    explicit Renderer(const Layout& layout);

    // Draw the board's current state: cells, grid lines, and sub-square borders.
    void drawBoard(const Board& board) const;

    // Draw the three offered pieces in the tray area below the board.
    void drawTray(const std::array<Piece, 3>& tray) const;

    // Score and streak panel on the right side of the window.
    void drawHud(int score, int streak) const;

private:
    Layout layout_;
};

} // namespace td
