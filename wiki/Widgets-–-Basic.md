# Widgets – Basic

## UILabel

```cpp
UILabel(x, y, w, h, "text", color, textSize);
void setText(const char* text);
const char* getText() const;
void setTextColor(uint32_t color);
void setTextSize(float s);
void setBgColor(uint32_t color);
void setAlign(textdatum_t datum);
```

---

## UIButton

```cpp
UIButton(x, y, w, h, "label", bgColor, textColor, textSize);
void setLabel(const char* label);
const char* getLabel() const;
void setBgColor(uint32_t c);
void setPressedColor(uint32_t c);
void setCornerRadius(int16_t r);
void setBorderColor(uint32_t c);
```

---

## UIIconButton

A button that displays a 32×32 PROGMEM PNG icon instead of text.  Falls back to label text if no icon data is provided.  Has the same visual style and touch behavior as `UIButton`.

```cpp
#include "icons/icon_home.h"   // Include the desired icon header

UIIconButton(x, y, w, h, "label", iconData, iconSize, bgColor, textColor, textSize);
void setLabel(const char* label);
const char* getLabel() const;
void setIcon(const uint8_t* data, uint32_t size);  // Change icon at runtime
void setBgColor(uint32_t c);
void setPressedColor(uint32_t c);
void setCornerRadius(int16_t r);
void setBorderColor(uint32_t c);
```

**Example:**
```cpp
#include "icons/icon_home.h"

UIIconButton homeBtn(100, 200, 56, 56, "Home", icon_home, icon_home_size, Tab5Theme::PRIMARY);
homeBtn.setOnTouchRelease([](TouchEvent e) {
    Serial.println("Home tapped!");
});
```

> The `icons/` directory contains 55 ready-to-use icon headers converted from [IconPark](https://github.com/bytedance/IconPark) (Apache 2.0).  See `icons/README.md` for the full list.

---

## UISlider

A horizontal slider with a draggable thumb.  Tap anywhere on the track or drag the thumb to set the value.  An `onChange` callback fires whenever the value changes.  Both the label and numeric value display are optional (off by default).

```cpp
UISlider(x, y, w, h, minVal, maxVal, value, trackColor, fillColor, thumbColor);
void setValue(int v);
int  getValue() const;
void setRange(int minVal, int maxVal);
int  getMin() const;
int  getMax() const;
void setTrackColor(uint32_t c);
void setFillColor(uint32_t c);
void setThumbColor(uint32_t c);
void setThumbRadius(int16_t r);
void setLabel(const char* label);    // Built-in label text (drawn above track)
void setShowLabel(bool show);        // Show/hide label (default: off)
void setShowValue(bool show);        // Show/hide numeric value (default: off)
void setOnChange(SliderChangeCallback cb);  // void(int value)
```

**Example:**
```cpp
UISlider brightness(100, 200, 400, 60, 0, 100, 50);
brightness.setShowLabel(true);
brightness.setShowValue(true);
brightness.setLabel("Brightness: 50");
brightness.setOnChange([](int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d", value);
    brightness.setLabel(buf);
});
```

---

## UITitleBar

```cpp
UITitleBar("title", bgColor, textColor);
void setTitle(const char* title);
const char* getTitle() const;
void setLeftText(const char* text);    // e.g. "< Back"
void setRightText(const char* text);   // e.g. "Settings"
const char* getLeftText() const;
const char* getRightText() const;
void setOnLeftTouch(TouchCallback cb);
void setOnRightTouch(TouchCallback cb);
```

---

## UIStatusBar

```cpp
UIStatusBar("text", bgColor, textColor);
void setText(const char* text);
void setLeftText(const char* text);
void setRightText(const char* text);
const char* getText() const;
const char* getLeftText() const;
const char* getRightText() const;
```

---

## UITextRow

```cpp
UITextRow(x, y, w, "Label", "Value", bgColor, labelColor, valueColor);
void setLabel(const char* label);
void setValue(const char* value);
const char* getLabel() const;
const char* getValue() const;
void setShowDivider(bool show);
```

---

## UIIconSquare

```cpp
UIIconSquare(x, y, size, fillColor, borderColor);
void setFillColor(uint32_t c);
void setCornerRadius(int16_t r);
void setIconChar(const char* ch);     // Single char drawn centered
void setIconCharColor(uint32_t c);
```

---

## UIIconCircle

```cpp
UIIconCircle(x, y, radius, fillColor, borderColor);
void setFillColor(uint32_t c);
void setRadius(int16_t r);
void setIconChar(const char* ch);
void setIconCharColor(uint32_t c);
```

---

**Next:** [[Widgets – Input]] · [[Widgets – Lists]] · [[Widgets – Containers & Popups]]
