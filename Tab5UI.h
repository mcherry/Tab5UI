/*******************************************************************************
 * Tab5UI.h - Touchscreen UI Library for M5Stack Tab5
 * 
 * A lightweight UI widget library built on M5GFX for the M5Stack Tab5's
 * 5-inch 1280x720 IPS capacitive touchscreen.
 *
 * Widgets: Label, Button, TitleBar, StatusBar, TextRow, IconSquare, IconCircle,
 *          Menu, TextInput, Keyboard, TabView
 * All widgets support touch and touch-release event callbacks.
 *
 * License: MIT
 ******************************************************************************/
#ifndef TAB5UI_H
#define TAB5UI_H

#include <M5GFX.h>
#include <vector>
#include <functional>

// ─── Screen Constants (M5Stack Tab5: 1280x720) ─────────────────────────────
#define TAB5_SCREEN_W   1280
#define TAB5_SCREEN_H   720

// ─── Default Sizing (scaled for 5" 1280x720 display) ───────────────────────
#define TAB5_TITLE_H        48      // Title bar height
#define TAB5_STATUS_H       36      // Status bar height
#define TAB5_BTN_H          52      // Default button height
#define TAB5_BTN_W          160     // Default button width
#define TAB5_BTN_R          8       // Button corner radius
#define TAB5_LABEL_H        32      // Default label height
#define TAB5_TEXTROW_H      40      // Text row height
#define TAB5_ICON_SIZE      44      // Default icon size
#define TAB5_PADDING        12      // General padding
#define TAB5_MENU_ITEM_H    48      // Menu item row height
#define TAB5_MENU_W         260     // Default menu popup width
#define TAB5_MENU_MAX_ITEMS 12      // Max items per menu
#define TAB5_KB_KEY_W       88      // Keyboard key width
#define TAB5_KB_KEY_H       56      // Keyboard key height
#define TAB5_KB_KEY_GAP     6       // Gap between keys
#define TAB5_KB_ROWS        4       // Number of key rows
#define TAB5_KB_MAX_COLS    12      // Max keys per row
#define TAB5_KB_H           290     // Total keyboard panel height
#define TAB5_INPUT_H        44      // Text input field height
#define TAB5_INPUT_MAX_LEN  128     // Max text input length
#define TAB5_LIST_ITEM_H    48      // List item row height
#define TAB5_LIST_MAX_ITEMS 64      // Max items in a list
#define TAB5_LIST_SCROLLBAR_W 6     // Scrollbar width
#define TAB5_TAB_BAR_H      48      // Tab bar height
#define TAB5_TAB_MAX_PAGES  8       // Max pages in a tab view
#define TAB5_TAB_MAX_CHILDREN 24    // Max child elements per page
#define TAB5_FONT_SIZE_SM   1.4f    // Small text
#define TAB5_FONT_SIZE_MD   1.8f    // Medium text (labels, buttons)
#define TAB5_FONT_SIZE_LG   2.4f    // Large text (title bar)

// ─── Default Theme Colors ───────────────────────────────────────────────────
namespace Tab5Theme {
    // Primary palette
    constexpr uint32_t PRIMARY       = 0x2196F3;  // Blue
    constexpr uint32_t PRIMARY_DARK  = 0x1565C0;  // Darker blue (pressed)
    constexpr uint32_t SECONDARY     = 0x4CAF50;  // Green
    constexpr uint32_t ACCENT        = 0xFF9800;  // Orange
    constexpr uint32_t DANGER        = 0xF44336;  // Red

    // Surface colors
    constexpr uint32_t BG_DARK       = 0x1A1A2E;  // Dark background
    constexpr uint32_t BG_MEDIUM     = 0x16213E;  // Medium background
    constexpr uint32_t SURFACE       = 0x0F3460;  // Surface / card
    constexpr uint32_t TITLE_BG      = 0x0F3460;  // Title bar background
    constexpr uint32_t STATUS_BG     = 0x1A1A2E;  // Status bar background

    // Text colors
    constexpr uint32_t TEXT_PRIMARY   = 0xFFFFFF;  // White text
    constexpr uint32_t TEXT_SECONDARY = 0xB0BEC5;  // Grey text
    constexpr uint32_t TEXT_DISABLED  = 0x546E7A;  // Disabled text

    // Outline / border
    constexpr uint32_t BORDER        = 0x37474F;
    constexpr uint32_t DIVIDER       = 0x263238;
}

// ─── Touch Event Types ──────────────────────────────────────────────────────
enum class TouchEvent {
    NONE,
    TOUCH,          // Finger down on element
    TOUCH_RELEASE   // Finger lifted from element
};

// ─── Callback Signature ─────────────────────────────────────────────────────
using TouchCallback = std::function<void(TouchEvent event)>;

// Forward declaration
class UIManager;

/*******************************************************************************
 * UIElement — Abstract base class for all UI widgets
 ******************************************************************************/
class UIElement {
public:
    UIElement(int16_t x, int16_t y, int16_t w, int16_t h);
    virtual ~UIElement() = default;

    // ── Drawing ──
    virtual void draw(M5GFX& gfx) = 0;

    // ── Geometry ──
    void setPosition(int16_t x, int16_t y);
    void setSize(int16_t w, int16_t h);
    int16_t getX() const { return _x; }
    int16_t getY() const { return _y; }
    int16_t getWidth() const { return _w; }
    int16_t getHeight() const { return _h; }

    // ── Visibility & Enable ──
    void setVisible(bool v)  { _visible = v; }
    bool isVisible() const   { return _visible; }
    void setEnabled(bool e)  { _enabled = e; }
    bool isEnabled() const   { return _enabled; }

    // ── Touch ──
    bool hitTest(int16_t tx, int16_t ty) const;
    void setOnTouch(TouchCallback cb)        { _onTouch = cb; }
    void setOnTouchRelease(TouchCallback cb) { _onRelease = cb; }

    // Called by UIManager
    virtual void handleTouchDown(int16_t tx, int16_t ty);
    virtual void handleTouchMove(int16_t tx, int16_t ty) {}
    virtual void handleTouchUp(int16_t tx, int16_t ty);

    // Type identification (avoids RTTI / dynamic_cast)
    virtual bool isCircleIcon() const { return false; }
    virtual bool isMenu() const       { return false; }
    virtual bool isKeyboard() const   { return false; }
    virtual bool isPopup() const      { return false; }
    virtual bool isTabView() const    { return false; }

    // ── Dirty flag (needs redraw) ──
    void setDirty(bool d = true) { _dirty = d; }
    bool isDirty() const         { return _dirty; }

    // ── Tag for identification ──
    void setTag(const char* tag) { _tag = tag; }
    const char* getTag() const   { return _tag; }

protected:
    int16_t  _x, _y, _w, _h;
    bool     _visible  = true;
    bool     _enabled  = true;
    bool     _pressed  = false;
    bool     _dirty    = true;
    const char* _tag   = "";

    TouchCallback _onTouch   = nullptr;
    TouchCallback _onRelease = nullptr;
};

/*******************************************************************************
 * UILabel — Static or dynamic text label
 ******************************************************************************/
class UILabel : public UIElement {
public:
    UILabel(int16_t x, int16_t y, int16_t w, int16_t h,
            const char* text = "",
            uint32_t textColor = Tab5Theme::TEXT_PRIMARY,
            float textSize = TAB5_FONT_SIZE_MD);

    void draw(M5GFX& gfx) override;

    void setText(const char* text);
    const char* getText() const { return _text; }

    void setTextColor(uint32_t color) { _textColor = color; _dirty = true; }
    void setTextSize(float s)         { _textSize = s; _dirty = true; }
    void setBgColor(uint32_t color)   { _bgColor = color; _hasBg = true; _dirty = true; }
    void clearBgColor()               { _hasBg = false; _dirty = true; }
    void setAlign(textdatum_t datum)  { _align = datum; _dirty = true; }

private:
    char     _text[128];
    uint32_t _textColor;
    uint32_t _bgColor = Tab5Theme::BG_DARK;
    float    _textSize;
    bool     _hasBg = false;
    textdatum_t _align = textdatum_t::middle_left;
};

/*******************************************************************************
 * UIButton — Rounded-rectangle button with text and touch feedback
 ******************************************************************************/
class UIButton : public UIElement {
public:
    UIButton(int16_t x, int16_t y, int16_t w, int16_t h,
             const char* label = "",
             uint32_t bgColor   = Tab5Theme::PRIMARY,
             uint32_t textColor = Tab5Theme::TEXT_PRIMARY,
             float textSize     = TAB5_FONT_SIZE_MD);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    void setLabel(const char* label);
    const char* getLabel() const { return _label; }

    void setBgColor(uint32_t c)      { _bgColor = c; _dirty = true; }
    void setPressedColor(uint32_t c)  { _pressedColor = c; }
    void setTextColor(uint32_t c)     { _textColor = c; _dirty = true; }
    void setTextSize(float s)         { _textSize = s; _dirty = true; }
    void setCornerRadius(int16_t r)   { _radius = r; _dirty = true; }
    void setBorderColor(uint32_t c)   { _borderColor = c; _hasBorder = true; _dirty = true; }

private:
    char     _label[64];
    uint32_t _bgColor;
    uint32_t _pressedColor;
    uint32_t _textColor;
    uint32_t _borderColor = Tab5Theme::BORDER;
    float    _textSize;
    int16_t  _radius    = TAB5_BTN_R;
    bool     _hasBorder = false;
};

/*******************************************************************************
 * UITitleBar — Full-width bar at the top of the screen
 ******************************************************************************/
class UITitleBar : public UIElement {
public:
    UITitleBar(const char* title = "Tab5 App",
               uint32_t bgColor   = Tab5Theme::TITLE_BG,
               uint32_t textColor = Tab5Theme::TEXT_PRIMARY);

    void draw(M5GFX& gfx) override;

    void setTitle(const char* title);
    const char* getTitle() const { return _title; }

    void setBgColor(uint32_t c)   { _bgColor = c; _dirty = true; }
    void setTextColor(uint32_t c) { _textColor = c; _dirty = true; }

    // Optional left/right text (e.g. back, menu)
    void setLeftText(const char* text);
    void setRightText(const char* text);
    void setOnLeftTouch(TouchCallback cb)  { _onLeftTouch = cb; }
    void setOnRightTouch(TouchCallback cb) { _onRightTouch = cb; }

    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

private:
    char     _title[64];
    char     _leftText[32];
    char     _rightText[32];
    uint32_t _bgColor;
    uint32_t _textColor;

    TouchCallback _onLeftTouch  = nullptr;
    TouchCallback _onRightTouch = nullptr;

    bool _leftPressed  = false;
    bool _rightPressed = false;

    // Touch zones
    static constexpr int16_t ZONE_W = 120;
};

/*******************************************************************************
 * UIStatusBar — Full-width bar at the bottom of the screen
 ******************************************************************************/
class UIStatusBar : public UIElement {
public:
    UIStatusBar(const char* text = "",
                uint32_t bgColor   = Tab5Theme::STATUS_BG,
                uint32_t textColor = Tab5Theme::TEXT_SECONDARY);

    void draw(M5GFX& gfx) override;

    void setText(const char* text);
    void setLeftText(const char* text);
    void setRightText(const char* text);

    void setBgColor(uint32_t c)   { _bgColor = c; _dirty = true; }
    void setTextColor(uint32_t c) { _textColor = c; _dirty = true; }

private:
    char     _text[128];
    char     _leftText[64];
    char     _rightText[64];
    uint32_t _bgColor;
    uint32_t _textColor;
};

/*******************************************************************************
 * UITextRow — A full-width row with label and optional value text
 ******************************************************************************/
class UITextRow : public UIElement {
public:
    UITextRow(int16_t x, int16_t y, int16_t w,
              const char* label = "",
              const char* value = "",
              uint32_t bgColor    = Tab5Theme::BG_MEDIUM,
              uint32_t labelColor = Tab5Theme::TEXT_PRIMARY,
              uint32_t valueColor = Tab5Theme::TEXT_SECONDARY);

    void draw(M5GFX& gfx) override;

    void setLabel(const char* label);
    void setValue(const char* value);
    const char* getLabel() const { return _label; }
    const char* getValue() const { return _value; }

    void setBgColor(uint32_t c)      { _bgColor = c; _dirty = true; }
    void setLabelColor(uint32_t c)   { _labelColor = c; _dirty = true; }
    void setValueColor(uint32_t c)   { _valueColor = c; _dirty = true; }
    void setShowDivider(bool show)   { _showDivider = show; _dirty = true; }

private:
    char     _label[128];
    char     _value[128];
    uint32_t _bgColor;
    uint32_t _labelColor;
    uint32_t _valueColor;
    bool     _showDivider = true;
};

/*******************************************************************************
 * UIIconSquare — Simple square icon shape with fill color
 ******************************************************************************/
class UIIconSquare : public UIElement {
public:
    UIIconSquare(int16_t x, int16_t y, int16_t size = TAB5_ICON_SIZE,
                 uint32_t fillColor   = Tab5Theme::PRIMARY,
                 uint32_t borderColor = Tab5Theme::BORDER);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    void setFillColor(uint32_t c)   { _fillColor = c; _dirty = true; }
    void setBorderColor(uint32_t c) { _borderColor = c; _dirty = true; }
    void setCornerRadius(int16_t r) { _radius = r; _dirty = true; }

    // Optional single character or short text drawn centered on the icon
    void setIconChar(const char* ch) { strncpy(_iconChar, ch, 7); _iconChar[7] = '\0'; _dirty = true; }
    void setIconCharColor(uint32_t c) { _iconCharColor = c; _dirty = true; }

private:
    uint32_t _fillColor;
    uint32_t _borderColor;
    uint32_t _pressedColor;
    uint32_t _iconCharColor = Tab5Theme::TEXT_PRIMARY;
    char     _iconChar[8]   = "";
    int16_t  _radius        = 4;
};

/*******************************************************************************
 * UIIconCircle — Simple circle icon shape with fill color
 ******************************************************************************/
class UIIconCircle : public UIElement {
public:
    UIIconCircle(int16_t x, int16_t y, int16_t radius = TAB5_ICON_SIZE / 2,
                 uint32_t fillColor   = Tab5Theme::SECONDARY,
                 uint32_t borderColor = Tab5Theme::BORDER);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // For circles, hit test is circular
    bool hitTestCircle(int16_t tx, int16_t ty) const;

    void setFillColor(uint32_t c)   { _fillColor = c; _dirty = true; }
    void setBorderColor(uint32_t c) { _borderColor = c; _dirty = true; }
    void setRadius(int16_t r)       { _circRadius = r; _w = _h = r * 2; _dirty = true; }

    // Optional single character drawn centered
    void setIconChar(const char* ch) { strncpy(_iconChar, ch, 7); _iconChar[7] = '\0'; _dirty = true; }
    void setIconCharColor(uint32_t c) { _iconCharColor = c; _dirty = true; }

private:
    int16_t  _circRadius;
    uint32_t _fillColor;
    uint32_t _borderColor;
    uint32_t _pressedColor;
    uint32_t _iconCharColor = Tab5Theme::TEXT_PRIMARY;
    char     _iconChar[8]   = "";
};

/*******************************************************************************
 * UIMenuItem — Data for a single menu entry
 ******************************************************************************/
struct UIMenuItem {
    char     label[48];
    bool     enabled;
    bool     separator;   // If true, draws a divider line instead of a label
    TouchCallback onSelect;

    UIMenuItem()
        : enabled(true), separator(false), onSelect(nullptr)
    { label[0] = '\0'; }
};

/*******************************************************************************
 * UIMenu — Popup menu with selectable items
 *
 * Usage:
 *   UIMenu menu(200, 60, 260);          // x, y, width
 *   menu.addItem("Open",    [](TouchEvent){ ... });
 *   menu.addItem("Save",    [](TouchEvent){ ... });
 *   menu.addSeparator();
 *   menu.addItem("Quit",    [](TouchEvent){ ... });
 *   menu.show();                         // make visible
 *
 * When visible the menu draws on top of other elements and captures
 * all touch input.  Tapping an item fires its callback and hides
 * the menu.  Tapping outside the menu dismisses it.
 ******************************************************************************/
class UIMenu : public UIElement {
public:
    UIMenu(int16_t x, int16_t y, int16_t w = TAB5_MENU_W,
           uint32_t bgColor    = Tab5Theme::SURFACE,
           uint32_t textColor  = Tab5Theme::TEXT_PRIMARY,
           uint32_t hlColor    = Tab5Theme::PRIMARY);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // Type identification
    bool isMenu() const override { return true; }

    // ── Item management ──
    /// Add a selectable menu item.  Returns the item index.
    int  addItem(const char* label, TouchCallback onSelect = nullptr);
    /// Add a non-interactive horizontal separator line.
    void addSeparator();
    /// Remove all items.
    void clearItems();
    /// Enable / disable a specific item by index.
    void setItemEnabled(int index, bool enabled);
    /// Change the label of an existing item.
    void setItemLabel(int index, const char* label);
    /// Get current item count.
    int  itemCount() const { return _itemCount; }

    // ── Show / Hide ──
    void show();    // Sets visible + dirty
    void hide();    // Sets invisible + dirty, clears highlight
    bool isOpen() const { return _visible; }

    // ── Called when the menu is dismissed (tap outside) ──
    void setOnDismiss(TouchCallback cb) { _onDismiss = cb; }

    // ── Colors ──
    void setBgColor(uint32_t c)        { _bgColor = c; _dirty = true; }
    void setTextColor(uint32_t c)      { _textColor = c; _dirty = true; }
    void setHighlightColor(uint32_t c) { _hlColor = c; _dirty = true; }
    void setBorderColor(uint32_t c)    { _borderColor = c; _dirty = true; }

private:
    UIMenuItem _items[TAB5_MENU_MAX_ITEMS];
    int        _itemCount    = 0;
    int        _pressedIndex = -1;   // Currently-pressed item

    uint32_t   _bgColor;
    uint32_t   _textColor;
    uint32_t   _hlColor;
    uint32_t   _borderColor = Tab5Theme::BORDER;

    TouchCallback _onDismiss = nullptr;

    /// Return item index under (tx,ty) or -1
    int itemIndexAt(int16_t tx, int16_t ty) const;
    /// Recalculate total height from item count
    void recalcHeight();
};

/******************************************************************************* * UIKeyboard — Full-screen popup touch keyboard
 *
 * Provides a QWERTY keyboard with lowercase, uppercase (Shift), and
 * a symbols layer.  Special keys: Backspace, Shift, Symbols ("123"),
 * Space, Enter/Done, and Hide ("↓").
 *
 * The keyboard is modal: when visible it captures all touch input.
 * It is normally shown/hidden by a UITextInput, but can also be used
 * standalone via show() / hide().
 ******************************************************************************/

// Callback fired whenever a visible character is typed or a special key
// is pressed.  `ch` is the character ('\b' for backspace, '\n' for
// enter/done, '\0' for hide).
using KeyCallback = std::function<void(char ch)>;

struct UIKey {
    char     label[8];      // Display text for the key cap
    char     value;         // Character value produced (0 = special)
    float    widthMult;     // Width multiplier (1.0 = normal key)
    uint32_t bgColor;       // Key background color
};

class UIKeyboard : public UIElement {
public:
    UIKeyboard();

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // Type identification
    bool isKeyboard() const override { return true; }

    // Show / Hide
    void show();
    void hide();
    bool isOpen() const { return _visible; }

    // Key callback — receives each character typed
    void setOnKey(KeyCallback cb)  { _onKey = cb; }

    // Colors
    void setBgColor(uint32_t c)     { _bgColor = c; _dirty = true; }
    void setKeyColor(uint32_t c)    { _keyColor = c; _dirty = true; }
    void setTextColor(uint32_t c)   { _textColor = c; _dirty = true; }

private:
    enum Layer { LOWER, UPPER, SYMBOLS };
    Layer    _layer = LOWER;

    uint32_t _bgColor    = Tab5Theme::BG_DARK;
    uint32_t _keyColor   = Tab5Theme::SURFACE;
    uint32_t _textColor  = Tab5Theme::TEXT_PRIMARY;

    KeyCallback _onKey = nullptr;

    // Pressed key tracking
    int      _pressedRow = -1;
    int      _pressedCol = -1;

    // Key layout tables (built once in constructor)
    UIKey    _keysLower[TAB5_KB_ROWS][TAB5_KB_MAX_COLS];
    int      _colsLower[TAB5_KB_ROWS];  // key count per row

    UIKey    _keysUpper[TAB5_KB_ROWS][TAB5_KB_MAX_COLS];
    int      _colsUpper[TAB5_KB_ROWS];

    UIKey    _keysSymbols[TAB5_KB_ROWS][TAB5_KB_MAX_COLS];
    int      _colsSymbols[TAB5_KB_ROWS];

    // Current active layout pointers
    UIKey   (*_keys)[TAB5_KB_MAX_COLS];
    int*     _cols;

    void buildLayouts();
    void setLayer(Layer layer);
    void buildRow(UIKey* dst, int& count, const char* chars, int len);
    // Get the pixel rect of a specific key
    void keyRect(int row, int col, int16_t& kx, int16_t& ky, int16_t& kw, int16_t& kh) const;
    // Find which key (row,col) is at touch coordinates; returns false if none
    bool keyAt(int16_t tx, int16_t ty, int& row, int& col) const;
};

/*******************************************************************************
 * UITextInput — Single-line text input field
 *
 * Renders as a bordered rectangle with current text and a blinking cursor.
 * Tapping it opens the built-in UIKeyboard.  Characters are routed into
 * the text buffer.  When the user presses Enter/Done or Hide, the keyboard
 * closes and the onSubmit / onChange callbacks fire.
 *
 * Usage:
 *   UIKeyboard keyboard;
 *   UITextInput input(20, 100, 400, "Name:");
 *   input.attachKeyboard(&keyboard);
 *   input.setOnSubmit([](const char* text) { ... });
 *   ui.addElement(&input);
 *   ui.addElement(&keyboard);   // add last so it draws on top
 ******************************************************************************/
using TextSubmitCallback = std::function<void(const char* text)>;

class UITextInput : public UIElement {
public:
    UITextInput(int16_t x, int16_t y, int16_t w,
                const char* placeholder = "",
                int16_t h = TAB5_INPUT_H,
                uint32_t bgColor    = Tab5Theme::BG_MEDIUM,
                uint32_t textColor  = Tab5Theme::TEXT_PRIMARY,
                uint32_t borderColor = Tab5Theme::BORDER);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // Attach a keyboard instance (required for typing)
    void attachKeyboard(UIKeyboard* kb) { _keyboard = kb; }

    // Text access
    void setText(const char* text);
    const char* getText() const { return _text; }
    void clear();

    // Placeholder shown when text is empty
    void setPlaceholder(const char* ph);

    // Max character limit
    void setMaxLength(int len) { _maxLen = (len < TAB5_INPUT_MAX_LEN) ? len : TAB5_INPUT_MAX_LEN; }

    // Callbacks
    void setOnSubmit(TextSubmitCallback cb)  { _onSubmit = cb; }
    void setOnChange(TextSubmitCallback cb)  { _onChange = cb; }

    // Focus state
    bool isFocused() const { return _focused; }
    void focus();    // Open keyboard
    void blur();     // Close keyboard

    // Colors
    void setBgColor(uint32_t c)        { _bgColor = c; _dirty = true; }
    void setTextColor(uint32_t c)      { _textColor = c; _dirty = true; }
    void setBorderColor(uint32_t c)    { _borderColor = c; _dirty = true; }
    void setFocusBorderColor(uint32_t c) { _focusBorderColor = c; }
    void setPlaceholderColor(uint32_t c) { _phColor = c; _dirty = true; }

private:
    char     _text[TAB5_INPUT_MAX_LEN];
    char     _placeholder[64];
    int      _cursorPos = 0;
    int      _maxLen    = TAB5_INPUT_MAX_LEN - 1;
    bool     _focused   = false;

    uint32_t _bgColor;
    uint32_t _textColor;
    uint32_t _borderColor;
    uint32_t _focusBorderColor = Tab5Theme::PRIMARY;
    uint32_t _phColor          = Tab5Theme::TEXT_DISABLED;

    UIKeyboard*        _keyboard  = nullptr;
    TextSubmitCallback _onSubmit  = nullptr;
    TextSubmitCallback _onChange  = nullptr;

    void onKeyPress(char ch);  // Internal handler for keyboard input
};

/*******************************************************************************
 * UITabView — Multi-page tabbed container
 *
 * Each tab page has a label (shown in the tab bar) and holds its own
 * set of child UI elements.  Only the active page's children are drawn
 * and receive touch events.
 *
 * The tab bar can be placed at the top or bottom of the widget.
 *
 * Usage:
 *   UITabView tabs(0, 48, 1280, 636);    // x, y, w, h
 *   int page0 = tabs.addPage("Controls");
 *   int page1 = tabs.addPage("Settings");
 *   tabs.addChild(page0, &myButton);
 *   tabs.addChild(page0, &myLabel);
 *   tabs.addChild(page1, &myList);
 *   ui.addElement(&tabs);
 *
 * The child element positions should be relative to the content area
 * of the tab view.  The tab view translates coordinates internally.
 ******************************************************************************/
enum class TabPosition {
    TOP,
    BOTTOM
};

using TabChangeCallback = std::function<void(int pageIndex)>;

struct UITabPage {
    char       label[32];
    UIElement* children[TAB5_TAB_MAX_CHILDREN];
    int        childCount;

    UITabPage() : childCount(0) {
        label[0] = '\0';
        for (int i = 0; i < TAB5_TAB_MAX_CHILDREN; i++) children[i] = nullptr;
    }
};

class UITabView : public UIElement {
public:
    UITabView(int16_t x, int16_t y, int16_t w, int16_t h,
              TabPosition pos = TabPosition::TOP,
              uint32_t barColor    = Tab5Theme::SURFACE,
              uint32_t activeColor = Tab5Theme::PRIMARY,
              uint32_t textColor   = Tab5Theme::TEXT_PRIMARY);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchMove(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // Type identification
    bool isTabView() const override { return true; }

    // ── Page management ──
    /// Add a new tab page with the given label.  Returns the page index.
    int  addPage(const char* label);
    /// Add a child element to a specific page.
    void addChild(int pageIndex, UIElement* child);
    /// Remove a child element from a page.
    void removeChild(int pageIndex, UIElement* child);
    /// Remove all children from a page.
    void clearPage(int pageIndex);
    /// Remove all pages.
    void clearAllPages();
    /// Get page count.
    int  pageCount() const { return _pageCount; }

    // ── Active page ──
    int  getActivePage() const { return _activePage; }
    void setActivePage(int index);

    // ── Page label ──
    void setPageLabel(int pageIndex, const char* label);
    const char* getPageLabel(int pageIndex) const;

    // ── Callback when the active tab changes ──
    void setOnTabChange(TabChangeCallback cb) { _onTabChange = cb; }

    // ── Tab bar position ──
    void setTabPosition(TabPosition pos) { _tabPos = pos; _dirty = true; }
    TabPosition getTabPosition() const { return _tabPos; }

    // ── Colors ──
    void setBarColor(uint32_t c)        { _barColor = c; _dirty = true; }
    void setActiveColor(uint32_t c)     { _activeColor = c; _dirty = true; }
    void setInactiveColor(uint32_t c)   { _inactiveColor = c; _dirty = true; }
    void setTextColor(uint32_t c)       { _textColor = c; _dirty = true; }
    void setActiveTextColor(uint32_t c) { _activeTextColor = c; _dirty = true; }
    void setBorderColor(uint32_t c)     { _borderColor = c; _dirty = true; }
    void setTabBarHeight(int16_t h)     { _tabBarH = h; _dirty = true; }

    // ── Content area geometry (for positioning children) ──
    int16_t contentX() const { return _x; }
    int16_t contentY() const;
    int16_t contentW() const { return _w; }
    int16_t contentH() const;

    // Check if any child on the active page is dirty (used by UIManager)
    bool hasActiveDirtyChild() const;

private:
    UITabPage _pages[TAB5_TAB_MAX_PAGES];
    int       _pageCount  = 0;
    int       _activePage = 0;
    int16_t   _tabBarH    = TAB5_TAB_BAR_H;

    TabPosition _tabPos;

    uint32_t _barColor;
    uint32_t _activeColor;
    uint32_t _inactiveColor = Tab5Theme::BG_MEDIUM;
    uint32_t _textColor;
    uint32_t _activeTextColor = Tab5Theme::TEXT_PRIMARY;
    uint32_t _borderColor  = Tab5Theme::BORDER;

    TabChangeCallback _onTabChange = nullptr;

    // Touch tracking for child dispatch
    UIElement* _touchedChild = nullptr;

    // Returns true if (tx,ty) is in the tab bar area
    bool hitTestTabBar(int16_t tx, int16_t ty) const;
    // Returns the page index under (tx,ty) in the tab bar, or -1
    int  tabIndexAt(int16_t tx, int16_t ty) const;
    // Draw the tab bar
    void drawTabBar(M5GFX& gfx);
    // Get the tab bar Y position
    int16_t tabBarY() const;
};

/*******************************************************************************
 * UIInfoPopup — Modal popup window with title, message, and OK button
 *
 * Usage:
 *   UIInfoPopup popup("About", "My App v1.0");
 *   popup.show();                       // display centered on screen
 *   ui.addElement(&popup);              // add last so it draws on top
 *
 * Tapping OK (or outside the popup) closes it.
 ******************************************************************************/
class UIInfoPopup : public UIElement {
public:
    UIInfoPopup(const char* title = "Info",
                const char* message = "");

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // Type identification
    bool isPopup() const override { return true; }

    // Show / Hide
    void show();
    void hide();
    bool isOpen() const { return _visible; }

    // Content
    void setTitle(const char* title);
    void setMessage(const char* msg);
    void setButtonLabel(const char* label);

    // Callback when dismissed
    void setOnDismiss(TouchCallback cb) { _onDismiss = cb; }

    // Colors
    void setBgColor(uint32_t c)     { _bgColor = c; _dirty = true; }
    void setTitleColor(uint32_t c)  { _titleColor = c; _dirty = true; }
    void setTextColor(uint32_t c)   { _textColor = c; _dirty = true; }
    void setBtnColor(uint32_t c)    { _btnColor = c; _dirty = true; }
    void setBorderColor(uint32_t c) { _borderColor = c; _dirty = true; }

private:
    char     _title[64];
    char     _message[256];
    char     _btnLabel[32];
    bool     _btnPressed = false;
    bool     _needsAutoSize = true;

    uint32_t _bgColor      = Tab5Theme::SURFACE;
    uint32_t _titleColor   = Tab5Theme::TEXT_PRIMARY;
    uint32_t _textColor    = Tab5Theme::TEXT_SECONDARY;
    uint32_t _btnColor     = Tab5Theme::PRIMARY;
    uint32_t _borderColor  = Tab5Theme::BORDER;

    TouchCallback _onDismiss = nullptr;

    // OK button rect (calculated in draw)
    int16_t _btnX, _btnY, _btnW, _btnH;
    bool hitTestBtn(int16_t tx, int16_t ty) const;

    // Auto-size the popup based on text content
    void autoSize(M5GFX& gfx);
    // Word-wrap helper: returns number of lines, fills lineStarts/lineLengths
    static int wordWrap(M5GFX& gfx, const char* text, float textSize,
                        int16_t maxWidth, int16_t* lineStarts,
                        int16_t* lineLengths, int maxLines);
};

/*******************************************************************************
 * UIList — Scrollable list with selectable items
 *
 * Usage:
 *   UIList myList(20, 60, 400, 500);
 *   myList.addItem("Item 1");
 *   myList.addItem("Item 2");
 *   myList.addItem("Item 3");
 *   myList.setOnSelect([](int index, const char* text) {
 *       Serial.printf("Selected %d: %s\n", index, text);
 *   });
 *   ui.addElement(&myList);
 *
 * Touch-scroll by dragging up/down.  Tap an item to select it.
 * Use getSelectedIndex() / getSelectedText() to query the selection.
 ******************************************************************************/
using ListSelectCallback = std::function<void(int index, const char* text)>;

struct UIListItem {
    char     text[64];
    bool     enabled;

    // Optional right-aligned icon
    bool     hasIcon;
    bool     iconCircle;      // true = circle, false = square
    char     iconChar[8];     // Character drawn on the icon
    uint32_t iconColor;       // Icon fill color
    uint32_t iconBorderColor; // Icon border color
    uint32_t iconCharColor;   // Icon character color

    UIListItem()
        : enabled(true), hasIcon(false), iconCircle(false)
        , iconColor(0x2196F3), iconBorderColor(0x37474F)
        , iconCharColor(0xFFFFFF)
    {
        text[0] = '\0';
        iconChar[0] = '\0';
    }
};

class UIList : public UIElement {
public:
    UIList(int16_t x, int16_t y, int16_t w, int16_t h,
           uint32_t bgColor    = Tab5Theme::BG_MEDIUM,
           uint32_t textColor  = Tab5Theme::TEXT_PRIMARY,
           uint32_t selectColor = Tab5Theme::PRIMARY);

    void draw(M5GFX& gfx) override;
    void handleTouchDown(int16_t tx, int16_t ty) override;
    void handleTouchMove(int16_t tx, int16_t ty) override;
    void handleTouchUp(int16_t tx, int16_t ty) override;

    // ── Item management ──
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
    int  itemCount() const { return _itemCount; }

    // ── Item icons ──
    void setItemIcon(int index, const char* iconChar,
                     uint32_t iconColor = Tab5Theme::PRIMARY,
                     bool circle = false,
                     uint32_t iconBorderColor = Tab5Theme::BORDER,
                     uint32_t iconCharColor = Tab5Theme::TEXT_PRIMARY);
    void clearItemIcon(int index);

    // ── Selection ──
    int  getSelectedIndex() const { return _selectedIndex; }
    const char* getSelectedText() const;
    void setSelectedIndex(int index);
    void clearSelection();

    // ── Callbacks ──
    void setOnSelect(ListSelectCallback cb) { _onSelect = cb; }

    // ── Scroll ──
    void scrollTo(int16_t offset);
    void scrollToItem(int index);

    // ── Colors ──
    void setBgColor(uint32_t c)        { _bgColor = c; _dirty = true; }
    void setTextColor(uint32_t c)      { _textColor = c; _dirty = true; }
    void setSelectColor(uint32_t c)    { _selectColor = c; _dirty = true; }
    void setBorderColor(uint32_t c)    { _borderColor = c; _dirty = true; }
    void setItemHeight(int16_t h)      { _itemH = h; _autoScale = false; _dirty = true; }
    void setTextSize(float s)          { _textSize = s; _autoScale = true; _dirty = true; }

private:
    UIListItem _items[TAB5_LIST_MAX_ITEMS];
    int        _itemCount     = 0;
    int        _selectedIndex = -1;
    int16_t    _scrollOffset  = 0;   // Pixels scrolled from top
    int16_t    _itemH         = TAB5_LIST_ITEM_H;
    float      _textSize      = TAB5_FONT_SIZE_MD;
    bool       _autoScale     = true;   // Auto-scale _itemH from _textSize

    uint32_t   _bgColor;
    uint32_t   _textColor;
    uint32_t   _selectColor;
    uint32_t   _borderColor   = Tab5Theme::BORDER;

    ListSelectCallback _onSelect = nullptr;

    // Touch-scroll state
    bool     _dragging      = false;
    int16_t  _touchStartY   = 0;
    int16_t  _scrollStart   = 0;
    int16_t  _touchDownY    = 0;     // For tap-vs-drag detection
    bool     _wasDrag        = false; // True if moved enough to be a drag
    static constexpr int16_t DRAG_THRESHOLD = 8;  // px to distinguish tap from drag

    int16_t  totalContentHeight() const { return _itemCount * _itemH; }
    int16_t  maxScroll() const;
    void     clampScroll();
    int      itemAtY(int16_t ty) const;
};

/******************************************************************************* * UIManager — Manages all UI elements, handles drawing and touch dispatch
 *****************************************************************************/
class UIManager {
public:
    UIManager(M5GFX& gfx);

    // ── Element Management ──
    void addElement(UIElement* element);
    void removeElement(UIElement* element);
    void clearElements();

    // ── Drawing ──
    void drawAll();          // Draw all visible elements
    void drawDirty();        // Draw only elements flagged dirty
    void setBackground(uint32_t color);
    void clearScreen();      // Fill screen with background color

    // ── Touch Processing ── call this in loop()
    void update();

    // ── Accessors ──
    M5GFX& getDisplay() { return _gfx; }
    UIElement* findByTag(const char* tag);

    // ── Content area helpers (accounting for title/status bars) ──
    int16_t contentTop() const    { return _contentTop; }
    int16_t contentBottom() const { return _contentBottom; }
    int16_t contentHeight() const { return _contentBottom - _contentTop; }
    void setContentArea(int16_t top, int16_t bottom) { _contentTop = top; _contentBottom = bottom; }

private:
    M5GFX& _gfx;
    std::vector<UIElement*> _elements;
    uint32_t _bgColor = Tab5Theme::BG_DARK;

    // Touch state tracking
    bool    _wasTouched     = false;
    int16_t _lastTouchX     = -1;
    int16_t _lastTouchY     = -1;
    int16_t _touchStartX    = -1;
    int16_t _touchStartY    = -1;
    UIElement* _touchedElem = nullptr;

    int16_t _contentTop    = 0;
    int16_t _contentBottom = TAB5_SCREEN_H;

    // Debounce
    unsigned long _lastTouchTime = 0;
    static constexpr unsigned long TOUCH_DEBOUNCE_MS = 30;
};

#endif // TAB5UI_H
