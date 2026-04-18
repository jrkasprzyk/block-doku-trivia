## Trivia API

The canonical trivia JSON for the project lives in the `assets/` folder.

The repository exposes a small, zero-dependency API in `scripts/trivia_api.py`.

- Canonical location: `assets/block-doku-trivia.json` (preferred).
- Backwards-compatible fallback: `assets/trivia.json`.

Usage examples:

- Serve the trivia over HTTP (useful for local development):

```sh
python -m scripts.trivia_api serve --port 8000 --open
```

- Print the current trivia JSON to stdout:

```sh
python -m scripts.trivia_api load
```

- Write a canonical copy at `assets/block-doku-trivia.json` (non-destructive):

```sh
python -m scripts.trivia_api write-canonical
```

If you want the repo to carry an explicit canonical file, run the `write-canonical`
command (or pass `--force` to overwrite). External tools such as `promptukit`
can point at `http://localhost:8000/trivia` while developing against this
repository to ensure they use the canonical content.
