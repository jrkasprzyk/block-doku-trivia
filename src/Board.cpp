// Board.cpp — logic implementation for the 9x9 grid.

#include "Board.h"

namespace td {

Board::Board() {
    for (auto& row : grid_) {
        row.fill(CellState::Empty);
    }
}

CellState Board::at(int row, int col) const {
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) {
        return CellState::Empty;
    }
    return grid_[row][col];
}

bool Board::canPlace(const std::vector<Cell>& shape, int anchorRow, int anchorCol) const {
    for (const auto& offset : shape) {
        const int r = anchorRow + offset.row;
        const int c = anchorCol + offset.col;
        if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize) return false;
        if (grid_[r][c] != CellState::Empty) return false;
    }
    return true;
}

void Board::place(const std::vector<Cell>& shape, int anchorRow, int anchorCol, bool isTrivia) {
    const CellState state = isTrivia ? CellState::Trivia : CellState::Filled;
    for (const auto& offset : shape) {
        grid_[anchorRow + offset.row][anchorCol + offset.col] = state;
    }
}

bool Board::rowIsClearable(int row) const {
    for (int c = 0; c < kBoardSize; ++c) {
        const auto s = grid_[row][c];
        if (s == CellState::Empty || s == CellState::Stone) return false;
    }
    return true;
}

bool Board::colIsClearable(int col) const {
    for (int r = 0; r < kBoardSize; ++r) {
        const auto s = grid_[r][col];
        if (s == CellState::Empty || s == CellState::Stone) return false;
    }
    return true;
}

bool Board::subIsClearable(int subRow, int subCol) const {
    const int r0 = subRow * kSubGrid;
    const int c0 = subCol * kSubGrid;
    for (int r = r0; r < r0 + kSubGrid; ++r) {
        for (int c = c0; c < c0 + kSubGrid; ++c) {
            const auto s = grid_[r][c];
            if (s == CellState::Empty || s == CellState::Stone) return false;
        }
    }
    return true;
}

Board::ClearResult Board::detectAndClear() {
    // Mark phase: build a boolean mask of cells-to-clear without mutating the
    // grid yet. This avoids the bug where clearing a row mid-scan would hide a
    // column clear that depended on one of that row's cells.
    std::array<std::array<bool, kBoardSize>, kBoardSize> mark{};

    for (int r = 0; r < kBoardSize; ++r) {
        if (rowIsClearable(r)) {
            for (int c = 0; c < kBoardSize; ++c) mark[r][c] = true;
        }
    }
    for (int c = 0; c < kBoardSize; ++c) {
        if (colIsClearable(c)) {
            for (int r = 0; r < kBoardSize; ++r) mark[r][c] = true;
        }
    }
    for (int sr = 0; sr < kSubGrid; ++sr) {
        for (int sc = 0; sc < kSubGrid; ++sc) {
            if (subIsClearable(sr, sc)) {
                for (int r = sr * kSubGrid; r < (sr + 1) * kSubGrid; ++r) {
                    for (int c = sc * kSubGrid; c < (sc + 1) * kSubGrid; ++c) {
                        mark[r][c] = true;
                    }
                }
            }
        }
    }

    // Sweep phase: collect cleared cells, capture trivia cells BEFORE erasing them.
    ClearResult result;
    result.triggeredTrivia = false;
    for (int r = 0; r < kBoardSize; ++r) {
        for (int c = 0; c < kBoardSize; ++c) {
            if (mark[r][c]) {
                if (grid_[r][c] == CellState::Trivia) {
                    result.triviaCells.push_back({r, c});
                    result.triggeredTrivia = true;
                }
                result.clearedCells.push_back({r, c});
                grid_[r][c] = CellState::Empty;
            }
        }
    }
    return result;
}

void Board::placeStone(int row, int col) {
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) return;
    grid_[row][col] = CellState::Stone;
}

int Board::shatterStones() {
    int count = 0;
    for (auto& row : grid_) {
        for (auto& cell : row) {
            if (cell == CellState::Stone) {
                cell = CellState::Empty;
                ++count;
            }
        }
    }
    return count;
}

bool Board::anyPlacementPossible(const std::vector<std::vector<Cell>>& shapes) const {
    for (const auto& shape : shapes) {
        if (shape.empty()) continue; // already-placed piece in the tray
        for (int r = 0; r < kBoardSize; ++r) {
            for (int c = 0; c < kBoardSize; ++c) {
                if (canPlace(shape, r, c)) return true;
            }
        }
    }
    return false;
}

bool Board::canPlaceAnywhere(const std::vector<Cell>& cells) const {
    if (cells.empty()) return false;
    for (int r = 0; r < kBoardSize; ++r)
        for (int c = 0; c < kBoardSize; ++c)
            if (canPlace(cells, r, c)) return true;
    return false;
}

} // namespace td
