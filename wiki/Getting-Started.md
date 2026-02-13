# Getting Started

## Installation

### Arduino IDE (manual)

1. Copy the entire `Tab5UI` folder into your Arduino libraries directory:
   - **macOS:** `~/Documents/Arduino/libraries/Tab5UI/`
   - **Windows:** `Documents\Arduino\libraries\Tab5UI\`
   - **Linux:** `~/Arduino/libraries/Tab5UI/`

2. Install the **M5GFX** library via the Arduino Library Manager:
   - *Sketch → Include Library → Manage Libraries → search "M5GFX"*

3. Select board **M5Stack Tab5** (or the appropriate ESP32-P4 board).

### PlatformIO

Add to `platformio.ini`:

```ini
[env:tab5]
platform = espressif32
board = m5stack-tab5
framework = arduino
lib_deps =
    m5stack/M5GFX
    Tab5UI
```

---

## Quick Start

```cpp
#include <M5GFX.h>
#include <Tab5UI.h>

M5GFX display;
UIManager ui(display);

UITitleBar  titleBar("My App");
UIStatusBar statusBar("Ready");
UIButton    btn(50, 100, 200, 52, "Press Me");

void setup() {
    display.init();
    display.setRotation(1);           // Landscape (use 0 for portrait)
    Tab5UI::init(display);            // Must be called after init + rotation
    ui.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    btn.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Button pressed!");
    });

    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();
    ui.addElement(&titleBar);
    ui.addElement(&btn);
    ui.addElement(&statusBar);
    ui.setContentArea(TAB5_TITLE_H, Tab5UI::screenH() - TAB5_STATUS_H);
    ui.drawAll();
    ui.setSleepTimeout(5);           // Screen off after 5 min idle
    ui.setLightSleep(true);          // Low-power idle with touch-to-wake
}

void loop() {
    ui.update();
    yield();
}
```

---

## Orientation Support

Tab5UI works in both **landscape** (1280×720, rotation 1) and **portrait** (720×1280, rotation 0) orientations.

Call `Tab5UI::init(display)` once in `setup()` **after** `display.init()` and `display.setRotation()`:

```cpp
void setup() {
    display.init();
    display.setRotation(0);        // 0 = portrait, 1 = landscape
    Tab5UI::init(display);         // Captures runtime screen dimensions
    // ...
}
```

### What adapts automatically

| Widget | Adaptation |
|---|---|
| **UITitleBar** | Stretches to screen width |
| **UIStatusBar** | Stretches to screen width, repositions to screen bottom |
| **UIKeyboard** | Repositions to screen bottom, keys scale to fit width |
| **UIInfoPopup** | Auto-sizes and centers within actual screen bounds |
| **UIConfirmPopup** | Auto-sizes and centers within actual screen bounds |
| **UIDropdown** | Overflow detection uses actual screen height |
| **UIManager** | Content area bottom defaults to actual screen height |

### Runtime dimension queries

```cpp
int16_t w = Tab5UI::screenW();   // Actual screen width  (720 or 1280)
int16_t h = Tab5UI::screenH();   // Actual screen height (1280 or 720)
```

Use these instead of the compile-time `TAB5_SCREEN_W` / `TAB5_SCREEN_H` macros when you need values that match the current orientation.

### Positioning widgets for portrait

Since widget objects are constructed globally (before `setup()`), use compile-time defaults or placeholder values in constructors, then call `setPosition()` / `setSize()` in `setup()` after `Tab5UI::init()`. See the **WiFi Scanner** demo for a complete portrait example.

---

## Example Sketches

| Example | Orientation | Description |
|---|---|---|
| `Tab5UI_Demo` | Landscape | Full demo with buttons, menus, keyboard, popups |
| `Tab5UI_List_Demo` | Landscape | List widget with icons and selection |
| `Tab5UI_Tab_Demo` | Landscape | Tab view with controls, data list, and scrollable text |
| `Tab5UI_WiFi_Demo` | Portrait | WiFi scanner using portrait orientation |
| `Tab5UI_TextArea_Demo` | Portrait | Multi-line text input |
| `Tab5UI_ColumnList_Demo` | Landscape | Column list with sorting, colored cells, and icons |

---

**Next:** [[API Reference]] · [[Widgets – Basic]] · [[Screenshots]]
