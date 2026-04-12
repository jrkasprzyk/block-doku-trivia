Audio assets — developer guide

This file documents the current on-disk layout and the code paths that load audio so developers can add, replace, or re-map audio assets.

Where to put files

- `assets/bg_music/` — background music files (use `.ogg` when possible). The program scans this directory at startup for supported audio files and loads them into the music map. Each file's stem is converted into an ID prefixed with `bg_` (non-alphanumeric characters become underscores) and the resulting IDs are used to build a playlist that the renderer shuffles and plays in random order. Add or remove files here and restart the game — no code changes required.

- `assets/sfx/` — short sound effects (WAV/OGG recommended). Filenames in this folder do not have to match in-game action names: the mapping from action → filename is defined in code (see "Sound effect mapping" below).

Supported extensions

- `.ogg` (preferred for music), `.mp3`, `.wav`, `.flac`, `.m4a` for music and SFX. For lowest latency use uncompressed WAV for short SFX.

How the game loads audio (developer notes)

- At startup `main.cpp` calls `renderer.initAudio()`, scans `assets/bg_music/` and loads supported music files. Discovered files are loaded with IDs of the form `bg_<stem>` and collected into `playlistIds`, which is then passed to `renderer.playMusicPlaylist(playlistIds, true)` so tracks play back-to-back in a randomized (shuffled) order. See the directory-scan and playlist startup logic in [src/main.cpp](src/main.cpp#L248-L292).

- If `assets/bg_music/` is missing or empty the code falls back to a small hard-coded list; you can update that fallback in the same file.

- Sound effects are loaded explicitly by mapping an in-game ID to a file path at startup. The current mappings are created in `main.cpp` (example range): [src/main.cpp](src/main.cpp#L299-L308). The IDs used by the gameplay code are:
  - `place`, `clear`, `correct`, `wrong`, `pickup`, `cancel`, `deal`, `shatter`, `gameover`, `restart`

Changing or overriding mappings

- To change which file is played for an action, edit the mappings in `src/main.cpp` (the `tryLoad`/`loadSoundEffect` calls) or add your own `renderer.loadSoundEffect(...)` calls after `renderer.initAudio()`.

Example (developer-focused):

```cpp
// after renderer.initAudio();
renderer.loadSoundEffect("place", "assets/sfx/my-place.wav"); // override the 'place' SFX
renderer.loadMusic("bg_custom", "assets/bg_music/mytrack.ogg");     // manual music load
renderer.playMusicPlaylist({"bg_custom", "bg_hiphop", "bg_synth"}, true);
```

Runtime requirements

- `Renderer::updateAudio()` must be called each frame to keep streaming music playing correctly. The main loop already calls this (see `Renderer::updateAudio()` in [src/Renderer.h](src/Renderer.h#L139)).

Notes and troubleshooting

- If your build of raylib lacks particular decoders some file formats will fail to load. Convert to WAV/OGG or install the needed decoders (for example `libsndfile`, `libvorbis`, `libmpg123`) and rebuild raylib.
- The renderer uses `std::filesystem::exists()` before loading files, so missing files are ignored and will not crash the game — they will generate a warning in the log instead.

If you want me to wire additional UI events (hover/click sounds, menu UI, etc.) into the startup mapping, tell me which action IDs you'd like and I can add the load-and-log lines in `main.cpp`.


