# Widgets – Selection

## UICheckbox

```cpp
UICheckbox(x, y, w, h, "label", checked, boxColor, textColor, textSize);

// State
void setChecked(bool c);              // Set checked state
bool isChecked() const;               // Get checked state

// Label
void setLabel(const char* label);
const char* getLabel() const;

// Appearance
void setBoxColor(uint32_t c);         // Checkbox fill when checked (default: PRIMARY)
uint32_t getBoxColor() const;
void setCheckColor(uint32_t c);       // Checkmark color (default: TEXT_PRIMARY)
uint32_t getCheckColor() const;
void setTextColor(uint32_t c);
uint32_t getTextColor() const;
void setTextSize(float s);
float getTextSize() const;
void setBorderColor(uint32_t c);
```

**Behavior:** Displays a 28×28 rounded checkbox with an adjacent text label.
Tapping anywhere on the widget toggles the checked state.  When checked, the
box fills with `boxColor` and displays a checkmark in `checkColor`.  When
unchecked, the box shows an empty bordered square.  The `TOUCH_RELEASE`
callback fires after the state has been toggled, so `isChecked()` returns
the new value inside the callback.

---

## UIRadioButton / UIRadioGroup

```cpp
// Group (manages mutual exclusion)
UIRadioGroup group;
group.addButton(UIRadioButton* btn);  // Add a button to the group
group.select(UIRadioButton* btn);     // Programmatically select a button
int  getSelectedIndex() const;        // Index of selected button (-1 if none)
UIRadioButton* getSelected() const;   // Pointer to selected button

// Button
UIRadioButton(x, y, w, h, "label", &group, circleColor, textColor, textSize);

// State
void setSelected(bool s);             // Set selection state
bool isSelected() const;              // Get selection state

// Group
void setGroup(UIRadioGroup* g);       // Assign to a group
UIRadioGroup* getGroup() const;

// Label
void setLabel(const char* label);
const char* getLabel() const;

// Appearance
void setCircleColor(uint32_t c);      // Outer ring color (default: PRIMARY)
uint32_t getCircleColor() const;
void setDotColor(uint32_t c);         // Inner dot color (default: TEXT_PRIMARY)
uint32_t getDotColor() const;
void setTextColor(uint32_t c);
uint32_t getTextColor() const;
void setTextSize(float s);
float getTextSize() const;
void setBorderColor(uint32_t c);
```

**Behavior:** Displays a circular radio button (radius 14) with an adjacent
text label.  When a `UIRadioGroup` is provided (via constructor or
`setGroup()`), tapping one button automatically deselects the previously
selected button in the group.  The first button added to a group is selected
by default.  The group supports up to 12 buttons.  Pass the group pointer in
the constructor to auto-register, or call `group.addButton()` manually.

---

**Next:** [[UIManager]] · [[Rendering]] · [[Screenshots]]
