Audio assets — how to add them to Trivia-Doku

This document explains where to put music and sound effects and the filenames that the game will try to load automatically. It also shows how to load and play custom files from code using the `Renderer` API.

Where to place files

- Put all audio files inside the existing `assets/` directory in the project root.
- The game will try to load a few conventional names on startup (no-op if a file is missing):
  - `assets/music.ogg` — background music (streamed)
  - `assets/sfx_place.wav` — short placement sound effect
  - `assets/sfx_clear.wav` — clear/line-clear sound
  - `assets/sfx_correct.wav` — trivia correct
  - `assets/sfx_wrong.wav` — trivia incorrect
  - `assets/sfx_pickup.wav` — piece pickup (tray -> drag)
  - `assets/sfx_cancel.wav` — placement cancelled / drop returned
  - `assets/sfx_deal.wav` — new tray dealt
  - `assets/sfx_shatter.wav` — stones shatter (3-in-a-row redemption)
  - `assets/sfx_gameover.wav` — game over
  - `assets/sfx_restart.wav` — restart (press R)

Supported formats and recommendations

- Short sound effects: WAV or OGG are good. Use uncompressed WAV for lowest latency.
- Background music: OGG is preferred (streamed). MP3 may work depending on how raylib was built.
- Keep SFX under a few seconds. Music can be longer — streaming reduces memory use.

How the code loads assets

- At startup the renderer calls `initAudio()` and attempts to load the conventional filenames above. Loading is guarded with a file-exists check, so missing files are ignored (no crash).
- The renderer exposes a small API (declared in `src/Renderer.h`):
  - `initAudio()` — initialize the audio subsystem
  - `shutdownAudio()` — unload audio and close the device
  - `loadSoundEffect(id, path)` — load a short SFX and register it under `id`
  - `loadMusic(id, path)` — load a streaming music file and register it under `id`
  - `playSoundEffect(id)` — play a previously loaded SFX (no-op if missing)
  - `playMusicStream(id)` — start playing a previously loaded music stream
  - `stopMusic()` — stop currently playing music
  - `updateAudio()` — must be called each frame to keep streaming music alive

Example: add your own files

1. Copy your files into `assets/`, e.g. `assets/my-click.wav` and `assets/ambient.ogg`.
2. In code (for example in `main.cpp`, after creating the `Renderer`):

```cpp
renderer.initAudio();
renderer.loadSoundEffect("click", "assets/my-click.wav");
renderer.loadMusic("ambient", "assets/ambient.ogg");
renderer.playMusicStream("ambient");
```

3. Play the SFX when appropriate:

```cpp
renderer.playSoundEffect("click");
```

Notes and troubleshooting

- If your builds of raylib do not include particular decoders (rare when using the vcpkg-provided raylib), some formats may fail to load. In that case you can either convert your files to WAV/OGG or install the missing decoder packages used by your raylib build (e.g. `libsndfile`, `libvorbis`, `libmpg123`) and rebuild raylib.
- The renderer will not add or modify asset files — you are expected to add your own audio files to `assets/`.
- The code uses `std::filesystem::exists()` to check for files before attempting to load them, so no special runtime flags are required.

I've wired common events in code: pickup, cancel, deal, shatter, game-over, and restart.
If you'd like more events wired (UI clicks, button hover, help overlay), tell me which ones.


