#!/usr/bin/env python3
"""Interactively add a trivia question to assets/trivia.json."""

import json
import sys
from pathlib import Path

TRIVIA_PATH = Path(__file__).resolve().parent.parent / "assets" / "trivia.json"
DIFFICULTIES = ["easy", "medium", "hard"]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load(path: Path) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def save(path: Path, data: dict) -> None:
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")


def prompt(label: str, *, default: str = "") -> str:
    """Read a non-empty string from stdin."""
    suffix = f" [{default}]" if default else ""
    while True:
        value = input(f"  {label}{suffix}: ").strip()
        if not value and default:
            return default
        if value:
            return value
        print("    (required — please enter a value)")


def pick(label: str, options: list[str]) -> str:
    """Show a numbered menu and return the chosen option."""
    print(f"\n  {label}")
    for i, opt in enumerate(options, 1):
        print(f"    {i}) {opt}")
    while True:
        raw = input("  Choice: ").strip()
        if raw.isdigit() and 1 <= int(raw) <= len(options):
            return options[int(raw) - 1]
        print(f"    Please enter a number between 1 and {len(options)}.")


def next_id(questions: list[dict], category: str) -> str:
    """Return the next unused id for a category, e.g. motorsport_014."""
    existing = [
        q["id"] for q in questions
        if q.get("id", "").startswith(f"{category}_")
    ]
    nums = []
    for qid in existing:
        suffix = qid[len(category) + 1:]
        if suffix.isdigit():
            nums.append(int(suffix))
    next_num = (max(nums) + 1) if nums else 1
    return f"{category}_{next_num:03d}"


def insert_after_category(questions: list[dict], new_q: dict) -> list[dict]:
    """Insert new_q after the last question that shares its category."""
    cat = new_q["category"]
    last_idx = -1
    for i, q in enumerate(questions):
        if q.get("category") == cat:
            last_idx = i
    if last_idx == -1:
        # New category — append at end
        return questions + [new_q]
    return questions[: last_idx + 1] + [new_q] + questions[last_idx + 1 :]


def preview(q: dict) -> None:
    print("\n  ┌─ Preview " + "─" * 50)
    print(f"  │  id         : {q['id']}")
    print(f"  │  category   : {q['category']}")
    print(f"  │  difficulty : {q['difficulty']}")
    print(f"  │  prompt     : {q['prompt']}")
    for i, choice in enumerate(q["choices"]):
        marker = "✓" if i == q["answer"] else " "
        print(f"  │  {marker} [{i}] {choice}")
    if q.get("quip_correct"):
        print(f"  │  quip_correct : {q['quip_correct']}")
    if q.get("quip_wrong"):
        print(f"  │  quip_wrong   : {q['quip_wrong']}")
    print("  └" + "─" * 59)


# ---------------------------------------------------------------------------
# Interactive flow
# ---------------------------------------------------------------------------

def collect_question(categories: list[str], questions: list[dict]) -> dict:
    print("\n" + "=" * 62)
    print("  NEW QUESTION")
    print("=" * 62)

    category = pick("Category", categories)
    difficulty = pick("Difficulty", DIFFICULTIES)

    print()
    q_prompt = prompt("Prompt (the question text)")

    print("\n  Enter the four answer choices:")
    choices = []
    for letter in "ABCD":
        choices.append(prompt(f"  Choice {letter}"))

    print("\n  Which choice is correct?")
    for i, (letter, text) in enumerate(zip("ABCD", choices)):
        print(f"    {i}) {letter} — {text}")
    while True:
        raw = input("  Answer (0–3): ").strip()
        if raw in ("0", "1", "2", "3"):
            answer = int(raw)
            break
        print("    Please enter 0, 1, 2, or 3.")

    print("\n  Quips are optional — press Enter to skip.")
    quip_correct = input("  quip_correct: ").strip()
    quip_wrong = input("  quip_wrong  : ").strip()

    qid = next_id(questions, category)

    q: dict = {
        "id": qid,
        "category": category,
        "difficulty": difficulty,
        "prompt": q_prompt,
        "choices": choices,
        "answer": answer,
    }
    if quip_correct:
        q["quip_correct"] = quip_correct
    if quip_wrong:
        q["quip_wrong"] = quip_wrong

    return q


def confirm(msg: str) -> bool:
    while True:
        raw = input(f"  {msg} [y/n]: ").strip().lower()
        if raw in ("y", "yes"):
            return True
        if raw in ("n", "no"):
            return False


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else TRIVIA_PATH
    print(f"Loading: {path}")

    try:
        data = load(path)
    except json.JSONDecodeError as e:
        print(f"FATAL: invalid JSON — {e}")
        return 1

    categories: list[str] = data.get("categories", [])
    questions: list[dict] = data.get("questions", [])

    added = 0
    while True:
        q = collect_question(categories, questions)
        preview(q)

        if confirm("Save this question?"):
            questions = insert_after_category(questions, q)
            data["questions"] = questions
            save(path, data)
            added += 1
            print(f"\n  Saved — {q['id']} added ({len(questions)} questions total).")
        else:
            print("\n  Discarded.")

        if not confirm("Add another question?"):
            break

    print(f"\nDone. {added} question(s) added.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
