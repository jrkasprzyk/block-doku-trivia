// Piece.h — polyomino piece definitions and tray generation.
//
// A "piece" is a set of cells (relative offsets from a 0,0 anchor) that a
// player can place on the board. The catalog covers all classic block-doku
// shapes: singles through the 3×3 square.
//
// No raylib here. Piece is pure game data.

#pragma once

#include "Board.h"   // for Cell

#include <array>
#include <vector>

namespace td {

// Immutable shape definition: a name and the list of cell offsets.
// Offsets are normalized so the bounding box starts at (0,0).
struct PieceShape {
    const char*       name;
    std::vector<Cell> cells;
};

// The full catalog of available shapes. Defined in Piece.cpp.
extern const std::vector<PieceShape> kAllShapes;

// One piece in the player's tray. Wraps a shape and carries the trivia flag.
struct Piece {
    PieceShape shape;
    bool       isTrivia = false;
};

// Build a fresh tray of 3 distinct randomly-chosen pieces.
// Each slot has roughly a 20% chance of being a trivia piece.
std::array<Piece, 3> makeTray();

} // namespace td
