/*******************************************************************************
 * Tab5UI.cpp - Touchscreen UI Library for M5Stack Tab5
 *
 * Implementation of all UI widget classes.
 ******************************************************************************/
#include "Tab5UI.h"
#include <string.h>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: Convert 24-bit RGB888 to M5GFX-compatible uint32_t color
// ─────────────────────────────────────────────────────────────────────────────
static inline uint32_t rgb888(uint32_t c) {
    return (uint32_t)lgfx::color888((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

// Darken a color for pressed states
static inline uint32_t darken(uint32_t c, uint8_t amount = 40) {
    uint8_t r = ((c >> 16) & 0xFF);
    uint8_t g = ((c >> 8)  & 0xFF);
    uint8_t b = ( c        & 0xFF);
    r = (r > amount) ? r - amount : 0;
    g = (g > amount) ? g - amount : 0;
    b = (b > amount) ? b - amount : 0;
    return (uint32_t)((r << 16) | (g << 8) | b);
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIElement — Base class
// ═════════════════════════════════════════════════════════════════════════════

UIElement::UIElement(int16_t x, int16_t y, int16_t w, int16_t h)
    : _x(x), _y(y), _w(w), _h(h) {}

void UIElement::setPosition(int16_t x, int16_t y) {
    _x = x; _y = y; _dirty = true;
}

void UIElement::setSize(int16_t w, int16_t h) {
    _w = w; _h = h; _dirty = true;
}

bool UIElement::hitTest(int16_t tx, int16_t ty) const {
    return _visible && _enabled &&
           tx >= _x && tx < (_x + _w) &&
           ty >= _y && ty < (_y + _h);
}

void UIElement::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIElement::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UILabel
// ═════════════════════════════════════════════════════════════════════════════

UILabel::UILabel(int16_t x, int16_t y, int16_t w, int16_t h,
                 const char* text, uint32_t textColor, float textSize)
    : UIElement(x, y, w, h)
    , _textColor(textColor)
    , _textSize(textSize)
{
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
}

void UILabel::setText(const char* text) {
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
    _dirty = true;
}

void UILabel::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background
    if (_hasBg) {
        gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));
    }

    // Text
    gfx.setTextSize(_textSize);
    gfx.setTextDatum(_align);
    gfx.setTextColor(rgb888(_textColor));

    int16_t tx = _x + TAB5_PADDING;
    int16_t ty = _y + _h / 2;

    if (_align == textdatum_t::middle_center || _align == textdatum_t::top_center) {
        tx = _x + _w / 2;
    } else if (_align == textdatum_t::middle_right || _align == textdatum_t::top_right) {
        tx = _x + _w - TAB5_PADDING;
    }

    gfx.drawString(_text, tx, ty);
    _dirty = false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIButton
// ═════════════════════════════════════════════════════════════════════════════

UIButton::UIButton(int16_t x, int16_t y, int16_t w, int16_t h,
                   const char* label, uint32_t bgColor,
                   uint32_t textColor, float textSize)
    : UIElement(x, y, w, h)
    , _bgColor(bgColor)
    , _pressedColor(darken(bgColor))
    , _textColor(textColor)
    , _textSize(textSize)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
}

void UIButton::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UIButton::draw(M5GFX& gfx) {
    if (!_visible) return;

    uint32_t bg = _pressed ? rgb888(_pressedColor) : rgb888(_bgColor);

    if (!_enabled) {
        bg = rgb888(Tab5Theme::BORDER);
    }

    // Rounded rectangle body
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, _radius, bg);

    // Optional border
    if (_hasBorder) {
        gfx.drawRoundRect(_x, _y, _w, _h, _radius, rgb888(_borderColor));
    }

    // Centered label text
    gfx.setTextSize(_textSize);
    gfx.setTextDatum(textdatum_t::middle_center);

    uint32_t tc = _enabled ? rgb888(_textColor) : rgb888(Tab5Theme::TEXT_DISABLED);
    gfx.setTextColor(tc);
    gfx.drawString(_label, _x + _w / 2, _y + _h / 2);

    _dirty = false;
}

void UIButton::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIButton::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UITitleBar
// ═════════════════════════════════════════════════════════════════════════════

UITitleBar::UITitleBar(const char* title, uint32_t bgColor, uint32_t textColor)
    : UIElement(0, 0, TAB5_SCREEN_W, TAB5_TITLE_H)
    , _bgColor(bgColor)
    , _textColor(textColor)
{
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    _leftText[0] = '\0';
    _rightText[0] = '\0';
}

void UITitleBar::setTitle(const char* title) {
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    _dirty = true;
}

void UITitleBar::setLeftText(const char* text) {
    strncpy(_leftText, text, sizeof(_leftText) - 1);
    _leftText[sizeof(_leftText) - 1] = '\0';
    _dirty = true;
}

void UITitleBar::setRightText(const char* text) {
    strncpy(_rightText, text, sizeof(_rightText) - 1);
    _rightText[sizeof(_rightText) - 1] = '\0';
    _dirty = true;
}

void UITitleBar::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));

    // Bottom divider line
    gfx.drawFastHLine(_x, _y + _h - 1, _w, rgb888(Tab5Theme::DIVIDER));

    // Center title
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    gfx.setTextDatum(textdatum_t::middle_center);
    gfx.setTextColor(rgb888(_textColor));
    gfx.drawString(_title, _w / 2, _h / 2);

    // Left text (e.g. "< Back")
    if (_leftText[0] != '\0') {
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_left);
        uint32_t lc = _leftPressed ? rgb888(Tab5Theme::ACCENT) : rgb888(_textColor);
        gfx.setTextColor(lc);
        gfx.drawString(_leftText, TAB5_PADDING, _h / 2);
    }

    // Right text (e.g. "Menu")
    if (_rightText[0] != '\0') {
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_right);
        uint32_t rc = _rightPressed ? rgb888(Tab5Theme::ACCENT) : rgb888(_textColor);
        gfx.setTextColor(rc);
        gfx.drawString(_rightText, _w - TAB5_PADDING, _h / 2);
    }

    _dirty = false;
}

void UITitleBar::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;

    // Check left zone
    if (_leftText[0] != '\0' && tx < ZONE_W) {
        _leftPressed = true;
        _dirty = true;
        if (_onLeftTouch) _onLeftTouch(TouchEvent::TOUCH);
        return;
    }
    // Check right zone
    if (_rightText[0] != '\0' && tx > (_w - ZONE_W)) {
        _rightPressed = true;
        _dirty = true;
        if (_onRightTouch) _onRightTouch(TouchEvent::TOUCH);
        return;
    }

    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UITitleBar::handleTouchUp(int16_t tx, int16_t ty) {
    if (_leftPressed) {
        _leftPressed = false;
        _dirty = true;
        if (_onLeftTouch) _onLeftTouch(TouchEvent::TOUCH_RELEASE);
    }
    if (_rightPressed) {
        _rightPressed = false;
        _dirty = true;
        if (_onRightTouch) _onRightTouch(TouchEvent::TOUCH_RELEASE);
    }
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIStatusBar
// ═════════════════════════════════════════════════════════════════════════════

UIStatusBar::UIStatusBar(const char* text, uint32_t bgColor, uint32_t textColor)
    : UIElement(0, TAB5_SCREEN_H - TAB5_STATUS_H, TAB5_SCREEN_W, TAB5_STATUS_H)
    , _bgColor(bgColor)
    , _textColor(textColor)
{
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
    _leftText[0] = '\0';
    _rightText[0] = '\0';
}

void UIStatusBar::setText(const char* text) {
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
    _dirty = true;
}

void UIStatusBar::setLeftText(const char* text) {
    strncpy(_leftText, text, sizeof(_leftText) - 1);
    _leftText[sizeof(_leftText) - 1] = '\0';
    _dirty = true;
}

void UIStatusBar::setRightText(const char* text) {
    strncpy(_rightText, text, sizeof(_rightText) - 1);
    _rightText[sizeof(_rightText) - 1] = '\0';
    _dirty = true;
}

void UIStatusBar::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));

    // Top divider line
    gfx.drawFastHLine(_x, _y, _w, rgb888(Tab5Theme::DIVIDER));

    gfx.setTextSize(TAB5_FONT_SIZE_SM);
    gfx.setTextColor(rgb888(_textColor));

    // Center text
    if (_text[0] != '\0') {
        gfx.setTextDatum(textdatum_t::middle_center);
        gfx.drawString(_text, _w / 2, _y + _h / 2);
    }

    // Left text
    if (_leftText[0] != '\0') {
        gfx.setTextDatum(textdatum_t::middle_left);
        gfx.drawString(_leftText, _x + TAB5_PADDING, _y + _h / 2);
    }

    // Right text
    if (_rightText[0] != '\0') {
        gfx.setTextDatum(textdatum_t::middle_right);
        gfx.drawString(_rightText, _x + _w - TAB5_PADDING, _y + _h / 2);
    }

    _dirty = false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  UITextRow
// ═════════════════════════════════════════════════════════════════════════════

UITextRow::UITextRow(int16_t x, int16_t y, int16_t w,
                     const char* label, const char* value,
                     uint32_t bgColor, uint32_t labelColor, uint32_t valueColor)
    : UIElement(x, y, w, TAB5_TEXTROW_H)
    , _bgColor(bgColor)
    , _labelColor(labelColor)
    , _valueColor(valueColor)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    strncpy(_value, value, sizeof(_value) - 1);
    _value[sizeof(_value) - 1] = '\0';
}

void UITextRow::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UITextRow::setValue(const char* value) {
    strncpy(_value, value, sizeof(_value) - 1);
    _value[sizeof(_value) - 1] = '\0';
    _dirty = true;
}

void UITextRow::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background
    uint32_t bg = _pressed ? rgb888(darken(_bgColor, 20)) : rgb888(_bgColor);
    gfx.fillRect(_x, _y, _w, _h, bg);

    // Label (left aligned)
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_left);
    gfx.setTextColor(rgb888(_labelColor));
    gfx.drawString(_label, _x + TAB5_PADDING, _y + _h / 2);

    // Value (right aligned)
    if (_value[0] != '\0') {
        gfx.setTextDatum(textdatum_t::middle_right);
        gfx.setTextColor(rgb888(_valueColor));
        gfx.drawString(_value, _x + _w - TAB5_PADDING, _y + _h / 2);
    }

    // Bottom divider
    if (_showDivider) {
        gfx.drawFastHLine(_x + TAB5_PADDING, _y + _h - 1,
                          _w - TAB5_PADDING * 2, rgb888(Tab5Theme::DIVIDER));
    }

    _dirty = false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIIconSquare
// ═════════════════════════════════════════════════════════════════════════════

UIIconSquare::UIIconSquare(int16_t x, int16_t y, int16_t size,
                           uint32_t fillColor, uint32_t borderColor)
    : UIElement(x, y, size, size)
    , _fillColor(fillColor)
    , _borderColor(borderColor)
    , _pressedColor(darken(fillColor))
{}

void UIIconSquare::draw(M5GFX& gfx) {
    if (!_visible) return;

    uint32_t fc = _pressed ? rgb888(_pressedColor) : rgb888(_fillColor);

    // Filled rounded square
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, _radius, fc);

    // Border
    gfx.drawRoundRect(_x, _y, _w, _h, _radius, rgb888(_borderColor));

    // Icon character
    if (_iconChar[0] != '\0') {
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_center);
        gfx.setTextColor(rgb888(_iconCharColor));
        gfx.drawString(_iconChar, _x + _w / 2, _y + _h / 2);
    }

    _dirty = false;
}

void UIIconSquare::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIIconSquare::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIIconCircle
// ═════════════════════════════════════════════════════════════════════════════

UIIconCircle::UIIconCircle(int16_t x, int16_t y, int16_t radius,
                           uint32_t fillColor, uint32_t borderColor)
    : UIElement(x, y, radius * 2, radius * 2)
    , _circRadius(radius)
    , _fillColor(fillColor)
    , _borderColor(borderColor)
    , _pressedColor(darken(fillColor))
{}

bool UIIconCircle::hitTestCircle(int16_t tx, int16_t ty) const {
    if (!_visible || !_enabled) return false;
    int16_t cx = _x + _circRadius;
    int16_t cy = _y + _circRadius;
    int32_t dx = tx - cx;
    int32_t dy = ty - cy;
    return (dx * dx + dy * dy) <= (_circRadius * _circRadius);
}

void UIIconCircle::draw(M5GFX& gfx) {
    if (!_visible) return;

    int16_t cx = _x + _circRadius;
    int16_t cy = _y + _circRadius;

    uint32_t fc = _pressed ? rgb888(_pressedColor) : rgb888(_fillColor);

    // Filled circle
    gfx.fillSmoothCircle(cx, cy, _circRadius, fc);

    // Border
    gfx.drawCircle(cx, cy, _circRadius, rgb888(_borderColor));

    // Icon character
    if (_iconChar[0] != '\0') {
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_center);
        gfx.setTextColor(rgb888(_iconCharColor));
        gfx.drawString(_iconChar, cx, cy);
    }

    _dirty = false;
}

void UIIconCircle::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTestCircle(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIIconCircle::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIMenu
// ═════════════════════════════════════════════════════════════════════════════

UIMenu::UIMenu(int16_t x, int16_t y, int16_t w,
               uint32_t bgColor, uint32_t textColor, uint32_t hlColor)
    : UIElement(x, y, w, TAB5_PADDING * 2)  // height recalculated on addItem
    , _bgColor(bgColor)
    , _textColor(textColor)
    , _hlColor(hlColor)
{
    _visible = false;   // menus start hidden
}

void UIMenu::recalcHeight() {
    // Top + bottom padding, each item is TAB5_MENU_ITEM_H (separators are thinner)
    int16_t h = TAB5_PADDING;  // top padding
    for (int i = 0; i < _itemCount; ++i) {
        h += _items[i].separator ? (TAB5_PADDING + 1) : TAB5_MENU_ITEM_H;
    }
    h += TAB5_PADDING / 2;     // bottom padding
    _h = h;
}

int UIMenu::addItem(const char* label, TouchCallback onSelect) {
    if (_itemCount >= TAB5_MENU_MAX_ITEMS) return -1;
    UIMenuItem& item = _items[_itemCount];
    strncpy(item.label, label, sizeof(item.label) - 1);
    item.label[sizeof(item.label) - 1] = '\0';
    item.enabled   = true;
    item.separator = false;
    item.onSelect  = onSelect;
    _itemCount++;
    recalcHeight();
    _dirty = true;
    return _itemCount - 1;
}

void UIMenu::addSeparator() {
    if (_itemCount >= TAB5_MENU_MAX_ITEMS) return;
    UIMenuItem& item = _items[_itemCount];
    item.label[0]  = '\0';
    item.enabled   = true;
    item.separator = true;
    item.onSelect  = nullptr;
    _itemCount++;
    recalcHeight();
    _dirty = true;
}

void UIMenu::clearItems() {
    _itemCount = 0;
    _pressedIndex = -1;
    recalcHeight();
    _dirty = true;
}

void UIMenu::setItemEnabled(int index, bool enabled) {
    if (index >= 0 && index < _itemCount) {
        _items[index].enabled = enabled;
        _dirty = true;
    }
}

void UIMenu::setItemLabel(int index, const char* label) {
    if (index >= 0 && index < _itemCount) {
        strncpy(_items[index].label, label, sizeof(_items[index].label) - 1);
        _items[index].label[sizeof(_items[index].label) - 1] = '\0';
        _dirty = true;
    }
}

void UIMenu::show() {
    _visible = true;
    _pressedIndex = -1;
    _dirty = true;
}

void UIMenu::hide() {
    _visible = false;
    _pressedIndex = -1;
    _dirty = true;
}

int UIMenu::itemIndexAt(int16_t tx, int16_t ty) const {
    if (tx < _x || tx >= _x + _w) return -1;

    int16_t yOff = _y + TAB5_PADDING;  // past top padding
    for (int i = 0; i < _itemCount; ++i) {
        int16_t itemH = _items[i].separator ? (TAB5_PADDING + 1) : TAB5_MENU_ITEM_H;
        if (ty >= yOff && ty < yOff + itemH) {
            if (_items[i].separator) return -1;  // separators aren't selectable
            return i;
        }
        yOff += itemH;
    }
    return -1;
}

void UIMenu::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Shadow (offset dark rect)
    gfx.fillRect(_x + 3, _y + 3, _w, _h, rgb888(0x0A0A14));

    // Background
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, 6, rgb888(_bgColor));

    // Border
    gfx.drawRoundRect(_x, _y, _w, _h, 6, rgb888(_borderColor));

    // Items
    int16_t yOff = _y + TAB5_PADDING;
    for (int i = 0; i < _itemCount; ++i) {
        const UIMenuItem& item = _items[i];

        if (item.separator) {
            // Horizontal divider
            int16_t lineY = yOff + TAB5_PADDING / 2;
            gfx.drawFastHLine(_x + TAB5_PADDING, lineY,
                              _w - TAB5_PADDING * 2,
                              rgb888(Tab5Theme::DIVIDER));
            yOff += TAB5_PADDING + 1;
            continue;
        }

        // Highlight for pressed item
        if (i == _pressedIndex && item.enabled) {
            gfx.fillRect(_x + 2, yOff, _w - 4, TAB5_MENU_ITEM_H,
                         rgb888(_hlColor));
        }

        // Label
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_left);

        uint32_t tc = item.enabled
                    ? rgb888(_textColor)
                    : rgb888(Tab5Theme::TEXT_DISABLED);
        // When pressed, keep text white for contrast on highlight
        if (i == _pressedIndex && item.enabled) {
            tc = rgb888(Tab5Theme::TEXT_PRIMARY);
        }
        gfx.setTextColor(tc);
        gfx.drawString(item.label, _x + TAB5_PADDING, yOff + TAB5_MENU_ITEM_H / 2);

        yOff += TAB5_MENU_ITEM_H;
    }

    _dirty = false;
}

void UIMenu::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    int idx = itemIndexAt(tx, ty);
    if (idx >= 0 && _items[idx].enabled) {
        _pressedIndex = idx;
        _dirty = true;
    }
}

void UIMenu::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    // If touch is inside the menu body
    if (hitTest(tx, ty)) {
        int idx = itemIndexAt(tx, ty);
        if (idx >= 0 && idx == _pressedIndex && _items[idx].enabled) {
            // Fire the item callback
            if (_items[idx].onSelect) {
                _items[idx].onSelect(TouchEvent::TOUCH_RELEASE);
            }
            hide();  // auto-close after selection
        } else {
            _pressedIndex = -1;
            _dirty = true;
        }
    } else {
        // Touch outside — dismiss
        hide();
        if (_onDismiss) _onDismiss(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIKeyboard
// ═════════════════════════════════════════════════════════════════════════════

// Helper to set a key in a layout row
static void setKey(UIKey& k, const char* label, char value,
                   float wm = 1.0f, uint32_t bg = Tab5Theme::SURFACE) {
    strncpy(k.label, label, sizeof(k.label) - 1);
    k.label[sizeof(k.label) - 1] = '\0';
    k.value     = value;
    k.widthMult = wm;
    k.bgColor   = bg;
}

UIKeyboard::UIKeyboard()
    : UIElement(0, TAB5_SCREEN_H - TAB5_KB_H, TAB5_SCREEN_W, TAB5_KB_H)
{
    _visible = false;
    buildLayouts();
    setLayer(LOWER);
}

void UIKeyboard::buildRow(UIKey* dst, int& count, const char* chars, int len) {
    count = len;
    for (int i = 0; i < len; ++i) {
        char lbl[2] = { chars[i], '\0' };
        setKey(dst[i], lbl, chars[i]);
    }
}

void UIKeyboard::buildLayouts() {
    // ─── LOWERCASE ───
    {
        const char r0[] = "qwertyuiop";
        buildRow(_keysLower[0], _colsLower[0], r0, 10);

        const char r1[] = "asdfghjkl";
        buildRow(_keysLower[1], _colsLower[1], r1, 9);

        // Row 2: Shift + zxcvbnm + Backspace
        _colsLower[2] = 9;
        setKey(_keysLower[2][0], "Shft", 0, 1.4f, Tab5Theme::BG_MEDIUM);  // Shift
        const char r2[] = "zxcvbnm";
        for (int i = 0; i < 7; ++i) {
            char lbl[2] = { r2[i], '\0' };
            setKey(_keysLower[2][i + 1], lbl, r2[i]);
        }
        setKey(_keysLower[2][8], "Bksp", '\b', 1.4f, Tab5Theme::BG_MEDIUM); // Backspace

        // Row 3: 123 + Space + . + Done + Hide
        _colsLower[3] = 5;
        setKey(_keysLower[3][0], "123",  0, 1.4f, Tab5Theme::BG_MEDIUM);
        setKey(_keysLower[3][1], " ",  ' ', 5.0f, Tab5Theme::SURFACE);      // Space bar
        setKey(_keysLower[3][2], ".",  '.', 1.0f, Tab5Theme::SURFACE);
        setKey(_keysLower[3][3], "Done", '\n', 1.6f, Tab5Theme::PRIMARY);
        setKey(_keysLower[3][4], "Hide",  '\0', 1.2f, Tab5Theme::BG_MEDIUM); // Hide
    }

    // ─── UPPERCASE ───
    {
        const char r0[] = "QWERTYUIOP";
        buildRow(_keysUpper[0], _colsUpper[0], r0, 10);

        const char r1[] = "ASDFGHJKL";
        buildRow(_keysUpper[1], _colsUpper[1], r1, 9);

        _colsUpper[2] = 9;
        setKey(_keysUpper[2][0], "Shft", 0, 1.4f, Tab5Theme::PRIMARY);  // Shift active
        const char r2[] = "ZXCVBNM";
        for (int i = 0; i < 7; ++i) {
            char lbl[2] = { r2[i], '\0' };
            setKey(_keysUpper[2][i + 1], lbl, r2[i]);
        }
        setKey(_keysUpper[2][8], "Bksp", '\b', 1.4f, Tab5Theme::BG_MEDIUM);

        _colsUpper[3] = 5;
        setKey(_keysUpper[3][0], "123",  0, 1.4f, Tab5Theme::BG_MEDIUM);
        setKey(_keysUpper[3][1], " ",  ' ', 5.0f, Tab5Theme::SURFACE);
        setKey(_keysUpper[3][2], ".",  '.', 1.0f, Tab5Theme::SURFACE);
        setKey(_keysUpper[3][3], "Done", '\n', 1.6f, Tab5Theme::PRIMARY);
        setKey(_keysUpper[3][4], "Hide",  '\0', 1.2f, Tab5Theme::BG_MEDIUM);
    }

    // ─── SYMBOLS ───
    {
        const char r0[] = "1234567890";
        buildRow(_keysSymbols[0], _colsSymbols[0], r0, 10);

        _colsSymbols[1] = 10;
        const char s1[] = "-/:;()$&@\"";
        for (int i = 0; i < 10; ++i) {
            char lbl[2] = { s1[i], '\0' };
            setKey(_keysSymbols[1][i], lbl, s1[i]);
        }

        _colsSymbols[2] = 9;
        setKey(_keysSymbols[2][0], "ABC", 0, 1.4f, Tab5Theme::BG_MEDIUM);
        const char s2[] = ".,?!'_#%";
        for (int i = 0; i < (int)strlen(s2); ++i) {
            char lbl[2] = { s2[i], '\0' };
            // Handle special display for some chars
            setKey(_keysSymbols[2][i + 1], lbl, s2[i]);
        }

        // Pad with a dummy if needed to leave room for backspace
        // s2 has 8 chars → indices 1..8, backspace at 9 → but max 9 so index 8
        // Actually: index 0=ABC, 1..8=chars, total=9 — let's trim to 8 chars +backspace
        // s2 has 8 chars → 0=ABC, 1-8=8chars = 9 entries. Need backspace too.
        // Adjust: use only 7 symbol chars in row 2 + ABC + backspace = 9
        _colsSymbols[2] = 9;
        setKey(_keysSymbols[2][0], "ABC", 0, 1.4f, Tab5Theme::BG_MEDIUM);
        const char s2b[] = ".,?!'_#";
        for (int i = 0; i < 7; ++i) {
            char lbl[2] = { s2b[i], '\0' };
            setKey(_keysSymbols[2][i + 1], lbl, s2b[i]);
        }
        setKey(_keysSymbols[2][8], "Bksp", '\b', 1.4f, Tab5Theme::BG_MEDIUM);

        _colsSymbols[3] = 5;
        setKey(_keysSymbols[3][0], "ABC",  0, 1.4f, Tab5Theme::BG_MEDIUM);
        setKey(_keysSymbols[3][1], " ",  ' ', 5.0f, Tab5Theme::SURFACE);
        setKey(_keysSymbols[3][2], ".",  '.', 1.0f, Tab5Theme::SURFACE);
        setKey(_keysSymbols[3][3], "Done", '\n', 1.6f, Tab5Theme::PRIMARY);
        setKey(_keysSymbols[3][4], "Hide",  '\0', 1.2f, Tab5Theme::BG_MEDIUM);
    }
}

void UIKeyboard::setLayer(Layer layer) {
    _layer = layer;
    switch (layer) {
        case UPPER:   _keys = _keysUpper;   _cols = _colsUpper;   break;
        case SYMBOLS: _keys = _keysSymbols; _cols = _colsSymbols; break;
        default:      _keys = _keysLower;   _cols = _colsLower;   break;
    }
    _dirty = true;
}

void UIKeyboard::show() {
    _visible = true;
    _pressedRow = -1;
    _pressedCol = -1;
    setLayer(LOWER);
    _dirty = true;
}

void UIKeyboard::hide() {
    _visible = false;
    _pressedRow = -1;
    _pressedCol = -1;
    _dirty = true;
}

void UIKeyboard::keyRect(int row, int col,
                          int16_t& kx, int16_t& ky,
                          int16_t& kw, int16_t& kh) const {
    kh = TAB5_KB_KEY_H;
    ky = _y + TAB5_PADDING + row * (TAB5_KB_KEY_H + TAB5_KB_KEY_GAP);

    // Calculate total width units for this row to center the row
    float totalUnits = 0;
    for (int c = 0; c < _cols[row]; ++c) {
        totalUnits += _keys[row][c].widthMult;
    }
    float unitW = (float)(TAB5_KB_KEY_W + TAB5_KB_KEY_GAP);
    float rowPixelW = totalUnits * unitW - TAB5_KB_KEY_GAP;
    float startX = (_w - rowPixelW) / 2.0f;

    float cx = startX;
    for (int c = 0; c < col; ++c) {
        cx += _keys[row][c].widthMult * unitW;
    }
    kx = _x + (int16_t)cx;
    kw = (int16_t)(_keys[row][col].widthMult * unitW) - TAB5_KB_KEY_GAP;
}

bool UIKeyboard::keyAt(int16_t tx, int16_t ty, int& row, int& col) const {
    for (int r = 0; r < TAB5_KB_ROWS; ++r) {
        int16_t kx, ky, kw, kh;
        for (int c = 0; c < _cols[r]; ++c) {
            keyRect(r, c, kx, ky, kw, kh);
            if (tx >= kx && tx < kx + kw && ty >= ky && ty < ky + kh) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}

void UIKeyboard::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background panel
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));
    // Top border
    gfx.drawFastHLine(_x, _y, _w, rgb888(Tab5Theme::BORDER));

    // Draw each key
    for (int r = 0; r < TAB5_KB_ROWS; ++r) {
        for (int c = 0; c < _cols[r]; ++c) {
            int16_t kx, ky, kw, kh;
            keyRect(r, c, kx, ky, kw, kh);

            const UIKey& key = _keys[r][c];
            bool isPressed = (r == _pressedRow && c == _pressedCol);

            uint32_t bg = isPressed
                        ? rgb888(darken(key.bgColor, 30))
                        : rgb888(key.bgColor);

            gfx.fillSmoothRoundRect(kx, ky, kw, kh, 4, bg);

            // Key label
            gfx.setTextSize(TAB5_FONT_SIZE_MD);
            gfx.setTextDatum(textdatum_t::middle_center);
            gfx.setTextColor(rgb888(_textColor));
            gfx.drawString(key.label, kx + kw / 2, ky + kh / 2);
        }
    }

    _dirty = false;
}

void UIKeyboard::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    int row, col;
    if (keyAt(tx, ty, row, col)) {
        _pressedRow = row;
        _pressedCol = col;
        _dirty = true;
    }
}

void UIKeyboard::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    int row, col;
    if (keyAt(tx, ty, row, col) && row == _pressedRow && col == _pressedCol) {
        const UIKey& key = _keys[row][col];

        if (key.value != 0) {
            // Regular character or special (backspace, enter, hide)
            if (key.value == '\0') {
                // Hide key
                hide();
            }
            if (_onKey) _onKey(key.value);

            // After typing a letter in UPPER mode, revert to LOWER
            if (_layer == UPPER && key.value >= 'A' && key.value <= 'Z') {
                setLayer(LOWER);
            }
        } else {
            // Special mode-switch keys (value == 0, label determines action)
            if (strcmp(key.label, "Shft") == 0) {
                // Shift toggle
                setLayer(_layer == UPPER ? LOWER : UPPER);
            } else if (strcmp(key.label, "123") == 0) {
                setLayer(SYMBOLS);
            } else if (strcmp(key.label, "ABC") == 0) {
                setLayer(LOWER);
            } else if (strcmp(key.label, "Hide") == 0) {
                // Hide
                hide();
                if (_onKey) _onKey('\0');
            }
        }
    }

    _pressedRow = -1;
    _pressedCol = -1;
    _dirty = true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  UITextInput
// ═════════════════════════════════════════════════════════════════════════════

UITextInput::UITextInput(int16_t x, int16_t y, int16_t w,
                         const char* placeholder, int16_t h,
                         uint32_t bgColor, uint32_t textColor,
                         uint32_t borderColor)
    : UIElement(x, y, w, h)
    , _bgColor(bgColor)
    , _textColor(textColor)
    , _borderColor(borderColor)
{
    _text[0] = '\0';
    strncpy(_placeholder, placeholder, sizeof(_placeholder) - 1);
    _placeholder[sizeof(_placeholder) - 1] = '\0';
}

void UITextInput::setText(const char* text) {
    strncpy(_text, text, _maxLen);
    _text[_maxLen] = '\0';
    _cursorPos = (int)strlen(_text);
    _dirty = true;
}

void UITextInput::clear() {
    _text[0] = '\0';
    _cursorPos = 0;
    _dirty = true;
}

void UITextInput::setPlaceholder(const char* ph) {
    strncpy(_placeholder, ph, sizeof(_placeholder) - 1);
    _placeholder[sizeof(_placeholder) - 1] = '\0';
    _dirty = true;
}

void UITextInput::focus() {
    if (_focused) return;
    _focused = true;
    _dirty = true;
    if (_keyboard) {
        _keyboard->setOnKey([this](char ch) { this->onKeyPress(ch); });
        _keyboard->show();
    }
}

void UITextInput::blur() {
    if (!_focused) return;
    _focused = false;
    _dirty = true;
    if (_keyboard && _keyboard->isOpen()) {
        _keyboard->hide();
    }
}

void UITextInput::onKeyPress(char ch) {
    if (ch == '\0') {
        // Hide key pressed
        blur();
        return;
    }
    if (ch == '\n') {
        // Enter/Done
        if (_onSubmit) _onSubmit(_text);
        blur();
        return;
    }
    if (ch == '\b') {
        // Backspace
        if (_cursorPos > 0) {
            _cursorPos--;
            _text[_cursorPos] = '\0';
            _dirty = true;
            if (_onChange) _onChange(_text);
        }
        return;
    }
    // Regular character
    if (_cursorPos < _maxLen) {
        _text[_cursorPos] = ch;
        _cursorPos++;
        _text[_cursorPos] = '\0';
        _dirty = true;
        if (_onChange) _onChange(_text);
    }
}

void UITextInput::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Background
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));

    // Border (highlight when focused)
    uint32_t bc = _focused ? rgb888(_focusBorderColor) : rgb888(_borderColor);
    gfx.drawRect(_x, _y, _w, _h, bc);
    if (_focused) {
        gfx.drawRect(_x + 1, _y + 1, _w - 2, _h - 2, bc);  // 2px border
    }

    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_left);

    if (_text[0] != '\0') {
        gfx.setTextColor(rgb888(_textColor));
        gfx.drawString(_text, _x + TAB5_PADDING, _y + _h / 2);

        // Draw cursor blinking (simple: always show when focused)
        if (_focused) {
            int16_t tw = gfx.textWidth(_text);
            int16_t cx = _x + TAB5_PADDING + tw + 2;
            int16_t cy1 = _y + 6;
            int16_t cy2 = _y + _h - 6;
            gfx.drawFastVLine(cx, cy1, cy2 - cy1, rgb888(Tab5Theme::TEXT_PRIMARY));
        }
    } else {
        // Placeholder
        gfx.setTextColor(rgb888(_phColor));
        gfx.drawString(_placeholder, _x + TAB5_PADDING, _y + _h / 2);

        if (_focused) {
            int16_t cx = _x + TAB5_PADDING;
            int16_t cy1 = _y + 6;
            int16_t cy2 = _y + _h - 6;
            gfx.drawFastVLine(cx, cy1, cy2 - cy1, rgb888(Tab5Theme::TEXT_PRIMARY));
        }
    }

    _dirty = false;
}

void UITextInput::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UITextInput::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        focus();  // Open keyboard on tap
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UITabView
// ═════════════════════════════════════════════════════════════════════════════

UITabView::UITabView(int16_t x, int16_t y, int16_t w, int16_t h,
                     TabPosition pos, uint32_t barColor,
                     uint32_t activeColor, uint32_t textColor)
    : UIElement(x, y, w, h)
    , _tabPos(pos)
    , _barColor(barColor)
    , _activeColor(activeColor)
    , _textColor(textColor)
{}

int UITabView::addPage(const char* label) {
    if (_pageCount >= TAB5_TAB_MAX_PAGES) return -1;
    int idx = _pageCount++;
    strncpy(_pages[idx].label, label, 31);
    _pages[idx].label[31] = '\0';
    _pages[idx].childCount = 0;
    _dirty = true;
    return idx;
}

void UITabView::addChild(int pageIndex, UIElement* child) {
    if (pageIndex < 0 || pageIndex >= _pageCount) return;
    UITabPage& page = _pages[pageIndex];
    if (page.childCount >= TAB5_TAB_MAX_CHILDREN) return;
    page.children[page.childCount++] = child;
    _dirty = true;
}

void UITabView::removeChild(int pageIndex, UIElement* child) {
    if (pageIndex < 0 || pageIndex >= _pageCount) return;
    UITabPage& page = _pages[pageIndex];
    for (int i = 0; i < page.childCount; i++) {
        if (page.children[i] == child) {
            // Shift remaining children down
            for (int j = i; j < page.childCount - 1; j++) {
                page.children[j] = page.children[j + 1];
            }
            page.childCount--;
            page.children[page.childCount] = nullptr;
            _dirty = true;
            return;
        }
    }
}

void UITabView::clearPage(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= _pageCount) return;
    _pages[pageIndex].childCount = 0;
    for (int i = 0; i < TAB5_TAB_MAX_CHILDREN; i++)
        _pages[pageIndex].children[i] = nullptr;
    _dirty = true;
}

void UITabView::clearAllPages() {
    for (int i = 0; i < _pageCount; i++) {
        clearPage(i);
    }
    _pageCount = 0;
    _activePage = 0;
    _dirty = true;
}

void UITabView::setActivePage(int index) {
    if (index < 0 || index >= _pageCount || index == _activePage) return;
    _activePage = index;
    _touchedChild = nullptr;
    _dirty = true;
    if (_onTabChange) _onTabChange(index);
}

void UITabView::setPageLabel(int pageIndex, const char* label) {
    if (pageIndex < 0 || pageIndex >= _pageCount) return;
    strncpy(_pages[pageIndex].label, label, 31);
    _pages[pageIndex].label[31] = '\0';
    _dirty = true;
}

const char* UITabView::getPageLabel(int pageIndex) const {
    if (pageIndex < 0 || pageIndex >= _pageCount) return "";
    return _pages[pageIndex].label;
}

int16_t UITabView::tabBarY() const {
    if (_tabPos == TabPosition::TOP) {
        return _y;
    } else {
        return _y + _h - _tabBarH;
    }
}

int16_t UITabView::contentY() const {
    if (_tabPos == TabPosition::TOP) {
        return _y + _tabBarH;
    } else {
        return _y;
    }
}

int16_t UITabView::contentH() const {
    return _h - _tabBarH;
}

bool UITabView::hasActiveDirtyChild() const {
    if (_activePage < 0 || _activePage >= _pageCount) return false;
    const UITabPage& page = _pages[_activePage];
    for (int i = 0; i < page.childCount; i++) {
        if (page.children[i] && page.children[i]->isVisible() && page.children[i]->isDirty())
            return true;
    }
    return false;
}

void UITabView::drawDirtyChildren(M5GFX& gfx) {
    if (!_visible || _activePage < 0 || _activePage >= _pageCount) return;

    int16_t cy = contentY();
    int16_t ch = contentH();

    UITabPage& page = _pages[_activePage];
    gfx.setClipRect(_x, cy, _w, ch);
    for (int i = 0; i < page.childCount; i++) {
        UIElement* child = page.children[i];
        if (child && child->isVisible() && child->isDirty()) {
            child->draw(gfx);
            child->setDirty(false);
        }
    }
    gfx.clearClipRect();
}

bool UITabView::hitTestTabBar(int16_t tx, int16_t ty) const {
    int16_t barY = tabBarY();
    return tx >= _x && tx < _x + _w &&
           ty >= barY && ty < barY + _tabBarH;
}

int UITabView::tabIndexAt(int16_t tx, int16_t ty) const {
    if (!hitTestTabBar(tx, ty) || _pageCount == 0) return -1;
    int16_t tabW = _w / _pageCount;
    int idx = (tx - _x) / tabW;
    if (idx >= _pageCount) idx = _pageCount - 1;
    return idx;
}

void UITabView::drawTabBar(M5GFX& gfx) {
    int16_t barY = tabBarY();

    // Tab bar background
    gfx.fillRect(_x, barY, _w, _tabBarH, rgb888(_barColor));

    if (_pageCount == 0) return;

    int16_t tabW = _w / _pageCount;

    for (int i = 0; i < _pageCount; i++) {
        int16_t tx = _x + i * tabW;
        int16_t tw = (i == _pageCount - 1) ? (_x + _w - tx) : tabW; // last tab gets remainder

        if (i == _activePage) {
            // Active tab highlight
            gfx.fillRect(tx, barY, tw, _tabBarH, rgb888(_activeColor));
            // Active indicator bar (3px thick at the content-facing edge)
            if (_tabPos == TabPosition::TOP) {
                gfx.fillRect(tx, barY + _tabBarH - 3, tw, 3, rgb888(_activeColor));
            } else {
                gfx.fillRect(tx, barY, tw, 3, rgb888(_activeColor));
            }
        } else {
            // Inactive tab
            gfx.fillRect(tx, barY, tw, _tabBarH, rgb888(_inactiveColor));
        }

        // Tab label
        gfx.setTextSize(TAB5_FONT_SIZE_MD);
        gfx.setTextDatum(textdatum_t::middle_center);
        uint32_t tc = (i == _activePage) ? rgb888(_activeTextColor) : rgb888(_textColor);
        gfx.setTextColor(tc);
        gfx.drawString(_pages[i].label, tx + tw / 2, barY + _tabBarH / 2);

        // Divider between tabs (except last)
        if (i < _pageCount - 1) {
            gfx.drawFastVLine(tx + tw, barY + 6, _tabBarH - 12,
                              rgb888(_borderColor));
        }
    }

    // Bottom border for top tabs, top border for bottom tabs
    if (_tabPos == TabPosition::TOP) {
        gfx.drawFastHLine(_x, barY + _tabBarH - 1, _w, rgb888(_borderColor));
    } else {
        gfx.drawFastHLine(_x, barY, _w, rgb888(_borderColor));
    }
}

void UITabView::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Fill the content area background
    int16_t cy = contentY();
    int16_t ch = contentH();
    gfx.fillRect(_x, cy, _w, ch, rgb888(Tab5Theme::BG_DARK));

    // Draw the tab bar
    drawTabBar(gfx);

    // Draw active page's children
    if (_activePage >= 0 && _activePage < _pageCount) {
        UITabPage& page = _pages[_activePage];
        // Set clip to content area so children don't bleed into tab bar
        gfx.setClipRect(_x, cy, _w, ch);
        for (int i = 0; i < page.childCount; i++) {
            if (page.children[i] && page.children[i]->isVisible()) {
                page.children[i]->draw(gfx);
                page.children[i]->setDirty(false);
            }
        }
        gfx.clearClipRect();
    }

    _dirty = false;
}

void UITabView::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;

    _pressed = true;
    _touchedChild = nullptr;

    // Check if touch is on the tab bar
    if (hitTestTabBar(tx, ty)) {
        int idx = tabIndexAt(tx, ty);
        if (idx >= 0 && idx != _activePage) {
            setActivePage(idx);
        }
        if (_onTouch) _onTouch(TouchEvent::TOUCH);
        return;
    }

    // Touch is in the content area — dispatch to active page's children
    if (_activePage >= 0 && _activePage < _pageCount) {
        UITabPage& page = _pages[_activePage];
        // Reverse iterate for z-order
        for (int i = page.childCount - 1; i >= 0; --i) {
            UIElement* child = page.children[i];
            if (!child || !child->isVisible() || !child->isEnabled()) continue;

            bool hit = child->isCircleIcon()
                     ? static_cast<UIIconCircle*>(child)->hitTestCircle(tx, ty)
                     : child->hitTest(tx, ty);

            if (hit) {
                _touchedChild = child;
                child->handleTouchDown(tx, ty);
                return;
            }
        }
    }

    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UITabView::handleTouchMove(int16_t tx, int16_t ty) {
    if (_touchedChild) {
        _touchedChild->handleTouchMove(tx, ty);
        // If the child is dirty, the tab view is dirty too
        if (_touchedChild->isDirty()) _dirty = true;
    }
}

void UITabView::handleTouchUp(int16_t tx, int16_t ty) {
    if (_touchedChild) {
        _touchedChild->handleTouchUp(tx, ty);
        if (_touchedChild->isDirty()) _dirty = true;
        _touchedChild = nullptr;
    }
    _pressed = false;
    if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIInfoPopup
// ═════════════════════════════════════════════════════════════════════════════

UIInfoPopup::UIInfoPopup(const char* title, const char* message)
    : UIElement(0, 0, 10, 10)
    , _btnX(0), _btnY(0), _btnW(100), _btnH(40)
    , _needsAutoSize(true)
{
    _visible = false;
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    strncpy(_message, message, sizeof(_message) - 1);
    _message[sizeof(_message) - 1] = '\0';
    strncpy(_btnLabel, "OK", sizeof(_btnLabel) - 1);
    _btnLabel[sizeof(_btnLabel) - 1] = '\0';
}

void UIInfoPopup::setTitle(const char* title) {
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIInfoPopup::setMessage(const char* msg) {
    strncpy(_message, msg, sizeof(_message) - 1);
    _message[sizeof(_message) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIInfoPopup::setButtonLabel(const char* label) {
    strncpy(_btnLabel, label, sizeof(_btnLabel) - 1);
    _btnLabel[sizeof(_btnLabel) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIInfoPopup::show() {
    // Actual sizing happens in draw() where we have gfx reference
    _visible = true;
    _btnPressed = false;
    _needsAutoSize = true;
    _dirty = true;
}

void UIInfoPopup::hide() {
    _visible = false;
    _btnPressed = false;
    _dirty = true;
}

bool UIInfoPopup::hitTestBtn(int16_t tx, int16_t ty) const {
    return tx >= _btnX && tx < _btnX + _btnW &&
           ty >= _btnY && ty < _btnY + _btnH;
}

// ── Word-wrap helper ────────────────────────────────────────────────────────
// Returns the number of lines produced.  lineStarts[] receives the byte
// offset of each line inside `text`, lineLengths[] the byte length of that
// line (excluding the trailing space that caused the wrap).
int UIInfoPopup::wordWrap(M5GFX& gfx, const char* text, float textSize,
                          int16_t maxWidth, int16_t* lineStarts,
                          int16_t* lineLengths, int maxLines)
{
    gfx.setTextSize(textSize);
    int lines = 0;
    int len   = strlen(text);
    int pos   = 0;

    while (pos < len && lines < maxLines) {
        // Find how many chars fit on this line
        int bestBreak = -1;
        int i = pos;
        char buf[257];

        while (i < len) {
            int runLen = i - pos + 1;
            if (runLen > 255) runLen = 255;
            memcpy(buf, text + pos, runLen);
            buf[runLen] = '\0';
            int16_t tw = gfx.textWidth(buf);
            if (tw > maxWidth && bestBreak > pos) {
                // Exceeded width — break at last space
                break;
            }
            if (text[i] == ' ' || text[i] == '-') {
                bestBreak = i;
            }
            if (text[i] == '\n') {
                // Explicit newline
                bestBreak = i;
                break;
            }
            i++;
        }

        int lineEnd;
        int nextPos;
        if (i >= len) {
            // Rest of string fits
            lineEnd = len;
            nextPos = len;
        } else if (text[i] == '\n' || (bestBreak >= pos && text[bestBreak] == '\n')) {
            int brk = (text[i] == '\n') ? i : bestBreak;
            lineEnd = brk;
            nextPos = brk + 1; // skip the newline
        } else if (bestBreak > pos) {
            lineEnd = bestBreak + 1; // include the space/hyphen
            nextPos = bestBreak + 1;
        } else {
            // Single long word — force break at width
            lineEnd = (i > pos) ? i : pos + 1;
            nextPos = lineEnd;
        }

        lineStarts[lines]  = pos;
        lineLengths[lines] = lineEnd - pos;
        lines++;
        pos = nextPos;
    }

    if (lines == 0) {
        lineStarts[0]  = 0;
        lineLengths[0] = 0;
        lines = 1;
    }
    return lines;
}

// ── Auto-size popup based on text content ───────────────────────────────────
void UIInfoPopup::autoSize(M5GFX& gfx) {
    const int16_t hPad        = TAB5_PADDING * 2;    // left + right padding
    const int16_t vPad        = TAB5_PADDING;         // top/bottom padding
    const int16_t titleGap    = 42;                    // title text + divider
    const int16_t btnAreaH    = 56;                    // button + spacing
    const int16_t minW        = 200;
    const int16_t minH        = 140;
    const int16_t screenMargin = 40;                   // min gap from screen edge
    const int16_t maxW        = TAB5_SCREEN_W - screenMargin * 2;
    const int16_t maxH        = TAB5_SCREEN_H - screenMargin * 2;

    // Measure title width
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    int16_t titleW = gfx.textWidth(_title);
    int16_t titleH_px = gfx.fontHeight() * TAB5_FONT_SIZE_LG;

    // Measure button label width
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t btnLabelW = gfx.textWidth(_btnLabel);
    int16_t btnW = btnLabelW + 60;
    if (btnW < 100) btnW = 100;

    // Start with width based on title (plus padding)
    int16_t neededW = titleW + hPad + 40;        // extra for visual breathing room
    if (btnW + hPad > neededW) neededW = btnW + hPad;

    // Clamp width before doing word-wrap (so we wrap to final width)
    if (neededW < minW)  neededW = minW;

    // Check message width — may widen the popup
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t rawMsgW = gfx.textWidth(_message);
    int16_t msgOneLineW = rawMsgW + hPad + 20;
    if (msgOneLineW > neededW && msgOneLineW <= maxW) {
        neededW = msgOneLineW;
    }
    if (neededW > maxW) neededW = maxW;

    // Word-wrap the message at the chosen width
    int16_t contentW = neededW - hPad - 10; // available text area
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(gfx, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    // Compute line height
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t lineH = (int16_t)(gfx.fontHeight() * TAB5_FONT_SIZE_MD) + 4;

    // Total height
    int16_t neededH = vPad          // top padding
                    + titleGap      // title + divider
                    + 10            // gap after divider
                    + lineH * numLines  // message lines
                    + 10            // gap before button
                    + btnAreaH      // button area
                    + vPad;         // bottom padding

    if (neededH < minH) neededH = minH;
    if (neededH > maxH) neededH = maxH;

    // Apply dimensions and center on screen
    _w = neededW;
    _h = neededH;
    _x = (TAB5_SCREEN_W - _w) / 2;
    _y = (TAB5_SCREEN_H - _h) / 2;

    _needsAutoSize = false;
}

void UIInfoPopup::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Auto-size on first draw or after content change
    if (_needsAutoSize) {
        autoSize(gfx);
    }

    // Shadow
    gfx.fillRect(_x + 4, _y + 4, _w, _h, rgb888(0x0A0A14));

    // Background
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, 8, rgb888(_bgColor));

    // Border
    gfx.drawRoundRect(_x, _y, _w, _h, 8, rgb888(_borderColor));

    // Title
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    gfx.setTextDatum(textdatum_t::top_center);
    gfx.setTextColor(rgb888(_titleColor));
    gfx.drawString(_title, _x + _w / 2, _y + TAB5_PADDING + 4);

    // Divider below title
    int16_t divY = _y + TAB5_PADDING + 38;
    gfx.drawFastHLine(_x + TAB5_PADDING, divY,
                      _w - TAB5_PADDING * 2, rgb888(Tab5Theme::DIVIDER));

    // Message — word-wrapped
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::top_center);
    gfx.setTextColor(rgb888(_textColor));

    int16_t contentW = _w - TAB5_PADDING * 2 - 10;
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(gfx, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    int16_t lineH = (int16_t)(gfx.fontHeight() * TAB5_FONT_SIZE_MD) + 4;
    int16_t msgStartY = divY + 14;

    char lineBuf[257];
    for (int i = 0; i < numLines; i++) {
        int len = lineLengths[i];
        if (len > 255) len = 255;
        memcpy(lineBuf, _message + lineStarts[i], len);
        // Trim trailing spaces
        while (len > 0 && lineBuf[len - 1] == ' ') len--;
        lineBuf[len] = '\0';
        gfx.drawString(lineBuf, _x + _w / 2, msgStartY + i * lineH);
    }

    // OK button (centered at bottom)
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    _btnW = gfx.textWidth(_btnLabel) + 60;
    if (_btnW < 100) _btnW = 100;
    _btnH = 40;
    _btnX = _x + (_w - _btnW) / 2;
    _btnY = _y + _h - _btnH - TAB5_PADDING;

    uint32_t btnBg = _btnPressed ? rgb888(darken(_btnColor)) : rgb888(_btnColor);
    gfx.fillSmoothRoundRect(_btnX, _btnY, _btnW, _btnH, 6, btnBg);

    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_center);
    gfx.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    gfx.drawString(_btnLabel, _btnX + _btnW / 2, _btnY + _btnH / 2);

    _dirty = false;
}

void UIInfoPopup::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (hitTestBtn(tx, ty)) {
        _btnPressed = true;
        _dirty = true;
    }
}

void UIInfoPopup::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (_btnPressed && hitTestBtn(tx, ty)) {
        // OK button tapped
        _btnPressed = false;
        hide();
        if (_onDismiss) _onDismiss(TouchEvent::TOUCH_RELEASE);
    } else if (!hitTest(tx, ty)) {
        // Tap outside popup — dismiss
        _btnPressed = false;
        hide();
        if (_onDismiss) _onDismiss(TouchEvent::TOUCH_RELEASE);
    } else {
        _btnPressed = false;
        _dirty = true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIConfirmPopup
// ═════════════════════════════════════════════════════════════════════════════

UIConfirmPopup::UIConfirmPopup(const char* title, const char* message)
    : UIElement(0, 0, 10, 10)
    , _yesBtnX(0), _yesBtnY(0), _yesBtnW(100), _yesBtnH(40)
    , _noBtnX(0), _noBtnY(0), _noBtnW(100), _noBtnH(40)
    , _needsAutoSize(true)
{
    _visible = false;
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    strncpy(_message, message, sizeof(_message) - 1);
    _message[sizeof(_message) - 1] = '\0';
    strncpy(_yesLabel, "Yes", sizeof(_yesLabel) - 1);
    _yesLabel[sizeof(_yesLabel) - 1] = '\0';
    strncpy(_noLabel, "No", sizeof(_noLabel) - 1);
    _noLabel[sizeof(_noLabel) - 1] = '\0';
}

void UIConfirmPopup::setTitle(const char* title) {
    strncpy(_title, title, sizeof(_title) - 1);
    _title[sizeof(_title) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIConfirmPopup::setMessage(const char* msg) {
    strncpy(_message, msg, sizeof(_message) - 1);
    _message[sizeof(_message) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIConfirmPopup::setYesLabel(const char* label) {
    strncpy(_yesLabel, label, sizeof(_yesLabel) - 1);
    _yesLabel[sizeof(_yesLabel) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIConfirmPopup::setNoLabel(const char* label) {
    strncpy(_noLabel, label, sizeof(_noLabel) - 1);
    _noLabel[sizeof(_noLabel) - 1] = '\0';
    _needsAutoSize = true;
    _dirty = true;
}

void UIConfirmPopup::show() {
    _visible = true;
    _yesBtnPressed = false;
    _noBtnPressed = false;
    _result = ConfirmResult::NO;
    _needsAutoSize = true;
    _dirty = true;
}

void UIConfirmPopup::hide() {
    _visible = false;
    _yesBtnPressed = false;
    _noBtnPressed = false;
    _dirty = true;
}

bool UIConfirmPopup::hitTestYesBtn(int16_t tx, int16_t ty) const {
    return tx >= _yesBtnX && tx < _yesBtnX + _yesBtnW &&
           ty >= _yesBtnY && ty < _yesBtnY + _yesBtnH;
}

bool UIConfirmPopup::hitTestNoBtn(int16_t tx, int16_t ty) const {
    return tx >= _noBtnX && tx < _noBtnX + _noBtnW &&
           ty >= _noBtnY && ty < _noBtnY + _noBtnH;
}

// ── Word-wrap helper (identical to UIInfoPopup's) ───────────────────────────
int UIConfirmPopup::wordWrap(M5GFX& gfx, const char* text, float textSize,
                             int16_t maxWidth, int16_t* lineStarts,
                             int16_t* lineLengths, int maxLines)
{
    gfx.setTextSize(textSize);
    int lines = 0;
    int len   = strlen(text);
    int pos   = 0;

    while (pos < len && lines < maxLines) {
        int bestBreak = -1;
        int i = pos;
        char buf[257];

        while (i < len) {
            int runLen = i - pos + 1;
            if (runLen > 255) runLen = 255;
            memcpy(buf, text + pos, runLen);
            buf[runLen] = '\0';
            int16_t tw = gfx.textWidth(buf);
            if (tw > maxWidth && bestBreak > pos) {
                break;
            }
            if (text[i] == ' ' || text[i] == '-') {
                bestBreak = i;
            }
            if (text[i] == '\n') {
                bestBreak = i;
                break;
            }
            i++;
        }

        int lineEnd;
        int nextPos;
        if (i >= len) {
            lineEnd = len;
            nextPos = len;
        } else if (text[i] == '\n' || (bestBreak >= pos && text[bestBreak] == '\n')) {
            int brk = (text[i] == '\n') ? i : bestBreak;
            lineEnd = brk;
            nextPos = brk + 1;
        } else if (bestBreak > pos) {
            lineEnd = bestBreak + 1;
            nextPos = bestBreak + 1;
        } else {
            lineEnd = (i > pos) ? i : pos + 1;
            nextPos = lineEnd;
        }

        lineStarts[lines]  = pos;
        lineLengths[lines] = lineEnd - pos;
        lines++;
        pos = nextPos;
    }

    if (lines == 0) {
        lineStarts[0]  = 0;
        lineLengths[0] = 0;
        lines = 1;
    }
    return lines;
}

// ── Auto-size popup based on text content ───────────────────────────────────
void UIConfirmPopup::autoSize(M5GFX& gfx) {
    const int16_t hPad         = TAB5_PADDING * 2;
    const int16_t vPad         = TAB5_PADDING;
    const int16_t titleGap     = 42;
    const int16_t btnAreaH     = 56;
    const int16_t btnGap       = 20;    // gap between Yes and No buttons
    const int16_t minW         = 260;
    const int16_t minH         = 140;
    const int16_t screenMargin = 40;
    const int16_t maxW         = TAB5_SCREEN_W - screenMargin * 2;
    const int16_t maxH         = TAB5_SCREEN_H - screenMargin * 2;

    // Measure title width
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    int16_t titleW = gfx.textWidth(_title);

    // Measure button label widths
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t yesLabelW = gfx.textWidth(_yesLabel);
    int16_t noLabelW  = gfx.textWidth(_noLabel);
    int16_t yesW = yesLabelW + 60;
    int16_t noW  = noLabelW + 60;
    if (yesW < 100) yesW = 100;
    if (noW < 100)  noW  = 100;
    int16_t totalBtnW = yesW + btnGap + noW;

    // Start with width based on title
    int16_t neededW = titleW + hPad + 40;
    if (totalBtnW + hPad > neededW) neededW = totalBtnW + hPad;

    if (neededW < minW) neededW = minW;

    // Check message width
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t rawMsgW = gfx.textWidth(_message);
    int16_t msgOneLineW = rawMsgW + hPad + 20;
    if (msgOneLineW > neededW && msgOneLineW <= maxW) {
        neededW = msgOneLineW;
    }
    if (neededW > maxW) neededW = maxW;

    // Word-wrap the message
    int16_t contentW = neededW - hPad - 10;
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(gfx, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    int16_t lineH = (int16_t)(gfx.fontHeight() * TAB5_FONT_SIZE_MD) + 4;

    // Total height
    int16_t neededH = vPad + titleGap + 10
                    + lineH * numLines + 10
                    + btnAreaH + vPad;

    if (neededH < minH) neededH = minH;
    if (neededH > maxH) neededH = maxH;

    _w = neededW;
    _h = neededH;
    _x = (TAB5_SCREEN_W - _w) / 2;
    _y = (TAB5_SCREEN_H - _h) / 2;

    _needsAutoSize = false;
}

void UIConfirmPopup::draw(M5GFX& gfx) {
    if (!_visible) return;

    if (_needsAutoSize) {
        autoSize(gfx);
    }

    // Shadow
    gfx.fillRect(_x + 4, _y + 4, _w, _h, rgb888(0x0A0A14));

    // Background
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, 8, rgb888(_bgColor));

    // Border
    gfx.drawRoundRect(_x, _y, _w, _h, 8, rgb888(_borderColor));

    // Title
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    gfx.setTextDatum(textdatum_t::top_center);
    gfx.setTextColor(rgb888(_titleColor));
    gfx.drawString(_title, _x + _w / 2, _y + TAB5_PADDING + 4);

    // Divider below title
    int16_t divY = _y + TAB5_PADDING + 38;
    gfx.drawFastHLine(_x + TAB5_PADDING, divY,
                      _w - TAB5_PADDING * 2, rgb888(Tab5Theme::DIVIDER));

    // Message — word-wrapped
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::top_center);
    gfx.setTextColor(rgb888(_textColor));

    int16_t contentW = _w - TAB5_PADDING * 2 - 10;
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(gfx, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    int16_t lineH = (int16_t)(gfx.fontHeight() * TAB5_FONT_SIZE_MD) + 4;
    int16_t msgStartY = divY + 14;

    char lineBuf[257];
    for (int i = 0; i < numLines; i++) {
        int len = lineLengths[i];
        if (len > 255) len = 255;
        memcpy(lineBuf, _message + lineStarts[i], len);
        while (len > 0 && lineBuf[len - 1] == ' ') len--;
        lineBuf[len] = '\0';
        gfx.drawString(lineBuf, _x + _w / 2, msgStartY + i * lineH);
    }

    // ── Yes / No buttons (side by side, centered at bottom) ──
    const int16_t btnGap = 20;
    gfx.setTextSize(TAB5_FONT_SIZE_MD);

    _yesBtnW = gfx.textWidth(_yesLabel) + 60;
    if (_yesBtnW < 100) _yesBtnW = 100;
    _yesBtnH = 40;

    _noBtnW = gfx.textWidth(_noLabel) + 60;
    if (_noBtnW < 100) _noBtnW = 100;
    _noBtnH = 40;

    int16_t totalBtnW = _yesBtnW + btnGap + _noBtnW;
    int16_t btnStartX = _x + (_w - totalBtnW) / 2;
    int16_t btnY = _y + _h - _yesBtnH - TAB5_PADDING;

    // No button (left)
    _noBtnX = btnStartX;
    _noBtnY = btnY;

    uint32_t noBg = _noBtnPressed ? rgb888(darken(_noBtnColor)) : rgb888(_noBtnColor);
    gfx.fillSmoothRoundRect(_noBtnX, _noBtnY, _noBtnW, _noBtnH, 6, noBg);

    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_center);
    gfx.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    gfx.drawString(_noLabel, _noBtnX + _noBtnW / 2, _noBtnY + _noBtnH / 2);

    // Yes button (right)
    _yesBtnX = btnStartX + _noBtnW + btnGap;
    _yesBtnY = btnY;

    uint32_t yesBg = _yesBtnPressed ? rgb888(darken(_yesBtnColor)) : rgb888(_yesBtnColor);
    gfx.fillSmoothRoundRect(_yesBtnX, _yesBtnY, _yesBtnW, _yesBtnH, 6, yesBg);

    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_center);
    gfx.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    gfx.drawString(_yesLabel, _yesBtnX + _yesBtnW / 2, _yesBtnY + _yesBtnH / 2);

    _dirty = false;
}

void UIConfirmPopup::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (hitTestYesBtn(tx, ty)) {
        _yesBtnPressed = true;
        _dirty = true;
    } else if (hitTestNoBtn(tx, ty)) {
        _noBtnPressed = true;
        _dirty = true;
    }
}

void UIConfirmPopup::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (_yesBtnPressed && hitTestYesBtn(tx, ty)) {
        // Yes button tapped
        _yesBtnPressed = false;
        _result = ConfirmResult::YES;
        hide();
        if (_onConfirm) _onConfirm(ConfirmResult::YES);
    } else if (_noBtnPressed && hitTestNoBtn(tx, ty)) {
        // No button tapped
        _noBtnPressed = false;
        _result = ConfirmResult::NO;
        hide();
        if (_onConfirm) _onConfirm(ConfirmResult::NO);
    } else if (!hitTest(tx, ty)) {
        // Tap outside popup — treat as No
        _yesBtnPressed = false;
        _noBtnPressed = false;
        _result = ConfirmResult::NO;
        hide();
        if (_onConfirm) _onConfirm(ConfirmResult::NO);
    } else {
        _yesBtnPressed = false;
        _noBtnPressed = false;
        _dirty = true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIScrollText  (with basic Markdown rendering)
// ═════════════════════════════════════════════════════════════════════════════

UIScrollText::UIScrollText(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint32_t bgColor, uint32_t textColor)
    : UIElement(x, y, w, h)
    , _bgColor(bgColor), _textColor(textColor)
{
    _text[0] = '\0';
}

void UIScrollText::setText(const char* text) {
    strncpy(_text, text, TAB5_SCROLLTEXT_MAX_LEN - 1);
    _text[TAB5_SCROLLTEXT_MAX_LEN - 1] = '\0';
    _needsWrap = true;
    _scrollOffset = 0;
    _dirty = true;
}

void UIScrollText::scrollTo(int16_t offset) {
    _scrollOffset = offset;
    clampScroll();
    _dirty = true;
}

void UIScrollText::scrollToBottom() {
    _scrollOffset = maxScroll();
    _dirty = true;
}

int16_t UIScrollText::totalContentHeight() const {
    int16_t total = 0;
    for (int i = 0; i < _lineCount; i++) {
        total += _lines[i].height;
    }
    return total;
}

int16_t UIScrollText::maxScroll() const {
    int16_t contentH = totalContentHeight();
    int16_t innerH = _h - TAB5_PADDING * 2;
    if (contentH <= innerH) return 0;
    return contentH - innerH;
}

void UIScrollText::clampScroll() {
    int16_t ms = maxScroll();
    if (_scrollOffset < 0) _scrollOffset = 0;
    if (_scrollOffset > ms) _scrollOffset = ms;
}

// ── Measure the display width of markdown text, ignoring marker characters ──
int16_t UIScrollText::markdownTextWidth(M5GFX& gfx, const char* text, int len,
                                        float textSize) {
    gfx.setTextSize(textSize);
    int16_t totalW = 0;
    int i = 0;
    char buf[257];

    while (i < len) {
        // Skip markdown markers — they don't contribute to visible width
        // Check for ** (bold)
        if (i + 1 < len && text[i] == '*' && text[i + 1] == '*') {
            i += 2;
            continue;
        }
        // Check for * (italic) — but not ** which we already handled
        if (text[i] == '*') {
            i += 1;
            continue;
        }
        // Check for ` (code)
        if (text[i] == '`') {
            i += 1;
            continue;
        }
        // Regular character — measure it
        int runStart = i;
        while (i < len) {
            if (text[i] == '*' || text[i] == '`') break;
            i++;
        }
        int runLen = i - runStart;
        if (runLen > 255) runLen = 255;
        memcpy(buf, text + runStart, runLen);
        buf[runLen] = '\0';
        totalW += gfx.textWidth(buf);
    }
    return totalW;
}

// ── Reflow: parse markdown blocks and word-wrap each paragraph ──
void UIScrollText::reflow(M5GFX& gfx) {
    int16_t contentW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 4;
    _lineCount = 0;

    // Compute line heights for each level
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    int16_t h1H = (int16_t)(gfx.fontHeight() * TAB5_FONT_SIZE_LG) + 10; // extra spacing
    gfx.setTextSize((_textSize + TAB5_FONT_SIZE_LG) * 0.5f);
    int16_t h2H = (int16_t)(gfx.fontHeight() * ((_textSize + TAB5_FONT_SIZE_LG) * 0.5f)) + 8;
    gfx.setTextSize(_textSize * 1.1f);
    int16_t h3H = (int16_t)(gfx.fontHeight() * (_textSize * 1.1f)) + 6;
    gfx.setTextSize(_textSize);
    int16_t normalH = (int16_t)(gfx.fontHeight() * _textSize) + 4;
    int16_t ruleH   = normalH;  // Rule takes a full line height
    int16_t bulletIndent = 28;  // Pixels to indent bullet text

    int len = strlen(_text);
    int pos = 0;

    while (pos < len && _lineCount < TAB5_SCROLLTEXT_MAX_LINES) {
        // Find end of this source line (up to \n)
        int lineEnd = pos;
        while (lineEnd < len && _text[lineEnd] != '\n') lineEnd++;

        int srcLen = lineEnd - pos;
        const char* linePtr = _text + pos;

        // ── Detect block-level markdown ──
        uint8_t heading = 0;
        bool bullet = false;
        bool rule = false;
        int contentStart = 0;  // Offset into linePtr where display text begins

        // Horizontal rule: "---", "***", "___" (3+ chars, optionally with spaces)
        if (srcLen >= 3) {
            bool isRule = true;
            char rc = 0;
            int ruleChars = 0;
            for (int j = 0; j < srcLen; j++) {
                if (linePtr[j] == ' ') continue;
                if (rc == 0) rc = linePtr[j];
                if (linePtr[j] == rc && (rc == '-' || rc == '*' || rc == '_')) {
                    ruleChars++;
                } else {
                    isRule = false;
                    break;
                }
            }
            if (isRule && ruleChars >= 3) rule = true;
        }

        // Headings: #, ##, ###
        if (!rule && srcLen >= 2 && linePtr[0] == '#') {
            if (linePtr[1] == '#' && srcLen >= 3 && linePtr[2] == '#' && srcLen >= 4 && linePtr[3] == ' ') {
                heading = 3; contentStart = 4;
            } else if (linePtr[1] == '#' && srcLen >= 3 && linePtr[2] == ' ') {
                heading = 2; contentStart = 3;
            } else if (linePtr[1] == ' ') {
                heading = 1; contentStart = 2;
            }
        }

        // Bullet: "- " or "* " at start (only if not a rule)
        if (!rule && !heading && srcLen >= 2) {
            if ((linePtr[0] == '-' || linePtr[0] == '*') && linePtr[1] == ' ') {
                bullet = true;
                contentStart = 2;
            }
        }

        // ── Empty line → blank spacer ──
        if (srcLen == 0 && !rule) {
            ScrollTextLine& sl = _lines[_lineCount];
            sl.start = pos;
            sl.length = 0;
            sl.height = normalH / 2;  // Half-height blank
            sl.heading = 0;
            sl.bullet = false;
            sl.rule = false;
            sl.textStart = 0;
            sl.textLength = 0;
            _lineCount++;
            pos = lineEnd + 1;
            continue;
        }

        // ── Horizontal rule ──
        if (rule) {
            ScrollTextLine& sl = _lines[_lineCount];
            sl.start = pos;
            sl.length = srcLen;
            sl.height = ruleH;
            sl.heading = 0;
            sl.bullet = false;
            sl.rule = true;
            sl.textStart = 0;
            sl.textLength = 0;
            _lineCount++;
            pos = lineEnd + 1;
            continue;
        }

        // ── Determine font size and available width for wrapping ──
        float fontSize;
        int16_t lineH;
        if (heading == 1)      { fontSize = TAB5_FONT_SIZE_LG; lineH = h1H; }
        else if (heading == 2) { fontSize = (_textSize + TAB5_FONT_SIZE_LG) * 0.5f; lineH = h2H; }
        else if (heading == 3) { fontSize = _textSize * 1.1f; lineH = h3H; }
        else                   { fontSize = _textSize; lineH = normalH; }

        int16_t availW = contentW;
        if (bullet) availW -= bulletIndent;

        // ── Word-wrap this paragraph's display text ──
        const char* dispText = linePtr + contentStart;
        int dispLen = srcLen - contentStart;

        gfx.setTextSize(fontSize);

        // Walk through dispText, wrapping at word boundaries
        int dPos = 0;
        bool firstWrap = true;

        while (dPos < dispLen && _lineCount < TAB5_SCROLLTEXT_MAX_LINES) {
            int bestBreak = -1;
            int di = dPos;
            char buf[257];

            while (di < dispLen) {
                // Measure visible width (skipping markdown markers)
                int runLen = di - dPos + 1;
                if (runLen > 255) runLen = 255;
                int16_t tw = markdownTextWidth(gfx, dispText + dPos, runLen, fontSize);
                if (tw > availW && bestBreak >= 0) {
                    break;
                }
                if (dispText[di] == ' ' || dispText[di] == '-') {
                    bestBreak = di;
                }
                di++;
            }

            int wrapEnd, nextDPos;
            if (di >= dispLen) {
                wrapEnd = dispLen;
                nextDPos = dispLen;
            } else if (bestBreak >= dPos) {
                wrapEnd = bestBreak + 1;
                nextDPos = bestBreak + 1;
            } else {
                wrapEnd = (di > dPos) ? di : dPos + 1;
                nextDPos = wrapEnd;
            }

            ScrollTextLine& sl = _lines[_lineCount];
            sl.start = (dispText + dPos) - _text;  // Absolute offset into _text
            sl.length = wrapEnd - dPos;
            sl.height = lineH;
            sl.heading = firstWrap ? heading : 0;  // Only first wrapped line gets heading style
            sl.bullet = firstWrap ? bullet : false;
            sl.rule = false;
            sl.textStart = sl.start;
            sl.textLength = sl.length;

            _lineCount++;
            dPos = nextDPos;
            firstWrap = false;

            // Continuation lines of bullets are also indented
            // (heading is only on first line, but bullet indent continues)
            if (bullet && !firstWrap) {
                // Keep availW the same (still indented)
            }
        }

        pos = lineEnd + 1;
    }

    if (_lineCount == 0) {
        _lines[0].start = 0;
        _lines[0].length = 0;
        _lines[0].height = normalH;
        _lines[0].heading = 0;
        _lines[0].bullet = false;
        _lines[0].rule = false;
        _lines[0].textStart = 0;
        _lines[0].textLength = 0;
        _lineCount = 1;
    }

    clampScroll();
    _needsWrap = false;
}

// ── Draw a line with inline markdown spans ──
void UIScrollText::drawMarkdownLine(M5GFX& gfx, const char* text, int len,
                                    int16_t x, int16_t y, float textSize,
                                    uint32_t defaultColor) {
    gfx.setTextSize(textSize);
    gfx.setTextDatum(textdatum_t::top_left);

    int16_t curX = x;
    int i = 0;
    char buf[257];

    while (i < len) {
        // Check for ** (bold)
        if (i + 1 < len && text[i] == '*' && text[i + 1] == '*') {
            i += 2;  // skip opening **
            int spanStart = i;
            while (i < len) {
                if (i + 1 < len && text[i] == '*' && text[i + 1] == '*') break;
                i++;
            }
            int spanLen = i - spanStart;
            if (spanLen > 255) spanLen = 255;
            memcpy(buf, text + spanStart, spanLen);
            buf[spanLen] = '\0';
            gfx.setTextColor(rgb888(_boldColor));
            gfx.setTextSize(textSize);
            gfx.drawString(buf, curX, y);
            curX += gfx.textWidth(buf);
            if (i + 1 < len && text[i] == '*' && text[i + 1] == '*') i += 2; // skip closing **
            continue;
        }

        // Check for ` (code)
        if (text[i] == '`') {
            i += 1;  // skip opening `
            int spanStart = i;
            while (i < len && text[i] != '`') i++;
            int spanLen = i - spanStart;
            if (spanLen > 255) spanLen = 255;
            memcpy(buf, text + spanStart, spanLen);
            buf[spanLen] = '\0';
            gfx.setTextSize(textSize);
            int16_t codeW = gfx.textWidth(buf);
            int16_t fh = (int16_t)(gfx.fontHeight() * textSize);
            // Code background
            gfx.fillRect(curX - 2, y, codeW + 4, fh, rgb888(_codeBgColor));
            gfx.setTextColor(rgb888(_codeColor));
            gfx.drawString(buf, curX, y);
            curX += codeW;
            if (i < len && text[i] == '`') i += 1; // skip closing `
            continue;
        }

        // Check for * (italic) — single *, not **
        if (text[i] == '*') {
            i += 1;  // skip opening *
            int spanStart = i;
            while (i < len && text[i] != '*') i++;
            int spanLen = i - spanStart;
            if (spanLen > 255) spanLen = 255;
            memcpy(buf, text + spanStart, spanLen);
            buf[spanLen] = '\0';
            gfx.setTextColor(rgb888(_italicColor));
            gfx.setTextSize(textSize);
            gfx.drawString(buf, curX, y);
            curX += gfx.textWidth(buf);
            if (i < len && text[i] == '*') i += 1; // skip closing *
            continue;
        }

        // Regular text run — up to next marker
        int runStart = i;
        while (i < len && text[i] != '*' && text[i] != '`') i++;
        int runLen = i - runStart;
        if (runLen > 255) runLen = 255;
        memcpy(buf, text + runStart, runLen);
        // Trim trailing spaces on last segment
        while (runLen > 0 && buf[runLen - 1] == ' ' && i >= len) runLen--;
        buf[runLen] = '\0';
        gfx.setTextColor(rgb888(defaultColor));
        gfx.setTextSize(textSize);
        gfx.drawString(buf, curX, y);
        curX += gfx.textWidth(buf);
    }
}

void UIScrollText::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Reflow text if needed
    if (_needsWrap) {
        reflow(gfx);
    }

    // Background fill
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));

    // Border
    gfx.drawRect(_x, _y, _w, _h, rgb888(_borderColor));

    // Inner content area (padded)
    int16_t innerX = _x + TAB5_PADDING;
    int16_t innerY = _y + TAB5_PADDING;
    int16_t innerW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 2;
    int16_t innerH = _h - TAB5_PADDING * 2;

    // Clip to content area
    gfx.setClipRect(_x + 1, _y + 1, _w - 2, _h - 2);

    int16_t bulletIndent = 28;

    // Accumulate Y position
    int16_t curY = innerY - _scrollOffset;

    for (int i = 0; i < _lineCount; i++) {
        const ScrollTextLine& sl = _lines[i];
        int16_t lineY = curY;
        curY += sl.height;

        // Skip lines outside visible area
        if (lineY + sl.height <= _y) continue;
        if (lineY >= _y + _h) break;

        // ── Horizontal rule ──
        if (sl.rule) {
            int16_t ruleY = lineY + sl.height / 2;
            gfx.drawFastHLine(innerX, ruleY, innerW, rgb888(_ruleColor));
            continue;
        }

        // ── Empty line (spacer) ──
        if (sl.textLength == 0) continue;

        // Determine text properties
        float fontSize;
        uint32_t textColor;
        int16_t drawX = innerX;

        if (sl.heading == 1) {
            fontSize = TAB5_FONT_SIZE_LG;
            textColor = _headingColor;
        } else if (sl.heading == 2) {
            fontSize = (_textSize + TAB5_FONT_SIZE_LG) * 0.5f;
            textColor = _headingColor;
        } else if (sl.heading == 3) {
            fontSize = _textSize * 1.1f;
            textColor = _headingColor;
        } else {
            fontSize = _textSize;
            textColor = _textColor;
        }

        // ── Bullet prefix ──
        if (sl.bullet) {
            gfx.setTextSize(_textSize);
            gfx.setTextDatum(textdatum_t::top_left);
            gfx.setTextColor(rgb888(_bulletColor));
            gfx.drawString("\xe2\x80\xa2", innerX + 4, lineY);  // UTF-8 bullet "•"
            drawX = innerX + bulletIndent;
        }
        // Continuation lines of bullets (not first) still get indent
        // We detect this by checking if the previous line was a bullet
        if (!sl.bullet && i > 0 && _lines[i - 1].bullet && sl.heading == 0) {
            drawX = innerX + bulletIndent;
        }

        // ── Draw text with inline markdown ──
        drawMarkdownLine(gfx, _text + sl.textStart, sl.textLength,
                         drawX, lineY, fontSize, textColor);

        // ── Heading underline for H1 ──
        if (sl.heading == 1) {
            int16_t ulY = lineY + sl.height - 4;
            gfx.drawFastHLine(innerX, ulY, innerW, rgb888(_ruleColor));
        }
    }

    // Clear clip
    gfx.clearClipRect();

    // Scrollbar (only if content overflows)
    int16_t contentH = totalContentHeight();
    if (contentH > innerH) {
        int16_t sbX = _x + _w - TAB5_LIST_SCROLLBAR_W - 1;
        int16_t sbAreaH = _h - 2;

        // Scrollbar track
        gfx.fillRect(sbX, _y + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                     rgb888(darken(_bgColor, 60)));

        // Scrollbar thumb
        float visibleRatio = (float)innerH / (float)contentH;
        int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
        if (thumbH < 20) thumbH = 20;

        int16_t ms = maxScroll();
        float scrollRatio = (ms > 0) ? (float)_scrollOffset / (float)ms : 0.0f;
        int16_t thumbY = _y + 1 + (int16_t)((sbAreaH - thumbH) * scrollRatio);

        gfx.fillSmoothRoundRect(sbX, thumbY, TAB5_LIST_SCROLLBAR_W, thumbH,
                                 3, rgb888(Tab5Theme::TEXT_DISABLED));
    }

    _dirty = false;
}

void UIScrollText::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dragging = false;
    _wasDrag = false;
    _touchStartY = ty;
    _touchDownY = ty;
    _scrollStart = _scrollOffset;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIScrollText::handleTouchMove(int16_t tx, int16_t ty) {
    if (!_pressed) return;

    int16_t dy = _touchStartY - ty;

    // Check if movement exceeds drag threshold
    int16_t totalDy = ty - _touchDownY;
    if (!_wasDrag && (totalDy > DRAG_THRESHOLD || totalDy < -DRAG_THRESHOLD)) {
        _wasDrag = true;
    }

    if (_wasDrag) {
        _scrollOffset = _scrollStart + dy;
        clampScroll();
        _dirty = true;
    }
}

void UIScrollText::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_pressed) return;
    _pressed = false;
    _dragging = false;
    _wasDrag = false;
    if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIList
// ═════════════════════════════════════════════════════════════════════════════

UIList::UIList(int16_t x, int16_t y, int16_t w, int16_t h,
               uint32_t bgColor, uint32_t textColor, uint32_t selectColor)
    : UIElement(x, y, w, h)
    , _bgColor(bgColor), _textColor(textColor), _selectColor(selectColor)
{
}

int UIList::addItem(const char* text) {
    if (_itemCount >= TAB5_LIST_MAX_ITEMS) return -1;
    _items[_itemCount] = UIListItem();  // reset to defaults
    strncpy(_items[_itemCount].text, text, sizeof(_items[0].text) - 1);
    _items[_itemCount].text[sizeof(_items[0].text) - 1] = '\0';
    _dirty = true;
    return _itemCount++;
}

int UIList::addItem(const char* text, const char* iconChar,
                    uint32_t iconColor, bool circle,
                    uint32_t iconBorderColor, uint32_t iconCharColor) {
    if (_itemCount >= TAB5_LIST_MAX_ITEMS) return -1;
    _items[_itemCount] = UIListItem();
    strncpy(_items[_itemCount].text, text, sizeof(_items[0].text) - 1);
    _items[_itemCount].text[sizeof(_items[0].text) - 1] = '\0';
    _items[_itemCount].hasIcon = true;
    _items[_itemCount].iconCircle = circle;
    strncpy(_items[_itemCount].iconChar, iconChar, sizeof(_items[0].iconChar) - 1);
    _items[_itemCount].iconChar[sizeof(_items[0].iconChar) - 1] = '\0';
    _items[_itemCount].iconColor = iconColor;
    _items[_itemCount].iconBorderColor = iconBorderColor;
    _items[_itemCount].iconCharColor = iconCharColor;
    _dirty = true;
    return _itemCount++;
}

void UIList::setItemIcon(int index, const char* iconChar,
                         uint32_t iconColor, bool circle,
                         uint32_t iconBorderColor, uint32_t iconCharColor) {
    if (index < 0 || index >= _itemCount) return;
    _items[index].hasIcon = true;
    _items[index].iconCircle = circle;
    strncpy(_items[index].iconChar, iconChar, sizeof(_items[0].iconChar) - 1);
    _items[index].iconChar[sizeof(_items[0].iconChar) - 1] = '\0';
    _items[index].iconColor = iconColor;
    _items[index].iconBorderColor = iconBorderColor;
    _items[index].iconCharColor = iconCharColor;
    _dirty = true;
}

void UIList::clearItemIcon(int index) {
    if (index < 0 || index >= _itemCount) return;
    _items[index].hasIcon = false;
    _items[index].iconChar[0] = '\0';
    _dirty = true;
}

void UIList::removeItem(int index) {
    if (index < 0 || index >= _itemCount) return;
    for (int i = index; i < _itemCount - 1; i++) {
        _items[i] = _items[i + 1];
    }
    _itemCount--;
    if (_selectedIndex == index) _selectedIndex = -1;
    else if (_selectedIndex > index) _selectedIndex--;
    clampScroll();
    _dirty = true;
}

void UIList::clearItems() {
    _itemCount = 0;
    _selectedIndex = -1;
    _scrollOffset = 0;
    _dirty = true;
}

void UIList::setItemText(int index, const char* text) {
    if (index < 0 || index >= _itemCount) return;
    strncpy(_items[index].text, text, sizeof(_items[0].text) - 1);
    _items[index].text[sizeof(_items[0].text) - 1] = '\0';
    _dirty = true;
}

void UIList::setItemEnabled(int index, bool enabled) {
    if (index < 0 || index >= _itemCount) return;
    _items[index].enabled = enabled;
    _dirty = true;
}

const char* UIList::getSelectedText() const {
    if (_selectedIndex < 0 || _selectedIndex >= _itemCount) return "";
    return _items[_selectedIndex].text;
}

void UIList::setSelectedIndex(int index) {
    if (index < -1 || index >= _itemCount) return;
    _selectedIndex = index;
    _dirty = true;
}

void UIList::clearSelection() {
    _selectedIndex = -1;
    _dirty = true;
}

void UIList::scrollTo(int16_t offset) {
    _scrollOffset = offset;
    clampScroll();
    _dirty = true;
}

void UIList::scrollToItem(int index) {
    if (index < 0 || index >= _itemCount) return;
    int16_t itemTop = index * _itemH;
    int16_t itemBottom = itemTop + _itemH;

    // If item is above visible area, scroll up to it
    if (itemTop < _scrollOffset) {
        _scrollOffset = itemTop;
    }
    // If item is below visible area, scroll down
    else if (itemBottom > _scrollOffset + _h) {
        _scrollOffset = itemBottom - _h;
    }
    clampScroll();
    _dirty = true;
}

int16_t UIList::maxScroll() const {
    int16_t contentH = totalContentHeight();
    if (contentH <= _h) return 0;
    return contentH - _h;
}

void UIList::clampScroll() {
    int16_t ms = maxScroll();
    if (_scrollOffset < 0) _scrollOffset = 0;
    if (_scrollOffset > ms) _scrollOffset = ms;
}

int UIList::itemAtY(int16_t ty) const {
    if (ty < _y || ty >= _y + _h) return -1;
    int16_t relY = ty - _y + _scrollOffset;
    int idx = relY / _itemH;
    if (idx < 0 || idx >= _itemCount) return -1;
    return idx;
}

void UIList::draw(M5GFX& gfx) {
    if (!_visible) return;

    // Auto-scale item height from text size if enabled
    if (_autoScale) {
        gfx.setTextSize(_textSize);
        int16_t fh = (int16_t)(gfx.fontHeight() * _textSize);
        _itemH = fh + TAB5_PADDING * 2;  // text height + top/bottom padding
        if (_itemH < 32) _itemH = 32;    // minimum
    }

    // Icon size derived from item height (fits inside with padding)
    int16_t iconSize = _itemH - TAB5_PADDING;
    if (iconSize < 16) iconSize = 16;

    // Background fill
    gfx.fillRect(_x, _y, _w, _h, rgb888(_bgColor));

    // Border
    gfx.drawRect(_x, _y, _w, _h, rgb888(_borderColor));

    // Clip region
    gfx.setClipRect(_x + 1, _y + 1, _w - 2, _h - 2);

    // Draw visible items
    for (int i = 0; i < _itemCount; i++) {
        int16_t itemY = _y + (i * _itemH) - _scrollOffset;

        // Skip items fully outside visible area
        if (itemY + _itemH <= _y || itemY >= _y + _h) continue;

        // Selected highlight
        if (i == _selectedIndex) {
            gfx.fillRect(_x + 1, itemY, _w - TAB5_LIST_SCROLLBAR_W - 2, _itemH,
                         rgb888(_selectColor));
        }

        // Item text
        gfx.setTextSize(_textSize);
        gfx.setTextDatum(textdatum_t::middle_left);

        uint32_t tc;
        if (!_items[i].enabled) {
            tc = rgb888(Tab5Theme::TEXT_DISABLED);
        } else if (i == _selectedIndex) {
            tc = rgb888(Tab5Theme::TEXT_PRIMARY);
        } else {
            tc = rgb888(_textColor);
        }
        gfx.setTextColor(tc);
        gfx.drawString(_items[i].text, _x + TAB5_PADDING, itemY + _itemH / 2);

        // Right-aligned icon (if present)
        if (_items[i].hasIcon) {
            int16_t iconX = _x + _w - TAB5_LIST_SCROLLBAR_W - TAB5_PADDING - iconSize - 2;
            int16_t iconY = itemY + (_itemH - iconSize) / 2;

            if (_items[i].iconCircle) {
                // Circle icon
                int16_t cr = iconSize / 2;
                int16_t cx = iconX + cr;
                int16_t cy = iconY + cr;
                gfx.fillCircle(cx, cy, cr, rgb888(_items[i].iconColor));
                gfx.drawCircle(cx, cy, cr, rgb888(_items[i].iconBorderColor));

                // Icon character
                if (_items[i].iconChar[0] != '\0') {
                    gfx.setTextSize(_textSize * 0.8f);
                    gfx.setTextDatum(textdatum_t::middle_center);
                    gfx.setTextColor(rgb888(_items[i].iconCharColor));
                    gfx.drawString(_items[i].iconChar, cx, cy);
                }
            } else {
                // Square icon (rounded)
                gfx.fillSmoothRoundRect(iconX, iconY, iconSize, iconSize, 4,
                                         rgb888(_items[i].iconColor));
                gfx.drawRoundRect(iconX, iconY, iconSize, iconSize, 4,
                                   rgb888(_items[i].iconBorderColor));

                // Icon character
                if (_items[i].iconChar[0] != '\0') {
                    gfx.setTextSize(_textSize * 0.8f);
                    gfx.setTextDatum(textdatum_t::middle_center);
                    gfx.setTextColor(rgb888(_items[i].iconCharColor));
                    gfx.drawString(_items[i].iconChar,
                                   iconX + iconSize / 2, iconY + iconSize / 2);
                }
            }
        }

        // Divider between items
        if (i < _itemCount - 1) {
            int16_t divY = itemY + _itemH - 1;
            gfx.drawFastHLine(_x + TAB5_PADDING, divY,
                              _w - TAB5_LIST_SCROLLBAR_W - TAB5_PADDING * 2,
                              rgb888(Tab5Theme::DIVIDER));
        }
    }

    // Clear clip
    gfx.clearClipRect();

    // Scrollbar (only if content overflows)
    int16_t contentH = totalContentHeight();
    if (contentH > _h) {
        int16_t sbX = _x + _w - TAB5_LIST_SCROLLBAR_W - 1;
        int16_t sbAreaH = _h - 2;

        // Scrollbar track
        gfx.fillRect(sbX, _y + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                     rgb888(darken(_bgColor, 60)));

        // Scrollbar thumb
        float visibleRatio = (float)_h / (float)contentH;
        int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
        if (thumbH < 20) thumbH = 20;

        float scrollRatio = (float)_scrollOffset / (float)maxScroll();
        int16_t thumbY = _y + 1 + (int16_t)((sbAreaH - thumbH) * scrollRatio);

        gfx.fillSmoothRoundRect(sbX, thumbY, TAB5_LIST_SCROLLBAR_W, thumbH,
                                 3, rgb888(Tab5Theme::TEXT_DISABLED));
    }

    _dirty = false;
}

void UIList::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dragging = false;
    _wasDrag = false;
    _touchStartY = ty;
    _touchDownY = ty;
    _scrollStart = _scrollOffset;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIList::handleTouchMove(int16_t tx, int16_t ty) {
    if (!_pressed) return;

    int16_t dy = _touchStartY - ty;

    // Check if movement exceeds drag threshold
    int16_t totalDy = ty - _touchDownY;
    if (!_wasDrag && (totalDy > DRAG_THRESHOLD || totalDy < -DRAG_THRESHOLD)) {
        _wasDrag = true;
    }

    if (_wasDrag) {
        _scrollOffset = _scrollStart + dy;
        clampScroll();
        _dirty = true;
    }
}

void UIList::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_pressed) return;
    _pressed = false;

    // If it was a tap (not a drag), select the item
    if (!_wasDrag) {
        int idx = itemAtY(ty);
        if (idx >= 0 && idx < _itemCount && _items[idx].enabled) {
            _selectedIndex = idx;
            _dirty = true;
            if (_onSelect) _onSelect(idx, _items[idx].text);
        }
    }

    _dragging = false;
    _wasDrag = false;
    if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIManager
// ═════════════════════════════════════════════════════════════════════════════

UIManager::UIManager(M5GFX& gfx)
    : _gfx(gfx) {}

void UIManager::addElement(UIElement* element) {
    _elements.push_back(element);
}

void UIManager::removeElement(UIElement* element) {
    _elements.erase(
        std::remove(_elements.begin(), _elements.end(), element),
        _elements.end()
    );
}

void UIManager::clearElements() {
    _elements.clear();
}

void UIManager::setBackground(uint32_t color) {
    _bgColor = color;
}

void UIManager::clearScreen() {
    _gfx.fillScreen(rgb888(_bgColor));
}

void UIManager::drawAll() {
    _gfx.startWrite();
    for (auto* elem : _elements) {
        if (elem->isVisible()) {
            elem->draw(_gfx);
            elem->setDirty(false);
        }
    }
    _gfx.endWrite();
}

void UIManager::drawDirty() {
    _gfx.startWrite();
    for (auto* elem : _elements) {
        if (!elem->isVisible()) continue;

        if (elem->isTabView()) {
            UITabView* tv = static_cast<UITabView*>(elem);
            if (tv->isDirty()) {
                // Full redraw (page switch, tab bar change, etc.)
                tv->draw(_gfx);
                tv->setDirty(false);
            } else if (tv->hasActiveDirtyChild()) {
                // Partial redraw — only dirty children, no background clear
                tv->drawDirtyChildren(_gfx);
            }
        } else if (elem->isDirty()) {
            elem->draw(_gfx);
            elem->setDirty(false);
        }
    }
    _gfx.endWrite();
}

UIElement* UIManager::findByTag(const char* tag) {
    for (auto* elem : _elements) {
        if (strcmp(elem->getTag(), tag) == 0) return elem;
    }
    return nullptr;
}

void UIManager::update() {
    unsigned long now = millis();
    if (now - _lastTouchTime < TOUCH_DEBOUNCE_MS) return;

    // Check if any modal overlay is open (keyboard, menu, or popup) — it gets exclusive touch priority
    UIElement* modalElem = nullptr;
    for (auto* elem : _elements) {
        if ((elem->isKeyboard() || elem->isMenu() || elem->isPopup()) && elem->isVisible()) {
            modalElem = elem;
            // Keyboard takes priority over others if somehow both open
            if (elem->isKeyboard()) break;
        }
    }

    lgfx::touch_point_t tp;
    uint_fast8_t count = _gfx.getTouch(&tp, 1);

    if (count > 0) {
        int16_t tx = (int16_t)tp.x;
        int16_t ty = (int16_t)tp.y;

        if (!_wasTouched) {
            _touchedElem = nullptr;
            _touchStartX = tx;
            _touchStartY = ty;

            // If a modal overlay is open, it captures all touch
            if (modalElem) {
                _touchedElem = modalElem;
                modalElem->handleTouchDown(tx, ty);
            } else {
                // Normal hit-testing (reverse for z-order)
                for (int i = (int)_elements.size() - 1; i >= 0; --i) {
                    UIElement* elem = _elements[i];
                    if (!elem->isVisible() || !elem->isEnabled()) continue;

                    bool hit = elem->isCircleIcon()
                             ? static_cast<UIIconCircle*>(elem)->hitTestCircle(tx, ty)
                             : elem->hitTest(tx, ty);

                    if (hit) {
                        _touchedElem = elem;
                        elem->handleTouchDown(tx, ty);
                        break;
                    }
                }
            }
            _wasTouched = true;
        } else if (_touchedElem) {
            // Sustained touch — dispatch move event
            if (tx != _lastTouchX || ty != _lastTouchY) {
                _touchedElem->handleTouchMove(tx, ty);
            }
        }

        _lastTouchX = tx;
        _lastTouchY = ty;
        _lastTouchTime = now;
    } else {
        // Touch released
        if (_wasTouched && _touchedElem) {
            bool wasModal = (_touchedElem->isMenu() || _touchedElem->isKeyboard() || _touchedElem->isPopup())
                          && _touchedElem->isVisible();
            _touchedElem->handleTouchUp(_lastTouchX, _lastTouchY);

            // If a modal overlay just closed, redraw everything beneath it
            if (wasModal && !_touchedElem->isVisible()) {
                for (auto* e : _elements) {
                    e->setDirty(true);
                }
                clearScreen();
            }
            _touchedElem = nullptr;
        }
        _wasTouched = false;
        _lastTouchTime = now;
    }

    // Redraw dirty elements
    drawDirty();
}
