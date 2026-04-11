// main.cpp — entry point and main game loop.
//
// Day 2: piece tray is live. Three randomly-chosen polyomino shapes appear
// below the board. No interaction yet — that lands in Day 3 (drag-and-drop).

#include "Board.h"
#include "Piece.h"
#include "Renderer.h"
#include <raylib.h>

int main() {
    td::Layout layout;
    InitWindow(layout.windowWidth, layout.windowHeight, "Trivia-Doku");
    SetTargetFPS(60);

    td::Board    board;
    td::Renderer renderer(layout);

    auto tray = td::makeTray();

    int score  = 0;
    int streak = 0;

    while (!WindowShouldClose()) {
        // update() will live here once we have interaction (Day 3+).

        BeginDrawing();
            ClearBackground({ 24, 28, 38, 255 });
            renderer.drawBoard(board);
            renderer.drawTray(tray);
            renderer.drawHud(score, streak);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
