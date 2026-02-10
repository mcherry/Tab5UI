# Changelog

All notable changes to the Tab5UI library are documented here.

## [1.1.0] — 2026-02-10

### New Widgets
- **UIIconButton** — Button that displays a 32×32 PROGMEM PNG icon with text fallback; same styling and touch behavior as UIButton.
- **UISlider** — Horizontal slider with draggable thumb, configurable min/max range, and `onChange` callback. Label and numeric value display are optional (off by default).

### Sprite-Buffered Rendering
- Added **shared `M5Canvas` sprite infrastructure** in PSRAM for flicker-free double-buffered rendering. A single static sprite is lazily allocated and reused across all widgets.
- Applied sprite buffering to **9 widgets**: UIList, UIScrollText, UITextArea, UIDropdown, UISlider, UIKeyboard, UIMenu, UIInfoPopup, UIConfirmPopup.
- All draw() signatures changed from `M5GFX&` to `LovyanGFX&` (the common base class of both `M5GFX` and `M5Canvas`), enabling widgets to render transparently to either target.
- Added `TAB5_RENDER_MODE` compile-time define (0 = auto with fallback, 1 = always sprite, 2 = always direct).
- Automatic fallback to direct rendering if PSRAM sprite allocation fails.

### Flicker Fixes
- **Scroll flashing** — Removed child→TabView dirty propagation from `UITabView::handleTouchMove` and `handleTouchUp`, preventing full-TabView redraws during child scrolling.
- **Popup/keyboard close flash** — Removed `clearScreen()` from UIManager modal close path. Modal close now erases only the modal footprint and marks overlapping children dirty for targeted redraw.
- **Dropdown close flash** — Added deferred list erase via `_needsListErase` flag. Dropdown list area is erased in the next `draw()` call, and overlapping siblings are marked dirty instead of the parent TabView.
- **Dropdown shadow flash** — Shadow is now included inside the sprite buffer instead of being drawn directly to the display.
- **Label text overwrite** — `UILabel::draw()` now always clears its background rect before drawing text, preventing ghosting when text changes.
- **Dropdown overlapping status bar** — `calcListGeometry()` now constrains the list overlay between `TAB5_TITLE_H` and `screenH() - TAB5_STATUS_H`.
- **Modal redraw efficiency** — `UIManager::drawDirty()` modal safety pass now only redraws modals that are actually dirty, not unconditionally.

### API Additions
- `UIManager::setSleepTimeout(uint32_t minutes)` / `getSleepTimeout()` — Configurable screen sleep after idle timeout (0 = never). Turns off backlight; touch-to-wake consumes the wake touch.
- `UIManager::sleep()` / `wake()` / `isScreenAsleep()` — Manual sleep/wake control.
- `UIManager::setBrightness(uint8_t b)` — Set display brightness (also used as the wake-restore level).
- `UITabView::getChildCount(int pageIndex)` / `getChild(int pageIndex, int childIndex)` — Accessors for iterating page children.
- `UITabView::drawTabBar()` — Made public for targeted tab bar redraws during modal close.
- `UIDropdown::setContentBounds(int16_t top, int16_t bottom)` — Constrain the dropdown list overlay to a custom vertical region.

### Performance
- **Keyboard typing speed** — Key press/release no longer triggers a full 1280×290 sprite redraw of the entire keyboard (~371K pixels). Added `drawKey()` method that redraws only the single affected key directly to the display (~5K pixels). Press highlight and release unhighlight are now near-instantaneous.
- **Touch release debounce removed** — `_lastTouchTime` is no longer reset on touch release, eliminating the 30ms dead zone between consecutive key presses for faster typing.

### Other Changes
- Replaced `delay(10)` with `yield()` in all 5 example sketch `loop()` functions for better ESP32 task scheduling.
- Renamed internal `_min` / `_max` members to `_minVal` / `_maxVal` to avoid conflicts with Arduino's `_min` / `_max` macros.
- PROGMEM icon set: 55 ready-to-use 32×32 PNG icon headers converted from IconPark (Apache 2.0), with attribution README and license.

## [1.0.0] — 2026-02-06

### Initial Release
- Core widgets: UILabel, UIButton, UITitleBar, UIStatusBar, UITextRow, UIIconSquare, UIIconCircle, UIMenu, UIKeyboard, UITextInput, UIList, UITabView, UIInfoPopup, UIConfirmPopup, UIScrollText, UICheckbox, UIRadioButton, UIRadioGroup, UIDropdown, UITextArea.
- UIManager with touch dispatch, dirty-region redraws, and modal overlay system.
- Landscape (1280×720) and portrait (720×1280) orientation support via `Tab5UI::init()`.
- Tab5Theme color palette.
- 5 example sketches: full demo, list demo, tab demo, WiFi scanner (portrait), text area demo (portrait).
