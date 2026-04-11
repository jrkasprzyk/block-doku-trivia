# trivia.json — Structure & Splitting Analysis

_Last reviewed: 2026-04-11_

## Current state

- **Path:** `assets/trivia.json`
- **Size:** ~28 KB, 913 lines
- **Questions:** 60 total

### Category breakdown

| Category   | Count |
|------------|-------|
| motorsport | 9     |
| music      | 10    |
| film       | 8     |
| general    | 13    |
| meta       | 8     |
| asia       | 12    |

### Difficulty breakdown

| Difficulty | Count |
|------------|-------|
| easy       | 19    |
| medium     | 20    |
| hard       | 21    |

### Question schema

Required fields: `id`, `category`, `difficulty`, `prompt`, `choices` (4 items), `answer` (0–3 index)  
Optional fields: `quip_correct`, `quip_wrong`

---

## Who reads the file

| Consumer | Path | Access |
|---|---|---|
| `add_trivia.py` | `scripts/add_trivia.py` | Read + write |
| `validate_trivia.py` | `scripts/validate_trivia.py` | Read-only |
| CMakeLists.txt | root | Copies entire `assets/` dir to build output |
| C++ runtime | `src/` | Not yet — planned for Day 6 (uses `nlohmann_json`) |

---

## Should we split into one file per category?

**Verdict: No — not yet.**

At 60 questions the file is small and manageable. The tradeoffs:

| | Split (6 files) | Single file |
|---|---|---|
| Git diffs | Cleaner per-category changes | Noisier as it grows |
| Tooling | `add_trivia.py` and `validate_trivia.py` need rework | No changes needed |
| C++ loader (Day 6) | Must glob/load N files | One `json::parse` call |
| `categories` list | Needs a new home | Lives naturally at the top |

`add_trivia.py` already inserts questions grouped by category, so the single file stays organized.  
**Revisit this decision around 200–300 questions.**
