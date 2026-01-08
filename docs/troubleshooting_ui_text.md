# Troubleshooting UI Text Rendering

If UI panels appear but text labels are missing, the game now logs detailed SDL_ttf diagnostics and
provides a bitmap fallback font so menus remain readable even without a `.ttf` file.

## Font search order

At startup the renderer attempts to load a font in this order:

1. `./assets/fonts/Roboto-Regular.ttf`
2. Windows: `C:\Windows\Fonts\segoeui.ttf`, then `C:\Windows\Fonts\arial.ttf`
3. macOS: `/Library/Fonts/Arial.ttf`
4. Linux: `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`, then `/usr/share/fonts/truetype/freefont/FreeSans.ttf`

If none are found or SDL_ttf is unavailable, the built-in bitmap font is used.

## Logs and diagnostics

The game logs SDL_ttf initialization and font loading errors via `SDL_Log`. Look for lines like:

* `TTF_Init succeeded` / `TTF_Init failed: ...`
* `Attempting to load font: <path>`
* `TTF_OpenFont failed for '<path>': ...`
* `TTF_RenderUTF8_Blended failed for '<text>': ...`
* `SDL_CreateTextureFromSurface failed for '<text>': ...`

These logs appear in the console output (or the debugger output on Windows).

## Debug overlay (F9)

Press **F9** in-game to toggle the on-screen text diagnostics overlay. It shows:

* `TTF: OK/FAIL`
* `Font: OK/FAIL (<resolved path>)`
* `Last error: ...` (the most recent SDL_ttf render or load error)

This overlay uses the bitmap fallback, so it works even when SDL_ttf fails.

## If no `.ttf` file is present

No binary fonts are required. The renderer falls back to a built-in 5x7 bitmap font, so UI text
remains visible. To use a different font, add a `.ttf` to `assets/fonts/` or install a system font
that matches one of the paths above.
