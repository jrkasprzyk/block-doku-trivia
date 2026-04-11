// TriviaBank.cpp — JSON loading and deck management.

#include "TriviaBank.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <random>

namespace td {

bool TriviaBank::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    nlohmann::json root;
    try {
        file >> root;
    } catch (...) {
        return false;
    }

    if (!root.contains("questions") || !root["questions"].is_array()) return false;

    for (const auto& obj : root["questions"]) {
        // Required fields — skip malformed entries rather than crashing.
        if (!obj.contains("prompt") || !obj.contains("choices") ||
            !obj.contains("answer") || !obj["choices"].is_array() ||
            obj["choices"].size() != 4) {
            continue;
        }

        Question q;
        q.id         = obj.value("id",         "");
        q.category   = obj.value("category",   "");
        q.difficulty = obj.value("difficulty",  "medium");
        q.prompt     = obj["prompt"].get<std::string>();
        q.answer     = obj["answer"].get<int>();

        for (int i = 0; i < 4; ++i)
            q.choices[i] = obj["choices"][i].get<std::string>();

        // Optional quips — fall back to empty string if absent.
        q.quipCorrect = obj.value("quip_correct", "");
        q.quipWrong   = obj.value("quip_wrong",   "");

        questions_.push_back(std::move(q));
    }

    if (questions_.empty()) return false;

    reshuffleDeck();
    return true;
}

const Question& TriviaBank::pick() {
    if (questions_.empty()) {
        // Shouldn't happen in normal play; return a static fallback.
        static const Question kFallback = {
            "fallback", "general", "easy",
            "What colour is the sky?",
            {"Blue", "Red", "Green", "Yellow"},
            0, "Correct!", "Wrong."
        };
        return kFallback;
    }

    if (deckIdx_ >= static_cast<int>(deck_.size())) {
        reshuffleDeck();
        deckIdx_ = 0;
    }

    return questions_[deck_[deckIdx_++]];
}

float TriviaBank::multiplier(const std::string& difficulty) {
    if (difficulty == "easy")   return 1.5f;
    if (difficulty == "medium") return 2.0f;
    if (difficulty == "hard")   return 3.0f;
    return 1.0f;
}

void TriviaBank::reshuffleDeck() {
    deck_.resize(questions_.size());
    std::iota(deck_.begin(), deck_.end(), 0);

    static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(deck_.begin(), deck_.end(), rng);

    deckIdx_ = 0;
}

} // namespace td
