# Rendering

## Flicker-Free Sprite Buffering

Ten widgets use **sprite-buffered (double-buffered) rendering** via a shared `M5Canvas` allocated in PSRAM. Instead of clearing and redrawing directly to the display (which causes visible flicker), each widget renders into an off-screen sprite buffer and pushes the completed frame to the display in a single DMA transfer. If PSRAM allocation fails, widgets automatically fall back to direct rendering.

### Sprite-Buffered Widgets

| Widget | Why it needs buffering |
|--------|----------------------|
| **UIList** | Scrollable item list with selection highlighting |
| **UIScrollText** | Scrollable Markdown-rendered text |
| **UITextArea** | Multi-line text input with cursor and scrolling |
| **UIDropdown** | Scrollable overlay list |
| **UIColumnList** | Multi-column list with sortable headers |
| **UISlider** | Animated thumb and fill track |
| **UIKeyboard** | Full-screen modal key grid |
| **UIMenu** | Modal popup menu |
| **UIInfoPopup** | Modal info dialog |
| **UIConfirmPopup** | Modal confirmation dialog |

All ten widgets share a single static `M5Canvas` instance that is lazily allocated and reused, keeping PSRAM usage to one buffer at a time.

---

## Render Mode Override

You can override the default behaviour by defining `TAB5_RENDER_MODE` **before** including `Tab5UI.h`:

| Value | Constant | Effect |
|:-----:|----------|--------|
| `0` | *(default)* Auto | Sprite-buffered with automatic fallback to direct if PSRAM allocation fails |
| `1` | Sprite | Always sprite-buffer — the pixel-count safety cap is removed (use when you know your PSRAM budget) |
| `2` | Direct | Always draw directly to the display — no sprite allocation at all |

```cpp
#define TAB5_RENDER_MODE 2   // force direct rendering
#include <Tab5UI.h>
```

### When to use each mode

- **Auto (0):** Best for most projects. Sprite-buffers the widgets that benefit from it, falls back gracefully if memory is tight.
- **Sprite (1):** Use when you have plenty of PSRAM and want to ensure buffering even for very large widgets.
- **Direct (2):** Use for debugging, or when you need all PSRAM for application data and can tolerate some flicker on scrollable widgets.

---

**Next:** [[Screenshots]] · [[Tips & Best Practices]] · [[Home]]
