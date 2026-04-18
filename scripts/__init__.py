"""Package marker for the scripts module so it can be installed editable.

Most of the real work lives in the ``promptukit`` package; the modules in
this directory are thin shims that default paths to this repo's assets.
"""

__all__ = [
    "add_trivia",
    "extract_trivia",
    "validate_trivia",
]
