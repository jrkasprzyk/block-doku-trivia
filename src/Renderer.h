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
    // HUD on the right and the tray below.
    void recalcForWindow(int windowW, int windowH) {
        windowWidth = windowW;
        windowHeight = windowH;

        // Desired margins / reserved space (heuristic values).
        const int leftMargin = 80;
        const int topMargin = 80;
        const int hudReserve = 380;   // space reserved for HUD on the right
        const int trayReserve = 160;  // space reserved below board for the tray

        int maxCellW = (windowWidth - leftMargin - hudReserve) / kBoardSize;
        if (maxCellW < 20) maxCellW = 20;
        int maxCellH = (windowHeight - topMargin - trayReserve) / kBoardSize;
        if (maxCellH < 20) maxCellH = 20;

        // Pick the limiting dimension but clamp to a sensible minimum.
        int cp = (maxCellW < maxCellH) ? maxCellW : maxCellH;
        if (cp < 28) cp = 28;
        cellPixels = cp;

        boardOriginX = leftMargin;
        boardOriginY = topMargin;
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
