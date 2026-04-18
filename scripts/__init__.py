"""Package marker for the scripts module so it can be installed editable.

Most of the real work lives in the ``promptukit`` package; the modules in
this directory are thin shims that default paths to this repo's assets.
"""

from .trivia_api import load_trivia, dump_trivia, serve, write_canonical

__all__ = [
    "add_trivia",
    "extract_trivia",
    "validate_trivia",
    "load_trivia",
    "dump_trivia",
    "serve",
    "write_canonical",
]
