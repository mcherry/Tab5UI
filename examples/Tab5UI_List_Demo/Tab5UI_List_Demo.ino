/*******************************************************************************
 * Tab5UI_List_Demo.ino — UIList demo for the Tab5UI library
 *
 * Demonstrates the scrollable UIList widget with item selection.
 * A button reads the currently-selected item and displays it in
 * an info popup.
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
UITitleBar  titleBar("List Demo");
UIStatusBar statusBar("Select an item from the list");

// Scrollable list — left side of screen
UIList myList(TAB5_PADDING, TAB5_TITLE_H + TAB5_PADDING,
              600, TAB5_SCREEN_H - TAB5_TITLE_H - TAB5_STATUS_H - TAB5_PADDING * 2);

// Right-side controls
UILabel     selLabel(640, TAB5_TITLE_H + TAB5_PADDING, 620, TAB5_LABEL_H,
                     "Selected: (none)");

UIButton    btnShow(640, TAB5_TITLE_H + TAB5_PADDING + 50, 280, TAB5_BTN_H,
                    "Show Selected", Tab5Theme::PRIMARY);

UIButton    btnClear(940, TAB5_TITLE_H + TAB5_PADDING + 50, 280, TAB5_BTN_H,
                     "Clear Selection", Tab5Theme::DANGER);

UIInfoPopup popup("Selection", "No item selected");

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    display.init();
    display.setRotation(1);           // Landscape
    display.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // ── Populate the list with sample items ──
    const char* fruits[] = {
        "Apple", "Banana", "Cherry", "Date", "Elderberry",
        "Fig", "Grape", "Honeydew", "Indian Fig", "Jackfruit",
        "Kiwi", "Lemon", "Mango", "Nectarine", "Orange",
        "Papaya", "Quince", "Raspberry", "Strawberry", "Tangerine",
        "Ugli Fruit", "Vanilla Bean", "Watermelon", "Ximenia", "Yuzu",
        "Zucchini"
    };
    for (int i = 0; i < 26; i++) {
        myList.addItem(fruits[i]);
    }

    // ── List selection callback ──
    myList.setOnSelect([](int index, const char* text) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Selected: %s (#%d)", text, index + 1);
        selLabel.setText(buf);
        statusBar.setText(buf);
    });

    // ── Show Selected button ──
    btnShow.setOnTouchRelease([](TouchEvent e) {
        int idx = myList.getSelectedIndex();
        if (idx >= 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "You selected item #%d:\n%s",
                     idx + 1, myList.getSelectedText());
            popup.setTitle("Selection");
            popup.setMessage(buf);
        } else {
            popup.setTitle("No Selection");
            popup.setMessage("Please tap an item\nin the list first.");
        }
        popup.show();
    });

    // ── Clear Selection button ──
    btnClear.setOnTouchRelease([](TouchEvent e) {
        myList.clearSelection();
        selLabel.setText("Selected: (none)");
        statusBar.setText("Selection cleared");
    });

    // ── Style tweaks ──
    selLabel.setTextColor(Tab5Theme::ACCENT);
    selLabel.setTextSize(TAB5_FONT_SIZE_MD);

    statusBar.setLeftText("Tab5");
    statusBar.setRightText("List Demo");

    // ── Register all elements ──
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&myList);
    ui.addElement(&selLabel);
    ui.addElement(&btnShow);
    ui.addElement(&btnClear);
    ui.addElement(&statusBar);
    ui.addElement(&popup);           // Popup drawn on top

    ui.setContentArea(TAB5_TITLE_H, TAB5_SCREEN_H - TAB5_STATUS_H);
    ui.drawAll();
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    ui.update();
    delay(10);
}
