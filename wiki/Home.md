# Tab5UI — Touchscreen UI Library for M5Stack Tab5

A lightweight, Arduino-compatible UI widget library built on **M5GFX** for the M5Stack Tab5's 5-inch **1280×720** IPS capacitive touchscreen. Supports both **landscape** (1280×720) and **portrait** (720×1280) orientations.

## Features

| Widget | Description |
|---|---|
| **UILabel** | Static/dynamic text with alignment, color, and background options |
| **UIButton** | Rounded-rect button with press feedback and customizable colors |
| **UIIconButton** | Button with PROGMEM PNG icon (32×32) and text fallback, same styling as UIButton |
| **UISlider** | Horizontal slider with draggable thumb, configurable min/max range, and onChange callback |
| **UITitleBar** | Full-width top bar with center title, optional left/right touch zones |
| **UIStatusBar** | Full-width bottom bar with left/center/right text |
| **UITextRow** | Key-value row with label on left, value on right, and dividers |
| **UIIconSquare** | Colored square icon with optional character overlay |
| **UIIconCircle** | Colored circle icon with circular hit-testing |
| **UIMenu** | Modal popup menu with selectable items, separators, and auto-dismiss |
| **UITextInput** | Single-line text input field with placeholder and focus highlight |
| **UIKeyboard** | Full-screen modal QWERTY touch keyboard with Shift, Symbols, and Enter |
| **UIList** | Scrollable list with touch-drag scrolling, item selection, and scrollbar |
| **UITabView** | Multi-page tabbed container with configurable top/bottom tab bar |
| **UIInfoPopup** | Auto-sized modal info popup with title, message, and OK button |
| **UIConfirmPopup** | Auto-sized modal confirm popup with title, message, and Yes/No buttons |
| **UIScrollText** | Scrollable text display with basic Markdown rendering (headings, bold, italic, code, bullets, rules) |
| **UICheckbox** | Toggleable checkbox with label, checked state, and touch callbacks |
| **UIRadioButton** | Selectable radio button with label, managed by UIRadioGroup for mutual exclusion |
| **UIDropdown** | Compact dropdown selector with scrollable list overlay, icons, and all UIList features |
| **UIColumnList** | Multi-column list with sortable headers, per-cell text/color/icon, scrolling, and selection |
| **UITextArea** | Multi-line text input with word wrapping, touch scrolling, and tap-to-place cursor |
| **UIManager** | Registers elements, dispatches touch events, manages dirty redraws |

## Quick Links

- [[Getting Started]] — Installation, quick start, and orientation support
- [[API Reference]] — Namespace, constants, and theme colors
- **Widget Reference:**
  - [[Widgets – Basic]] — Label, Button, IconButton, Slider, TitleBar, StatusBar, TextRow, Icons
  - [[Widgets – Input]] — TextInput, Keyboard, TextArea
  - [[Widgets – Lists]] — List, Dropdown, ColumnList
  - [[Widgets – Containers & Popups]] — TabView, Menu, InfoPopup, ConfirmPopup, ScrollText
  - [[Widgets – Selection]] — Checkbox, RadioButton / RadioGroup
- [[UIManager]] — Element management, touch dispatch, screen sleep
- [[Rendering]] — Flicker-free sprite buffering and render modes
- [[Screenshots]] — Demo screenshots from all example sketches
- [[Tips & Best Practices]]

## License

This project is licensed under the [GNU General Public License v3.0](https://github.com/mcherry/Tab5UI/blob/main/LICENSE).

### Icon Attribution

The icons included in the `icons/` directory are from [**IconPark**](https://github.com/bytedance/IconPark) by **ByteDance, Inc.**, licensed under the [Apache License 2.0](https://github.com/mcherry/Tab5UI/blob/main/icons/LICENSE). The original SVG icons were converted to 32×32 pixel PNG images and embedded as `PROGMEM` C byte arrays for use on embedded platforms.
