/*******************************************************************************
 * Tab5UI.cpp - Touchscreen UI Library for M5Stack Tab5
 *
 * Implementation of all UI widget classes.
 ******************************************************************************/
#include "Tab5UI.h"
#include <string.h>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  Tab5UI Runtime Screen Dimensions
// ═════════════════════════════════════════════════════════════════════════════

static int16_t _tab5ScreenW = TAB5_SCREEN_W;  // Default: landscape 1280
static int16_t _tab5ScreenH = TAB5_SCREEN_H;  // Default: landscape 720

void Tab5UI::init(M5GFX& gfx) {
    _tab5ScreenW = (int16_t)gfx.width();
    _tab5ScreenH = (int16_t)gfx.height();
}

int16_t Tab5UI::screenW() { return _tab5ScreenW; }
int16_t Tab5UI::screenH() { return _tab5ScreenH; }

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

// ─────────────────────────────────────────────────────────────────────────────
//  Shared off-screen sprite for flicker-free drawing
// ─────────────────────────────────────────────────────────────────────────────
// A single M5Canvas is lazily allocated in PSRAM and reused by any widget
// that needs double-buffered rendering.  The sprite is resized on demand.
// Using a shared sprite avoids allocating/freeing memory every frame.
static M5Canvas* _sharedSprite = nullptr;
static int16_t   _spriteW = 0;
static int16_t   _spriteH = 0;

// Acquire the shared sprite sized to at least (w × h).
// Returns nullptr if PSRAM allocation fails (caller should fall back to
// direct drawing in that case).
static M5Canvas* acquireSprite(LovyanGFX* parent, int16_t w, int16_t h) {
    // TAB5_RENDER_MODE: 0=auto, 1=sprite always, 2=direct always
#if TAB5_RENDER_MODE == 2
    // Force direct rendering — skip sprite entirely
    (void)parent; (void)w; (void)h;
    return nullptr;
#else
    // Cap at full-screen size (1280×720 = 921600 px ≈ 1.8 MB at 16bpp).
    // In sprite-forced mode (1), skip the cap — the user knows their budget.
#if TAB5_RENDER_MODE != 1
    if ((int32_t)w * h > 921600L) return nullptr;
#endif

    if (!_sharedSprite) {
        _sharedSprite = new (std::nothrow) M5Canvas(parent);
        if (!_sharedSprite) return nullptr;
        _sharedSprite->setColorDepth(16);  // RGB565 — 2 bytes per pixel
        _sharedSprite->setPsram(true);     // Allocate buffer in PSRAM
    } else {
        _sharedSprite->setColorDepth(16);
    }

    // Resize if needed (createSprite frees previous buffer internally)
    if (w != _spriteW || h != _spriteH) {
        _sharedSprite->deleteSprite();
        if (!_sharedSprite->createSprite(w, h)) {
            _spriteW = 0;
            _spriteH = 0;
            return nullptr;  // Allocation failed
        }
        _spriteW = w;
        _spriteH = h;
    }

    // Inherit the font from the parent display so text renders at the
    // correct size.  M5Canvas starts with the tiny default built-in font;
    // without this, all sprite-rendered text appears much smaller.
    _sharedSprite->setFont(parent->getFont());

    return _sharedSprite;
#endif  // TAB5_RENDER_MODE != 2
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

void UILabel::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Always clear the label area first to prevent old text from showing
    // through when the label text changes.  Use the explicit background
    // color when set, otherwise the default dark background.
    gfx.fillRect(_x, _y, _w, _h,
                 rgb888(_hasBg ? _bgColor : Tab5Theme::BG_DARK));

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

void UIButton::draw(LovyanGFX& gfx) {
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
//  UIIconButton
// ═════════════════════════════════════════════════════════════════════════════

UIIconButton::UIIconButton(int16_t x, int16_t y, int16_t w, int16_t h,
                           const char* label, const uint8_t* iconData,
                           uint32_t iconSize, uint32_t bgColor,
                           uint32_t textColor, float textSize)
    : UIElement(x, y, w, h)
    , _iconData(iconData)
    , _iconSize(iconSize)
    , _bgColor(bgColor)
    , _pressedColor(darken(bgColor))
    , _textColor(textColor)
    , _textSize(textSize)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
}

void UIIconButton::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UIIconButton::draw(LovyanGFX& gfx) {
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

    // Draw icon if available, otherwise fall back to text label
    if (_iconData && _iconSize > 0) {
        // Center the 32×32 icon in the button
        int16_t ix = _x + (_w - 32) / 2;
        int16_t iy = _y + (_h - 32) / 2;
        gfx.drawPng(_iconData, _iconSize, ix, iy, 32, 32);
    } else {
        // Text fallback (same as UIButton)
        gfx.setTextSize(_textSize);
        gfx.setTextDatum(textdatum_t::middle_center);
        uint32_t tc = _enabled ? rgb888(_textColor) : rgb888(Tab5Theme::TEXT_DISABLED);
        gfx.setTextColor(tc);
        gfx.drawString(_label, _x + _w / 2, _y + _h / 2);
    }

    _dirty = false;
}

void UIIconButton::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIIconButton::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UISlider
// ═════════════════════════════════════════════════════════════════════════════

UISlider::UISlider(int16_t x, int16_t y, int16_t w, int16_t h,
                   int minVal, int maxVal, int value,
                   uint32_t trackColor, uint32_t fillColor, uint32_t thumbColor)
    : UIElement(x, y, w, h)
    , _minVal(minVal)
    , _maxVal(maxVal)
    , _value(value)
    , _trackColor(trackColor)
    , _fillColor(fillColor)
    , _thumbColor(thumbColor)
{
    if (_value < _minVal) _value = _minVal;
    if (_value > _maxVal) _value = _maxVal;
}

void UISlider::setValue(int v) {
    if (v < _minVal) v = _minVal;
    if (v > _maxVal) v = _maxVal;
    if (v != _value) {
        _value = v;
        _dirty = true;
    }
}

void UISlider::setRange(int minVal, int maxVal) {
    _minVal = minVal;
    _maxVal = maxVal;
    if (_value < _minVal) _value = _minVal;
    if (_value > _maxVal) _value = _maxVal;
    _dirty = true;
}

void UISlider::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UISlider::_updateFromTouch(int16_t tx) {
    // Track area: inset by thumb radius on each side
    int16_t trackLeft  = _x + _thumbR;
    int16_t trackRight = _x + _w - _thumbR;
    int16_t trackW     = trackRight - trackLeft;
    if (trackW <= 0) return;

    // Clamp touch to track bounds
    int16_t clamped = tx;
    if (clamped < trackLeft)  clamped = trackLeft;
    if (clamped > trackRight) clamped = trackRight;

    // Map position to value
    int range = _maxVal - _minVal;
    int newVal = _minVal + (int)(((int32_t)(clamped - trackLeft) * range + trackW / 2) / trackW);
    if (newVal < _minVal) newVal = _minVal;
    if (newVal > _maxVal) newVal = _maxVal;

    if (newVal != _value) {
        _value = newVal;
        _dirty = true;
        if (_onChange) _onChange(_value);
    }
}

void UISlider::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // ── Try sprite-buffered rendering for flicker-free drag ──
    M5Canvas* spr = acquireSprite(&gfx, _w, _h);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Clear background area
    dst.fillRect(ox, oy, _w, _h, rgb888(Tab5Theme::BG_DARK));

    // Optional label (drawn in top portion of the widget area)
    int16_t labelOffset = 0;
    if (_showLabel && _label[0] != '\0') {
        labelOffset = 22;
        dst.setTextSize(TAB5_FONT_SIZE_SM);
        dst.setTextDatum(textdatum_t::top_left);
        dst.setTextColor(rgb888(Tab5Theme::TEXT_SECONDARY));
        dst.drawString(_label, ox, oy);
    }

    int16_t trackLeft  = ox + _thumbR;
    int16_t trackRight = ox + _w - _thumbR;
    int16_t trackW     = trackRight - trackLeft;
    int16_t sliderCenterY = oy + labelOffset + (_h - labelOffset) / 2;
    int16_t trackY     = sliderCenterY - _trackH / 2;

    // Value label area (takes ~40px from right if enabled)
    int16_t labelW = 0;
    if (_showValue) {
        labelW = 50;
        trackRight = ox + _w - _thumbR - labelW;
        trackW = trackRight - trackLeft;
    }

    // Draw background track (full width, rounded)
    int16_t trackR = _trackH / 2;
    dst.fillSmoothRoundRect(trackLeft, trackY, trackW, _trackH, trackR,
                            rgb888(_trackColor));

    // Calculate thumb position
    float ratio = 0.0f;
    if (_maxVal > _minVal) {
        ratio = (float)(_value - _minVal) / (float)(_maxVal - _minVal);
    }
    int16_t thumbX = trackLeft + (int16_t)(ratio * trackW);

    // Draw filled portion (left of thumb)
    if (thumbX > trackLeft) {
        int16_t fillW = thumbX - trackLeft;
        dst.fillSmoothRoundRect(trackLeft, trackY, fillW, _trackH, trackR,
                                rgb888(_fillColor));
    }

    // Draw thumb circle
    uint32_t tc = _dragging ? rgb888(darken(_thumbColor)) : rgb888(_thumbColor);
    dst.fillSmoothCircle(thumbX, sliderCenterY, _thumbR, tc);

    // Optional border on thumb
    dst.drawCircle(thumbX, sliderCenterY, _thumbR, rgb888(darken(_fillColor)));

    // Value text
    if (_showValue) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", _value);
        dst.setTextSize(TAB5_FONT_SIZE_SM);
        dst.setTextDatum(textdatum_t::middle_left);
        dst.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
        dst.drawString(buf, ox + _w - labelW + 8, sliderCenterY);
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
    }

    _dirty = false;
}

void UISlider::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _dragging = true;
    _pressed = true;
    _updateFromTouch(tx);
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UISlider::handleTouchMove(int16_t tx, int16_t ty) {
    if (_dragging) {
        _updateFromTouch(tx);
    }
}

void UISlider::handleTouchUp(int16_t tx, int16_t ty) {
    if (_dragging) {
        _dragging = false;
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

void UITitleBar::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Auto-adjust width to match runtime screen width
    _w = Tab5UI::screenW();

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

void UIStatusBar::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Auto-adjust position and width for runtime screen dimensions
    _w = Tab5UI::screenW();
    _y = Tab5UI::screenH() - _h;

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

void UITextRow::draw(LovyanGFX& gfx) {
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

void UIIconSquare::draw(LovyanGFX& gfx) {
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

void UIIconCircle::draw(LovyanGFX& gfx) {
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

void UIMenu::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Sprite covers menu + shadow (shadow offset +3,+3)
    int16_t sprW = _w + 3;
    int16_t sprH = _h + 3;
    M5Canvas* spr = acquireSprite(&gfx, sprW, sprH);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Clear sprite background
    if (spr) dst.fillRect(0, 0, sprW, sprH, rgb888(Tab5Theme::BG_DARK));

    // Shadow (offset dark rect)
    dst.fillRect(ox + 3, oy + 3, _w, _h, rgb888(0x0A0A14));

    // Background
    dst.fillSmoothRoundRect(ox, oy, _w, _h, 6, rgb888(_bgColor));

    // Border
    dst.drawRoundRect(ox, oy, _w, _h, 6, rgb888(_borderColor));

    // Items
    int16_t yOff = oy + TAB5_PADDING;
    for (int i = 0; i < _itemCount; ++i) {
        const UIMenuItem& item = _items[i];

        if (item.separator) {
            // Horizontal divider
            int16_t lineY = yOff + TAB5_PADDING / 2;
            dst.drawFastHLine(ox + TAB5_PADDING, lineY,
                              _w - TAB5_PADDING * 2,
                              rgb888(Tab5Theme::DIVIDER));
            yOff += TAB5_PADDING + 1;
            continue;
        }

        // Highlight for pressed item
        if (i == _pressedIndex && item.enabled) {
            dst.fillRect(ox + 2, yOff, _w - 4, TAB5_MENU_ITEM_H,
                         rgb888(_hlColor));
        }

        // Label
        dst.setTextSize(TAB5_FONT_SIZE_MD);
        dst.setTextDatum(textdatum_t::middle_left);

        uint32_t tc = item.enabled
                    ? rgb888(_textColor)
                    : rgb888(Tab5Theme::TEXT_DISABLED);
        // When pressed, keep text white for contrast on highlight
        if (i == _pressedIndex && item.enabled) {
            tc = rgb888(Tab5Theme::TEXT_PRIMARY);
        }
        dst.setTextColor(tc);
        dst.drawString(item.label, ox + TAB5_PADDING, yOff + TAB5_MENU_ITEM_H / 2);

        yOff += TAB5_MENU_ITEM_H;
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
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

        // Row 3: 123 + Space + . + Done + Enter
        _colsLower[3] = 5;
        setKey(_keysLower[3][0], "123",  0, 1.4f, Tab5Theme::BG_MEDIUM);
        setKey(_keysLower[3][1], " ",  ' ', 5.0f, Tab5Theme::SURFACE);      // Space bar
        setKey(_keysLower[3][2], ".",  '.', 1.0f, Tab5Theme::SURFACE);
        setKey(_keysLower[3][3], "Done", '\n', 1.6f, Tab5Theme::PRIMARY);
        setKey(_keysLower[3][4], "Ent",  '\r', 1.2f, Tab5Theme::BG_MEDIUM);  // Enter
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
        setKey(_keysUpper[3][4], "Ent",  '\r', 1.2f, Tab5Theme::BG_MEDIUM);
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
        setKey(_keysSymbols[3][4], "Ent",  '\r', 1.2f, Tab5Theme::BG_MEDIUM);
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
    // Reposition for current screen dimensions
    _w = Tab5UI::screenW();
    _y = Tab5UI::screenH() - TAB5_KB_H;
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
    // Dynamic unit width: scale from screen width (fits 10 unit-keys + gaps + padding)
    float unitW = (_w - TAB5_PADDING * 2) / 10.2f;
    if (unitW > (float)(TAB5_KB_KEY_W + TAB5_KB_KEY_GAP))
        unitW = (float)(TAB5_KB_KEY_W + TAB5_KB_KEY_GAP);  // Cap at landscape size
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

void UIKeyboard::drawKey(LovyanGFX& gfx, int row, int col, bool pressed) {
    if (row < 0 || col < 0 || row >= TAB5_KB_ROWS || col >= _cols[row]) return;

    int16_t kx, ky, kw, kh;
    keyRect(row, col, kx, ky, kw, kh);

    const UIKey& key = _keys[row][col];
    uint32_t bg = pressed
                ? rgb888(darken(key.bgColor, 30))
                : rgb888(key.bgColor);

    gfx.startWrite();
    gfx.fillSmoothRoundRect(kx, ky, kw, kh, 4, bg);
    gfx.setTextSize(TAB5_FONT_SIZE_MD);
    gfx.setTextDatum(textdatum_t::middle_center);
    gfx.setTextColor(rgb888(_textColor));
    gfx.drawString(key.label, kx + kw / 2, ky + kh / 2);
    gfx.endWrite();
}

void UIKeyboard::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Cache display pointer so touch handlers can do single-key redraws
    _lastDisplay = &gfx;

    // ── Try sprite-buffered rendering for flicker-free key presses ──
    M5Canvas* spr = acquireSprite(&gfx, _w, _h);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Background panel
    dst.fillRect(ox, oy, _w, _h, rgb888(_bgColor));
    // Top border
    dst.drawFastHLine(ox, oy, _w, rgb888(Tab5Theme::BORDER));

    // Draw each key
    for (int r = 0; r < TAB5_KB_ROWS; ++r) {
        for (int c = 0; c < _cols[r]; ++c) {
            int16_t kx, ky, kw, kh;
            keyRect(r, c, kx, ky, kw, kh);
            // Offset into sprite coordinates
            if (spr) { kx -= _x; ky -= _y; }

            const UIKey& key = _keys[r][c];
            bool isPressed = (r == _pressedRow && c == _pressedCol);

            uint32_t bg = isPressed
                        ? rgb888(darken(key.bgColor, 30))
                        : rgb888(key.bgColor);

            dst.fillSmoothRoundRect(kx, ky, kw, kh, 4, bg);

            // Key label
            dst.setTextSize(TAB5_FONT_SIZE_MD);
            dst.setTextDatum(textdatum_t::middle_center);
            dst.setTextColor(rgb888(_textColor));
            dst.drawString(key.label, kx + kw / 2, ky + kh / 2);
        }
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
    }

    _dirty = false;
}

void UIKeyboard::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    int row, col;
    if (keyAt(tx, ty, row, col)) {
        _pressedRow = row;
        _pressedCol = col;
        // Draw only the pressed key highlight directly — no full redraw
        if (_lastDisplay) drawKey(*_lastDisplay, row, col, true);
    }
}

void UIKeyboard::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    int prevRow = _pressedRow;
    int prevCol = _pressedCol;
    _pressedRow = -1;
    _pressedCol = -1;

    int row, col;
    if (keyAt(tx, ty, row, col) && row == prevRow && col == prevCol) {
        const UIKey& key = _keys[row][col];

        if (key.value != 0) {
            // Regular character or special (backspace, enter, done)
            if (_onKey) _onKey(key.value);

            // After typing a letter in UPPER mode, revert to LOWER
            if (_layer == UPPER && key.value >= 'A' && key.value <= 'Z') {
                setLayer(LOWER);  // setLayer sets _dirty for full redraw
            } else {
                // Unhighlight only the released key — no full redraw
                if (_lastDisplay) drawKey(*_lastDisplay, row, col, false);
            }
        } else {
            // Special mode-switch keys (value == 0, label determines action)
            if (strcmp(key.label, "Shft") == 0) {
                setLayer(_layer == UPPER ? LOWER : UPPER);
            } else if (strcmp(key.label, "123") == 0) {
                setLayer(SYMBOLS);
            } else if (strcmp(key.label, "ABC") == 0) {
                setLayer(LOWER);
            }
            // setLayer already sets _dirty for full redraw
        }
    } else if (prevRow >= 0 && prevCol >= 0) {
        // Finger moved off the key — just unhighlight it
        if (_lastDisplay) drawKey(*_lastDisplay, prevRow, prevCol, false);
    }
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
        // Hide key pressed (legacy)
        blur();
        return;
    }
    if (ch == '\r') {
        // Enter key — for single-line, just hide keyboard
        blur();
        return;
    }
    if (ch == '\n') {
        // Done key — submit and hide
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

void UITextInput::draw(LovyanGFX& gfx) {
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

void UITabView::drawDirtyChildren(LovyanGFX& gfx) {
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

void UITabView::drawTabBar(LovyanGFX& gfx) {
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

void UITabView::draw(LovyanGFX& gfx) {
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

    // Check if any child on the active page is a modal overlay (menu/popup)
    // If so, route ALL touch to it exclusively (same pattern as UIManager)
    if (_activePage >= 0 && _activePage < _pageCount) {
        UITabPage& page = _pages[_activePage];
        for (int i = page.childCount - 1; i >= 0; --i) {
            UIElement* child = page.children[i];
            if (!child || !child->isVisible()) continue;
            if (child->isMenu() || child->isPopup()) {
                _touchedChild = child;
                child->handleTouchDown(tx, ty);
                return;
            }
        }
    }

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
        // NOTE: Do NOT propagate child dirty → TabView dirty here.
        // UIManager::drawDirty() already handles dirty children via
        // drawDirtyChildren(), which skips the background clear.
        // Propagating would force UITabView::draw() with its fillRect,
        // causing a visible flash between the clear and the sprite push.
    }
}

void UITabView::handleTouchUp(int16_t tx, int16_t ty) {
    if (_touchedChild) {
        // Check if the child was a modal overlay before the touch-up
        bool wasModal = (_touchedChild->isMenu() || _touchedChild->isPopup())
                      && _touchedChild->isVisible();
        _touchedChild->handleTouchUp(tx, ty);

        // If a modal child just closed (isMenu/isPopup went from true→false),
        // mark overlapping siblings dirty so they repaint over the stale
        // overlay area.  Do NOT mark the TabView itself dirty — that triggers
        // UITabView::draw() with its full-content fillRect flash.  The
        // closing widget is responsible for erasing its own overlay footprint
        // (see UIDropdown::draw() _needsListErase path).
        if (wasModal && !_touchedChild->isMenu() && !_touchedChild->isPopup()) {
            if (_activePage >= 0 && _activePage < _pageCount) {
                UITabPage& page = _pages[_activePage];
                for (int i = 0; i < page.childCount; i++) {
                    UIElement* sibling = page.children[i];
                    if (sibling && sibling != _touchedChild && sibling->isVisible()) {
                        sibling->setDirty(true);
                    }
                }
            }
        }
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
int UIInfoPopup::wordWrap(LovyanGFX& gfx, const char* text, float textSize,
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
void UIInfoPopup::autoSize(LovyanGFX& gfx) {
    const int16_t hPad        = TAB5_PADDING * 2;    // left + right padding
    const int16_t vPad        = TAB5_PADDING;         // top/bottom padding
    const int16_t titleGap    = 42;                    // title text + divider
    const int16_t btnAreaH    = 56;                    // button + spacing
    const int16_t minW        = 200;
    const int16_t minH        = 140;
    const int16_t screenMargin = 40;                   // min gap from screen edge
    const int16_t maxW        = Tab5UI::screenW() - screenMargin * 2;
    const int16_t maxH        = Tab5UI::screenH() - screenMargin * 2;

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
    _x = (Tab5UI::screenW() - _w) / 2;
    _y = (Tab5UI::screenH() - _h) / 2;

    _needsAutoSize = false;
}

void UIInfoPopup::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Auto-size on first draw or after content change
    if (_needsAutoSize) {
        autoSize(gfx);
    }

    // Sprite covers popup + shadow (shadow is offset +4,+4)
    int16_t sprW = _w + 4;
    int16_t sprH = _h + 4;
    M5Canvas* spr = acquireSprite(&gfx, sprW, sprH);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Clear sprite background (transparent area around popup)
    if (spr) dst.fillRect(0, 0, sprW, sprH, rgb888(Tab5Theme::BG_DARK));

    // Shadow
    dst.fillRect(ox + 4, oy + 4, _w, _h, rgb888(0x0A0A14));

    // Background
    dst.fillSmoothRoundRect(ox, oy, _w, _h, 8, rgb888(_bgColor));

    // Border
    dst.drawRoundRect(ox, oy, _w, _h, 8, rgb888(_borderColor));

    // Title
    dst.setTextSize(TAB5_FONT_SIZE_LG);
    dst.setTextDatum(textdatum_t::top_center);
    dst.setTextColor(rgb888(_titleColor));
    dst.drawString(_title, ox + _w / 2, oy + TAB5_PADDING + 4);

    // Divider below title
    int16_t divY = oy + TAB5_PADDING + 38;
    dst.drawFastHLine(ox + TAB5_PADDING, divY,
                      _w - TAB5_PADDING * 2, rgb888(Tab5Theme::DIVIDER));

    // Message — word-wrapped
    dst.setTextSize(TAB5_FONT_SIZE_MD);
    dst.setTextDatum(textdatum_t::top_center);
    dst.setTextColor(rgb888(_textColor));

    int16_t contentW = _w - TAB5_PADDING * 2 - 10;
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(dst, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    int16_t lineH = (int16_t)(dst.fontHeight() * TAB5_FONT_SIZE_MD) + 4;
    int16_t msgStartY = divY + 14;

    char lineBuf[257];
    for (int i = 0; i < numLines; i++) {
        int len = lineLengths[i];
        if (len > 255) len = 255;
        memcpy(lineBuf, _message + lineStarts[i], len);
        // Trim trailing spaces
        while (len > 0 && lineBuf[len - 1] == ' ') len--;
        lineBuf[len] = '\0';
        dst.drawString(lineBuf, ox + _w / 2, msgStartY + i * lineH);
    }

    // OK button (centered at bottom)
    dst.setTextSize(TAB5_FONT_SIZE_MD);
    _btnW = dst.textWidth(_btnLabel) + 60;
    if (_btnW < 100) _btnW = 100;
    _btnH = 40;
    _btnX = _x + (_w - _btnW) / 2;
    _btnY = _y + _h - _btnH - TAB5_PADDING;
    int16_t btnOx = spr ? (_btnX - _x) : _btnX;
    int16_t btnOy = spr ? (_btnY - _y) : _btnY;

    uint32_t btnBg = _btnPressed ? rgb888(darken(_btnColor)) : rgb888(_btnColor);
    dst.fillSmoothRoundRect(btnOx, btnOy, _btnW, _btnH, 6, btnBg);

    dst.setTextSize(TAB5_FONT_SIZE_MD);
    dst.setTextDatum(textdatum_t::middle_center);
    dst.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    dst.drawString(_btnLabel, btnOx + _btnW / 2, btnOy + _btnH / 2);

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
    }

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
int UIConfirmPopup::wordWrap(LovyanGFX& gfx, const char* text, float textSize,
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
void UIConfirmPopup::autoSize(LovyanGFX& gfx) {
    const int16_t hPad         = TAB5_PADDING * 2;
    const int16_t vPad         = TAB5_PADDING;
    const int16_t titleGap     = 42;
    const int16_t btnAreaH     = 56;
    const int16_t btnGap       = 20;    // gap between Yes and No buttons
    const int16_t minW         = 260;
    const int16_t minH         = 140;
    const int16_t screenMargin = 40;
    const int16_t maxW         = Tab5UI::screenW() - screenMargin * 2;
    const int16_t maxH         = Tab5UI::screenH() - screenMargin * 2;

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
    _x = (Tab5UI::screenW() - _w) / 2;
    _y = (Tab5UI::screenH() - _h) / 2;

    _needsAutoSize = false;
}

void UIConfirmPopup::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    if (_needsAutoSize) {
        autoSize(gfx);
    }

    // Sprite covers popup + shadow (shadow is offset +4,+4)
    int16_t sprW = _w + 4;
    int16_t sprH = _h + 4;
    M5Canvas* spr = acquireSprite(&gfx, sprW, sprH);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Clear sprite background (transparent area around popup)
    if (spr) dst.fillRect(0, 0, sprW, sprH, rgb888(Tab5Theme::BG_DARK));

    // Shadow
    dst.fillRect(ox + 4, oy + 4, _w, _h, rgb888(0x0A0A14));

    // Background
    dst.fillSmoothRoundRect(ox, oy, _w, _h, 8, rgb888(_bgColor));

    // Border
    dst.drawRoundRect(ox, oy, _w, _h, 8, rgb888(_borderColor));

    // Title
    dst.setTextSize(TAB5_FONT_SIZE_LG);
    dst.setTextDatum(textdatum_t::top_center);
    dst.setTextColor(rgb888(_titleColor));
    dst.drawString(_title, ox + _w / 2, oy + TAB5_PADDING + 4);

    // Divider below title
    int16_t divY = oy + TAB5_PADDING + 38;
    dst.drawFastHLine(ox + TAB5_PADDING, divY,
                      _w - TAB5_PADDING * 2, rgb888(Tab5Theme::DIVIDER));

    // Message — word-wrapped
    dst.setTextSize(TAB5_FONT_SIZE_MD);
    dst.setTextDatum(textdatum_t::top_center);
    dst.setTextColor(rgb888(_textColor));

    int16_t contentW = _w - TAB5_PADDING * 2 - 10;
    int16_t lineStarts[32], lineLengths[32];
    int numLines = wordWrap(dst, _message, TAB5_FONT_SIZE_MD,
                            contentW, lineStarts, lineLengths, 32);

    int16_t lineH = (int16_t)(dst.fontHeight() * TAB5_FONT_SIZE_MD) + 4;
    int16_t msgStartY = divY + 14;

    char lineBuf[257];
    for (int i = 0; i < numLines; i++) {
        int len = lineLengths[i];
        if (len > 255) len = 255;
        memcpy(lineBuf, _message + lineStarts[i], len);
        while (len > 0 && lineBuf[len - 1] == ' ') len--;
        lineBuf[len] = '\0';
        dst.drawString(lineBuf, ox + _w / 2, msgStartY + i * lineH);
    }

    // ── Yes / No buttons (side by side, centered at bottom) ──
    const int16_t btnGap = 20;
    dst.setTextSize(TAB5_FONT_SIZE_MD);

    _yesBtnW = dst.textWidth(_yesLabel) + 60;
    if (_yesBtnW < 100) _yesBtnW = 100;
    _yesBtnH = 40;

    _noBtnW = dst.textWidth(_noLabel) + 60;
    if (_noBtnW < 100) _noBtnW = 100;
    _noBtnH = 40;

    int16_t totalBtnW = _yesBtnW + btnGap + _noBtnW;
    int16_t btnStartX = _x + (_w - totalBtnW) / 2;
    int16_t btnY = _y + _h - _yesBtnH - TAB5_PADDING;

    // No button (left) — store absolute coords for hit testing
    _noBtnX = btnStartX;
    _noBtnY = btnY;
    int16_t noBtnOx = spr ? (_noBtnX - _x) : _noBtnX;
    int16_t noBtnOy = spr ? (_noBtnY - _y) : _noBtnY;

    uint32_t noBg = _noBtnPressed ? rgb888(darken(_noBtnColor)) : rgb888(_noBtnColor);
    dst.fillSmoothRoundRect(noBtnOx, noBtnOy, _noBtnW, _noBtnH, 6, noBg);

    dst.setTextSize(TAB5_FONT_SIZE_MD);
    dst.setTextDatum(textdatum_t::middle_center);
    dst.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    dst.drawString(_noLabel, noBtnOx + _noBtnW / 2, noBtnOy + _noBtnH / 2);

    // Yes button (right)
    _yesBtnX = btnStartX + _noBtnW + btnGap;
    _yesBtnY = btnY;
    int16_t yesBtnOx = spr ? (_yesBtnX - _x) : _yesBtnX;
    int16_t yesBtnOy = spr ? (_yesBtnY - _y) : _yesBtnY;

    uint32_t yesBg = _yesBtnPressed ? rgb888(darken(_yesBtnColor)) : rgb888(_yesBtnColor);
    dst.fillSmoothRoundRect(yesBtnOx, yesBtnOy, _yesBtnW, _yesBtnH, 6, yesBg);

    dst.setTextSize(TAB5_FONT_SIZE_MD);
    dst.setTextDatum(textdatum_t::middle_center);
    dst.setTextColor(rgb888(Tab5Theme::TEXT_PRIMARY));
    dst.drawString(_yesLabel, yesBtnOx + _yesBtnW / 2, yesBtnOy + _yesBtnH / 2);

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
    }

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
int16_t UIScrollText::markdownTextWidth(LovyanGFX& gfx, const char* text, int len,
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
void UIScrollText::reflow(LovyanGFX& gfx) {
    int16_t contentW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 4;
    _lineCount = 0;

    // Compute line heights for each level
    // Note: fontHeight() already returns the scaled height after setTextSize()
    gfx.setTextSize(TAB5_FONT_SIZE_LG);
    int16_t h1H = gfx.fontHeight() + 10;
    gfx.setTextSize((_textSize + TAB5_FONT_SIZE_LG) * 0.5f);
    int16_t h2H = gfx.fontHeight() + 8;
    gfx.setTextSize(_textSize * 1.1f);
    int16_t h3H = gfx.fontHeight() + 6;
    gfx.setTextSize(_textSize);
    int16_t normalH = gfx.fontHeight() + 4;
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
void UIScrollText::drawMarkdownLine(LovyanGFX& gfx, const char* text, int len,
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
            int16_t fh = gfx.fontHeight();  // already scaled by setTextSize()
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

void UIScrollText::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Reflow text if needed
    if (_needsWrap) {
        reflow(gfx);
    }

    // ── Try sprite-buffered rendering for flicker-free scrolling ──
    M5Canvas* spr = acquireSprite(&gfx, _w, _h);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Background fill
    dst.fillRect(ox, oy, _w, _h, rgb888(_bgColor));

    // Border
    dst.drawRect(ox, oy, _w, _h, rgb888(_borderColor));

    // Inner content area (padded)
    int16_t innerX = ox + TAB5_PADDING;
    int16_t innerY = oy + TAB5_PADDING;
    int16_t innerW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 2;
    int16_t innerH = _h - TAB5_PADDING * 2;

    // Clip to content area
    dst.setClipRect(ox + 1, oy + 1, _w - 2, _h - 2);

    int16_t bulletIndent = 28;

    // Accumulate Y position
    int16_t curY = innerY - _scrollOffset;

    for (int i = 0; i < _lineCount; i++) {
        const ScrollTextLine& sl = _lines[i];
        int16_t lineY = curY;
        curY += sl.height;

        // Skip lines outside visible area
        if (lineY + sl.height <= oy) continue;
        if (lineY >= oy + _h) break;

        // ── Horizontal rule ──
        if (sl.rule) {
            int16_t ruleY = lineY + sl.height / 2;
            dst.drawFastHLine(innerX, ruleY, innerW, rgb888(_ruleColor));
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
            dst.setTextSize(fontSize);
            int16_t bulletR = 4;
            int16_t bulletCX = innerX + 10;
            int16_t bulletCY = lineY + dst.fontHeight() / 2;
            dst.fillCircle(bulletCX, bulletCY, bulletR, rgb888(_bulletColor));
            drawX = innerX + bulletIndent;
        }
        // Continuation lines of bullets (not first) still get indent
        // We detect this by checking if the previous line was a bullet
        if (!sl.bullet && i > 0 && _lines[i - 1].bullet && sl.heading == 0) {
            drawX = innerX + bulletIndent;
        }

        // ── Draw text with inline markdown ──
        drawMarkdownLine(dst, _text + sl.textStart, sl.textLength,
                         drawX, lineY, fontSize, textColor);

        // ── Heading underline for H1 ──
        if (sl.heading == 1) {
            int16_t ulY = lineY + sl.height - 4;
            dst.drawFastHLine(innerX, ulY, innerW, rgb888(_ruleColor));
        }
    }

    // Clear clip
    dst.clearClipRect();

    // Scrollbar (only if content overflows)
    int16_t contentH = totalContentHeight();
    if (contentH > innerH) {
        int16_t sbX = ox + _w - TAB5_LIST_SCROLLBAR_W - 1;
        int16_t sbAreaH = _h - 2;

        // Scrollbar track
        dst.fillRect(sbX, oy + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                     rgb888(darken(_bgColor, 60)));

        // Scrollbar thumb
        float visibleRatio = (float)innerH / (float)contentH;
        int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
        if (thumbH < 20) thumbH = 20;

        int16_t ms = maxScroll();
        float scrollRatio = (ms > 0) ? (float)_scrollOffset / (float)ms : 0.0f;
        int16_t thumbY = oy + 1 + (int16_t)((sbAreaH - thumbH) * scrollRatio);

        dst.fillSmoothRoundRect(sbX, thumbY, TAB5_LIST_SCROLLBAR_W, thumbH,
                                 3, rgb888(Tab5Theme::TEXT_DISABLED));
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
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

void UIList::draw(LovyanGFX& gfx) {
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

    // ── Try sprite-buffered rendering for flicker-free scrolling ──
    M5Canvas* spr = acquireSprite(&gfx, _w, _h);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    // Offset: when drawing to sprite, origin is (0,0); otherwise (_x,_y)
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Background fill
    dst.fillRect(ox, oy, _w, _h, rgb888(_bgColor));

    // Border
    dst.drawRect(ox, oy, _w, _h, rgb888(_borderColor));

    // Clip region
    dst.setClipRect(ox + 1, oy + 1, _w - 2, _h - 2);

    // Draw visible items
    for (int i = 0; i < _itemCount; i++) {
        int16_t itemY = oy + (i * _itemH) - _scrollOffset;

        // Skip items fully outside visible area
        if (itemY + _itemH <= oy || itemY >= oy + _h) continue;

        // Selected highlight
        if (i == _selectedIndex) {
            dst.fillRect(ox + 1, itemY, _w - TAB5_LIST_SCROLLBAR_W - 2, _itemH,
                         rgb888(_selectColor));
        }

        // Item text
        dst.setTextSize(_textSize);
        dst.setTextDatum(textdatum_t::middle_left);

        uint32_t tc;
        if (!_items[i].enabled) {
            tc = rgb888(Tab5Theme::TEXT_DISABLED);
        } else if (i == _selectedIndex) {
            tc = rgb888(Tab5Theme::TEXT_PRIMARY);
        } else {
            tc = rgb888(_textColor);
        }
        dst.setTextColor(tc);
        dst.drawString(_items[i].text, ox + TAB5_PADDING, itemY + _itemH / 2);

        // Right-aligned icon (if present)
        if (_items[i].hasIcon) {
            int16_t iconX = ox + _w - TAB5_LIST_SCROLLBAR_W - TAB5_PADDING - iconSize - 2;
            int16_t iconY = itemY + (_itemH - iconSize) / 2;

            if (_items[i].iconCircle) {
                // Circle icon
                int16_t cr = iconSize / 2;
                int16_t cx = iconX + cr;
                int16_t cy = iconY + cr;
                dst.fillCircle(cx, cy, cr, rgb888(_items[i].iconColor));
                dst.drawCircle(cx, cy, cr, rgb888(_items[i].iconBorderColor));

                // Icon character
                if (_items[i].iconChar[0] != '\0') {
                    dst.setTextSize(_textSize * 0.8f);
                    dst.setTextDatum(textdatum_t::middle_center);
                    dst.setTextColor(rgb888(_items[i].iconCharColor));
                    dst.drawString(_items[i].iconChar, cx, cy);
                }
            } else {
                // Square icon (rounded)
                dst.fillSmoothRoundRect(iconX, iconY, iconSize, iconSize, 4,
                                         rgb888(_items[i].iconColor));
                dst.drawRoundRect(iconX, iconY, iconSize, iconSize, 4,
                                   rgb888(_items[i].iconBorderColor));

                // Icon character
                if (_items[i].iconChar[0] != '\0') {
                    dst.setTextSize(_textSize * 0.8f);
                    dst.setTextDatum(textdatum_t::middle_center);
                    dst.setTextColor(rgb888(_items[i].iconCharColor));
                    dst.drawString(_items[i].iconChar,
                                   iconX + iconSize / 2, iconY + iconSize / 2);
                }
            }
        }

        // Divider between items
        if (i < _itemCount - 1) {
            int16_t divY = itemY + _itemH - 1;
            dst.drawFastHLine(ox + TAB5_PADDING, divY,
                              _w - TAB5_LIST_SCROLLBAR_W - TAB5_PADDING * 2,
                              rgb888(Tab5Theme::DIVIDER));
        }
    }

    // Clear clip
    dst.clearClipRect();

    // Scrollbar (only if content overflows)
    int16_t contentH = totalContentHeight();
    if (contentH > _h) {
        int16_t sbX = ox + _w - TAB5_LIST_SCROLLBAR_W - 1;
        int16_t sbAreaH = _h - 2;

        // Scrollbar track
        dst.fillRect(sbX, oy + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                     rgb888(darken(_bgColor, 60)));

        // Scrollbar thumb
        float visibleRatio = (float)_h / (float)contentH;
        int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
        if (thumbH < 20) thumbH = 20;

        float scrollRatio = (float)_scrollOffset / (float)maxScroll();
        int16_t thumbY = oy + 1 + (int16_t)((sbAreaH - thumbH) * scrollRatio);

        dst.fillSmoothRoundRect(sbX, thumbY, TAB5_LIST_SCROLLBAR_W, thumbH,
                                 3, rgb888(Tab5Theme::TEXT_DISABLED));
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
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
//  UICheckbox
// ═════════════════════════════════════════════════════════════════════════════

UICheckbox::UICheckbox(int16_t x, int16_t y, int16_t w, int16_t h,
                       const char* label, bool checked,
                       uint32_t boxColor, uint32_t textColor, float textSize)
    : UIElement(x, y, w, h)
    , _checked(checked)
    , _boxColor(boxColor)
    , _textColor(textColor)
    , _textSize(textSize)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
}

void UICheckbox::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UICheckbox::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Vertical center of the element
    int16_t cy = _y + _h / 2;
    int16_t boxX = _x;
    int16_t boxY = cy - BOX_SIZE / 2;

    // Box background
    uint32_t bgCol = _checked ? rgb888(_boxColor) : rgb888(darken(_borderColor, 20));
    if (!_enabled) bgCol = rgb888(Tab5Theme::BORDER);
    gfx.fillSmoothRoundRect(boxX, boxY, BOX_SIZE, BOX_SIZE, 4, bgCol);

    // Box border
    gfx.drawRoundRect(boxX, boxY, BOX_SIZE, BOX_SIZE, 4,
                      _checked ? rgb888(_boxColor) : rgb888(_borderColor));

    // Checkmark (two lines forming a ✓)
    if (_checked) {
        uint32_t chkCol = rgb888(_checkColor);
        int16_t cx = boxX + BOX_SIZE / 2;
        // Draw checkmark as two thick lines
        // Short leg: bottom-left to bottom-center
        for (int t = -1; t <= 1; t++) {
            gfx.drawLine(boxX + 6, cy + t,       cx - 2, boxY + BOX_SIZE - 7 + t, chkCol);
            gfx.drawLine(cx - 2, boxY + BOX_SIZE - 7 + t, boxX + BOX_SIZE - 6, boxY + 7 + t, chkCol);
        }
    }

    // Label text
    gfx.setTextSize(_textSize);
    gfx.setTextDatum(textdatum_t::middle_left);
    uint32_t tc = _enabled ? rgb888(_textColor) : rgb888(Tab5Theme::TEXT_DISABLED);
    gfx.setTextColor(tc);
    gfx.drawString(_label, boxX + BOX_SIZE + BOX_GAP, cy);

    _dirty = false;
}

void UICheckbox::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true; _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UICheckbox::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        _checked = !_checked;  // Toggle on release
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIRadioGroup
// ═════════════════════════════════════════════════════════════════════════════

void UIRadioGroup::addButton(UIRadioButton* btn) {
    if (_count < MAX_BUTTONS) {
        _buttons[_count++] = btn;
        btn->_group = this;
        // Select the first button by default if none selected
        if (_count == 1 && !_selected) {
            _selected = btn;
            btn->_selected = true;
        }
    }
}

void UIRadioGroup::select(UIRadioButton* btn) {
    if (_selected == btn) return;
    // Deselect previous
    if (_selected) {
        _selected->_selected = false;
        _selected->_dirty = true;
    }
    _selected = btn;
    if (btn) {
        btn->_selected = true;
        btn->_dirty = true;
    }
}

int UIRadioGroup::getSelectedIndex() const {
    for (int i = 0; i < _count; i++) {
        if (_buttons[i] == _selected) return i;
    }
    return -1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIRadioButton
// ═════════════════════════════════════════════════════════════════════════════

UIRadioButton::UIRadioButton(int16_t x, int16_t y, int16_t w, int16_t h,
                             const char* label, UIRadioGroup* group,
                             uint32_t circleColor, uint32_t textColor,
                             float textSize)
    : UIElement(x, y, w, h)
    , _circleColor(circleColor)
    , _textColor(textColor)
    , _textSize(textSize)
    , _group(group)
{
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    if (_group) _group->addButton(this);
}

void UIRadioButton::setLabel(const char* label) {
    strncpy(_label, label, sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
    _dirty = true;
}

void UIRadioButton::setGroup(UIRadioGroup* g) {
    _group = g;
    if (g) g->addButton(this);
}

void UIRadioButton::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    int16_t cy = _y + _h / 2;
    int16_t cx = _x + CIRCLE_R;

    // Outer circle
    uint32_t ringCol = _selected ? rgb888(_circleColor) : rgb888(_borderColor);
    if (!_enabled) ringCol = rgb888(Tab5Theme::BORDER);
    gfx.drawCircle(cx, cy, CIRCLE_R, ringCol);
    gfx.drawCircle(cx, cy, CIRCLE_R - 1, ringCol);

    // Inner area — fill to clear previous state
    uint32_t innerBg = rgb888(Tab5Theme::BG_MEDIUM);
    gfx.fillCircle(cx, cy, CIRCLE_R - 3, innerBg);

    // Filled dot when selected
    if (_selected) {
        uint32_t dotCol = rgb888(_dotColor);
        gfx.fillCircle(cx, cy, CIRCLE_R - 5, dotCol);
    }

    // Label text
    gfx.setTextSize(_textSize);
    gfx.setTextDatum(textdatum_t::middle_left);
    uint32_t tc = _enabled ? rgb888(_textColor) : rgb888(Tab5Theme::TEXT_DISABLED);
    gfx.setTextColor(tc);
    gfx.drawString(_label, _x + CIRCLE_R * 2 + CIRCLE_GAP, cy);

    _dirty = false;
}

void UIRadioButton::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true; _dirty = true;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UIRadioButton::handleTouchUp(int16_t tx, int16_t ty) {
    if (_pressed) {
        _pressed = false;
        // Select this radio button (group handles deselecting others)
        if (_group) {
            _group->select(this);
        } else {
            _selected = true;
        }
        _dirty = true;
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UIDropdown
// ═════════════════════════════════════════════════════════════════════════════

UIDropdown::UIDropdown(int16_t x, int16_t y, int16_t w, int16_t h,
                       const char* placeholder,
                       uint32_t bgColor, uint32_t textColor, uint32_t selectColor)
    : UIElement(x, y, w, h)
    , _listX(x), _listY(y + h), _listW(w), _listH(0)
    , _bgColor(bgColor), _textColor(textColor), _selectColor(selectColor)
{
    strncpy(_placeholder, placeholder, sizeof(_placeholder) - 1);
    _placeholder[sizeof(_placeholder) - 1] = '\0';
}

// ── Item management ─────────────────────────────────────────────────────────

int UIDropdown::addItem(const char* text) {
    if (_itemCount >= TAB5_LIST_MAX_ITEMS) return -1;
    _items[_itemCount] = UIListItem();
    strncpy(_items[_itemCount].text, text, sizeof(_items[0].text) - 1);
    _items[_itemCount].text[sizeof(_items[0].text) - 1] = '\0';
    _dirty = true;
    return _itemCount++;
}

int UIDropdown::addItem(const char* text, const char* iconChar,
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

void UIDropdown::setItemIcon(int index, const char* iconChar,
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

void UIDropdown::clearItemIcon(int index) {
    if (index < 0 || index >= _itemCount) return;
    _items[index].hasIcon = false;
    _items[index].iconChar[0] = '\0';
    _dirty = true;
}

void UIDropdown::removeItem(int index) {
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

void UIDropdown::clearItems() {
    _itemCount = 0;
    _selectedIndex = -1;
    _scrollOffset = 0;
    _dirty = true;
}

void UIDropdown::setItemText(int index, const char* text) {
    if (index < 0 || index >= _itemCount) return;
    strncpy(_items[index].text, text, sizeof(_items[0].text) - 1);
    _items[index].text[sizeof(_items[0].text) - 1] = '\0';
    _dirty = true;
}

void UIDropdown::setItemEnabled(int index, bool enabled) {
    if (index < 0 || index >= _itemCount) return;
    _items[index].enabled = enabled;
    _dirty = true;
}

const char* UIDropdown::getSelectedText() const {
    if (_selectedIndex < 0 || _selectedIndex >= _itemCount) return "";
    return _items[_selectedIndex].text;
}

void UIDropdown::setSelectedIndex(int index) {
    if (index < -1 || index >= _itemCount) return;
    _selectedIndex = index;
    _dirty = true;
}

void UIDropdown::clearSelection() {
    _selectedIndex = -1;
    _dirty = true;
}

void UIDropdown::setPlaceholder(const char* text) {
    strncpy(_placeholder, text, sizeof(_placeholder) - 1);
    _placeholder[sizeof(_placeholder) - 1] = '\0';
    _dirty = true;
}

// ── Open / Close ────────────────────────────────────────────────────────────

void UIDropdown::open() {
    _open = true;
    _btnPressed = false;
    _scrollOffset = 0;
    // If an item is selected, scroll to make it visible
    if (_selectedIndex >= 0) {
        int16_t itemTop = _selectedIndex * _itemH;
        if (itemTop > 0) {
            _scrollOffset = itemTop;
            clampScroll();
        }
    }
    _dirty = true;
}

void UIDropdown::close() {
    // Save the list overlay footprint so draw() can erase it
    // (list geometry was calculated when the dropdown was opened/drawn)
    if (_open) {
        _needsListErase = true;
        _eraseX = _listX;
        _eraseY = _listY;
        _eraseW = _listW + 3;  // +3 for shadow offset
        _eraseH = _listH + 3;
    }
    _open = false;
    _btnPressed = false;
    _dragging = false;
    _wasDrag = false;
    _dirty = true;
}

// ── Geometry helpers ────────────────────────────────────────────────────────

int16_t UIDropdown::maxScroll() const {
    int16_t contentH = totalContentHeight();
    if (contentH <= _listH) return 0;
    return contentH - _listH;
}

void UIDropdown::clampScroll() {
    int16_t ms = maxScroll();
    if (_scrollOffset < 0) _scrollOffset = 0;
    if (_scrollOffset > ms) _scrollOffset = ms;
}

int UIDropdown::itemAtY(int16_t ty) const {
    if (ty < _listY || ty >= _listY + _listH) return -1;
    int16_t relY = ty - _listY + _scrollOffset;
    int idx = relY / _itemH;
    if (idx < 0 || idx >= _itemCount) return -1;
    return idx;
}

void UIDropdown::calcListGeometry() {
    // Position the dropdown list directly below the button
    _listX = _x;
    _listW = _w;

    // Calculate visible items (capped by maxVisible and actual count)
    int visCount = _itemCount;
    if (visCount > _maxVisible) visCount = _maxVisible;
    if (visCount < 1) visCount = 1;

    _listH = visCount * _itemH;

    // Determine usable vertical bounds.  If explicit content bounds were
    // set via setContentBounds(), use those.  Otherwise default to the
    // screen area between the standard title and status bars.
    int16_t minY = _boundsTop > 0 ? _boundsTop : TAB5_TITLE_H;
    int16_t maxY = _boundsBottom > 0 ? _boundsBottom
                 : (Tab5UI::screenH() - TAB5_STATUS_H);

    // Position below button, but flip upward if it would go off-bounds
    int16_t belowY = _y + _h;
    int16_t aboveY = _y - _listH;

    if (belowY + _listH <= maxY) {
        _listY = belowY;
    } else if (aboveY >= minY) {
        _listY = aboveY;
    } else {
        // Constrain to available space below
        _listY = belowY;
        _listH = maxY - belowY;
        if (_listH < _itemH) _listH = _itemH;  // at least one item
    }
}

// ── Drawing ─────────────────────────────────────────────────────────────────

void UIDropdown::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Erase the old list overlay area if the dropdown just closed
    if (_needsListErase) {
        _needsListErase = false;
        gfx.fillRect(_eraseX, _eraseY, _eraseW, _eraseH,
                     rgb888(Tab5Theme::BG_DARK));
    }

    // Auto-scale item height from text size
    gfx.setTextSize(_textSize);
    int16_t fh = (int16_t)(gfx.fontHeight() * _textSize);
    _itemH = fh + TAB5_PADDING * 2;
    if (_itemH < 32) _itemH = 32;

    // Icon size derived from item height
    int16_t iconSize = _itemH - TAB5_PADDING;
    if (iconSize < 16) iconSize = 16;

    // ── Draw collapsed button ──
    uint32_t btnBg = _btnPressed
                   ? rgb888(darken(_bgColor))
                   : rgb888(_bgColor);
    gfx.fillSmoothRoundRect(_x, _y, _w, _h, TAB5_BTN_R, btnBg);
    gfx.drawRoundRect(_x, _y, _w, _h, TAB5_BTN_R, rgb888(_borderColor));

    // Selected text or placeholder
    const char* displayText = (_selectedIndex >= 0)
                            ? _items[_selectedIndex].text
                            : _placeholder;
    uint32_t displayColor = (_selectedIndex >= 0)
                          ? rgb888(_textColor)
                          : rgb888(Tab5Theme::TEXT_SECONDARY);

    gfx.setTextSize(_textSize);
    gfx.setTextDatum(textdatum_t::middle_left);
    gfx.setTextColor(displayColor);

    // Clip text so it doesn't overlap the arrow area
    int16_t arrowSpace = 30;
    gfx.setClipRect(_x + TAB5_PADDING, _y, _w - TAB5_PADDING - arrowSpace, _h);
    gfx.drawString(displayText, _x + TAB5_PADDING, _y + _h / 2);
    gfx.clearClipRect();

    // ▼ arrow indicator on the right
    int16_t arrowX = _x + _w - 20;
    int16_t arrowY = _y + _h / 2;
    int16_t as = 5;  // Arrow half-size
    if (_open) {
        // ▲ when open
        gfx.fillTriangle(arrowX, arrowY - as,
                          arrowX - as, arrowY + as,
                          arrowX + as, arrowY + as,
                          rgb888(Tab5Theme::TEXT_SECONDARY));
    } else {
        // ▼ when closed
        gfx.fillTriangle(arrowX - as, arrowY - as,
                          arrowX + as, arrowY - as,
                          arrowX, arrowY + as,
                          rgb888(Tab5Theme::TEXT_SECONDARY));
    }

    // ── Draw expanded list overlay ──
    if (_open) {
        calcListGeometry();

        // Sprite covers list + shadow (shadow offset +3,+3)
        int16_t sprW = _listW + 3;
        int16_t sprH = _listH + 3;
        M5Canvas* spr = acquireSprite(&gfx, sprW, sprH);
        LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
        int16_t lox = spr ? 0 : _listX;
        int16_t loy = spr ? 0 : _listY;

        // Clear sprite area (background behind shadow)
        if (spr) dst.fillRect(0, 0, sprW, sprH, rgb888(Tab5Theme::BG_DARK));

        // Shadow
        dst.fillRect(lox + 3, loy + 3, _listW, _listH, rgb888(0x0A0A14));

        // Background
        dst.fillRect(lox, loy, _listW, _listH, rgb888(_bgColor));

        // Border
        dst.drawRect(lox, loy, _listW, _listH, rgb888(_borderColor));

        // Clip region for items
        dst.setClipRect(lox + 1, loy + 1, _listW - 2, _listH - 2);

        // Draw visible items
        for (int i = 0; i < _itemCount; i++) {
            int16_t itemY = loy + (i * _itemH) - _scrollOffset;

            // Skip items fully outside visible area
            if (itemY + _itemH <= loy || itemY >= loy + _listH) continue;

            // Selected highlight
            if (i == _selectedIndex) {
                dst.fillRect(lox + 1, itemY,
                             _listW - TAB5_LIST_SCROLLBAR_W - 2, _itemH,
                             rgb888(_selectColor));
            }

            // Item text
            dst.setTextSize(_textSize);
            dst.setTextDatum(textdatum_t::middle_left);

            uint32_t tc;
            if (!_items[i].enabled) {
                tc = rgb888(Tab5Theme::TEXT_DISABLED);
            } else if (i == _selectedIndex) {
                tc = rgb888(Tab5Theme::TEXT_PRIMARY);
            } else {
                tc = rgb888(_textColor);
            }
            dst.setTextColor(tc);
            dst.drawString(_items[i].text, lox + TAB5_PADDING,
                           itemY + _itemH / 2);

            // Right-aligned icon (if present)
            if (_items[i].hasIcon) {
                int16_t iconX = lox + _listW - TAB5_LIST_SCROLLBAR_W
                              - TAB5_PADDING - iconSize - 2;
                int16_t iconY_ = itemY + (_itemH - iconSize) / 2;

                if (_items[i].iconCircle) {
                    int16_t cr = iconSize / 2;
                    int16_t cx = iconX + cr;
                    int16_t cy = iconY_ + cr;
                    dst.fillCircle(cx, cy, cr, rgb888(_items[i].iconColor));
                    dst.drawCircle(cx, cy, cr, rgb888(_items[i].iconBorderColor));
                    if (_items[i].iconChar[0] != '\0') {
                        dst.setTextSize(_textSize * 0.8f);
                        dst.setTextDatum(textdatum_t::middle_center);
                        dst.setTextColor(rgb888(_items[i].iconCharColor));
                        dst.drawString(_items[i].iconChar, cx, cy);
                    }
                } else {
                    dst.fillSmoothRoundRect(iconX, iconY_, iconSize, iconSize,
                                             4, rgb888(_items[i].iconColor));
                    dst.drawRoundRect(iconX, iconY_, iconSize, iconSize,
                                       4, rgb888(_items[i].iconBorderColor));
                    if (_items[i].iconChar[0] != '\0') {
                        dst.setTextSize(_textSize * 0.8f);
                        dst.setTextDatum(textdatum_t::middle_center);
                        dst.setTextColor(rgb888(_items[i].iconCharColor));
                        dst.drawString(_items[i].iconChar,
                                       iconX + iconSize / 2,
                                       iconY_ + iconSize / 2);
                    }
                }
            }

            // Divider between items
            if (i < _itemCount - 1) {
                int16_t divY = itemY + _itemH - 1;
                dst.drawFastHLine(lox + TAB5_PADDING, divY,
                                  _listW - TAB5_LIST_SCROLLBAR_W - TAB5_PADDING * 2,
                                  rgb888(Tab5Theme::DIVIDER));
            }
        }

        // Clear clip
        dst.clearClipRect();

        // Scrollbar (only if content overflows)
        int16_t contentH = totalContentHeight();
        if (contentH > _listH) {
            int16_t sbX = lox + _listW - TAB5_LIST_SCROLLBAR_W - 1;
            int16_t sbAreaH = _listH - 2;

            // Scrollbar track
            dst.fillRect(sbX, loy + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                         rgb888(darken(_bgColor, 60)));

            // Scrollbar thumb
            float visibleRatio = (float)_listH / (float)contentH;
            int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
            if (thumbH < 20) thumbH = 20;

            float scrollRatio = (float)_scrollOffset / (float)maxScroll();
            int16_t thumbY = loy + 1
                           + (int16_t)((sbAreaH - thumbH) * scrollRatio);

            dst.fillSmoothRoundRect(sbX, thumbY,
                                     TAB5_LIST_SCROLLBAR_W, thumbH,
                                     3, rgb888(Tab5Theme::TEXT_DISABLED));
        }

        // Push sprite to display in one transfer (flicker-free)
        if (spr) {
            spr->pushSprite(&gfx, _listX, _listY);
        }
    }

    _dirty = false;
}

// ── Touch handling ──────────────────────────────────────────────────────────

void UIDropdown::handleTouchDown(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (_open) {
        // Check if touch is inside the dropdown list
        if (tx >= _listX && tx < _listX + _listW &&
            ty >= _listY && ty < _listY + _listH) {
            // Start potential scroll/tap on list
            _dragging = false;
            _wasDrag = false;
            _touchStartY = ty;
            _touchDownY = ty;
            _scrollStart = _scrollOffset;
            _pressed = true;
        }
        // Touch on the button area while open — will close on touch-up
        // Touch outside both — will close on touch-up
    } else {
        // Collapsed: press the button
        if (hitTest(tx, ty)) {
            _btnPressed = true;
            _dirty = true;
            if (_onTouch) _onTouch(TouchEvent::TOUCH);
        }
    }
}

void UIDropdown::handleTouchMove(int16_t tx, int16_t ty) {
    if (!_open || !_pressed) return;

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

void UIDropdown::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_visible) return;

    if (_open) {
        // Check if tap landed in the list area
        bool inList = (tx >= _listX && tx < _listX + _listW &&
                       ty >= _listY && ty < _listY + _listH);

        if (inList && _pressed && !_wasDrag) {
            // Tap on an item — select it and close
            int idx = itemAtY(ty);
            if (idx >= 0 && idx < _itemCount && _items[idx].enabled) {
                _selectedIndex = idx;
                if (_onSelect) _onSelect(idx, _items[idx].text);
            }
            close();
        } else if (inList && _pressed) {
            // Was a drag — just end the drag, stay open
            _pressed = false;
            _dragging = false;
            _wasDrag = false;
        } else {
            // Touch outside the list — dismiss
            close();
        }
        _pressed = false;
        _dragging = false;
        _wasDrag = false;
    } else {
        // Collapsed: toggle open
        if (_btnPressed && hitTest(tx, ty)) {
            _btnPressed = false;
            open();
        } else {
            _btnPressed = false;
            _dirty = true;
        }
        if (_onRelease) _onRelease(TouchEvent::TOUCH_RELEASE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  UITextArea
// ═════════════════════════════════════════════════════════════════════════════

UITextArea::UITextArea(int16_t x, int16_t y, int16_t w, int16_t h,
                       const char* placeholder,
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

void UITextArea::setText(const char* text) {
    strncpy(_text, text, _maxLen);
    _text[_maxLen] = '\0';
    _cursorPos = (int)strlen(_text);
    _needsWrap = true;
    _scrollOffset = 0;
    _dirty = true;
}

void UITextArea::clear() {
    _text[0] = '\0';
    _cursorPos = 0;
    _needsWrap = true;
    _scrollOffset = 0;
    _dirty = true;
}

void UITextArea::setPlaceholder(const char* ph) {
    strncpy(_placeholder, ph, sizeof(_placeholder) - 1);
    _placeholder[sizeof(_placeholder) - 1] = '\0';
    _dirty = true;
}

void UITextArea::focus() {
    if (_focused) return;
    _focused = true;
    _dirty = true;
    if (_keyboard) {
        _keyboard->setOnKey([this](char ch) { this->onKeyPress(ch); });
        _keyboard->show();
    }
}

void UITextArea::blur() {
    if (!_focused) return;
    _focused = false;
    _dirty = true;
    if (_keyboard && _keyboard->isOpen()) {
        _keyboard->hide();
    }
}

void UITextArea::onKeyPress(char ch) {
    if (ch == '\0') {
        // Hide key pressed (legacy)
        blur();
        return;
    }
    if (ch == '\n') {
        // Done key — submit and close keyboard
        if (_onSubmit) _onSubmit(_text);
        blur();
        return;
    }
    if (ch == '\r') {
        // Enter key — insert a newline into the text
        int len = (int)strlen(_text);
        if (len < _maxLen) {
            for (int i = len; i >= _cursorPos; i--) {
                _text[i + 1] = _text[i];
            }
            _text[_cursorPos] = '\n';
            _cursorPos++;
            _needsWrap = true;
            _dirty = true;
            if (_onChange) _onChange(_text);
        }
        return;
    }
    if (ch == '\b') {
        // Backspace
        if (_cursorPos > 0) {
            int len = (int)strlen(_text);
            // Shift characters after cursor left by 1
            for (int i = _cursorPos - 1; i < len; i++) {
                _text[i] = _text[i + 1];
            }
            _cursorPos--;
            _needsWrap = true;
            _dirty = true;
            if (_onChange) _onChange(_text);
        }
        return;
    }
    // Regular character — insert at cursor position
    int len = (int)strlen(_text);
    if (len < _maxLen) {
        // Shift characters after cursor right by 1
        for (int i = len; i >= _cursorPos; i--) {
            _text[i + 1] = _text[i];
        }
        _text[_cursorPos] = ch;
        _cursorPos++;
        _needsWrap = true;
        _dirty = true;
        if (_onChange) _onChange(_text);
    }
}

void UITextArea::scrollTo(int16_t offset) {
    _scrollOffset = offset;
    clampScroll();
    _dirty = true;
}

void UITextArea::scrollToBottom() {
    _scrollOffset = maxScroll();
    _dirty = true;
}

int16_t UITextArea::totalContentHeight() const {
    int16_t total = 0;
    for (int i = 0; i < _lineCount; i++) {
        total += _lines[i].height;
    }
    return total;
}

int16_t UITextArea::maxScroll() const {
    int16_t contentH = totalContentHeight();
    int16_t innerH = _h - TAB5_PADDING * 2;
    if (contentH <= innerH) return 0;
    return contentH - innerH;
}

void UITextArea::clampScroll() {
    int16_t ms = maxScroll();
    if (_scrollOffset < 0) _scrollOffset = 0;
    if (_scrollOffset > ms) _scrollOffset = ms;
}

// ── Word-wrap the text buffer into display lines ──
void UITextArea::reflow(LovyanGFX& gfx) {
    int16_t contentW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 4;
    _lineCount = 0;

    gfx.setTextSize(_textSize);
    int16_t lineH = gfx.fontHeight() + 4;

    int len = (int)strlen(_text);
    if (len == 0) {
        // Empty text — one blank line
        _lines[0].start = 0;
        _lines[0].length = 0;
        _lines[0].height = lineH;
        _lineCount = 1;
        _needsWrap = false;
        return;
    }

    int pos = 0;
    while (pos <= len && _lineCount < TAB5_TEXTAREA_MAX_LINES) {
        // Check for explicit newline at the current position
        if (pos < len && _text[pos] == '\n') {
            // Empty line (newline character creates a blank line)
            TextAreaLine& sl = _lines[_lineCount++];
            sl.start = pos;
            sl.length = 0;
            sl.height = lineH;
            pos++;
            continue;
        }

        // Find the end of this logical line (up to next newline or end of text)
        int lineEnd = pos;
        while (lineEnd < len && _text[lineEnd] != '\n') lineEnd++;

        int srcLen = lineEnd - pos;

        if (srcLen == 0) {
            // End of text right after a newline — break
            break;
        }

        // Word-wrap this segment
        int dPos = 0;
        char buf[257];

        while (dPos < srcLen && _lineCount < TAB5_TEXTAREA_MAX_LINES) {
            int bestBreak = -1;
            int di = dPos;

            while (di < srcLen) {
                int runLen = di - dPos + 1;
                if (runLen > 255) runLen = 255;
                memcpy(buf, _text + pos + dPos, runLen);
                buf[runLen] = '\0';
                int16_t tw = gfx.textWidth(buf);
                if (tw > contentW && bestBreak >= 0) {
                    break;
                }
                if (_text[pos + di] == ' ' || _text[pos + di] == '-') {
                    bestBreak = di;
                }
                di++;
            }

            int wrapEnd, nextDPos;
            if (di >= srcLen) {
                wrapEnd = srcLen;
                nextDPos = srcLen;
            } else if (bestBreak >= dPos) {
                wrapEnd = bestBreak + 1;
                nextDPos = bestBreak + 1;
            } else {
                wrapEnd = (di > dPos) ? di : dPos + 1;
                nextDPos = wrapEnd;
            }

            TextAreaLine& sl = _lines[_lineCount++];
            sl.start = pos + dPos;
            sl.length = wrapEnd - dPos;
            sl.height = lineH;

            dPos = nextDPos;
        }

        pos = lineEnd;
        // Skip the newline character
        if (pos < len && _text[pos] == '\n') pos++;
    }

    if (_lineCount == 0) {
        _lines[0].start = 0;
        _lines[0].length = 0;
        _lines[0].height = lineH;
        _lineCount = 1;
    }

    clampScroll();
    _needsWrap = false;
}

// ── Determine cursor position from a touch coordinate ──
int UITextArea::cursorFromTouch(LovyanGFX& gfx, int16_t tx, int16_t ty) {
    if (_lineCount == 0) return 0;

    int16_t innerX = _x + TAB5_PADDING;
    int16_t innerY = _y + TAB5_PADDING;

    gfx.setTextSize(_textSize);

    // Find which line was tapped
    int16_t curY = innerY - _scrollOffset;
    int targetLine = _lineCount - 1;  // Default to last line

    for (int i = 0; i < _lineCount; i++) {
        if (ty >= curY && ty < curY + _lines[i].height) {
            targetLine = i;
            break;
        }
        curY += _lines[i].height;
    }

    // Now find character position within that line
    const TextAreaLine& sl = _lines[targetLine];
    if (sl.length == 0) return sl.start;

    int16_t relX = tx - innerX;
    if (relX <= 0) return sl.start;

    char buf[257];
    for (int c = 1; c <= sl.length; c++) {
        int runLen = (c > 255) ? 255 : c;
        memcpy(buf, _text + sl.start, runLen);
        buf[runLen] = '\0';
        int16_t tw = gfx.textWidth(buf);
        if (tw >= relX) {
            // Check if closer to this char or the previous
            if (c > 1) {
                memcpy(buf, _text + sl.start, c - 1);
                buf[c - 1] = '\0';
                int16_t prevW = gfx.textWidth(buf);
                if (relX - prevW < tw - relX) {
                    return sl.start + c - 1;
                }
            }
            return sl.start + c;
        }
    }
    return sl.start + sl.length;
}

void UITextArea::scrollToCursor() {
    ensureCursorVisible();
}

// ── Make sure the cursor line is visible in the viewport ──
void UITextArea::ensureCursorVisible() {
    if (_lineCount == 0) return;

    // Find which line the cursor is on
    int cursorLine = 0;
    for (int i = 0; i < _lineCount; i++) {
        int lineEnd = _lines[i].start + _lines[i].length;
        if (_cursorPos <= lineEnd) {
            cursorLine = i;
            break;
        }
        if (i == _lineCount - 1) cursorLine = i;
    }

    // Calculate the Y position of that line
    int16_t lineTop = 0;
    for (int i = 0; i < cursorLine; i++) {
        lineTop += _lines[i].height;
    }
    int16_t lineBottom = lineTop + _lines[cursorLine].height;

    int16_t innerH = _h - TAB5_PADDING * 2;

    // Scroll if cursor line is above or below the visible area
    if (lineTop < _scrollOffset) {
        _scrollOffset = lineTop;
    } else if (lineBottom > _scrollOffset + innerH) {
        _scrollOffset = lineBottom - innerH;
    }
    clampScroll();
}

void UITextArea::draw(LovyanGFX& gfx) {
    if (!_visible) return;

    // Reflow if needed
    if (_needsWrap) {
        reflow(gfx);
        if (_focused) ensureCursorVisible();
    }

    // Resolve pending cursor-placement tap (requires gfx for text measurement)
    if (_pendingTap) {
        _pendingTap = false;
        _cursorPos = cursorFromTouch(gfx, _pendingTapX, _pendingTapY);
    }

    // ── Try sprite-buffered rendering for flicker-free scrolling ──
    M5Canvas* spr = acquireSprite(&gfx, _w, _h);
    LovyanGFX& dst = spr ? (LovyanGFX&)*spr : gfx;
    int16_t ox = spr ? 0 : _x;
    int16_t oy = spr ? 0 : _y;

    // Background
    dst.fillRect(ox, oy, _w, _h, rgb888(_bgColor));

    // Border (highlight when focused)
    uint32_t bc = _focused ? rgb888(_focusBorderColor) : rgb888(_borderColor);
    dst.drawRect(ox, oy, _w, _h, bc);
    if (_focused) {
        dst.drawRect(ox + 1, oy + 1, _w - 2, _h - 2, bc);  // 2px border
    }

    int16_t innerX = ox + TAB5_PADDING;
    int16_t innerY = oy + TAB5_PADDING;
    int16_t innerW = _w - TAB5_PADDING * 2 - TAB5_LIST_SCROLLBAR_W - 2;
    int16_t innerH = _h - TAB5_PADDING * 2;

    // Clip to content area
    dst.setClipRect(ox + 1, oy + 1, _w - 2, _h - 2);

    dst.setTextSize(_textSize);
    dst.setTextDatum(textdatum_t::top_left);

    if (_text[0] == '\0' && !_focused) {
        // Show placeholder
        dst.setTextColor(rgb888(_phColor));
        dst.drawString(_placeholder, innerX, innerY);
    } else {
        dst.setTextColor(rgb888(_textColor));

        // Find which display line the cursor is on
        int cursorLine = -1;
        int cursorCharInLine = -1;
        if (_focused) {
            for (int i = 0; i < _lineCount; i++) {
                int lineEnd = _lines[i].start + _lines[i].length;
                if (_cursorPos >= _lines[i].start && _cursorPos <= lineEnd) {
                    cursorLine = i;
                    cursorCharInLine = _cursorPos - _lines[i].start;
                    break;
                }
            }
            // If cursor is at end of text past all lines, put it at end of last line
            if (cursorLine < 0 && _lineCount > 0) {
                cursorLine = _lineCount - 1;
                cursorCharInLine = _lines[cursorLine].length;
            }
        }

        // Draw visible lines
        int16_t curY = innerY - _scrollOffset;

        for (int i = 0; i < _lineCount; i++) {
            const TextAreaLine& sl = _lines[i];
            int16_t lineY = curY;
            curY += sl.height;

            // Skip lines outside visible area
            if (lineY + sl.height <= oy) continue;
            if (lineY >= oy + _h) break;

            if (sl.length > 0) {
                char buf[257];
                int drawLen = (sl.length > 255) ? 255 : sl.length;
                memcpy(buf, _text + sl.start, drawLen);
                buf[drawLen] = '\0';
                dst.setTextColor(rgb888(_textColor));
                dst.drawString(buf, innerX, lineY);
            }

            // Draw cursor on this line
            if (_focused && i == cursorLine) {
                int16_t cx;
                if (cursorCharInLine > 0) {
                    char buf[257];
                    int cLen = (cursorCharInLine > 255) ? 255 : cursorCharInLine;
                    memcpy(buf, _text + sl.start, cLen);
                    buf[cLen] = '\0';
                    cx = innerX + dst.textWidth(buf);
                } else {
                    cx = innerX;
                }
                int16_t cy1 = lineY + 2;
                int16_t cy2 = lineY + sl.height - 4;
                dst.drawFastVLine(cx, cy1, cy2 - cy1, rgb888(Tab5Theme::TEXT_PRIMARY));
                dst.drawFastVLine(cx + 1, cy1, cy2 - cy1, rgb888(Tab5Theme::TEXT_PRIMARY));
            }
        }
    }

    // Clear clip
    dst.clearClipRect();

    // Scrollbar (only if content overflows)
    int16_t contentH = totalContentHeight();
    if (contentH > innerH) {
        int16_t sbX = ox + _w - TAB5_LIST_SCROLLBAR_W - 1;
        int16_t sbAreaH = _h - 2;

        // Scrollbar track
        dst.fillRect(sbX, oy + 1, TAB5_LIST_SCROLLBAR_W, sbAreaH,
                     rgb888(darken(_bgColor, 60)));

        // Scrollbar thumb
        float visibleRatio = (float)innerH / (float)contentH;
        int16_t thumbH = (int16_t)(sbAreaH * visibleRatio);
        if (thumbH < 20) thumbH = 20;

        int16_t ms = maxScroll();
        float scrollRatio = (ms > 0) ? (float)_scrollOffset / (float)ms : 0.0f;
        int16_t thumbY = oy + 1 + (int16_t)((sbAreaH - thumbH) * scrollRatio);

        dst.fillSmoothRoundRect(sbX, thumbY, TAB5_LIST_SCROLLBAR_W, thumbH,
                                 3, rgb888(Tab5Theme::TEXT_DISABLED));
    }

    // Push sprite to display in one transfer (flicker-free)
    if (spr) {
        spr->pushSprite(&gfx, _x, _y);
    }

    _dirty = false;
}

void UITextArea::handleTouchDown(int16_t tx, int16_t ty) {
    if (!hitTest(tx, ty)) return;
    _pressed = true;
    _dragging = false;
    _wasDrag = false;
    _touchStartY = ty;
    _touchDownX = tx;
    _touchDownY = ty;
    _scrollStart = _scrollOffset;
    if (_onTouch) _onTouch(TouchEvent::TOUCH);
}

void UITextArea::handleTouchMove(int16_t tx, int16_t ty) {
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

void UITextArea::handleTouchUp(int16_t tx, int16_t ty) {
    if (!_pressed) return;
    _pressed = false;

    if (!_wasDrag) {
        // This was a tap, not a drag
        if (!_focused) {
            // First tap opens the keyboard
            focus();
        } else {
            // Already focused — schedule cursor placement (needs gfx for measurement)
            _pendingTap = true;
            _pendingTapX = tx;
            _pendingTapY = ty;
            _dirty = true;
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
    bool anyDrawn = false;
    for (auto* elem : _elements) {
        if (!elem->isVisible()) continue;

        if (elem->isTabView()) {
            UITabView* tv = static_cast<UITabView*>(elem);
            if (tv->isDirty()) {
                // Full redraw (page switch, tab bar change, etc.)
                tv->draw(_gfx);
                tv->setDirty(false);
                anyDrawn = true;
            } else if (tv->hasActiveDirtyChild()) {
                // Partial redraw — only dirty children, no background clear
                tv->drawDirtyChildren(_gfx);
                anyDrawn = true;
            }
        } else if (elem->isDirty()) {
            elem->draw(_gfx);
            elem->setDirty(false);
            anyDrawn = true;
        }
    }

    // If anything was redrawn and a modal overlay (keyboard, popup, menu) is
    // visible and dirty, redraw it on top so it isn't covered by a widget
    // that painted over its area (e.g. a UITextArea that extends beneath
    // the keyboard).  Only redraw if the overlay is actually dirty —
    // unconditional redraws cause a flash on every keystroke.
    if (anyDrawn) {
        for (auto* elem : _elements) {
            if (!elem->isVisible() || !elem->isDirty()) continue;
            if (elem->isKeyboard() || elem->isPopup() || elem->isMenu()) {
                elem->draw(_gfx);
                elem->setDirty(false);
            }
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

void UIManager::setSleepTimeout(uint32_t minutes) {
    _sleepTimeoutMin = minutes;
    _lastActivityTime = millis();
}

void UIManager::setBrightness(uint8_t b) {
    _brightness = b;
    if (!_screenAsleep) _gfx.setBrightness(b);
}

void UIManager::setLightSleep(bool enable) {
    _lightSleepEnabled = enable;
}

void UIManager::wake() {
    if (!_screenAsleep) return;
    _screenAsleep = false;
    _gfx.setBrightness(_brightness);
    _lastActivityTime = millis();
    if (_onWake) _onWake();
}

void UIManager::sleep() {
    if (_screenAsleep) return;
    _screenAsleep = true;
    _gfx.setBrightness(0);
    if (_onSleep) _onSleep();

#if defined(ESP32)
    if (_lightSleepEnabled) {
        // ── Low-power idle with touch-to-wake ──
        // The backlight is already off (the dominant power draw).
        // We poll getTouch() which internally checks the GT911 INT pin
        // via lgfx::gpio_in() — when INT is HIGH (no touch) it returns
        // immediately with zero I2C traffic.  delay() yields to the
        // FreeRTOS idle task so the CPU stays mostly in WFI.

        // Drain any pending touch data
        lgfx::touch_point_t tp;
        while (_gfx.getTouch(&tp, 1) > 0) { delay(2); }

        // Block until a new touch is detected
        while (_gfx.getTouch(&tp, 1) == 0) {
            delay(50);
        }

        // Drain remaining touch events so the wake touch is consumed
        delay(10);
        while (_gfx.getTouch(&tp, 1) > 0) { delay(2); }

        // ── Fully wake the display ──
        wake();
    }
#endif
}

void UIManager::update() {
    // Lazy-init content bottom from runtime screen height
    if (_contentBottom == 0) _contentBottom = Tab5UI::screenH();

    unsigned long now = millis();

    // ── Screen sleep timeout check ──
    if (_sleepTimeoutMin > 0 && !_screenAsleep) {
        unsigned long timeoutMs = (unsigned long)_sleepTimeoutMin * 60000UL;
        if (now - _lastActivityTime >= timeoutMs) {
            sleep();
            // If light sleep was active, sleep() blocks until touch-wake
            // and calls wake() internally.  Restart update() so 'now' is
            // refreshed — otherwise the stale timestamp triggers re-sleep.
            return;
        }
    }

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

        // If the screen is asleep, wake it and consume this touch
        if (_screenAsleep) {
            wake();
            _wasTouched = true;   // Suppress until finger lifts
            _lastTouchX = tx;
            _lastTouchY = ty;
            _lastTouchTime = now;
            return;
        }

        // Any touch counts as activity for sleep timer
        _lastActivityTime = now;

        if (!_wasTouched) {
            _touchedElem = nullptr;
            _touchStartX = tx;
            _touchStartY = ty;

            // If a modal overlay is open, it captures all touch
            if (modalElem) {
                // Keyboard special case: if the touch is outside the keyboard,
                // fall through to normal hit-testing so the UITextArea (or other
                // widgets) can still receive cursor-placement taps and scroll drags
                // while the keyboard is open.
                if (modalElem->isKeyboard() && !modalElem->hitTest(tx, ty)) {
                    // Normal hit-testing (reverse for z-order), skip the keyboard
                    for (int i = (int)_elements.size() - 1; i >= 0; --i) {
                        UIElement* elem = _elements[i];
                        if (!elem->isVisible() || !elem->isEnabled()) continue;
                        if (elem->isKeyboard()) continue;  // Skip keyboard itself

                        bool hit = elem->isCircleIcon()
                                 ? static_cast<UIIconCircle*>(elem)->hitTestCircle(tx, ty)
                                 : elem->hitTest(tx, ty);

                        if (hit) {
                            _touchedElem = elem;
                            elem->handleTouchDown(tx, ty);
                            break;
                        }
                    }
                } else {
                    _touchedElem = modalElem;
                    modalElem->handleTouchDown(tx, ty);
                }
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

            // If a modal overlay just closed, erase its footprint and
            // mark overlapping elements dirty.  Avoid marking the TabView
            // container itself dirty — that would trigger UITabView::draw()
            // which clears the entire content area with fillRect, causing a
            // visible flash.  Instead, drawDirtyChildren() will repaint only
            // the affected children without a background clear.
            if (wasModal && !_touchedElem->isVisible()) {
                // Erase the modal footprint (including shadow offset)
                int16_t mx = _touchedElem->getX();
                int16_t my = _touchedElem->getY();
                int16_t mw = _touchedElem->getWidth() + 4;  // shadow offset
                int16_t mh = _touchedElem->getHeight() + 4;
                _gfx.fillRect(mx, my, mw, mh, rgb888(_bgColor));

                // Mark elements that overlap the modal footprint as dirty
                for (auto* e : _elements) {
                    if (!e->isVisible()) continue;
                    // For TabViews, mark overlapping children dirty (not the
                    // TabView itself) so drawDirtyChildren() handles them.
                    if (e->isTabView()) {
                        UITabView* tv = static_cast<UITabView*>(e);
                        int ap = tv->getActivePage();
                        if (ap >= 0) {
                            int cc = tv->getChildCount(ap);
                            for (int ci = 0; ci < cc; ci++) {
                                UIElement* child = tv->getChild(ap, ci);
                                if (child && child->isVisible()) {
                                    // Check overlap with modal footprint
                                    int16_t cx = child->getX();
                                    int16_t cy = child->getY();
                                    int16_t cw = child->getWidth();
                                    int16_t ch = child->getHeight();
                                    if (cx < mx + mw && cx + cw > mx &&
                                        cy < my + mh && cy + ch > my) {
                                        child->setDirty(true);
                                    }
                                }
                            }
                        }
                        // Redraw the tab bar cheaply (no fillRect flash)
                        tv->drawTabBar(_gfx);
                    } else if (e != _touchedElem) {
                        int16_t ex = e->getX();
                        int16_t ey = e->getY();
                        int16_t ew = e->getWidth();
                        int16_t eh = e->getHeight();
                        if (ex < mx + mw && ex + ew > mx &&
                            ey < my + mh && ey + eh > my) {
                            e->setDirty(true);
                        }
                    }
                }
            }
            _touchedElem = nullptr;
        }
        _wasTouched = false;
        // Don't reset _lastTouchTime on release — allows immediate
        // re-touch for fast keyboard typing without debounce delay.
    }

    // Redraw dirty elements
    drawDirty();
}
