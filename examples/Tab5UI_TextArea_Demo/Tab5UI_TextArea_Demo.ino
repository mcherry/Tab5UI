/*******************************************************************************
 * Tab5UI_TextArea_Demo.ino — UITextArea demo for the Tab5UI library
 *
 * Demonstrates the multi-line text input widget in portrait orientation
 * (720×1280).  Shows the UITextArea with word wrapping, touch scrolling,
 * tap-to-place cursor, and the on-screen keyboard in portrait mode.
 *
 * Board:   M5Stack Tab5
 * Library: M5GFX (install via Arduino Library Manager)
 ******************************************************************************/

#include <M5GFX.h>
#include "Tab5UI.h"

// ── Display & UI Manager ────────────────────────────────────────────────────
M5GFX display;
UIManager ui(display);

// ── Runtime layout ──────────────────────────────────────────────────────────
static int16_t screenW;
static int16_t screenH;

// ── UI Elements ─────────────────────────────────────────────────────────────
UITitleBar  titleBar("Text Area Demo");
UIStatusBar statusBar("Tap the text area to start typing");

// Placeholder geometry — repositioned in setup()
UITextArea textArea(0, 0, 100, 100, "Type your notes here...");
UIKeyboard keyboard;

UILabel charCount(0, 0, 100, TAB5_LABEL_H, "Characters: 0");

UIButton btnClear(0, 0, 100, TAB5_BTN_H, "Clear", Tab5Theme::DANGER);
UIButton btnPreview(0, 0, 100, TAB5_BTN_H, "Preview", Tab5Theme::PRIMARY);

UIInfoPopup previewPopup("Text Preview", "");

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    display.init();
    display.setRotation(0);                // ◀ Portrait (720 × 1280)
    Tab5UI::init(display);
    ui.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    screenW = Tab5UI::screenW();           // 720
    screenH = Tab5UI::screenH();           // 1280

    // ── Layout for portrait ─────────────────────────────────────────────
    int16_t pad    = TAB5_PADDING;
    int16_t areaX  = pad;
    int16_t areaY  = TAB5_TITLE_H + pad;
    int16_t areaW  = screenW - pad * 2;

    // Text area fills from below the title bar to above the buttons
    // Leave room for: char count label + two buttons + status bar + gaps
    int16_t bottomControls = TAB5_LABEL_H + pad + TAB5_BTN_H + pad + TAB5_STATUS_H;
    int16_t areaH  = screenH - TAB5_TITLE_H - bottomControls - pad * 2;

    textArea.setPosition(areaX, areaY);
    textArea.setSize(areaW, areaH);
    textArea.setTextSize(TAB5_FONT_SIZE_MD);
    textArea.attachKeyboard(&keyboard);

    // Character count label
    int16_t labelY = areaY + areaH + pad;
    charCount.setPosition(areaX, labelY);
    charCount.setSize(areaW, TAB5_LABEL_H);
    charCount.setTextColor(Tab5Theme::TEXT_SECONDARY);
    charCount.setTextSize(TAB5_FONT_SIZE_SM);

    // Buttons row
    int16_t btnY = labelY + TAB5_LABEL_H + pad;
    int16_t btnW = (areaW - pad) / 2;
    btnClear.setPosition(areaX, btnY);
    btnClear.setSize(btnW, TAB5_BTN_H);
    btnPreview.setPosition(areaX + btnW + pad, btnY);
    btnPreview.setSize(btnW, TAB5_BTN_H);

    // ── Callbacks ───────────────────────────────────────────────────────

    // Update character count as user types
    textArea.setOnChange([](const char* text) {
        int len = (int)strlen(text);
        char buf[48];
        snprintf(buf, sizeof(buf), "Characters: %d", len);
        charCount.setText(buf);
    });

    // Submit callback (Done key)
    textArea.setOnSubmit([](const char* text) {
        int len = (int)strlen(text);
        char buf[48];
        snprintf(buf, sizeof(buf), "Saved (%d chars)", len);
        statusBar.setText(buf);
    });

    // Clear button
    btnClear.setOnTouchRelease([](TouchEvent e) {
        textArea.clear();
        charCount.setText("Characters: 0");
        statusBar.setText("Text cleared");
    });

    // Preview button — show text in popup
    btnPreview.setOnTouchRelease([](TouchEvent e) {
        const char* text = textArea.getText();
        if (text[0] == '\0') {
            previewPopup.setTitle("Empty");
            previewPopup.setMessage("No text entered yet.");
        } else {
            previewPopup.setTitle("Text Preview");
            previewPopup.setMessage(text);
        }
        previewPopup.show();
    });

    // ── Status bar extras ───────────────────────────────────────────────
    statusBar.setLeftText("Tab5");
    statusBar.setRightText("Portrait");

    // ── Register elements ───────────────────────────────────────────────
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&textArea);
    ui.addElement(&charCount);
    ui.addElement(&btnClear);
    ui.addElement(&btnPreview);
    ui.addElement(&statusBar);
    ui.addElement(&previewPopup);     // Popup on top
    ui.addElement(&keyboard);         // Keyboard on top of everything

    ui.setContentArea(TAB5_TITLE_H, screenH - TAB5_STATUS_H);
    ui.drawAll();
    ui.setSleepTimeout(5);           // Screen off after 5 min idle
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    ui.update();
    yield();
}
