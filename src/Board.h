// Board.h — the 9x9 grid and its rules.
//
// Design note: Board knows nothing about pixels, colors, or raylib. It deals
// only in cell coordinates and cell states. This makes it trivially unit-testable
// and keeps rendering swappable. Renderer asks Board what's where and draws it.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace td {

// What occupies a single cell on the board.
// - Empty:  nothing there, a piece can be placed
// - Filled: a normal block from a placed piece
// - Trivia: a "glowing ?" block — placing one that completes a clear triggers a question
// - Stone:  penalty block from a wrong trivia answer; blocks line clears until redeemed
enum class CellState : uint8_t {
    Empty,
    Filled,
    Trivia,
    Stone,
};

constexpr int kBoardSize = 9;
constexpr int kSubGrid   = 3;   // size of the sub-square (3x3 within the 9x9)

// A coordinate on the board. (0,0) is top-left; row grows downward.
struct Cell {
    int row;
    int col;
};

class Board {
public:
    Board();

    // Read-only cell access. Returns Empty for out-of-range queries so callers
    // don't have to bounds-check before asking.
    CellState at(int row, int col) const;

    // Can a piece with the given occupied cells (relative to a top-left anchor)
    // be legally placed with its anchor at (row, col)? A placement is legal iff
    // every target cell is in bounds AND currently Empty.
    bool canPlace(const std::vector<Cell>& shape, int anchorRow, int anchorCol) const;

    // Place a piece. Caller must have already confirmed canPlace() is true.
    // `isTrivia` marks the cells as Trivia rather than Filled; relevant for
    // deciding whether a resulting clear triggers a question.
    void place(const std::vector<Cell>& shape, int anchorRow, int anchorCol, bool isTrivia);

    // Detect any full rows/columns/sub-squares and clear them. Stone cells are
    // *not* counted as filled for clear-detection purposes — a line with a stone
    // in it can never clear, which is the whole point of stones.
    //
    // Returns the cells that were cleared, so the caller (scoring, FX) can react.
    // Also reports whether any cleared cell was a Trivia block — that's how the
    // caller knows to pop the trivia modal.
    struct ClearResult {
        std::vector<Cell> clearedCells;
        std::vector<Cell> triviaCells;    // subset that held CellState::Trivia before erasure
        bool              triggeredTrivia; // true iff triviaCells is non-empty
    };
    ClearResult detectAndClear();

    // Place a single Stone at (row, col). Called when a wrong trivia answer is
    // given; the trivia cell's position comes back as an unclearable penalty block.
    // No-op if the coordinates are out of range.
    void placeStone(int row, int col);

    // Shatter every stone on the board (called when the 3-in-a-row redemption
    // streak fires). Returns the number of stones removed, for bonus scoring.
    int shatterStones();

    // For the "game over" check: can ANY of the given shapes be placed anywhere?
    // Used by Game to end the round when the current tray is unplayable.
    bool anyPlacementPossible(const std::vector<std::vector<Cell>>& shapes) const;

    // Per-piece variant: true iff the given shape can be placed at any valid
    // anchor on the 9x9 board. Used to tint unplayable tray pieces red.
    bool canPlaceAnywhere(const std::vector<Cell>& cells) const;

private:
    std::array<std::array<CellState, kBoardSize>, kBoardSize> grid_{};

    // Helpers: a row/col/sub-square is "full" only if every cell is non-Empty
    // AND none of them are Stones. Stones explicitly poison the line.
    bool rowIsClearable(int row) const;
    bool colIsClearable(int col) const;
    bool subIsClearable(int subRow, int subCol) const;
};

} // namespace td
