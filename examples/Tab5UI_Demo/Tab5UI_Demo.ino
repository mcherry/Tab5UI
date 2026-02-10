/*******************************************************************************
 * Tab5UI_Demo.ino — Full demo of the Tab5UI library on M5Stack Tab5
 *
 * Demonstrates: TitleBar, StatusBar, Labels, Buttons, TextRows,
 *               Square Icons, Circle Icons, Popup Menu, Text Input
 *               with Keyboard, and touch event handling.
 *
 * Board:   M5Stack Tab5
 * Library: M5GFX (install via Arduino Library Manager)
 ******************************************************************************/

#include <M5GFX.h>
#include "Tab5UI.h"

// ── Display & UI Manager ────────────────────────────────────────────────────
M5GFX display;
UIManager ui(display);

// ── UI Elements ─────────────────────────────────────────────────────────────
UITitleBar  titleBar("Tab5UI Demo");
UIStatusBar statusBar("Ready");

UILabel     headerLabel(TAB5_PADDING, 60, 400, TAB5_LABEL_H,
                        "Touch any element below:");

// Buttons
UIButton    btnAction(TAB5_PADDING, 110, 180, TAB5_BTN_H,
                      "Action", Tab5Theme::PRIMARY);
UIButton    btnToggle(200, 110, 180, TAB5_BTN_H,
                      "Toggle", Tab5Theme::SECONDARY);
UIButton    btnDanger(390, 110, 180, TAB5_BTN_H,
                      "Reset", Tab5Theme::DANGER);

// Text rows (left column only — stop before right column at x=700)
UITextRow   row1(TAB5_PADDING, 180, 670,
                 "WiFi Status", "Connected");
UITextRow   row2(TAB5_PADDING, 220, 670,
                 "Battery", "87%");
UITextRow   row3(TAB5_PADDING, 260, 670,
                 "Temperature", "24.5 C");

// Labels for counters
UILabel     counterLabel(TAB5_PADDING, 310, 500, TAB5_LABEL_H,
                         "Button presses: 0");

// Square icons row
UIIconSquare iconSq1(TAB5_PADDING,       370, 56, Tab5Theme::PRIMARY);
UIIconSquare iconSq2(TAB5_PADDING + 70,  370, 56, Tab5Theme::SECONDARY);
UIIconSquare iconSq3(TAB5_PADDING + 140, 370, 56, Tab5Theme::ACCENT);
UIIconSquare iconSq4(TAB5_PADDING + 210, 370, 56, Tab5Theme::DANGER);

// Circle icons row
UIIconCircle iconCr1(TAB5_PADDING,       450, 28, Tab5Theme::PRIMARY);
UIIconCircle iconCr2(TAB5_PADDING + 70,  450, 28, Tab5Theme::SECONDARY);
UIIconCircle iconCr3(TAB5_PADDING + 140, 450, 28, Tab5Theme::ACCENT);
UIIconCircle iconCr4(TAB5_PADDING + 210, 450, 28, Tab5Theme::DANGER);

// Second column (right side of screen)
UIButton    btnLarge(700, 60, 560, 56,
                     "Large Button", Tab5Theme::SURFACE);

UILabel     infoLabel(700, 130, 560, TAB5_LABEL_H,
                      "Tab5UI Library v1.1");

UITextRow   row4(700, 170, 560, "Display", "1280x720");
UITextRow   row5(700, 210, 560, "Touch", "Capacitive");
UITextRow   row6(700, 250, 560, "UI Elements", "18 types");

// Popup menu (triggered from title bar "< Menu" tap)
UIMenu      mainMenu(TAB5_PADDING, TAB5_TITLE_H + 4, 280);

// Text input + shared keyboard
UIKeyboard  keyboard;
UITextInput inputName(700, 310, 560, "Enter your name...");
UITextInput inputMsg(700, 370, 560, "Type a message...");
UILabel     inputResult(700, 430, 560, TAB5_LABEL_H, "Submitted: (none)");

// Info popup (triggered from About menu item)
UIInfoPopup aboutPopup("About", "Tab5 UI Demo");

// Confirm popup
UIConfirmPopup confirmPopup("Confirm", "Are you sure you want\nto proceed with this action?");
UIButton    btnConfirm(TAB5_PADDING, 600, 280, TAB5_BTN_H,
                       "Confirm Action", Tab5Theme::ACCENT);
UILabel     confirmResult(300, 600, 370, TAB5_LABEL_H,
                          "Result: (waiting)");

// Checkboxes
UICheckbox  chk1(TAB5_PADDING, 510, 280, 40, "Enable WiFi", true);
UICheckbox  chk2(TAB5_PADDING, 550, 280, 40, "Notifications");

// Radio buttons
UIRadioGroup radioGroup;
UIRadioButton radio1(300, 510, 260, 40, "Option A", &radioGroup);
UIRadioButton radio2(300, 550, 260, 40, "Option B", &radioGroup);
UIRadioButton radio3(560, 510, 260, 40, "Option C", &radioGroup);

// ── State ───────────────────────────────────────────────────────────────────
int pressCount = 0;
bool toggleState = false;

// ── Helper: Update counter display ──────────────────────────────────────────
void updateCounter() {
    char buf[64];
    snprintf(buf, sizeof(buf), "Button presses: %d", pressCount);
    counterLabel.setText(buf);
}

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    // Initialize display
    display.init();
    display.setRotation(1);           // Landscape
    Tab5UI::init(display);
    display.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // ── Configure Title Bar ──
    titleBar.setLeftText("< Menu");
    titleBar.setRightText("Settings");
    titleBar.setOnLeftTouch([](TouchEvent e) {
        if (e == TouchEvent::TOUCH_RELEASE) {
            mainMenu.show();   // Open popup menu
        }
    });
    titleBar.setOnRightTouch([](TouchEvent e) {
        if (e == TouchEvent::TOUCH_RELEASE) {
            statusBar.setText("Settings pressed");
        }
    });

    // ── Configure Popup Menu ──
    mainMenu.addItem("New Project", [](TouchEvent e) {
        statusBar.setText("New Project selected");
    });
    mainMenu.addItem("Open File", [](TouchEvent e) {
        statusBar.setText("Open File selected");
    });
    mainMenu.addItem("Save", [](TouchEvent e) {
        statusBar.setText("Save selected");
    });
    mainMenu.addSeparator();
    mainMenu.addItem("About", [](TouchEvent e) {
        aboutPopup.show();
    });
    mainMenu.setItemEnabled(2, false);  // Disable "Save" as a demo
    mainMenu.setOnDismiss([](TouchEvent e) {
        statusBar.setText("Menu dismissed");
    });

    // ── Configure Text Inputs ──
    inputName.attachKeyboard(&keyboard);
    inputName.setMaxLength(32);
    inputName.setOnSubmit([](const char* text) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Submitted: %s", text);
        inputResult.setText(buf);
        statusBar.setText("Name submitted!");
    });
    inputName.setOnChange([](const char* text) {
        statusBar.setText(text);
    });

    inputMsg.attachKeyboard(&keyboard);
    inputMsg.setMaxLength(64);
    inputMsg.setOnSubmit([](const char* text) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Msg: %s", text);
        statusBar.setText(buf);
    });

    inputResult.setTextColor(Tab5Theme::ACCENT);
    inputResult.setTextSize(TAB5_FONT_SIZE_SM);

    // ── Configure Buttons ──
    btnAction.setOnTouchRelease([](TouchEvent e) {
        pressCount++;
        updateCounter();
        statusBar.setText("Action button pressed!");
    });

    btnToggle.setOnTouchRelease([](TouchEvent e) {
        toggleState = !toggleState;
        if (toggleState) {
            btnToggle.setLabel("ON");
            btnToggle.setBgColor(Tab5Theme::SECONDARY);
            statusBar.setText("Toggle: ON");
        } else {
            btnToggle.setLabel("OFF");
            btnToggle.setBgColor(Tab5Theme::BORDER);
            statusBar.setText("Toggle: OFF");
        }
    });

    btnDanger.setOnTouchRelease([](TouchEvent e) {
        pressCount = 0;
        updateCounter();
        toggleState = false;
        btnToggle.setLabel("Toggle");
        btnToggle.setBgColor(Tab5Theme::SECONDARY);
        statusBar.setText("Counters reset!");
    });

    btnLarge.setBorderColor(Tab5Theme::PRIMARY);
    btnLarge.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Large button tapped!");
    });

    // ── Configure Text Rows ──
    row1.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("WiFi row tapped");
    });
    row2.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Battery row tapped");
    });
    row3.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Temperature row tapped");
    });

    // ── Configure Square Icons ──
    iconSq1.setIconChar("W");
    iconSq2.setIconChar("B");
    iconSq3.setIconChar("T");
    iconSq4.setIconChar("!");

    iconSq1.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("WiFi icon tapped");
    });
    iconSq2.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Bluetooth icon tapped");
    });
    iconSq3.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Temp icon tapped");
    });
    iconSq4.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Alert icon tapped");
    });

    // ── Configure Circle Icons ──
    iconCr1.setIconChar("1");
    iconCr2.setIconChar("2");
    iconCr3.setIconChar("3");
    iconCr4.setIconChar("4");

    iconCr1.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Circle 1 tapped");
    });
    iconCr2.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Circle 2 tapped");
    });
    iconCr3.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Circle 3 tapped");
    });
    iconCr4.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Circle 4 tapped");
    });

    // ── Style tweaks ──
    headerLabel.setTextColor(Tab5Theme::TEXT_SECONDARY);
    headerLabel.setTextSize(TAB5_FONT_SIZE_SM);

    infoLabel.setTextColor(Tab5Theme::TEXT_SECONDARY);
    infoLabel.setTextSize(TAB5_FONT_SIZE_SM);
    infoLabel.setAlign(textdatum_t::middle_left);

    // ── Configure Confirm Popup ──
    confirmResult.setTextColor(Tab5Theme::ACCENT);
    confirmResult.setTextSize(TAB5_FONT_SIZE_SM);
    confirmResult.setAlign(textdatum_t::middle_left);

    btnConfirm.setOnTouchRelease([](TouchEvent e) {
        confirmPopup.show();
    });

    confirmPopup.setOnConfirm([](ConfirmResult result) {
        if (result == ConfirmResult::YES) {
            confirmResult.setText("Result: Yes");
            statusBar.setText("Confirmed: Yes");
        } else {
            confirmResult.setText("Result: No");
            statusBar.setText("Confirmed: No");
        }
    });

    // ── Configure Checkboxes ──
    chk1.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText(chk1.isChecked() ? "WiFi enabled" : "WiFi disabled");
    });
    chk2.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText(chk2.isChecked() ? "Notifications on" : "Notifications off");
    });

    // ── Configure Radio Buttons ──
    radio1.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Selected: Option A");
    });
    radio2.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Selected: Option B");
    });
    radio3.setOnTouchRelease([](TouchEvent e) {
        statusBar.setText("Selected: Option C");
    });

    row6.setShowDivider(false);

    // ── Status bar config ──
    statusBar.setLeftText("Tab5");
    statusBar.setRightText("v1.1");

    // ── Register all elements with the UI manager ──
    // (draw order = registration order; touch checks in reverse)
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&headerLabel);
    ui.addElement(&btnAction);
    ui.addElement(&btnToggle);
    ui.addElement(&btnDanger);
    ui.addElement(&row1);
    ui.addElement(&row2);
    ui.addElement(&row3);
    ui.addElement(&counterLabel);
    ui.addElement(&iconSq1);
    ui.addElement(&iconSq2);
    ui.addElement(&iconSq3);
    ui.addElement(&iconSq4);
    ui.addElement(&iconCr1);
    ui.addElement(&iconCr2);
    ui.addElement(&iconCr3);
    ui.addElement(&iconCr4);
    ui.addElement(&chk1);
    ui.addElement(&chk2);
    ui.addElement(&radio1);
    ui.addElement(&radio2);
    ui.addElement(&radio3);
    ui.addElement(&btnConfirm);
    ui.addElement(&confirmResult);
    ui.addElement(&btnLarge);
    ui.addElement(&infoLabel);
    ui.addElement(&row4);
    ui.addElement(&row5);
    ui.addElement(&row6);
    ui.addElement(&statusBar);
    ui.addElement(&inputName);
    ui.addElement(&inputMsg);
    ui.addElement(&inputResult);
    ui.addElement(&mainMenu);         // Menu added near-last — draws on top
    ui.addElement(&keyboard);         // Keyboard added near-last
    ui.addElement(&aboutPopup);       // Popup drawn on top of everything
    ui.addElement(&confirmPopup);     // Confirm popup also on top

    // Set content area between title and status bars
    ui.setContentArea(TAB5_TITLE_H, TAB5_SCREEN_H - TAB5_STATUS_H);

    // Initial full draw
    ui.drawAll();
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    // Process touch events and redraw dirty elements
    ui.update();

    // yield to RTOS (watchdog, WiFi, etc.) without blocking
    yield();
}
