"""Small importable API and CLI to expose the repository's trivia JSON.

Features:
- `load_trivia()` / `dump_trivia()` to read/write the JSON used by the game.
- `write_canonical()` to produce an assets/block-doku-trivia.json canonical copy.
- `serve()` to run a tiny HTTP server exposing `/trivia` (JSON).

The module prefers `assets/block-doku-trivia.json` but falls back to
`assets/trivia.json` for backwards compatibility.
"""
from __future__ import annotations

import argparse
import json
import sys
import threading
import webbrowser
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Dict, Optional


REPO_ROOT = Path(__file__).resolve().parent.parent
ASSETS_DIR = REPO_ROOT / "assets"
CANONICAL_NAME = "block-doku-trivia.json"
LEGACY_NAME = "trivia.json"
FALLBACK_NAMES = [CANONICAL_NAME, LEGACY_NAME]


def _resolve_path(path: Optional[str] = None) -> Path:
    if path:
        p = Path(path)
        return p if p.is_absolute() else (Path.cwd() / p)
    for name in FALLBACK_NAMES:
        cand = ASSETS_DIR / name
        if cand.exists():
            return cand
    return ASSETS_DIR / CANONICAL_NAME


def load_trivia(path: Optional[str] = None) -> Dict[str, Any]:
    p = _resolve_path(path)
    with p.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def dump_trivia(data: Dict[str, Any], path: Optional[str] = None, pretty: bool = False) -> Path:
    p = _resolve_path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("w", encoding="utf-8") as fh:
        if pretty:
            json.dump(data, fh, ensure_ascii=False, indent=2)
        else:
            json.dump(data, fh, ensure_ascii=False, separators=(",", ":"))
    return p


def write_canonical(source: Optional[str] = None, force: bool = False) -> Path:
    src = _resolve_path(source)
    dst = ASSETS_DIR / CANONICAL_NAME
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not dst.exists() or force:
        with src.open("r", encoding="utf-8") as inf, dst.open("w", encoding="utf-8") as outf:
            outf.write(inf.read())
    return dst


class _Handler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.rstrip("/") == "/trivia":
            try:
                data = load_trivia()
                payload = json.dumps(data, ensure_ascii=False).encode("utf-8")
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)
            except Exception as exc:  # pragma: no cover - simple CLI helper
                self.send_error(HTTPStatus.INTERNAL_SERVER_ERROR, str(exc))
            return
        if self.path in ("/", ""):
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b"/trivia -> JSON\n")
            return
        return super().do_GET()


def serve(host: str = "127.0.0.1", port: int = 8000, open_browser: bool = False) -> None:
    addr = (host, port)
    server = ThreadingHTTPServer(addr, _Handler)
    url = f"http://{host}:{port}/trivia"
    if open_browser:
        threading.Timer(0.25, lambda: webbrowser.open(url)).start()
    try:
        print(f"Serving trivia at {url}")
        server.serve_forever()
    except KeyboardInterrupt:
        print("Shutting down server")
        server.shutdown()


def _main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(prog="trivia_api")
    sub = parser.add_subparsers(dest="cmd")

    p_serve = sub.add_parser("serve", help="Run a tiny HTTP server exposing /trivia")
    p_serve.add_argument("--host", default="127.0.0.1")
    p_serve.add_argument("--port", type=int, default=8000)
    p_serve.add_argument("--open", action="store_true", help="Open browser to the /trivia URL")

    p_load = sub.add_parser("load", help="Load trivia JSON and print to stdout")
    p_load.add_argument("--path", default=None)

    p_dump = sub.add_parser("dump", help="Dump trivia JSON to a path or stdout")
    p_dump.add_argument("--path", default=None)
    p_dump.add_argument("--pretty", action="store_true")

    p_write = sub.add_parser("write-canonical", help="Write canonical assets/block-doku-trivia.json from existing trivia source")
    p_write.add_argument("--source", default=None)
    p_write.add_argument("--force", action="store_true")

    args = parser.parse_args(argv)
    if args.cmd == "serve":
        serve(args.host, args.port, args.open)
        return 0
    if args.cmd == "load":
        data = load_trivia(args.path)
        json.dump(data, sys.stdout, ensure_ascii=False)
        sys.stdout.write("\n")
        return 0
    if args.cmd == "dump":
        data = load_trivia()
        if args.path:
            dump_trivia(data, args.path, pretty=args.pretty)
        else:
            json.dump(data, sys.stdout, ensure_ascii=False, indent=2 if args.pretty else None)
            sys.stdout.write("\n")
        return 0
    if args.cmd == "write-canonical":
        out = write_canonical(args.source, force=args.force)
        print(out)
        return 0
    parser.print_help()
    return 2


if __name__ == "__main__":
    raise SystemExit(_main())
