You are helping the user add trivia questions to `assets/trivia.json`.

## Workflow

1. Ask the user how they'd like to add questions. Options:
   - **Give a topic** — e.g. "space exploration" or "90s hip-hop" — and you'll draft 3-5 questions across easy/medium/hard difficulties.
   - **Give a fully formed question** — they provide prompt, choices, correct answer, and you format it into the JSON schema.
   - **Bulk mode** — they name a category and a count, you generate that many questions.

2. For each question, ensure:
   - `id` follows the pattern `{category}_{NNN}` where NNN is the next unused number in that category.
   - `category` is one of: motorsport, music, film, general, meta.
   - `difficulty` is one of: easy, medium, hard.
   - `choices` has exactly 4 options.
   - `answer` is the 0-based index of the correct choice.
   - The correct answer is NOT always in the same position — vary across 0, 1, 2, 3.
   - Include `quip_correct` and `quip_wrong` — short, punchy one-liners.
   - All facts are accurate. If you're unsure about a fact, say so and let the user confirm.

3. Show the user the proposed questions in a readable format and ask for approval before writing.

4. After approval, append the questions to the `questions` array in `assets/trivia.json`. Keep this file orderly and easy to read, where the question categories keep the proper indexing and arrangement.

5. Run the validation script to confirm everything is clean:
   ```
   py scripts/validate_trivia.py
   ```

6. Report the updated stats (total count, per-category, answer distribution).

## Important

- `assets/trivia.json` is the source of truth. The build copies it automatically via CMake POST_BUILD.
- Do NOT edit `build/Release/assets/trivia.json` — it's a build artifact.
- Keep answer positions varied. Check the current distribution before choosing positions.