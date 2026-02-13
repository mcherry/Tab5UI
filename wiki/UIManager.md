# UIManager

```cpp
UIManager(M5GFX& gfx);
void addElement(UIElement* element);
void removeElement(UIElement* element);
void clearElements();
void setBackground(uint32_t color);
void clearScreen();
void drawAll();           // Full redraw
void drawDirty();         // Only changed elements
void update();            // Touch + dirty redraw (call in loop)
UIElement* findByTag(const char* tag);
void setContentArea(int16_t top, int16_t bottom);

// Screen sleep
void setSleepTimeout(uint32_t minutes); // 0 = never (default)
uint32_t getSleepTimeout() const;
bool isScreenAsleep() const;
void wake();                            // Manually wake
void sleep();                           // Manually sleep
void setBrightness(uint8_t b);          // Set brightness (wake level)
void setLightSleep(bool enable);        // Low-power idle when screen off
bool getLightSleep() const;
void setOnSleep(SleepCallback cb);      // Called just before sleeping
void setOnWake(SleepCallback cb);       // Called after waking up
```

---

## Element Management

- `addElement()` registers a widget for touch dispatch and dirty redraws.
- `removeElement()` unregisters a widget without destroying it.
- `findByTag()` looks up an element by its `setTag()` name.
- Elements are drawn in registration order — add modals (menus, keyboards, popups) **last** so they draw on top.

## Drawing

- `drawAll()` redraws every registered element (use once in `setup()`).
- `drawDirty()` redraws only elements whose `isDirty()` flag is set.
- `update()` is the main loop call — it polls touch, dispatches events, and calls `drawDirty()`.

## Content Area

`setContentArea(top, bottom)` defines the vertical range where most widgets live. This is used by some widgets for positioning calculations. Typically set to `(TAB5_TITLE_H, screenH() - TAB5_STATUS_H)`.

---

## Screen Sleep & Low-Power Idle

The sleep system has two modes:

### Backlight-only sleep (default)
Setting `setSleepTimeout(minutes)` turns off the backlight after the specified idle period. The CPU continues running `loop()` normally. Useful when background tasks (data logging, network polling) must not be interrupted.

### Light sleep (touch-to-wake)
Calling `setLightSleep(true)` enables low-power idle mode. When the screen sleeps:

1. The backlight turns off.
2. The CPU enters a tight GPIO-polling loop using `delay()` calls (which yield to the FreeRTOS idle task, minimizing power draw).
3. The GT911 touch controller pulls its INT pin (GPIO 23) LOW on touch.
4. The loop detects the touch within ~20 ms without any I2C traffic.
5. The wake touch is consumed (not passed to widgets) to prevent accidental button presses.

```cpp
ui.setSleepTimeout(5);              // Sleep after 5 min idle
ui.setLightSleep(true);             // Enable low-power idle + touch wake
ui.setOnWake([]() {
    statusBar.setText("Awake!");
});
```

---

**Next:** [[Rendering]] · [[Screenshots]] · [[Tips & Best Practices]]
