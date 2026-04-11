Find and remove any tracked files that should be ignored according to `.gitignore`.

## Workflow

1. Run `git ls-files --cached --ignored --exclude-standard` to find all tracked files that match `.gitignore` patterns.
2. If no files are found, tell the user everything is clean.
3. If files are found, list them and run `git rm --cached <files>` to untrack them (without deleting from disk).
4. Show the updated `git status` so the user can confirm the result.
