/*******************************************************************************
 * Tab5UI_Tab_Demo.ino — UITabView demo for the Tab5UI library
 *
 * Demonstrates the UITabView widget with two tab pages:
 *   Tab 1 ("Controls"):  Buttons, labels, text rows, and icons
 *   Tab 2 ("Data List"): A scrollable UIList with selectable items
 *
 * The tab bar is at the top by default.  A button on the Controls tab
 * toggles the tab bar between top and bottom placement.
 *
 * Board:   M5Stack Tab5
 * Library: M5GFX (install via Arduino Library Manager)
 ******************************************************************************/

#include <M5GFX.h>
#include "Tab5UI.h"

// ── Display & UI Manager ────────────────────────────────────────────────────
M5GFX display;
UIManager ui(display);

// ── Global bars ─────────────────────────────────────────────────────────────
UITitleBar  titleBar("Tab Demo");
UIStatusBar statusBar("Switch between tabs");

// ── Tab view ────────────────────────────────────────────────────────────────
// Fills the area between title bar and status bar
static const int16_t TAB_Y = TAB5_TITLE_H;
static const int16_t TAB_H = TAB5_SCREEN_H - TAB5_TITLE_H - TAB5_STATUS_H;

UITabView tabs(0, TAB_Y, TAB5_SCREEN_W, TAB_H, TabPosition::TOP);

// ═════════════════════════════════════════════════════════════════════════════
//  Tab 1 — "Controls"
// ═════════════════════════════════════════════════════════════════════════════

// Content origin relative to tab view's content area
static const int16_t CY = TAB_Y + TAB5_TAB_BAR_H;   // content top (when tabs at top)

// Header label
UILabel headerLabel(TAB5_PADDING, CY + TAB5_PADDING, 500, TAB5_LABEL_H,
                    "Interactive Controls", Tab5Theme::TEXT_SECONDARY, TAB5_FONT_SIZE_SM);

// Buttons
UIButton btnAction(TAB5_PADDING, CY + 50, 200, TAB5_BTN_H,
                   "Action", Tab5Theme::PRIMARY);
UIButton btnToggle(220, CY + 50, 200, TAB5_BTN_H,
                   "Toggle", Tab5Theme::SECONDARY);
UIButton btnAlert(440, CY + 50, 200, TAB5_BTN_H,
                  "Alert", Tab5Theme::DANGER);

// Counter label
UILabel counterLabel(TAB5_PADDING, CY + 120, 500, TAB5_LABEL_H,
                     "Button presses: 0");

// Text rows
UITextRow row1(TAB5_PADDING, CY + 165, 620,
               "Status", "Active");
UITextRow row3(TAB5_PADDING, CY + 205, 620,
               "Tab Position", "Top");

// Square icons
UIIconSquare iconSq1(TAB5_PADDING,       CY + 260, 52, Tab5Theme::PRIMARY);
UIIconSquare iconSq2(TAB5_PADDING + 66,  CY + 260, 52, Tab5Theme::SECONDARY);
UIIconSquare iconSq3(TAB5_PADDING + 132, CY + 260, 52, Tab5Theme::ACCENT);

// Circle icons
UIIconCircle iconCr1(TAB5_PADDING + 220,      CY + 260, 26, Tab5Theme::DANGER);
UIIconCircle iconCr2(TAB5_PADDING + 220 + 66, CY + 260, 26, Tab5Theme::PRIMARY);

// Right column
UIButton btnTabPos(700, CY + TAB5_PADDING, 540, TAB5_BTN_H,
                   "Move Tabs to Bottom", Tab5Theme::SURFACE);

UILabel  infoLabel(700, CY + 80, 540, TAB5_LABEL_H,
                   "This is the Controls tab", Tab5Theme::ACCENT, TAB5_FONT_SIZE_MD);

UITextRow rowDisplay(700, CY + 120, 540, "Display", "1280x720");
UITextRow rowTouch(700, CY + 160, 540, "Touch", "Capacitive");
UITextRow rowWidgets(700, CY + 200, 540, "Widgets", "14 types");

UIInfoPopup popup("Info", "Button pressed!");

// Counter for button presses
int pressCount = 0;

// ═════════════════════════════════════════════════════════════════════════════
//  Tab 2 — "Data List"
// ═════════════════════════════════════════════════════════════════════════════

// The list fills most of the content area
UIList dataList(TAB5_PADDING, CY + TAB5_PADDING,
                600, TAB_H - TAB5_TAB_BAR_H - TAB5_PADDING * 2);

UILabel listSelLabel(640, CY + TAB5_PADDING, 620, TAB5_LABEL_H,
                     "Selected: (none)");

UIButton btnShowSel(640, CY + TAB5_PADDING + 50, 280, TAB5_BTN_H,
                    "Show Selected", Tab5Theme::PRIMARY);

UIButton btnClearSel(940, CY + TAB5_PADDING + 50, 280, TAB5_BTN_H,
                     "Clear Selection", Tab5Theme::DANGER);

UIInfoPopup listPopup("Selection", "No item selected");

// ═════════════════════════════════════════════════════════════════════════════
//  Helper: reposition elements when tab bar moves top <-> bottom
// ═════════════════════════════════════════════════════════════════════════════
void repositionElements() {
    int16_t cy = tabs.contentY();

    // Tab 1 elements
    headerLabel.setPosition(TAB5_PADDING, cy + TAB5_PADDING);
    btnAction.setPosition(TAB5_PADDING, cy + 50);
    btnToggle.setPosition(220, cy + 50);
    btnAlert.setPosition(440, cy + 50);
    counterLabel.setPosition(TAB5_PADDING, cy + 120);
    row1.setPosition(TAB5_PADDING, cy + 165);
    row3.setPosition(TAB5_PADDING, cy + 205);
    iconSq1.setPosition(TAB5_PADDING, cy + 260);
    iconSq2.setPosition(TAB5_PADDING + 66, cy + 260);
    iconSq3.setPosition(TAB5_PADDING + 132, cy + 260);
    iconCr1.setPosition(TAB5_PADDING + 220, cy + 260);
    iconCr2.setPosition(TAB5_PADDING + 220 + 66, cy + 260);
    btnTabPos.setPosition(700, cy + TAB5_PADDING);
    infoLabel.setPosition(700, cy + 80);
    rowDisplay.setPosition(700, cy + 120);
    rowTouch.setPosition(700, cy + 160);
    rowWidgets.setPosition(700, cy + 200);

    // Tab 2 elements
    int16_t listH = tabs.contentH() - TAB5_PADDING * 2;
    dataList.setPosition(TAB5_PADDING, cy + TAB5_PADDING);
    dataList.setSize(600, listH);
    listSelLabel.setPosition(640, cy + TAB5_PADDING);
    btnShowSel.setPosition(640, cy + TAB5_PADDING + 50);
    btnClearSel.setPosition(940, cy + TAB5_PADDING + 50);
}

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    display.init();
    display.setRotation(1);           // Landscape
    display.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // ── Setup Tab View ──────────────────────────────────────────────────────
    int page0 = tabs.addPage("Controls");
    int page1 = tabs.addPage("Data List");

    // ── Page 0: Controls ────────────────────────────────────────────────────
    iconSq1.setIconChar("W");
    iconSq2.setIconChar("B");
    iconSq3.setIconChar("T");
    iconCr1.setIconChar("!");
    iconCr2.setIconChar("?");

    rowWidgets.setShowDivider(false);  // Last row, no divider needed

    // Button callbacks
    btnAction.setOnTouchRelease([](TouchEvent e) {
        pressCount++;
        char buf[48];
        snprintf(buf, sizeof(buf), "Button presses: %d", pressCount);
        counterLabel.setText(buf);
        statusBar.setText("Action pressed!");
    });

    btnToggle.setOnTouchRelease([](TouchEvent e) {
        pressCount++;
        char buf[48];
        snprintf(buf, sizeof(buf), "Button presses: %d", pressCount);
        counterLabel.setText(buf);

        // Toggle the status text
        static bool toggled = false;
        toggled = !toggled;
        row1.setValue(toggled ? "Toggled ON" : "Active");
        statusBar.setText(toggled ? "Toggled ON" : "Toggled OFF");
    });

    btnAlert.setOnTouchRelease([](TouchEvent e) {
        pressCount++;
        char buf[48];
        snprintf(buf, sizeof(buf), "Button presses: %d", pressCount);
        counterLabel.setText(buf);

        popup.setTitle("Alert");
        popup.setMessage("This is an alert\nfrom the Controls tab!");
        popup.show();
    });

    // Tab position toggle button
    btnTabPos.setBorderColor(Tab5Theme::PRIMARY);
    btnTabPos.setOnTouchRelease([](TouchEvent e) {
        if (tabs.getTabPosition() == TabPosition::TOP) {
            tabs.setTabPosition(TabPosition::BOTTOM);
            btnTabPos.setLabel("Move Tabs to Top");
            row3.setValue("Bottom");
            statusBar.setText("Tab bar moved to bottom");
        } else {
            tabs.setTabPosition(TabPosition::TOP);
            btnTabPos.setLabel("Move Tabs to Bottom");
            row3.setValue("Top");
            statusBar.setText("Tab bar moved to top");
        }
        repositionElements();
        tabs.setDirty(true);
    });

    // Add children to page 0
    tabs.addChild(page0, &headerLabel);
    tabs.addChild(page0, &btnAction);
    tabs.addChild(page0, &btnToggle);
    tabs.addChild(page0, &btnAlert);
    tabs.addChild(page0, &counterLabel);
    tabs.addChild(page0, &row1);
    tabs.addChild(page0, &row3);
    tabs.addChild(page0, &iconSq1);
    tabs.addChild(page0, &iconSq2);
    tabs.addChild(page0, &iconSq3);
    tabs.addChild(page0, &iconCr1);
    tabs.addChild(page0, &iconCr2);
    tabs.addChild(page0, &btnTabPos);
    tabs.addChild(page0, &infoLabel);
    tabs.addChild(page0, &rowDisplay);
    tabs.addChild(page0, &rowTouch);
    tabs.addChild(page0, &rowWidgets);

    // ── Page 1: Data List ───────────────────────────────────────────────────

    // Populate list with items (some with icons)
    dataList.addItem("Dashboard",    "D", Tab5Theme::PRIMARY);
    dataList.addItem("Messages",     "M", Tab5Theme::ACCENT);
    dataList.addItem("Contacts",     "C", Tab5Theme::SECONDARY);
    dataList.addItem("Calendar",     "C", Tab5Theme::DANGER);
    dataList.addItem("Tasks",        "T", Tab5Theme::PRIMARY,   true);
    dataList.addItem("Notes",        "N", Tab5Theme::SECONDARY, true);
    dataList.addItem("Files",        "F", Tab5Theme::ACCENT,    true);
    dataList.addItem("Photos");
    dataList.addItem("Music");
    dataList.addItem("Videos");
    dataList.addItem("Downloads");
    dataList.addItem("Bookmarks");
    dataList.addItem("History");
    dataList.addItem("Preferences");
    dataList.addItem("Network");
    dataList.addItem("Bluetooth");
    dataList.addItem("Display");
    dataList.addItem("Sound");
    dataList.addItem("Battery");
    dataList.addItem("Storage");

    dataList.setTextSize(TAB5_FONT_SIZE_MD);

    dataList.setOnSelect([](int index, const char* text) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Selected: %s (#%d)", text, index + 1);
        listSelLabel.setText(buf);
        statusBar.setText(buf);
    });

    btnShowSel.setOnTouchRelease([](TouchEvent e) {
        int idx = dataList.getSelectedIndex();
        if (idx >= 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Item #%d:\n%s", idx + 1, dataList.getSelectedText());
            listPopup.setTitle("Selected Item");
            listPopup.setMessage(buf);
        } else {
            listPopup.setTitle("No Selection");
            listPopup.setMessage("Tap an item in\nthe list first.");
        }
        listPopup.show();
    });

    btnClearSel.setOnTouchRelease([](TouchEvent e) {
        dataList.clearSelection();
        listSelLabel.setText("Selected: (none)");
        statusBar.setText("Selection cleared");
    });

    listSelLabel.setTextColor(Tab5Theme::ACCENT);
    listSelLabel.setTextSize(TAB5_FONT_SIZE_MD);

    // Add children to page 1
    tabs.addChild(page1, &dataList);
    tabs.addChild(page1, &listSelLabel);
    tabs.addChild(page1, &btnShowSel);
    tabs.addChild(page1, &btnClearSel);

    // ── Tab change callback ─────────────────────────────────────────────────
    tabs.setOnTabChange([](int pageIndex) {
        const char* name = tabs.getPageLabel(pageIndex);
        char buf[64];
        snprintf(buf, sizeof(buf), "Switched to: %s", name);
        statusBar.setText(buf);
    });

    // ── Style tweaks ────────────────────────────────────────────────────────
    titleBar.setLeftText("< Back");
    titleBar.setRightText("v1.0");
    statusBar.setLeftText("Tab5");
    statusBar.setRightText("Tab Demo");

    // ── Register all elements ───────────────────────────────────────────────
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&tabs);
    ui.addElement(&statusBar);

    // Modal popups added last for z-order
    ui.addElement(&popup);
    ui.addElement(&listPopup);

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
