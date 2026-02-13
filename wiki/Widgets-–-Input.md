# Widgets – Input

## UITextInput

```cpp
UITextInput(x, y, width, "placeholder", height, bgColor, textColor, borderColor);
void attachKeyboard(UIKeyboard* kb);  // Required — connect a keyboard
void setText(const char* text);
const char* getText() const;
void clear();
void setPlaceholder(const char* ph);
void setMaxLength(int len);
void focus();             // Open keyboard
void blur();              // Close keyboard
bool isFocused() const;
void setOnSubmit(TextSubmitCallback cb);  // Done key pressed
void setOnChange(TextSubmitCallback cb);  // Each character typed
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setBorderColor(uint32_t c);
void setFocusBorderColor(uint32_t c);
void setPlaceholderColor(uint32_t c);
```

**Behavior:** Tapping the input field opens the attached keyboard and
highlights the border.  Characters are inserted at the cursor.  Pressing
Done fires the `onSubmit` callback and closes the keyboard.
Enter closes the keyboard without firing `onSubmit`.

---

## UIKeyboard

```cpp
UIKeyboard();
void show();              // Open the keyboard
void hide();              // Close the keyboard
bool isOpen() const;
void setOnKey(KeyCallback cb);  // Receives each char typed
void setBgColor(uint32_t c);
void setKeyColor(uint32_t c);
void setTextColor(uint32_t c);
```

**Keyboard layers:**
- **Lowercase** — default QWERTY layout
- **Uppercase** — activated by Shift (⇧); auto-reverts after one character
- **Symbols** — numbers and punctuation, activated by "123" key

**Special keys:**

| Key | Action |
|---|---|
| ⇧ (Shift) | Toggle uppercase |
| ⌫ (Backspace) | Delete last character |
| 123 / ABC | Switch between letters and symbols |
| Space | Space character |
| Done | Fires submit callback and closes keyboard |
| Ent (Enter) | Single-line: closes keyboard. Multi-line: inserts newline |

**Behavior:** The keyboard is modal — when visible it captures all touch
input.  It occupies the bottom 290px of the screen.  It is normally
managed automatically by `UITextInput` but can also be used standalone
with `setOnKey()`.

---

## UITextArea

```cpp
UITextArea(x, y, w, h, "placeholder", bgColor, textColor, borderColor);

// Keyboard
void attachKeyboard(UIKeyboard* kb);  // Required — connect a keyboard

// Text access
void setText(const char* text);
const char* getText() const;
void clear();

// Placeholder shown when text is empty
void setPlaceholder(const char* ph);
void setMaxLength(int len);           // Default: 1024 chars

// Focus state
void focus();             // Open keyboard
void blur();              // Close keyboard
bool isFocused() const;

// Callbacks
void setOnSubmit(TextSubmitCallback cb);  // Done key pressed
void setOnChange(TextSubmitCallback cb);  // Each character typed

// Scroll control
void scrollTo(int16_t offset);            // Pixel offset from top
void scrollToBottom();
void scrollToCursor();                    // Ensure cursor is visible

// Appearance
void setTextSize(float s);                // Font size (triggers reflow)
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setBorderColor(uint32_t c);
void setFocusBorderColor(uint32_t c);
void setPlaceholderColor(uint32_t c);
```

**Behavior:** The text area renders a bordered, scrollable multi-line text
field.  Text is automatically word-wrapped to fit the widget width.  Tapping
the widget opens the attached keyboard (same as `UITextInput`).  While focused,
tapping within the text area places the cursor at the tapped position.
Drag up/down to scroll through long text.  A scrollbar appears when content
overflows.  Pressing Done fires `onSubmit` and closes the keyboard.  The
widget supports mid-text insertion and deletion — the cursor position tracks
correctly through edits.  Up to 1024 characters are supported.

---

**Next:** [[Widgets – Lists]] · [[Widgets – Containers & Popups]] · [[Widgets – Selection]]
