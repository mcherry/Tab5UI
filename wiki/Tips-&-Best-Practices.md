# Tips & Best Practices

- Add modal elements (`UIMenu`, `UIKeyboard`) **last** so they draw on top when opened.
- `UITabView` children should be positioned relative to `tabs.contentY()` — this accounts for tab bar placement.
- When switching tab bar position at runtime, reposition all children and call `tabs.setDirty(true)`.
- Multiple `UITextInput` fields can share a single `UIKeyboard` instance.
- Call `ui.update()` in `loop()` — it handles touch polling, event dispatch, and dirty redraws.
- Use `startWrite()` / `endWrite()` batching (handled internally by `drawAll` / `drawDirty`).
- Set `display.setFont(&fonts::DejaVu18)` for crisp text at the Tab5's resolution.
- Use `setTag("myBtn")` + `findByTag("myBtn")` to look up elements by name.
- All colors are specified as 24-bit RGB hex values (e.g. `0xFF9800`).
- For portrait orientation, call `display.setRotation(0)` then `Tab5UI::init(display)` — see the **WiFi Scanner** demo.

## Memory Considerations

- All ten sprite-buffered widgets share a single `M5Canvas` instance — only one buffer is allocated at a time.
- The sprite buffer is allocated in PSRAM. If allocation fails, the widget automatically falls back to direct rendering.
- `TAB5_LIST_MAX_ITEMS` (64) and `TAB5_COLLIST_MAX_COLS` (8) define compile-time limits for list and column list storage.
- `TAB5_INPUT_MAX_LEN` (128) limits single-line text input. `UITextArea` supports up to 1024 characters.

## Common Patterns

### Updating a status bar from a callback

```cpp
UIStatusBar statusBar("Ready");

btn.setOnTouchRelease([](TouchEvent e) {
    statusBar.setText("Button pressed!");
});
```

### Sharing a keyboard between multiple inputs

```cpp
UIKeyboard kb;
UITextInput input1(x1, y1, w, "Name");
UITextInput input2(x2, y2, w, "Email");

input1.attachKeyboard(&kb);
input2.attachKeyboard(&kb);
// Tapping either input opens the same keyboard
```

### Using UIColumnList with colored status cells

```cpp
int r = table.addRow();
table.setCellText(r, 0, "Server-01");
table.setCellText(r, 1, "Online", Tab5Theme::SECONDARY);   // green
table.setCellText(r, 2, "Warning", Tab5Theme::ACCENT);     // orange
table.setCellText(r, 3, "89%");
```

### Tab view child positioning

```cpp
UITabView tabs(0, TITLE_H, screenW, contentH, TabPosition::TOP);
int page = tabs.addPage("Settings");

// Position children relative to the tab content area
UIButton btn(tabs.contentX() + 20, tabs.contentY() + 20, 200, 52, "Save");
tabs.addChild(page, &btn);
```

---

**Back to:** [[Home]] · [[API Reference]]
