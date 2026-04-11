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

void Renderer::drawTray(const std::array<Piece, 3>& tray, int hiddenSlot) const {
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
        // Slot being dragged: show placeholder outline only, no piece.
        if (i == hiddenSlot) continue;
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

void Renderer::drawDragGhost(const Piece& piece, int mouseX, int mouseY) const {
    const int cell = layout_.cellPixels;

    // Compute bounding box of the piece so we can center it on the cursor.
    int maxR = 0, maxC = 0;
    for (const auto& c : piece.shape.cells) {
        maxR = std::max(maxR, c.row);
        maxC = std::max(maxC, c.col);
    }

    // Origin: shift so the piece's bounding-box centre sits under the cursor.
    const int originX = mouseX - (maxC + 1) * cell / 2;
    const int originY = mouseY - (maxR + 1) * cell / 2;

    // Semi-transparent fill
    const Color base = piece.isTrivia ? kCellTrivia : kCellFilled;
    const Color fill = { base.r, base.g, base.b, 190 };

    for (const auto& c : piece.shape.cells) {
        const int px = originX + c.col * cell;
        const int py = originY + c.row * cell;
        DrawRectangle(px + 1, py + 1, cell - 2, cell - 2, fill);

        if (piece.isTrivia) {
            const int fontSize = cell / 2;
            const char* q = "?";
            const int tw = MeasureText(q, fontSize);
            DrawText(q, px + (cell - tw) / 2, py + (cell - fontSize) / 2,
                     fontSize, { 40, 30, 10, 190 });
        }
    }
}

void Renderer::drawPlacementPreview(const Board& board, const Piece& piece,
                                    int anchorRow, int anchorCol) const {
    const int cell = layout_.cellPixels;
    const int ox   = layout_.boardOriginX;
    const int oy   = layout_.boardOriginY;

    const bool legal = board.canPlace(piece.shape.cells, anchorRow, anchorCol);
    // Green tint when legal, red when not — both semi-transparent.
    const Color overlay = legal
        ? Color{  80, 220, 120, 130 }
        : Color{ 220,  70,  70, 130 };

    for (const auto& c : piece.shape.cells) {
        const int r   = anchorRow + c.row;
        const int col = anchorCol + c.col;
        if (r < 0 || r >= kBoardSize || col < 0 || col >= kBoardSize) continue;
        const int px = ox + col * cell;
        const int py = oy + r  * cell;
        DrawRectangle(px + 1, py + 1, cell - 2, cell - 2, overlay);
    }
}

void Renderer::drawClearFlash(const std::vector<Cell>& cells, float t) const {
    // t = 1.0 at the start of the animation, 0.0 when done.
    // Hold full brightness for the first third, then fade out — gives a crisp
    // "pop" before the glow dissolves.
    const float fade = (t > 0.67f) ? 1.0f : (t / 0.67f);
    const uint8_t alpha = static_cast<uint8_t>(fade * 210);

    const Color flash = { 255, 240, 130, alpha };

    const int cell = layout_.cellPixels;
    const int ox   = layout_.boardOriginX;
    const int oy   = layout_.boardOriginY;

    for (const auto& c : cells) {
        const int px = ox + c.col * cell;
        const int py = oy + c.row  * cell;
        // Slightly inset so the flash doesn't bleed over grid lines.
        DrawRectangle(px + 1, py + 1, cell - 2, cell - 2, flash);
    }
}

void Renderer::drawGameOver(int finalScore) const {
    const int W = layout_.windowWidth;
    const int H = layout_.windowHeight;

    // Semi-transparent full-screen dimmer.
    DrawRectangle(0, 0, W, H, { 0, 0, 0, 175 });

    // Panel
    constexpr int kPanelW = 360;
    constexpr int kPanelH = 210;
    const int panelX = (W - kPanelW) / 2;
    const int panelY = (H - kPanelH) / 2;

    DrawRectangle(panelX,     panelY,     kPanelW,     kPanelH,     { 28,  32,  46, 248 });
    DrawRectangle(panelX,     panelY,     kPanelW,       2,          { 200, 156,  50, 255 }); // top accent bar
    DrawRectangleLines(panelX, panelY,    kPanelW,     kPanelH,     {  90, 100, 120, 200 });

    // Title
    const char* title = "GAME OVER";
    constexpr int kTitleSize = 44;
    const int tw = MeasureText(title, kTitleSize);
    DrawText(title, panelX + (kPanelW - tw) / 2, panelY + 22, kTitleSize, { 240, 196, 68, 255 });

    // Score
    const char* scoreStr = TextFormat("Final score:  %d", finalScore);
    constexpr int kScoreSize = 24;
    const int sw = MeasureText(scoreStr, kScoreSize);
    DrawText(scoreStr, panelX + (kPanelW - sw) / 2, panelY + 108, kScoreSize, { 230, 234, 244, 255 });

    // Restart prompt
    const char* prompt = "Press  R  to play again";
    constexpr int kPromptSize = 18;
    const int pw = MeasureText(prompt, kPromptSize);
    DrawText(prompt, panelX + (kPanelW - pw) / 2, panelY + 162, kPromptSize, { 130, 148, 175, 255 });
}

} // namespace td
