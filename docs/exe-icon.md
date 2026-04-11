# Adding an App Icon to trivia_doku.exe

The executable currently has no icon. This doc covers icon selection and CMake wiring.

## Icon Concept

A question mark inside a tetromino/block tile fits the game theme well — puzzle + trivia in one glyph. Suggested style: bold outline, arcade-bright color (electric blue, neon green, or orange).

## Free Icon Sources

- **[Game-Icons.net](https://game-icons.net)** — search "puzzle" or "question"; CC license; exports SVG/PNG
- **[Flaticon](https://www.flaticon.com)** — search "block puzzle" or "trivia quiz"; many free options
- **[Icons8](https://icons8.com)** / **[Noun Project](https://thenounproject.com)** — "game piece" + "question mark" combos

Convert your chosen image to `.ico` (multi-size: 16, 32, 48, 256px) using a tool like [IcoConvert](https://icoconvert.com) or ImageMagick:

```
magick input.png -define icon:auto-resize=256,48,32,16 app.ico
```

## File Layout

Place the icon and resource file here:

```
resources/
  app.ico
  app.rc
```

`resources/app.rc`:

```rc
IDI_ICON1 ICON "app.ico"
```

## CMakeLists.txt Changes

Add `resources/app.rc` to the `add_executable` source list:

```cmake
add_executable(trivia_doku
    src/main.cpp
    src/Board.cpp
    src/Piece.cpp
    src/Renderer.cpp
    src/TriviaBank.cpp
    resources/app.rc   # Windows resource — supplies the .exe icon
)
```

CMake already detects the RC compiler on Windows (confirmed in `CMakeRCCompiler.cmake`), so no other changes are needed. Rebuild after adding the file.

## Verification

After building, right-click `trivia_doku.exe` in Explorer — the icon should appear in the file properties thumbnail and in the taskbar when the game is running.
