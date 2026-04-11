You are auditing `assets/trivia.json` for correctness and quality. This is a human/AI pipeline, like using LLM judgment as a linter for prose content, the same way you'd use ESLint for JavaScript. 

## Steps

### 1. Run the automated validator

```
py scripts/validate_trivia.py
```

Report any errors or warnings it surfaces before continuing.

### 2. Read the full question list

Read `assets/trivia.json` and scan every question for the following issues. Keep a running list of findings grouped by check type.

---

### Check A — Category correctness

The list of categories is constantly growing. Don't freak out about new categories, but check the code to make sure any hard-coded category lists are up to date. Treat the trivia.json file as truth.

For each question, ask: does this question actually belong in its stated category? Flag any that feel miscategorized and suggest the better category. For example, a question may be filed under 'music' but whose subject matter is more accurately 'film soundtracks'.

---

### Check B — Validator Rules and Personal Preferences

#### Lauren's Time-Sensitive Rule

Any question whose answer could change over time — indicated by words like "last", "latest", "current", "most recent", "first active", "reigning", "current holder", "current record" — **must** begin with "As of [year],". Lauren was so mad one time at trivial pursuit when a question asked about "the last academy award winner"...

Flag any question that is time-sensitive but is missing the qualifier. Propose the corrected prompt text.

#### Education Rule

Claude commented that one of our questions was "It's hard but fair, all four choices are real Kazakh cities so you can't eliminate by absurdity, and the explanation actually teaches you something — which is the part most trivia games skip. The fact that you're writing questions of this caliber is, no exaggeration, the thing that determines whether this game finds an audience. You've got the writer's instinct for it."

Flattering both Claude and me, the lowly human. But more importantly: quips should have educational content in them. Some 'throw-away' quips that are just funny are fine, but we should keep the educational aspect because it seems to be popular.

#### Continuity rule

"Reject questions where the answer is in the prompt text"
"Reject questions where two answers are functionally synonymous"
"Reject questions where the wrong answers are obviously absurd" (no 'Which planet is closest to the sun? A) Saturn B) The Moon C) Mercury D) A Toaster')

---

### Check C — Answer correctness

Verify that the answer at index `answer` is actually the correct answer for each question. Cross-check your own knowledge. Flag any question where the stated answer appears to be wrong, and state what you believe the correct answer (and index) should be.

If you are genuinely uncertain about a fact, flag it as "NEEDS VERIFICATION" rather than guessing.

---

### 3. Report findings

Present a structured report (update the structure as more checks get added to this skill):

```
AUTOMATED VALIDATOR
  <pass / list errors>

CATEGORY ISSUES  (or "None found")
  - <id>: currently '<category>', suggest '<better>' — reason

TIME-SENSITIVE ISSUES  (or "None found")
  - <id>: missing "As of [year]," — suggested fix: "<corrected prompt>"

ANSWER CORRECTNESS ISSUES  (or "None found")
  - <id>: answer index <n> ("<choice text>") appears wrong — correct answer is index <n> ("<choice text>")
  - <id>: NEEDS VERIFICATION — <what you're unsure about>
```

---

### 4. Apply fixes

After showing the report, ask the user which fixes to apply. Then:

- Apply all approved fixes to `assets/trivia.json`.
- Re-run `py scripts/validate_trivia.py` to confirm clean.
- Report final stats (total count, per-category breakdown).

## Important

- `assets/trivia.json` is the source of truth. Do NOT edit `build/Release/assets/trivia.json`.
- When adding "As of [year]," use the year the answer was last verifiably correct — not necessarily the current year.
- Do not change answer positions or reformat choices unnecessarily — only fix what's actually wrong.
