# TODO

## Imminent date celebration effect

When a birthday or Christmas is 0-3 days away, the clock should do something fun automatically. Ideas (pick one):

- **Pulsing brightness** (~50-100 bytes) — Clock display slowly breathes between dim and bright. Easiest to implement, smallest footprint.
- **Checkerboard flash** (~100-150 bytes) — Alternate between two checkerboard patterns on the full matrix. Simple and eye-catching.
- **Scrolling message** (~200-300 bytes + string storage) — Scroll "Happy Birthday V!" or "Merry Christmas!" using MD_Parola's built-in `displayScroll()`. Requires changing loop control flow to call `displayAnimate()`.
- **Sparkle/fireworks** (~300-500 bytes) — Random pixels light up and fade across the matrix. Most visually impressive but tightest on flash.
- **Snake fill** (~200-400 bytes) — LEDs light up one by one in a snake pattern until the matrix is full, then clear and repeat.

### Constraints

- ~2 KB of flash remaining (28,726 / 30,720 used)
- Any single effect above should fit, but leaves little room for further features
- If more flash is needed, MD_Parola unused animation effects can be disabled via library configuration
