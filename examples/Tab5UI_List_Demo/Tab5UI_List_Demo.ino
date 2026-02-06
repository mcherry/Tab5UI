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

    // ── Populate the list with sample items (some with icons) ──
    // Items with square icons
    myList.addItem("Apple",       "A", Tab5Theme::DANGER);
    myList.addItem("Banana",      "B", Tab5Theme::ACCENT);
    myList.addItem("Cherry",      "C", Tab5Theme::DANGER);
    myList.addItem("Date",        "D", Tab5Theme::ACCENT);
    myList.addItem("Elderberry",  "E", Tab5Theme::SECONDARY);

    // Items with circle icons
    myList.addItem("Fig",         "F", Tab5Theme::SECONDARY, true);
    myList.addItem("Grape",       "G", Tab5Theme::PRIMARY,   true);
    myList.addItem("Honeydew",    "H", Tab5Theme::SECONDARY, true);

    // Items without icons
    myList.addItem("Indian Fig");
    myList.addItem("Jackfruit");
    myList.addItem("Kiwi");
    myList.addItem("Lemon");
    myList.addItem("Mango");
    myList.addItem("Nectarine");
    myList.addItem("Orange");

    // Add icons to existing items after the fact
    myList.setItemIcon(12, "M", Tab5Theme::ACCENT);           // Mango — square
    myList.setItemIcon(14, "O", Tab5Theme::PRIMARY, true);    // Orange — circle

    // More items (no icons)
    myList.addItem("Papaya");
    myList.addItem("Quince");
    myList.addItem("Raspberry");
    myList.addItem("Strawberry");
    myList.addItem("Tangerine");
    myList.addItem("Ugli Fruit");
    myList.addItem("Vanilla Bean");
    myList.addItem("Watermelon");
    myList.addItem("Ximenia");
    myList.addItem("Yuzu");
    myList.addItem("Zucchini");

    // Set the list text size (items auto-scale height to fit)
    myList.setTextSize(TAB5_FONT_SIZE_MD);

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
