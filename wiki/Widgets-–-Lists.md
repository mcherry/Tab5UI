# Widgets – Lists

## UIList

```cpp
UIList(x, y, w, h, bgColor, textColor, selectColor);

// Item management
int  addItem(const char* text);       // Returns item index
int  addItem(const char* text,        // Add item with right-aligned icon
             const char* iconChar,
             uint32_t iconColor = Tab5Theme::PRIMARY,
             bool circle = false,     // false = square, true = circle
             uint32_t iconBorderColor = Tab5Theme::BORDER,
             uint32_t iconCharColor = Tab5Theme::TEXT_PRIMARY);
void removeItem(int index);
void clearItems();
void setItemText(int index, const char* text);
void setItemEnabled(int index, bool enabled);
int  itemCount() const;

// Item icons (add/change/remove after creation)
void setItemIcon(int index, const char* iconChar,
                 uint32_t iconColor = Tab5Theme::PRIMARY,
                 bool circle = false,
                 uint32_t iconBorderColor = Tab5Theme::BORDER,
                 uint32_t iconCharColor = Tab5Theme::TEXT_PRIMARY);
void clearItemIcon(int index);

// Selection
int  getSelectedIndex() const;        // -1 if none
const char* getSelectedText() const;  // "" if none
void setSelectedIndex(int index);
void clearSelection();

// Scrolling
void scrollTo(int16_t offset);        // Pixel offset from top
void scrollToItem(int index);         // Ensure item is visible

// Callbacks
void setOnSelect(ListSelectCallback cb); // void(int index, const char* text)

// Appearance
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setSelectColor(uint32_t c);      // Highlight color for selected item
void setBorderColor(uint32_t c);
void setTextSize(float s);            // Font size; items auto-scale height
void setItemHeight(int16_t h);        // Fixed height (disables auto-scale)
```

**Icons:** Each list item can optionally display a right-aligned icon (square
or circle) matching the library's `UIIconSquare` / `UIIconCircle` style.  Use
the two-argument `addItem()` to create items with icons, or call
`setItemIcon()` / `clearItemIcon()` to change icons after creation.  Icon size
scales automatically with the item row height.

**Text size:** Call `setTextSize()` to change the font.  Item row heights
automatically scale to fit the text plus padding.  Calling `setItemHeight()`
switches to a fixed row height and disables auto-scaling.

**Behavior:** Drag up/down to scroll through the list. Tap an item to
select it (highlighted in the selection color). The widget distinguishes
taps from drags using an 8px threshold. A scrollbar appears automatically
when content overflows the visible area. Up to 64 items are supported.
Disabled items are drawn in gray and cannot be selected.

---

## UIDropdown

```cpp
UIDropdown(x, y, w, h, "placeholder", bgColor, textColor, selectColor);

// Item management (identical to UIList)
int  addItem(const char* text);
int  addItem(const char* text, const char* iconChar,
             uint32_t iconColor = Tab5Theme::PRIMARY,
             bool circle = false,
             uint32_t iconBorderColor = Tab5Theme::BORDER,
             uint32_t iconCharColor = Tab5Theme::TEXT_PRIMARY);
void removeItem(int index);
void clearItems();
void setItemText(int index, const char* text);
void setItemEnabled(int index, bool enabled);
int  itemCount() const;

// Item icons
void setItemIcon(int index, const char* iconChar,
                 uint32_t iconColor, bool circle,
                 uint32_t iconBorderColor, uint32_t iconCharColor);
void clearItemIcon(int index);

// Selection
int  getSelectedIndex() const;        // -1 if none
const char* getSelectedText() const;  // "" if none
void setSelectedIndex(int index);
void clearSelection();

// Open / Close
void open();
void close();
bool isOpen() const;

// Callbacks
void setOnSelect(ListSelectCallback cb);

// Appearance
void setPlaceholder(const char* text);
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setSelectColor(uint32_t c);
void setBorderColor(uint32_t c);
void setTextSize(float s);
void setMaxVisibleItems(int n);       // Max items shown when open (default 6)
```

**Behavior:** The dropdown appears as a compact button showing the selected
item's text (or a placeholder when nothing is selected) with a ▼ indicator.
Tapping opens a scrollable list overlay directly below (or above if near the
screen bottom).  The overlay supports drag-scrolling with scrollbar, item
icons (square or circle), and selection highlighting — identical to UIList.
Tapping an item selects it, fires the callback, and auto-closes the dropdown.
Tapping outside the list dismisses it.  The dropdown participates in the
modal overlay system, capturing all touch input while open.

---

## UIColumnList

```cpp
UIColumnList(x, y, w, h, bgColor, textColor, selectColor);

// Column definitions (call before adding rows; max TAB5_COLLIST_MAX_COLS = 8)
void addColumn(const char* header, int16_t width,
               TextAlign align = TextAlign::LEFT,
               bool sortable = true);

// Row management (max TAB5_LIST_MAX_ITEMS = 64 rows)
int  addRow();                        // Returns row index
void removeRow(int index);
void clearRows();

// Cell content
void setCellText(int row, int col, const char* text);
void setCellText(int row, int col, const char* text, uint32_t color); // custom color
void setCellIcon(int row, int col,
                 const uint8_t* pngData, size_t pngSize); // PROGMEM PNG icon

// Row state
void setRowEnabled(int row, bool enabled);
int  rowCount() const;
int  columnCount() const;

// Selection
int  getSelectedIndex() const;        // -1 if none
void setSelectedIndex(int index);
void clearSelection();

// Scrolling
void scrollTo(int16_t offset);        // Pixel offset from top
void scrollToItem(int index);         // Ensure row is visible

// Sorting
void sortByColumn(int col, SortDir dir); // Sort programmatically
void clearSort();                     // Remove sort, restore original order
void setSortable(bool enabled);       // Enable/disable all header sorting
void setColumnSortable(int col, bool sortable); // Per-column sortable flag
void setSortIndicatorColor(uint32_t c);  // Triangle indicator color

// Callbacks
void setOnSelect(ColumnListSelectCallback cb); // void(int index)

// Appearance
void setBgColor(uint32_t c);
void setTextColor(uint32_t c);
void setSelectColor(uint32_t c);
void setBorderColor(uint32_t c);
void setHeaderBgColor(uint32_t c);
void setHeaderTextColor(uint32_t c);
void setTextSize(float s);
void setItemHeight(int16_t h);
```

### Columns

Define columns with `addColumn()` before adding rows.  Each column has a header label, pixel width, text alignment, and a `sortable` flag.  Columns that contain only icons (no meaningful text) should set `sortable = false`.

### Cells

Each cell can hold either text or a PROGMEM PNG icon.  Text cells support per-cell custom colors via the color overload of `setCellText()` — useful for status indicators (green for OK, red for Critical, etc.).

### Sorting

Tap a sortable column header to cycle through ascending → descending → unsorted.  A triangle indicator (▲/▼) appears in the active sort column.  Sorting uses case-insensitive string comparison and operates through an indirection array — row data and icon pointers are never moved.

### Behavior

Identical scroll and selection mechanics to UIList — drag to scroll, tap to select, 8px tap-vs-drag threshold, automatic scrollbar. Disabled rows are drawn in gray and cannot be selected.  Up to 64 rows and 8 columns are supported.  Sprite-buffered for flicker-free rendering.

### Example

```cpp
UIColumnList table(PADDING, TITLE_H + PADDING, 920, 600,
                   Tab5Theme::BG_MEDIUM, Tab5Theme::TEXT_PRIMARY,
                   Tab5Theme::PRIMARY);

// Define columns
table.addColumn("Server",  280, TextAlign::LEFT,   true);
table.addColumn("Status",  150, TextAlign::CENTER, true);
table.addColumn("Health",  140, TextAlign::CENTER, true);
table.addColumn("Load",    160, TextAlign::CENTER, true);
table.addColumn("Action",  160, TextAlign::CENTER, false); // icon column, not sortable

// Add a row
int r = table.addRow();
table.setCellText(r, 0, "Alpha-01");
table.setCellText(r, 1, "Online", Tab5Theme::SECONDARY);   // green text
table.setCellText(r, 2, "OK",     Tab5Theme::SECONDARY);
table.setCellText(r, 3, "23%");
table.setCellIcon(r, 4, icon_home, icon_home_size);         // PROGMEM icon

table.setOnSelect([](int index) {
    Serial.printf("Selected row %d\n", index);
});
```

---

**Next:** [[Widgets – Containers & Popups]] · [[Widgets – Selection]] · [[UIManager]]
