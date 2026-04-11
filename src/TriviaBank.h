// TriviaBank.h — question data and the shuffled-deck loader.
//
// Keeps all JSON / trivia concerns out of Board and Renderer.
// TriviaBank owns the question list and a shuffled deck index so every question
// is shown exactly once before any can repeat.

#pragma once

#include <array>
#include <string>
#include <vector>

namespace td {

struct Question {
    std::string id;
    std::string category;
    std::string difficulty;  // "easy", "medium", "hard"
    std::string prompt;
    std::array<std::string, 4> choices;
    int         answer = 0;  // index into choices (0-3)
    std::string quipCorrect;
    std::string quipWrong;
};

class TriviaBank {
public:
    // Load questions from a JSON file. Returns false if the file is missing or
    // cannot be parsed. The bank is still usable (empty) after a failed load.
    bool loadFromFile(const std::string& path);

    // True if at least one question was loaded successfully.
    bool loaded() const { return !questions_.empty(); }

    // Return the next question in the shuffled deck. When all questions have
    // been shown the deck reshuffles automatically so no question repeats until
    // the full set has been exhausted.
    const Question& pick();

    // Scoring multiplier for a given difficulty string.
    // easy→1.5, medium→2.0, hard→3.0; anything else→1.0.
    static float multiplier(const std::string& difficulty);

private:
    std::vector<Question> questions_;
    std::vector<int>      deck_;      // shuffled indices into questions_
    int                   deckIdx_ = 0;

    void reshuffleDeck();
};

} // namespace td
