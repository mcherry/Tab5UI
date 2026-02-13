# Widgets – Containers & Popups

## UITabView

```cpp
UITabView(x, y, w, h, TabPosition::TOP, barColor, activeColor, textColor);

// Page management
int  addPage(const char* label);      // Returns page index
void addChild(int pageIndex, UIElement* child);
void removeChild(int pageIndex, UIElement* child);
void clearPage(int pageIndex);
void clearAllPages();
int  pageCount() const;

// Active page
int  getActivePage() const;
void setActivePage(int index);

// Page labels
void setPageLabel(int pageIndex, const char* label);
const char* getPageLabel(int pageIndex) const;

// Tab bar position
void setTabPosition(TabPosition pos); // TabPosition::TOP or TabPosition::BOTTOM
TabPosition getTabPosition() const;

// Callbacks
void setOnTabChange(TabChangeCallback cb); // void(int pageIndex)

// Content area helpers (for positioning children)
int16_t contentX() const;
int16_t contentY() const;             // Accounts for tab bar placement
int16_t contentW() const;
int16_t contentH() const;             // Total height minus tab bar

// Appearance
void setBarColor(uint32_t c);
void setActiveColor(uint32_t c);
void setInactiveColor(uint32_t c);
void setTextColor(uint32_t c);
void setActiveTextColor(uint32_t c);
void setBorderColor(uint32_t c);
void setTabBarHeight(int16_t h);      // Default: 48px
```

**Tab bar placement:** The tab bar can be at the `TOP` (default) or `BOTTOM`
of the widget.  Call `setTabPosition()` to move it at runtime.

**Child elements:** Each tab page holds up to 24 child elements.  Position
children relative to `contentY()` (which shifts depending on tab placement).
Only the active page's children are drawn and receive touch events.  The tab
view clips children to its content area.

**Touch dispatch:** Tapping a tab switches pages.  Touches in the content area
are forwarded to the active page's children, including drag/move events
for scrollable widgets like `UIList`.

---

## UIMenu

```cpp
UIMenu(x, y, width, bgColor, textColor, highlightColor);
int  addItem(const char* label, TouchCallback onSelect = nullptr);
void addSeparator();
void clearItems();
void setItemEnabled(int index, bool enabled);
void setItemLabel(int index, const char* label);
int  itemCount() const;
void show();              // Open the menu
void hide();              // Close the menu
bool isOpen() const;
void setOnDismiss(TouchCallback cb); // Called when tapping outside
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setHighlightColor(uint32_t c);
void setBorderColor(uint32_t c);
```

**Behavior:** The menu starts hidden. Call `show()` to open it. When visible it
captures all touch input (modal). Tapping an item fires its callback and
auto-closes the menu. Tapping outside dismisses it. Disabled items are drawn
in gray and cannot be selected. Separators render as horizontal divider lines.

---

## UIInfoPopup

```cpp
UIInfoPopup("Title", "Message text");
void show();                          // Display centered, auto-sized
void hide();
bool isOpen() const;
void setTitle(const char* title);
void setMessage(const char* msg);
void setButtonLabel(const char* label); // Default: "OK"
void setOnDismiss(TouchCallback cb);
void setBgColor(uint32_t c);
void setTitleColor(uint32_t c);
void setTextColor(uint32_t c);
void setBtnColor(uint32_t c);
void setBorderColor(uint32_t c);
```

**Behavior:** The popup auto-sizes to fit its title and message text,
word-wrapping long messages.  Dimensions are clamped so the popup never
exceeds the screen (40px margin on each side).  It is always centered.
Tapping the OK button or outside the popup dismisses it.  The popup is
modal — it captures all touch input while visible.

---

## UIConfirmPopup

```cpp
UIConfirmPopup("Title", "Are you sure?");
void show();                          // Display centered, auto-sized
void hide();
bool isOpen() const;
void setTitle(const char* title);
void setMessage(const char* msg);
void setYesLabel(const char* label);  // Default: "Yes"
void setNoLabel(const char* label);   // Default: "No"
ConfirmResult getResult() const;      // ConfirmResult::YES or ConfirmResult::NO
void setOnConfirm(ConfirmCallback cb); // void(ConfirmResult result)
void setBgColor(uint32_t c);
void setTitleColor(uint32_t c);
void setTextColor(uint32_t c);
void setYesBtnColor(uint32_t c);      // Default: SECONDARY (green)
void setNoBtnColor(uint32_t c);       // Default: DANGER (red)
void setBorderColor(uint32_t c);
```

**Behavior:** Works identically to `UIInfoPopup` (auto-sized, centered, modal,
word-wrapping) but presents two buttons: **No** (left, red) and **Yes** (right,
green).  Use `setOnConfirm()` to receive a `ConfirmResult` enum indicating
which button was pressed.  Tapping outside the popup is treated as **No**.
The last result is also available via `getResult()`.

---

## UIScrollText

```cpp
UIScrollText(x, y, w, h, bgColor, textColor);
void setText(const char* text);       // Set text content (up to 2048 chars)
const char* getText() const;

// Appearance
void setTextSize(float s);            // Font size (default: TAB5_FONT_SIZE_MD)
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setBorderColor(uint32_t c);

// Markdown colors
void setHeadingColor(uint32_t c);     // Heading text      (default: PRIMARY)
void setBoldColor(uint32_t c);        // **bold** text      (default: ACCENT)
void setItalicColor(uint32_t c);      // *italic* text      (default: TEXT_SECONDARY)
void setCodeColor(uint32_t c);        // `code` text        (default: SECONDARY)
void setCodeBgColor(uint32_t c);      // `code` background  (default: 0x0A0A1E)
void setRuleColor(uint32_t c);        // Horizontal rules   (default: DIVIDER)
void setBulletColor(uint32_t c);      // Bullet markers     (default: PRIMARY)

// Scroll control
void scrollTo(int16_t offset);        // Pixel offset from top
void scrollToTop();
void scrollToBottom();
```

**Behavior:** Displays read-only text with basic **Markdown rendering**.
Text is automatically word-wrapped to the widget width.  When the content
exceeds the visible area, a scrollbar appears on the right and the content
can be scrolled by touch-dragging up/down.

**Supported Markdown syntax:**

| Syntax | Rendering |
|--------|-----------|
| `# Heading` | Large heading (H1) with underline |
| `## Heading` | Medium heading (H2) |
| `### Heading` | Small heading (H3) |
| `**bold**` | Bold text (accent color) |
| `*italic*` | Italic text (secondary color) |
| `` `code` `` | Inline code (green on dark background) |
| `- item` or `* item` | Bullet list with • prefix |
| `---` or `***` | Horizontal rule |

Since the M5GFX environment has only one font loaded (DejaVu18), bold and
italic styles are rendered as **color changes** rather than font-weight or
font-style changes.  Headings use larger text sizes.  All markdown colors
are individually customizable via setter methods.

---

**Next:** [[Widgets – Selection]] · [[UIManager]] · [[Rendering]]
