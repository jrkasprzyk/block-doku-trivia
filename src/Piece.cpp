// Piece.cpp — shape catalog and tray generator.

#include "Piece.h"

#include <algorithm>
#include <numeric>
#include <random>

namespace td {

// ---------------------------------------------------------------------------
// Shape catalog
// Each shape's cells are listed with (row, col) offsets from the top-left of
// the bounding box.  Shapes are grouped by cell count for readability.
// ---------------------------------------------------------------------------
const std::vector<PieceShape> kAllShapes = {

    // ── 1-cell ──────────────────────────────────────────────────────────────
    { "Dot",           { {0,0} } },

    // ── 2-cell ──────────────────────────────────────────────────────────────
    { "H-Domino",      { {0,0},{0,1} } },
    { "V-Domino",      { {0,0},{1,0} } },

    // ── 3-cell ──────────────────────────────────────────────────────────────
    { "H-Tromino",     { {0,0},{0,1},{0,2} } },
    { "V-Tromino",     { {0,0},{1,0},{2,0} } },

    // L-tromino corners (one per rotation)
    { "Corner-TL",     { {0,0},{1,0},{1,1} } },   //  ·
                                                    //  ··
    { "Corner-TR",     { {0,1},{1,0},{1,1} } },    //   ·
                                                    //  ··
    { "Corner-BL",     { {0,0},{0,1},{1,0} } },    //  ··
                                                    //  ·
    { "Corner-BR",     { {0,0},{0,1},{1,1} } },    //  ··
                                                    //   ·

    // ── 4-cell ──────────────────────────────────────────────────────────────

    // I-tetromino
    { "I4-H",          { {0,0},{0,1},{0,2},{0,3} } },
    { "I4-V",          { {0,0},{1,0},{2,0},{3,0} } },

    // O-tetromino (2×2 square)
    { "Square-2x2",    { {0,0},{0,1},{1,0},{1,1} } },

    // L-tetromino (4 rotations)
    //  ·        ···     ··      ···  (rightward foot)
    //  ·        ·        ·       ··  (NB: col arrangement)
    //  ··
    { "L-0",           { {0,0},{1,0},{2,0},{2,1} } },
    { "L-90",          { {0,0},{0,1},{0,2},{1,0} } },
    { "L-180",         { {0,0},{0,1},{1,1},{2,1} } },
    { "L-270",         { {0,2},{1,0},{1,1},{1,2} } },

    // J-tetromino (4 rotations — mirror of L)
    { "J-0",           { {0,1},{1,1},{2,0},{2,1} } },
    { "J-90",          { {0,0},{1,0},{1,1},{1,2} } },
    { "J-180",         { {0,0},{0,1},{1,0},{2,0} } },
    { "J-270",         { {0,0},{0,1},{0,2},{1,2} } },

    // S-tetromino (2 forms)
    { "S-H",           { {0,1},{0,2},{1,0},{1,1} } },
    { "S-V",           { {0,0},{1,0},{1,1},{2,1} } },

    // Z-tetromino (2 forms)
    { "Z-H",           { {0,0},{0,1},{1,1},{1,2} } },
    { "Z-V",           { {0,1},{1,0},{1,1},{2,0} } },

    // T-tetromino (4 rotations)
    { "T-0",           { {0,0},{0,1},{0,2},{1,1} } },
    { "T-90",          { {0,0},{1,0},{1,1},{2,0} } },
    { "T-180",         { {0,1},{1,0},{1,1},{1,2} } },
    { "T-270",         { {0,1},{1,0},{1,1},{2,1} } },

    // ── 5-cell ──────────────────────────────────────────────────────────────

    // I-pentomino
    { "I5-H",          { {0,0},{0,1},{0,2},{0,3},{0,4} } },
    { "I5-V",          { {0,0},{1,0},{2,0},{3,0},{4,0} } },

    // ── 6-cell ──────────────────────────────────────────────────────────────

    { "Rect-2x3",      { {0,0},{0,1},{0,2},{1,0},{1,1},{1,2} } },
    { "Rect-3x2",      { {0,0},{0,1},{1,0},{1,1},{2,0},{2,1} } },

    // ── 9-cell ──────────────────────────────────────────────────────────────

    { "Square-3x3",    { {0,0},{0,1},{0,2},
                         {1,0},{1,1},{1,2},
                         {2,0},{2,1},{2,2} } },
};

// ---------------------------------------------------------------------------
// makeTray
// ---------------------------------------------------------------------------
std::array<Piece, 3> makeTray() {
    static std::mt19937 rng(std::random_device{}());

    // Shuffle a list of indices so we never repeat a shape within one tray.
    std::vector<int> indices(static_cast<int>(kAllShapes.size()));
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    std::uniform_real_distribution<float> triviaRoll(0.f, 1.f);

    std::array<Piece, 3> tray;
    for (int i = 0; i < 3; ++i) {
        tray[i].shape    = kAllShapes[indices[i]];
        tray[i].isTrivia = triviaRoll(rng) < 0.20f;
    }
    return tray;
}

} // namespace td
