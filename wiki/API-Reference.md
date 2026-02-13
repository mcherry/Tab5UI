# API Reference

## Tab5UI Namespace

```cpp
namespace Tab5UI {
    void     init(M5GFX& gfx);   // Capture runtime screen dimensions
    int16_t  screenW();           // Current screen width
    int16_t  screenH();           // Current screen height
}
```

---

## Screen Constants

| Constant | Value | Description |
|---|---|---|
| `TAB5_SCREEN_W` | 1280 | Screen width in pixels |
| `TAB5_SCREEN_H` | 720 | Screen height in pixels |
| `TAB5_TITLE_H` | 48 | Default title bar height |
| `TAB5_STATUS_H` | 36 | Default status bar height |
| `TAB5_BTN_H` | 52 | Default button height |
| `TAB5_BTN_W` | 160 | Default button width |
| `TAB5_BTN_R` | 8 | Default button corner radius |
| `TAB5_ICON_SIZE` | 44 | Default icon dimension |
| `TAB5_MENU_ITEM_H` | 48 | Menu item row height |
| `TAB5_MENU_W` | 260 | Default menu popup width |
| `TAB5_MENU_MAX_ITEMS` | 12 | Max items per menu |
| `TAB5_KB_KEY_W` | 88 | Keyboard key width |
| `TAB5_KB_KEY_H` | 56 | Keyboard key height |
| `TAB5_KB_H` | 290 | Keyboard panel height |
| `TAB5_INPUT_H` | 44 | Text input field height |
| `TAB5_INPUT_MAX_LEN` | 128 | Max text input length |
| `TAB5_LIST_ITEM_H` | 48 | List item row height |
| `TAB5_LIST_MAX_ITEMS` | 64 | Max items in a list |
| `TAB5_LIST_SCROLLBAR_W` | 6 | Scrollbar width |
| `TAB5_COLLIST_MAX_COLS` | 8 | Max columns in a column list |
| `TAB5_PADDING` | 12 | General padding |

---

## Theme Colors (`Tab5Theme::`)

| Color | Hex | Use |
|---|---|---|
| `PRIMARY` | `#2196F3` | Buttons, icons |
| `PRIMARY_DARK` | `#1565C0` | Pressed state |
| `SECONDARY` | `#4CAF50` | Toggle/success |
| `ACCENT` | `#FF9800` | Highlights |
| `DANGER` | `#F44336` | Destructive actions |
| `BG_DARK` | `#1A1A2E` | Screen background |
| `BG_MEDIUM` | `#16213E` | Row backgrounds |
| `SURFACE` | `#0F3460` | Cards/surfaces |
| `TEXT_PRIMARY` | `#FFFFFF` | Main text |
| `TEXT_SECONDARY` | `#B0BEC5` | Subtle text |
| `TEXT_DISABLED` | `#5C6B7A` | Disabled items |
| `BORDER` | `#1B3A5C` | Widget borders |
| `DIVIDER` | `#1E3A5F` | Separator lines |

---

## UIElement (base class)

All widgets inherit from `UIElement` and share these methods:

```cpp
void setPosition(int16_t x, int16_t y);
void setSize(int16_t w, int16_t h);
void setVisible(bool v);
void setEnabled(bool e);
void setTag(const char* tag);
void setOnTouch(TouchCallback cb);
void setOnTouchRelease(TouchCallback cb);
```

### Touch Handling

Every widget supports two callbacks:

```cpp
element.setOnTouch([](TouchEvent e) {
    // Finger touched down on this element
});

element.setOnTouchRelease([](TouchEvent e) {
    // Finger lifted from this element
});
```

The `UIManager::update()` method handles all touch detection, hit-testing (including circular hit-test for `UIIconCircle`), and dirty-region redraws automatically.

---

**Next:** [[Widgets – Basic]] · [[Widgets – Input]] · [[Widgets – Lists]] · [[UIManager]]
