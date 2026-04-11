// Renderer.cpp — raylib drawing implementation.

#include "Renderer.h"
#include <raylib.h>

#include <algorithm>

namespace td {

namespace {
// Palette. Kept private to this translation unit so they don't leak into headers
// (which would force every file that includes Renderer.h to also see raylib).
constexpr Color kBgColor       = { 24,  28,  38, 255 };
constexpr Color kGridLine      = { 60,  68,  84, 255 };
constexpr Color kSubBorder     = {120, 134, 160, 255 };
constexpr Color kCellEmpty     = { 38,  44,  58, 255 };
constexpr Color kCellFilled    = { 92, 170, 230, 255 };
constexpr Color kCellTrivia    = {240, 196,  68, 255 }; // glowing question mark color
constexpr Color kCellStone     = { 90,  90,  95, 255 };
constexpr Color kTextColor     = {230, 234, 244, 255 };
} // namespace

Renderer::Renderer(const Layout& layout) : layout_(layout) {}

void Renderer::drawBoard(const Board& board) const {
    const int cell = layout_.cellPixels;
    const int ox   = layout_.boardOriginX;
    const int oy   = layout_.boardOriginY;
    const int side = cell * kBoardSize;

    // Board background panel (slight inset so cells read cleanly against the window bg).
    DrawRectangle(ox - 4, oy - 4, side + 8, side + 8, kGridLine);

    for (int r = 0; r < kBoardSize; ++r) {
        for (int c = 0; c < kBoardSize; ++c) {
            const int x = ox + c * cell;
            const int y = oy + r * cell;

            Color fill = kCellEmpty;
            switch (board.at(r, c)) {
                case CellState::Empty:  fill = kCellEmpty;  break;
                case CellState::Filled: fill = kCellFilled; break;
                case CellState::Trivia: fill = kCellTrivia; break;
                case CellState::Stone:  fill = kCellStone;  break;
            }
            DrawRectangle(x + 1, y + 1, cell - 2, cell - 2, fill);

            // Little "?" glyph on trivia cells so they're legible even without art assets.
            if (board.at(r, c) == CellState::Trivia) {
                const char* q = "?";
                const int fontSize = cell / 2;
                const int tw = MeasureText(q, fontSize);
                DrawText(q, x + (cell - tw) / 2, y + (cell - fontSize) / 2,
                         fontSize, { 40, 30, 10, 255 });
            }
        }
    }

    // Overdraw the 3x3 sub-square borders on top, in a brighter color,
    // to visually separate the sudoku-style regions.
    for (int i = 0; i <= kBoardSize; i += kSubGrid) {
        DrawLineEx({ (float)(ox + i * cell), (float)oy },
                   { (float)(ox + i * cell), (float)(oy + side) }, 2.0f, kSubBorder);
        DrawLineEx({ (float)ox,              (float)(oy + i * cell) },
                   { (float)(ox + side),     (float)(oy + i * cell) }, 2.0f, kSubBorder);
    }
}

void Renderer::drawTray(const std::array<Piece, 3>& tray) const {
    const int boardCell = layout_.cellPixels;
    const int ox        = layout_.boardOriginX;
    const int oy        = layout_.boardOriginY;

    // Tray sits 12 px below the bottom edge of the board.
    const int trayY  = oy + kBoardSize * boardCell + 12;
    const int trayH  = layout_.windowHeight - trayY - 8;
    const int trayW  = kBoardSize * boardCell;          // same width as the board
    const int slotW  = trayW / 3;                       // 168 px each at default layout
    const int prev   = 20;                               // preview cell size in pixels

    // Colours reused from the board (defined in the anonymous namespace above).
    constexpr Color kSlotBg     = { 32,  38,  50, 255 };
    constexpr Color kSlotBorder = { 60,  68,  84, 255 };

    for (int i = 0; i < 3; ++i) {
        const int slotX = ox + i * slotW;

        // Slot background
        DrawRectangle(slotX + 4, trayY, slotW - 8, trayH, kSlotBg);
        DrawRectangleLines(slotX + 4, trayY, slotW - 8, trayH, kSlotBorder);

        const auto& piece = tray[i];
        if (piece.shape.cells.empty()) continue;

        // Bounding box of the piece
        int maxR = 0, maxC = 0;
        for (const auto& c : piece.shape.cells) {
            maxR = std::max(maxR, c.row);
            maxC = std::max(maxC, c.col);
        }
        const int pieceW = (maxC + 1) * prev;
        const int pieceH = (maxR + 1) * prev;

        // Centre the piece inside its slot
        const int startX = slotX + (slotW - pieceW) / 2;
        const int startY = trayY + (trayH - pieceH) / 2;

        const Color fill = piece.isTrivia ? kCellTrivia : kCellFilled;

        for (const auto& c : piece.shape.cells) {
            const int px = startX + c.col * prev;
            const int py = startY + c.row * prev;
            DrawRectangle(px + 1, py + 1, prev - 2, prev - 2, fill);

            // "?" glyph on every cell of a trivia piece
            if (piece.isTrivia) {
                const int fontSize = prev / 2;
                const char* q = "?";
                const int tw = MeasureText(q, fontSize);
                DrawText(q, px + (prev - tw) / 2, py + (prev - fontSize) / 2,
                         fontSize, { 40, 30, 10, 255 });
            }
        }
    }
}

void Renderer::drawHud(int score, int streak) const {
    DrawText("TRIVIA-DOKU", layout_.boardOriginX, 16, 28, kTextColor);

    const int hudX = layout_.boardOriginX + kBoardSize * layout_.cellPixels + 40;
    DrawText(TextFormat("Score:  %d", score),    hudX, 80,  22, kTextColor);
    DrawText(TextFormat("Streak: %d/3", streak), hudX, 120, 22, kTextColor);
    DrawText("ESC to quit",                      hudX, 180, 18, kGridLine);
    DrawText("? for help (soon)",                hudX, 206, 18, kGridLine);
}

} // namespace td
