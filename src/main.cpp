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
#include <filesystem>
#include <cctype>
// Logging helpers
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

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

// Gamepad cursor state for board navigation ——————————————————
struct GpadCursor {
    int  row      = 4;   // board cell row
    int  col      = 4;   // board cell col
    int  traySlot = 0;   // highlighted tray slot (0-2)
    bool holding  = false;
    int  heldSlot = -1;  // which tray slot was picked up
    td::Piece piece;     // copy of held piece

    // D-pad / stick repeat timing
    float repeatX = 0.0f;
    float repeatY = 0.0f;
    static constexpr float kFirstDelay = 0.25f; // initial hold delay
    static constexpr float kRepeatRate = 0.08f;  // repeat interval after first

    // Returns -1, 0, or +1 for an axis with repeat logic.
    static int axisRepeat(float& timer, bool negative, bool positive, float dt) {
        int dir = 0;
        if (negative) dir = -1;
        else if (positive) dir = 1;

        if (dir == 0) { timer = 0.0f; return 0; }

        if (timer <= 0.0f) {
            timer = kFirstDelay;
            return dir;
        }
        timer -= dt;
        if (timer <= 0.0f) {
            timer = kRepeatRate;
            return dir;
        }
        return 0;
    }
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

    int anchorCol = static_cast<int>(std::round((originX - ox) / static_cast<float>(cell)));
    int anchorRow = static_cast<int>(std::round((originY - oy) / static_cast<float>(cell)));

    // When the mouse is within the board area, clamp the anchor so the entire
    // piece fits on the board. This prevents the ghost and preview from
    // disagreeing and stops pieces from hanging off the edge.
    if (layout.isOverBoard(mouseX, mouseY)) {
        anchorCol = std::clamp(anchorCol, 0, td::kBoardSize - 1 - maxC);
        anchorRow = std::clamp(anchorRow, 0, td::kBoardSize - 1 - maxR);
    }
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

// Trace-to-file callback: writes raylib TraceLog output to `logs/trivia_doku.log`
namespace {
    static std::ofstream g_logFile;
    static std::mutex g_logMutex;

    static void TraceToFile(int logType, const char *text, va_list args) {
        // Also print to stderr so console output remains visible
        {
            va_list copy;
            va_copy(copy, args);
            vfprintf(stderr, text, copy);
            fprintf(stderr, "\n");
            va_end(copy);
        }

        // Format the message into a buffer
        char buf[4096];
        va_list copy2;
        va_copy(copy2, args);
        vsnprintf(buf, sizeof(buf), text, copy2);
        va_end(copy2);

        // Timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char timestr[64];
        std::strftime(timestr, sizeof(timestr), "%F %T", &tm);

        const char* level = "LOG";
        switch (logType) {
            case LOG_INFO: level = "INFO"; break;
            case LOG_WARNING: level = "WARN"; break;
            case LOG_ERROR: level = "ERROR"; break;
            case LOG_DEBUG: level = "DEBUG"; break;
            case LOG_TRACE: level = "TRACE"; break;
        }

        std::lock_guard<std::mutex> lk(g_logMutex);
        if (!g_logFile.is_open()) {
            try {
                std::filesystem::create_directories("logs");
                g_logFile.open("logs/trivia_doku.log", std::ios::app);
            } catch (...) {}
        }
        if (g_logFile.is_open()) {
            g_logFile << "[" << timestr << "] [" << level << "] " << buf << '\n';
            g_logFile.flush();
        }
    }
} // namespace

int main() {
    // Install trace callback to mirror logs to a file (logs/trivia_doku.log)
    SetTraceLogCallback(TraceToFile);

    td::Layout layout;
    // Allow the window to be resized by the OS; initialize at the preferred size.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(layout.windowWidth, layout.windowHeight, "Trivia-Doku");
    // Recalculate layout metrics based on the actual window size (handles DPI/fullscreen).
    layout.recalcForWindow(GetScreenWidth(), GetScreenHeight());
    SetTargetFPS(120);

    td::Board    board;
    td::Renderer renderer(layout);
    td::TriviaBank triviaBank;
    triviaBank.loadFromFile("assets/trivia.json");

    // Optional: initialize audio and attempt to load conventionally-named assets
    // (no-op if files don't exist). User-provided assets should live under
    // `assets/` and follow names documented in /docs/audio-assets.md.
    //
    // for examples see: https://www.raylib.com/examples/audio/loader.html?name=audio_music_stream
    
    renderer.initAudio();
    // Load background playlist tracks: scan `assets/bg_music` and load all
    // supported files (sorted alphabetically). If none are found, fall back
    // to the original hardcoded list.
    auto tryLoadMusic = [&](const std::string& id, const std::string& path) -> bool {
        if (!renderer.loadMusic(id, path)) {
            TraceLog(LOG_WARNING, "Missing music: %s", path.c_str());
            return false;
        }
        return true;
    };

    std::vector<std::string> playlistIds;
    namespace fs = std::filesystem;
    const fs::path musicDir("assets/bg_music");
    if (fs::exists(musicDir) && fs::is_directory(musicDir)) {
        std::vector<std::pair<std::string, fs::path>> files;
        for (const auto& entry : fs::directory_iterator(musicDir)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if (ext == ".ogg" || ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".m4a") {
                files.emplace_back(entry.path().stem().string(), entry.path());
            }
        }
        if (!files.empty()) {
            std::sort(files.begin(), files.end(), [](auto &a, auto &b){ return a.first < b.first; });
            for (const auto& p : files) {
                std::string stem = p.first;
                std::string id = "bg_" + stem;
                for (auto &ch : id) if (!(std::isalnum((unsigned char)ch) || ch == '_')) ch = '_';
                if (tryLoadMusic(id, p.second.string())) playlistIds.push_back(id);
            }
        }
    }

    // Fallback: if nothing was discovered/loaded, use the original hardcoded list.
    if (playlistIds.empty()) {
        if (tryLoadMusic("bg_amapiano", "assets/bg_music/amapiano_logging_the_drum_main.ogg")) playlistIds.push_back("bg_amapiano");
        if (tryLoadMusic("bg_hiphop",    "assets/bg_music/hip-hop-vodka-main.ogg")) playlistIds.push_back("bg_hiphop");
        if (tryLoadMusic("bg_synth",     "assets/bg_music/synthwave_cybertruck_chase_main.ogg")) playlistIds.push_back("bg_synth");
        if (tryLoadMusic("bg_west",      "assets/bg_music/westernaftrica_kalahari_main.ogg")) playlistIds.push_back("bg_west");
    }

    // Start the playlist (looping).
    renderer.playMusicPlaylist(playlistIds, true);
    // Helper to try loading sfx and log missing ones.
    auto tryLoad = [&](const std::string& id, const std::string& path){
        if (!renderer.loadSoundEffect(id, path)) {
            TraceLog(LOG_WARNING, "Missing sound effect: %s", path.c_str());
        }
    };
    tryLoad("place",    "assets/sfx/pro_memory_card_eject_insert_a.wav");
    tryLoad("clear",    "assets/sfx/healing_potion_001.wav");
    tryLoad("correct",  "assets/sfx/success_q.wav");
    tryLoad("wrong",    "assets/sfx/failure_q.wav");
    tryLoad("pickup",   "assets/sfx/handling_metal_equipment_b.wav");
    tryLoad("cancel",   "assets/sfx/server_hum_end.wav");
    tryLoad("deal",     "assets/sfx/stick_hit_001.wav");
    tryLoad("shatter",  "assets/sfx/director_chair_creaking_a.wav");
    tryLoad("gameover", "assets/sfx/jingle_win_003.wav");
    tryLoad("restart",  "assets/sfx/bad_button_001.wav");
    // (previous hardcoded playlist removed; dynamic playlist already started)

    auto tray = td::makeTray();

    // Track borderless/fullscreen state so we can auto-enter true fullscreen
    // when the user maximizes the window via the OS. We avoid fighting the
    // user's explicit F11 toggle by remembering whether we auto-entered.
    bool isBorderless = false;
    bool autoEnterFromMaximize = false;
    bool prevMaximized = IsWindowMaximized();

    int score  = 0;
    int streak = 0;
    ClearAnim  clearAnim;
    TriviaModal triviaModal;
    Phase phase = Phase::Playing;

    DragState drag;
    GpadCursor gpad;

    while (!WindowShouldClose()) {

        // ── Input / Update ──────────────────────────────────────────────────
        const int mx = GetMouseX();
        const int my = GetMouseY();

        // Tray geometry — recalculated each frame so it stays correct after resize/fullscreen.
        const float s = layout.cellPixels / 56.0f;
        const int gap = std::max(6, static_cast<int>(12 * s));
        const int trayY = layout.boardOriginY + td::kBoardSize * layout.cellPixels + gap;
        const int trayW = td::kBoardSize * layout.cellPixels;
        const int slotW = trayW / 3;
        // Debug test keys: F1=clear, F2=correct, F3=wrong
        if (IsKeyPressed(KEY_F1)) { renderer.playSoundEffect("clear"); TraceLog(LOG_INFO, "Debug: played SFX 'clear'"); }
        if (IsKeyPressed(KEY_F2)) { renderer.playSoundEffect("correct"); TraceLog(LOG_INFO, "Debug: played SFX 'correct'"); }
        if (IsKeyPressed(KEY_F3)) { renderer.playSoundEffect("wrong"); TraceLog(LOG_INFO, "Debug: played SFX 'wrong'"); }
        // Handle OS-driven resize events (including the frame after ToggleBorderlessWindowed).
        if (IsWindowResized()) {
            layout.recalcForWindow(GetScreenWidth(), GetScreenHeight());
            renderer.setLayout(layout);

            // If the user hit the window manager's maximize button, enter
            // borderless fullscreen automatically. If we auto-entered on
            // maximize, restore when un-maximizing.
            bool newMaximized = IsWindowMaximized();
            if (newMaximized != prevMaximized) {
                if (newMaximized && !isBorderless) {
                    ToggleBorderlessWindowed();
                    isBorderless = true;
                    autoEnterFromMaximize = true;
                } else if (!newMaximized && autoEnterFromMaximize && isBorderless) {
                    ToggleBorderlessWindowed();
                    isBorderless = false;
                    autoEnterFromMaximize = false;
                }
                prevMaximized = newMaximized;
            }
        }

        // Toggle fullscreen (F11); the actual resize is caught by IsWindowResized above.
        if (IsKeyPressed(KEY_F11)) {
            ToggleBorderlessWindowed();
            isBorderless = !isBorderless;
            autoEnterFromMaximize = false; // user-requested toggle overrides auto state
            prevMaximized = IsWindowMaximized();
        }

        // Restart from game-over screen.
        if (phase == Phase::GameOver &&
            (IsKeyPressed(KEY_R)
             || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
             || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))) {
            board        = td::Board{};
            tray         = td::makeTray();
            score        = 0;
            streak       = 0;
            clearAnim    = {};
            triviaModal  = {};
            drag         = {};
            gpad         = {};
            phase        = Phase::Playing;
            renderer.playSoundEffect("restart");
        }

        // Gamepad detection (use first connected gamepad).
        const int gp = 0; // gamepad index
        const bool gpAvail = IsGamepadAvailable(gp);

        // ── Trivia modal input ──────────────────────────────────────────────
        if (phase == Phase::Trivia) {
            if (!triviaModal.answered) {
                // Navigate choices — keyboard.
                if (IsKeyPressed(KEY_UP))   triviaModal.selected = (triviaModal.selected + 3) % 4;
                if (IsKeyPressed(KEY_DOWN)) triviaModal.selected = (triviaModal.selected + 1) % 4;
                if (IsKeyPressed(KEY_ONE))   triviaModal.selected = 0;
                if (IsKeyPressed(KEY_TWO))   triviaModal.selected = 1;
                if (IsKeyPressed(KEY_THREE)) triviaModal.selected = 2;
                if (IsKeyPressed(KEY_FOUR))  triviaModal.selected = 3;

                // Navigate choices — gamepad D-pad.
                if (gpAvail) {
                    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_UP))
                        triviaModal.selected = (triviaModal.selected + 3) % 4;
                    if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
                        triviaModal.selected = (triviaModal.selected + 1) % 4;
                }

                // Navigate choices — mouse click on choice rows.
                // Replicate the choice row geometry from drawTriviaModal.
                {
                    const int W = layout.windowWidth;
                    const int H = layout.windowHeight;
                    const int kPanelW = W * 45 / 100;
                    const int kPanelH = H * 60 / 100;
                    const float panelScale = kPanelW / 560.0f;
                    const int panelX  = (W - kPanelW) / 2;
                    const int panelY  = (H - kPanelH) / 2;
                    const int pad     = static_cast<int>(20 * panelScale);
                    const int kChoiceH = std::max(24, static_cast<int>(34 * panelScale));
                    const int choiceY0 = panelY + static_cast<int>(120 * panelScale);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        for (int i = 0; i < 4; ++i) {
                            const int rowY = choiceY0 + i * kChoiceH;
                            if (mx >= panelX + pad - 4 && mx < panelX + kPanelW - pad + 4 &&
                                my >= rowY - 2 && my < rowY - 2 + kChoiceH) {
                                if (triviaModal.selected == i) {
                                    // Clicking an already-selected choice submits it.
                                    goto submit_answer;
                                }
                                triviaModal.selected = i;
                                break;
                            }
                        }
                    }
                }

                // Submit answer — keyboard, gamepad A button, or mouse double-click (handled above).
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)
                    || (gpAvail && IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                    submit_answer:
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
                // After answer: wait for explicit input (no automatic timeout).
                bool anyInput = false;
                for (int k = 32; k < 350; ++k) {
                    if (IsKeyPressed(k)) { anyInput = true; break; }
                }
                if (!anyInput && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) anyInput = true;
                if (!anyInput && gpAvail) {
                    // Any gamepad button dismisses.
                    for (int b = 0; b <= 17; ++b) {
                        if (IsGamepadButtonPressed(gp, b)) { anyInput = true; break; }
                    }
                }
                if (anyInput) {
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

        // ── Gamepad cursor input (only when Playing, not mouse-dragging) ──
        if (phase == Phase::Playing && !drag.active && gpAvail) {
            const float dt = GetFrameTime();

            // D-pad (pressed) for single-step moves.
            bool dU = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_UP);
            bool dD = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
            bool dL = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
            bool dR = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

            // Left stick with deadzone.
            const float stickX = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_X);
            const float stickY = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_Y);
            constexpr float kDeadzone = 0.4f;
            if (stickX < -kDeadzone) dL = true;
            if (stickX >  kDeadzone) dR = true;
            if (stickY < -kDeadzone) dU = true;
            if (stickY >  kDeadzone) dD = true;

            const int moveY = GpadCursor::axisRepeat(gpad.repeatY, dU, dD, dt);
            const int moveX = GpadCursor::axisRepeat(gpad.repeatX, dL, dR, dt);

            gpad.row = std::clamp(gpad.row + moveY, 0, td::kBoardSize - 1);
            gpad.col = std::clamp(gpad.col + moveX, 0, td::kBoardSize - 1);

            // LB / RB cycle tray slot selection.
            if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
                // Cycle backward to next non-empty slot.
                for (int i = 0; i < 3; ++i) {
                    gpad.traySlot = (gpad.traySlot + 2) % 3;
                    if (!tray[gpad.traySlot].shape.cells.empty()) break;
                }
            }
            if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
                for (int i = 0; i < 3; ++i) {
                    gpad.traySlot = (gpad.traySlot + 1) % 3;
                    if (!tray[gpad.traySlot].shape.cells.empty()) break;
                }
            }

            // A button: pick up or place.
            if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                if (!gpad.holding) {
                    // Pick up the selected tray piece.
                    if (gpad.traySlot >= 0 && gpad.traySlot < 3 &&
                        !tray[gpad.traySlot].shape.cells.empty()) {
                        gpad.holding  = true;
                        gpad.heldSlot = gpad.traySlot;
                        gpad.piece    = tray[gpad.traySlot];
                        tray[gpad.traySlot].shape.cells.clear();
                        renderer.playSoundEffect("pickup");
                    }
                } else {
                    // Try to place at cursor position.
                    if (board.canPlace(gpad.piece.shape.cells, gpad.row, gpad.col)) {
                        board.place(gpad.piece.shape.cells, gpad.row, gpad.col,
                                    gpad.piece.isTrivia);
                        renderer.playSoundEffect("place");

                        const auto result = board.detectAndClear();
                        score += static_cast<int>(gpad.piece.shape.cells.size());

                        if (result.triggeredTrivia) {
                            triviaModal.question     = &triviaBank.pick();
                            triviaModal.clearedCells = result.clearedCells;
                            triviaModal.triviaOrigin = result.triviaCells[0];
                            triviaModal.selected     = 0;
                            triviaModal.answered     = false;
                            triviaModal.dismissFrames = 0;
                            if (!result.clearedCells.empty())
                                clearAnim.trigger(result.clearedCells);
                            if (!result.clearedCells.empty()) renderer.playSoundEffect("clear");
                            phase = Phase::Trivia;
                        } else {
                            score += static_cast<int>(result.clearedCells.size()) * 10;
                            if (!result.clearedCells.empty()) {
                                clearAnim.trigger(result.clearedCells);
                                renderer.playSoundEffect("clear");
                            }
                        }

                        int justPlacedSlot = gpad.heldSlot;
                        gpad.holding = false;
                        gpad.heldSlot = -1;

                        // Refill tray if all empty.
                        bool allEmpty = true;
                        for (const auto& p : tray) {
                            if (!p.shape.cells.empty()) { allEmpty = false; break; }
                        }
                        if (allEmpty) {
                            tray = td::makeTray();
                            renderer.playSoundEffect("deal");
                            gpad.traySlot = 0;  // fresh tray — start at slot 0
                        } else {
                            // Advance to next non-empty slot after the one just placed.
                            for (int i = 1; i <= 3; ++i) {
                                int candidate = (justPlacedSlot + i) % 3;
                                if (!tray[candidate].shape.cells.empty()) {
                                    gpad.traySlot = candidate;
                                    break;
                                }
                            }
                        }

                        if (phase == Phase::Playing) {
                            checkGameOver(phase, board, tray);
                            if (phase == Phase::GameOver) renderer.playSoundEffect("gameover");
                        }
                    }
                }
            }

            // B button: cancel held piece.
            if (gpad.holding && IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                tray[gpad.heldSlot] = gpad.piece;
                gpad.holding  = false;
                gpad.heldSlot = -1;
                renderer.playSoundEffect("cancel");
            }
        }

        // Reset gamepad cursor hold state on game restart.
        if (phase == Phase::Playing && gpad.holding) {
            // If the held slot piece reappeared (restart), drop the hold.
            if (!tray[gpad.heldSlot].shape.cells.empty()) {
                gpad.holding = false;
                gpad.heldSlot = -1;
            }
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

            // Gamepad placement preview (when holding a piece).
            if (gpAvail && gpad.holding && !drag.active && phase == Phase::Playing) {
                renderer.drawPlacementPreview(board, gpad.piece, gpad.row, gpad.col);
            }

            std::array<bool, 3> trayPlaceable;
            for (int i = 0; i < 3; ++i)
                trayPlaceable[i] = board.canPlaceAnywhere(tray[i].shape.cells);

            renderer.drawTray(tray,
                              drag.active ? drag.traySlot : (gpad.holding ? gpad.heldSlot : -1),
                              trayPlaceable);
            renderer.drawHud(score, streak);

            // Gamepad cursor and tray highlight.
            if (gpAvail && phase == Phase::Playing && !drag.active) {
                renderer.drawBoardCursor(gpad.row, gpad.col);
                if (!gpad.holding)
                    renderer.drawTrayHighlight(gpad.traySlot);
            }

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
