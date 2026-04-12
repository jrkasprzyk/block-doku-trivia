// Renderer.h — all raylib drawing lives here.
//
// Renderer is the ONLY file that includes raylib.h. Everything else in the
// game stays framework-agnostic. If we ever swap raylib for SDL (we won't,
// but if) this is the only file that needs rewriting.

#pragma once

#include "Board.h"
#include "Piece.h"
#include "TriviaBank.h"

#include <algorithm>
#include <cmath>

#include <array>
#include <string>
#include <memory>
#include <vector>

namespace td {

struct Layout {
    // Default window size (bumped up — can be overridden at runtime).
    int windowWidth  = 1600;
    int windowHeight = 1000;
    // Cell size and board origin are recalculated for the current window
    // via `recalcForWindow()` so the board can scale up on larger displays.
    int cellPixels   = 72;   // size of one board cell in pixels
    int boardOriginX = 80;   // top-left of the board in window coords
    int boardOriginY = 80;

    // --- Layout tuning knobs (ratios of window size) ---
    static constexpr float kMarginXRatio    = 0.05f;  // horizontal margin as % of window width
    static constexpr float kMarginYRatio    = 0.08f;  // vertical margin as % of window height
    static constexpr float kHudReserveRatio = 0.24f;  // HUD column width as % of window width
    static constexpr float kTrayReserveRatio= 0.16f;  // tray height as % of window height
    static constexpr int   kMarginXMin      = 40;     // minimum horizontal margin in px
    static constexpr int   kMarginYMin      = 40;     // minimum vertical margin in px
    static constexpr int   kHudReserveMin   = 260;    // minimum HUD width in px
    static constexpr int   kTrayReserveMin  = 100;    // minimum tray height in px
    static constexpr int   kHudGap          = 40;     // gap between board right edge and HUD text

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

    // Recalculate layout metrics when the window size changes (or on startup
    // after calling `InitWindow`). This adjusts `cellPixels` and origins so the
    // board scales to larger windows, while leaving a reserved area for the
    // HUD on the right and the tray below. The layout centers the board + HUD
    // block and the board + tray block so fullscreen and large windows feel
    // balanced instead of pinned to a fixed offset.
    void recalcForWindow(int windowW, int windowH) {
        windowWidth = windowW;
        windowHeight = windowH;

        const int leftMargin = std::max(static_cast<int>(windowWidth * kMarginXRatio), kMarginXMin);
        const int topMargin  = std::max(static_cast<int>(windowHeight * kMarginYRatio), kMarginYMin);
        const int hudReserve = std::max(static_cast<int>(windowWidth * kHudReserveRatio), kHudReserveMin);
        const int trayReserve= std::max(static_cast<int>(windowHeight * kTrayReserveRatio), kTrayReserveMin);

        int maxCellW = (windowWidth - 2 * leftMargin - hudReserve) / kBoardSize;
        if (maxCellW < 20) maxCellW = 20;
        int maxCellH = (windowHeight - 2 * topMargin - trayReserve) / kBoardSize;
        if (maxCellH < 20) maxCellH = 20;

        int cp = std::min(maxCellW, maxCellH);
        if (cp < 28) cp = 28;
        cellPixels = cp;

        // Compute centered origins for the content blocks.
        const int boardW = kBoardSize * cellPixels;
        const int contentW = boardW + hudReserve;
        boardOriginX = (windowWidth - contentW) / 2;
        if (boardOriginX < leftMargin) boardOriginX = leftMargin;

        const int boardH = kBoardSize * cellPixels;
        const int contentH = boardH + trayReserve;
        boardOriginY = (windowHeight - contentH) / 2;
        if (boardOriginY < topMargin) boardOriginY = topMargin;
    }
};

class Renderer {
public:
    explicit Renderer(const Layout& layout);
    ~Renderer();
    // Update renderer layout at runtime (e.g., after window resize/fullscreen).
    void setLayout(const Layout& layout);

    // Draw the board's current state: cells, grid lines, and sub-square borders.
    void drawBoard(const Board& board) const;

    // Draw the three offered pieces in the tray area below the board.
    // hiddenSlot (0-2) is grayed out when that piece is being dragged; pass -1 for none.
    // placeable[i] = true iff tray[i] can be placed somewhere on the board;
    // if false, the piece renders in red to signal it's unplayable.
    void drawTray(const std::array<Piece, 3>& tray, int hiddenSlot,
                  const std::array<bool, 3>& placeable) const;

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

    // Draw a pulsing cursor outline on a board cell (for gamepad navigation).
    void drawBoardCursor(int row, int col) const;

    // Draw a highlight border around a tray slot (for gamepad piece selection).
    void drawTrayHighlight(int slot) const;

    // Full-screen game-over overlay with final score and restart prompt.
    void drawGameOver(int finalScore) const;

    // Trivia modal overlay. Shows question, choices, and (after answering) feedback.
    // `selected` is the currently highlighted choice (0-3).
    // `answered` / `correct` control whether feedback is shown.
    // `quip` is the per-question flavour text (may be empty — falls back to generic).
    void drawTriviaModal(const Question& q, int selected,
                         bool answered, bool correct,
                         const std::string& quip) const;

    // Audio API — implemented in Renderer.cpp (keeps raylib types out of headers)
    // Initialize audio subsystem. Safe to call multiple times.
    bool initAudio();
    // Unload audio and shut down audio device.
    void shutdownAudio();
    // Load a short sound effect (WAV/OGG/etc) and register it under `id`.
    // Returns true on success.
    bool loadSoundEffect(const std::string& id, const std::string& path);
    // Load streaming music (OGG/MP3) and register under `id`.
    bool loadMusic(const std::string& id, const std::string& path);
    // Play a previously loaded sound effect (no-op if id missing).
    void playSoundEffect(const std::string& id);
    // Start playing a previously loaded music stream. Stops any other music.
    void playMusicStream(const std::string& id);
    // Start playing a playlist of previously loaded music IDs back-to-back.
    // `ids` are looked up in the loaded music map; missing ids are skipped.
    // If `loop` is true the playlist wraps when it reaches the end.
    void playMusicPlaylist(const std::vector<std::string>& ids, bool loop = true);
    // Stop currently playing music (if any).
    void stopMusic();
    // Must be called each frame to update streaming music playback.
    void updateAudio();

private:
    Layout layout_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace td
