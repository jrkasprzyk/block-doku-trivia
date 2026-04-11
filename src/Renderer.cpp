// Renderer.cpp — raylib drawing implementation.

#include "Renderer.h"
#include "Version.h"
#include <raylib.h>

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <vector>

namespace td {

// Private implementation to keep raylib types out of the public header.
struct Renderer::Impl {
    bool audioInitialized = false;
    std::unordered_map<std::string, Sound> sfx;
    std::unordered_map<std::string, Music> music;
    std::string currentMusicId;
    // Playlist support: ordered list of music IDs, current index, and loop flag.
    std::vector<std::string> playlist;
    size_t playlistIndex = 0;
    bool playlistLoop = true;
};

namespace fs = std::filesystem;

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

// Greedy word-wrap: split `text` into lines that fit within `maxWidth` px at `fontSize`.
std::vector<std::string> wrapText(const std::string& text, int maxWidth, int fontSize) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word, current;
    while (stream >> word) {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (MeasureText(candidate.c_str(), fontSize) <= maxWidth) {
            current = candidate;
        } else {
            if (!current.empty()) lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}
} // namespace

Renderer::Renderer(const Layout& layout) : layout_(layout), impl_(std::make_unique<Impl>()) {}

Renderer::~Renderer() {
    shutdownAudio();
}

// Update the renderer's layout when the window size or layout metrics change.
void Renderer::setLayout(const Layout& layout) {
    layout_ = layout;
}

bool Renderer::initAudio() {
    if (!impl_) impl_ = std::make_unique<Impl>();
    if (impl_->audioInitialized) return true;
    InitAudioDevice();
    impl_->audioInitialized = true;
    return true;
}

void Renderer::shutdownAudio() {
    if (!impl_ || !impl_->audioInitialized) return;

    // Unload sound effects
    for (auto &p : impl_->sfx) {
        UnloadSound(p.second);
    }
    impl_->sfx.clear();

    // Stop and unload music streams
    for (auto &p : impl_->music) {
        StopMusicStream(p.second);
        UnloadMusicStream(p.second);
    }
    impl_->music.clear();

    // Clear any playlist state
    impl_->playlist.clear();
    impl_->playlistIndex = 0;
    impl_->playlistLoop = true;

    impl_->currentMusicId.clear();
    CloseAudioDevice();
    impl_->audioInitialized = false;
}

bool Renderer::loadSoundEffect(const std::string& id, const std::string& path) {
    if (!impl_) impl_ = std::make_unique<Impl>();
    if (!impl_->audioInitialized) initAudio();
    if (!fs::exists(path)) return false;
    Sound s = LoadSound(path.c_str());
    impl_->sfx[id] = s;
    return true;
}

bool Renderer::loadMusic(const std::string& id, const std::string& path) {
    if (!impl_) impl_ = std::make_unique<Impl>();
    if (!impl_->audioInitialized) initAudio();
    if (!fs::exists(path)) return false;
    Music m = LoadMusicStream(path.c_str());
    impl_->music[id] = m;
    return true;
}

void Renderer::playSoundEffect(const std::string& id) {
    if (!impl_ || !impl_->audioInitialized) return;
    const auto it = impl_->sfx.find(id);
    if (it == impl_->sfx.end()) return;
    PlaySound(it->second);
}

void Renderer::playMusicStream(const std::string& id) {
    if (!impl_ || !impl_->audioInitialized) return;
    const auto it = impl_->music.find(id);
    if (it == impl_->music.end()) return;

    // Stop previous music if any
    if (!impl_->currentMusicId.empty()) {
        auto pit = impl_->music.find(impl_->currentMusicId);
        if (pit != impl_->music.end()) StopMusicStream(pit->second);
    }

    // Clear any playlist state: explicit single-track play overrides playlists.
    impl_->playlist.clear();
    impl_->playlistIndex = 0;
    impl_->playlistLoop = false;

    PlayMusicStream(it->second);
    impl_->currentMusicId = id;
}

void Renderer::playMusicPlaylist(const std::vector<std::string>& ids, bool loop) {
    if (!impl_) impl_ = std::make_unique<Impl>();
    if (!impl_->audioInitialized) initAudio();
    if (ids.empty()) return;

    // Stop previous music if any
    if (!impl_->currentMusicId.empty()) {
        auto pit = impl_->music.find(impl_->currentMusicId);
        if (pit != impl_->music.end()) StopMusicStream(pit->second);
    }

    impl_->playlist = ids;
    impl_->playlistIndex = 0;
    impl_->playlistLoop = loop;

    // Start the first available track in the playlist (skip missing ids).
    for (size_t i = 0; i < impl_->playlist.size(); ++i) {
        const auto& id = impl_->playlist[impl_->playlistIndex];
        auto it = impl_->music.find(id);
        if (it != impl_->music.end()) {
            PlayMusicStream(it->second);
            impl_->currentMusicId = id;
            return;
        }
        // advance index; if we wrapped, nothing was found
        impl_->playlistIndex = (impl_->playlistIndex + 1) % impl_->playlist.size();
        if (impl_->playlistIndex == 0) break;
    }

    // If no valid tracks were found, clear playlist state.
    impl_->playlist.clear();
    impl_->currentMusicId.clear();
}

void Renderer::stopMusic() {
    if (!impl_ || !impl_->audioInitialized) return;
    if (impl_->currentMusicId.empty()) return;
    auto it = impl_->music.find(impl_->currentMusicId);
    if (it != impl_->music.end()) StopMusicStream(it->second);
    impl_->currentMusicId.clear();
    // Clear any playlist state as playback has been stopped explicitly.
    impl_->playlist.clear();
    impl_->playlistIndex = 0;
    impl_->playlistLoop = true;
}

void Renderer::updateAudio() {
    if (!impl_ || !impl_->audioInitialized) return;
    if (impl_->currentMusicId.empty()) return;
    auto it = impl_->music.find(impl_->currentMusicId);
    if (it == impl_->music.end()) return;

    UpdateMusicStream(it->second);

    // If we're playing a playlist, auto-advance when the current track ends.
    if (!impl_->playlist.empty()) {
        // If the stream is still playing, nothing to do.
        if (IsMusicStreamPlaying(it->second)) return;

        // Advance to the next track in the playlist.
        size_t nextIndex = impl_->playlistIndex + 1;
        if (nextIndex >= impl_->playlist.size()) {
            if (impl_->playlistLoop) nextIndex = 0;
            else {
                // Playlist finished — clear state.
                impl_->currentMusicId.clear();
                impl_->playlist.clear();
                return;
            }
        }

        // Find the next loaded track, wrapping if necessary.
        bool found = false;
        for (size_t i = 0; i < impl_->playlist.size(); ++i) {
            size_t idx = (nextIndex + i) % impl_->playlist.size();
            const auto& nextId = impl_->playlist[idx];
            auto nit = impl_->music.find(nextId);
            if (nit != impl_->music.end()) {
                PlayMusicStream(nit->second);
                impl_->currentMusicId = nextId;
                impl_->playlistIndex = idx;
                found = true;
                break;
            }
            // If playlist doesn't loop and we've reached the end, stop trying.
            if (!impl_->playlistLoop && idx + 1 == impl_->playlist.size()) break;
        }
        if (!found) {
            impl_->currentMusicId.clear();
            impl_->playlist.clear();
        }
    }
}

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
    const int slotW  = trayW / 3;
    const float s     = layout_.cellPixels / 56.0f;
    const int prev    = std::max(12, static_cast<int>(20 * s)); // preview cell size scaled

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
    const float s = layout_.cellPixels / 56.0f;
    const int titleSize = std::max(16, static_cast<int>(28 * s));
    const int lineSize  = std::max(12, static_cast<int>(22 * s));
    const int smallSize = std::max(10, static_cast<int>(18 * s));

    DrawText("TRIVIA-DOKU", layout_.boardOriginX, 16, titleSize, kTextColor);

    const int hudX = layout_.boardOriginX + kBoardSize * layout_.cellPixels + 40;
    DrawText(TextFormat("Score:  %d", score),    hudX, 80,  lineSize, kTextColor);
    DrawText(TextFormat("Streak: %d/3", streak), hudX, 120, lineSize, kTextColor);
    DrawText("ESC to quit",                      hudX, 180, smallSize, kGridLine);
    DrawText("? for help (soon)",                hudX, 206, smallSize, kGridLine);

    // Version/build overlay in the lower-right corner.
    {
        const int margin = 8;
        const char* verText = TextFormat("v%s (%s)", kAppVersion.data(), kBuildId.data());
        const int tw = MeasureText(verText, smallSize);
        DrawText(verText,
                 layout_.windowWidth - tw - margin,
                 layout_.windowHeight - smallSize - margin,
                 smallSize, kGridLine);
    }
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

void Renderer::drawBoardCursor(int row, int col) const {
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) return;
    const int cell = layout_.cellPixels;
    const int ox   = layout_.boardOriginX;
    const int oy   = layout_.boardOriginY;
    const int px   = ox + col * cell;
    const int py   = oy + row * cell;
    // Bright gold outline, 3 px thick.
    DrawRectangleLinesEx({(float)px, (float)py, (float)cell, (float)cell}, 3.0f,
                         {240, 196, 68, 220});
}

void Renderer::drawTrayHighlight(int slot) const {
    if (slot < 0 || slot >= 3) return;
    const int boardCell = layout_.cellPixels;
    const int ox        = layout_.boardOriginX;
    const int oy        = layout_.boardOriginY;
    const int trayY     = oy + kBoardSize * boardCell + 12;
    const int trayH     = layout_.windowHeight - trayY - 8;
    const int trayW     = kBoardSize * boardCell;
    const int slotW     = trayW / 3;
    const int slotX     = ox + slot * slotW;
    // Gold highlight border around the selected slot.
    DrawRectangleLinesEx({(float)(slotX + 4), (float)trayY,
                          (float)(slotW - 8), (float)trayH}, 3.0f,
                         {240, 196, 68, 220});
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

    // Title (scaled to match board cell size)
    const float s = layout_.cellPixels / 56.0f;
    const char* title = "GAME OVER";
    const int kTitleSize = std::max(20, static_cast<int>(44 * s));
    const int tw = MeasureText(title, kTitleSize);
    DrawText(title, panelX + (kPanelW - tw) / 2, panelY + 22, kTitleSize, { 240, 196, 68, 255 });

    // Score
    const char* scoreStr = TextFormat("Final score:  %d", finalScore);
    const int kScoreSize = std::max(14, static_cast<int>(24 * s));
    const int sw = MeasureText(scoreStr, kScoreSize);
    DrawText(scoreStr, panelX + (kPanelW - sw) / 2, panelY + 108, kScoreSize, { 230, 234, 244, 255 });

    // Restart prompt
    const char* prompt = "Press R / click / A to play again";
    const int kPromptSize = std::max(12, static_cast<int>(18 * s));
    const int pw = MeasureText(prompt, kPromptSize);
    DrawText(prompt, panelX + (kPanelW - pw) / 2, panelY + 162, kPromptSize, { 130, 148, 175, 255 });
}

void Renderer::drawTriviaModal(const Question& q, int selected,
                               bool answered, bool correct,
                               const std::string& quip) const {
    const int W = layout_.windowWidth;
    const int H = layout_.windowHeight;

    // Semi-transparent full-screen dimmer.
    DrawRectangle(0, 0, W, H, { 0, 0, 0, 185 });

    // Panel occupies a fixed fraction of the window so it scales with any resolution.
    const int kPanelW = W * 45 / 100;   // 45% of window width
    const int kPanelH = H * 60 / 100;   // 60% of window height
    // Font scale derived from panel width relative to the 560 px baseline design.
    const float s = kPanelW / 560.0f;
    const int panelX    = (W - kPanelW) / 2;
    const int panelY    = (H - kPanelH) / 2;
    const int pad       = static_cast<int>(20 * s);

    DrawRectangle(panelX,  panelY,  kPanelW, kPanelH, { 28,  32,  46, 250 });
    DrawRectangle(panelX,  panelY,  kPanelW,       2,  {200, 156,  50, 255 }); // gold accent
    DrawRectangleLines(panelX, panelY, kPanelW, kPanelH, { 90, 100, 120, 200 });

    // Category + difficulty badges (top row).
    const int kBadgeSize = std::max(10, static_cast<int>(14 * s));
    DrawText(q.category.c_str(),   panelX + pad, panelY + static_cast<int>(10 * s), kBadgeSize, {200, 156, 50, 200});
    const char* diff = q.difficulty.c_str();
    const int dw = MeasureText(diff, kBadgeSize);
    DrawText(diff, panelX + kPanelW - pad - dw, panelY + static_cast<int>(10 * s), kBadgeSize, {130, 148, 175, 200});

    // Question text — word-wrapped.
    const int kQSize  = std::max(14, static_cast<int>(19 * s));
    const int kQTextW = kPanelW - pad * 2;
    const auto lines = wrapText(q.prompt, kQTextW, kQSize);
    int qY = panelY + static_cast<int>(34 * s);
    for (const auto& line : lines) {
        DrawText(line.c_str(), panelX + pad, qY, kQSize, kTextColor);
        qY += kQSize + static_cast<int>(4 * s);
    }

    // Choice rows.
    const int kChoiceSize = std::max(12, static_cast<int>(17 * s));
    const int kChoiceH    = std::max(24, static_cast<int>(34 * s));
    const char kLabels[]  = "ABCD";
    int choiceY = panelY + static_cast<int>(120 * s);

    for (int i = 0; i < 4; ++i) {
        const bool isSelected = (i == selected);

        // Highlight rect for selected (or correct/wrong) row.
        Color rowBg = { 0, 0, 0, 0 };
        if (answered) {
            if (i == q.answer)                   rowBg = { 20,  80,  20, 180 }; // correct answer
            else if (isSelected && !correct)     rowBg = { 80,  20,  20, 180 }; // wrong pick
        } else if (isSelected) {
            rowBg = { 60,  50,  10, 200 }; // gold tint for hover
        }
        if (rowBg.a > 0)
            DrawRectangle(panelX + pad - 4, choiceY - 2, kPanelW - pad * 2 + 8, kChoiceH, rowBg);

        // Selection arrow.
        if (isSelected && !answered)
            DrawText(">", panelX + pad - 4, choiceY + (kChoiceH - kChoiceSize) / 2,
                     kChoiceSize, {240, 196, 68, 255});

        // Label (A/B/C/D).
        const char label[3] = { kLabels[i], ' ', '\0' };
        DrawText(label, panelX + pad + static_cast<int>(12 * s), choiceY + (kChoiceH - kChoiceSize) / 2,
                 kChoiceSize, {170, 185, 210, 255});

        // Choice text.
        DrawText(q.choices[i].c_str(), panelX + pad + static_cast<int>(30 * s), choiceY + (kChoiceH - kChoiceSize) / 2,
                 kChoiceSize, kTextColor);

        choiceY += kChoiceH;
    }

    // Feedback row (shown after answering).
    if (answered) {
        const int feedY    = panelY + kPanelH - static_cast<int>(80 * s);
        const int feedSize = std::max(13, static_cast<int>(18 * s));
        if (correct) {
            const std::string msg = quip.empty() ? "CORRECT!" : "CORRECT!  " + quip;
            DrawText(msg.c_str(), panelX + pad, feedY, feedSize, {100, 220, 100, 255});
        } else {
            const std::string msg = quip.empty() ? "WRONG." : "WRONG.  " + quip;
            DrawText(msg.c_str(), panelX + pad, feedY, feedSize, {220, 80, 80, 255});
        }
    }

    // Hint line at the bottom.
    const char* hint = answered ? "Press any key / click to continue"
                                : "Click / 1-4 / \x18\x19 to choose   Enter / A to confirm";
    const int kHintSize = std::max(10, static_cast<int>(14 * s));
    const int hw = MeasureText(hint, kHintSize);
    DrawText(hint, panelX + (kPanelW - hw) / 2, panelY + kPanelH - static_cast<int>(26 * s),
             kHintSize, {130, 148, 175, 255});
}

} // namespace td
