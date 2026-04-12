# Plan: Improve Fullscreen Scaling & Layout

## Context

The game looks well-proportioned at its default 1600x1000 window size, but in fullscreen the layout has problems:
- The board is pinned to the top-left at fixed pixel offset (80, 80) -- never centered
- Margins (80px), HUD reserve (380px), and tray reserve (160px) are hardcoded pixel values that don't scale
- HUD text uses absolute Y positions (80, 120, 180, 206) instead of being relative to the board
- `ToggleFullscreen()` uses exclusive fullscreen which doesn't properly cover the Windows taskbar
- Result: lots of wasted space, board feels small, not truly fullscreen

## Changes

### 1. True borderless fullscreen -- `src/main.cpp` (line 343)

Replace `ToggleFullscreen()` with `ToggleBorderlessWindowed()` (raylib 4.5+). This gives proper edge-to-edge fullscreen on Windows, covering the taskbar. Keep F11 as the shortcut.

### 2. Layout constants as named, tunable values -- `src/Renderer.h`

Replace the magic numbers in `recalcForWindow()` with clearly named constants/ratios at the top of the Layout struct, so future contributors can easily adjust them:

```cpp
// --- Layout tuning knobs (ratios of window size) ---
static constexpr float kMarginXRatio    = 0.05f;  // horizontal margin as % of window width
static constexpr float kMarginYRatio    = 0.08f;  // vertical margin as % of window height
static constexpr float kHudReserveRatio = 0.24f;  // HUD column width as % of window width
static constexpr float kTrayReserveRatio= 0.16f;  // tray height as % of window height
static constexpr int   kMarginXMin      = 40;     // minimum horizontal margin in px
static constexpr int   kMarginYMin      = 40;     // minimum vertical margin in px
static constexpr int   kHudReserveMin   = 260;    // minimum HUD width in px
static constexpr int   kTrayReserveMin  = 100;    // minimum tray height in px
static constexpr int   kHudGap          = 40;     // gap between board right edge and HUD text
```

### 3. Proportional + centered layout -- `src/Renderer.h` `recalcForWindow()`

Rewrite to:
1. Compute margins and reserves from ratios (floored at minimums)
2. Compute `cellPixels` from available space (same min-of-two-dimensions logic)
3. **Center the content block** -- compute actual board width + hudReserve, center horizontally; compute board height + trayReserve, center vertically
4. Store `boardOriginX` and `boardOriginY` as the centered position

### 4. Relative HUD positioning -- `src/Renderer.cpp` `drawHud()`

- Title: position above the board using `boardOriginY - titleSize - gap`
- Score/Streak/help: position relative to `boardOriginY` (e.g., `boardOriginY`, `boardOriginY + lineHeight`, etc.) instead of absolute pixel values (80, 120, 180, 206)
- HUD X already uses board-relative math -- keep that

### 5. Scale tray gap -- `src/Renderer.cpp` `drawTray()`

Change the hardcoded `12` px gap to scale with `s` factor (the `cellPixels / 56.0f` ratio).

### 6. Scale game-over modal -- `src/Renderer.cpp` game-over modal

Change the fixed 360x210 panel to use percentage-of-window sizing (like the trivia modal already does at 45%x60%).

## Files to modify

| File | What changes |
|------|-------------|
| `src/Renderer.h` | Add named layout constants, rewrite `recalcForWindow()` |
| `src/Renderer.cpp` | `drawHud()`, `drawTray()`, game-over modal |
| `src/main.cpp` | `ToggleFullscreen()` -> `ToggleBorderlessWindowed()` |

## Verification

1. `cmake --build build` -- confirm it compiles
2. Run at default window size -- should look the same as before (the ratios are calibrated to match current 1600x1000 layout)
3. F11 for fullscreen -- board + HUD + tray should be centered, proportionally spaced, truly edge-to-edge
4. Drag window to various sizes -- layout adapts smoothly
5. Test drag-and-drop, placement preview, trivia modal, game-over modal at all sizes