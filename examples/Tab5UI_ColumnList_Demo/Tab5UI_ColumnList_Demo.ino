/*******************************************************************************
 * Tab5UI_ColumnList_Demo.ino — UIColumnList demo for the Tab5UI library
 *
 * Demonstrates the scrollable UIColumnList widget with multiple columns,
 * text cells, icon cells, per-cell coloring, header row, row selection,
 * and click-to-sort columns.
 *
 * Tap a column header to sort ascending; tap again for descending;
 * tap a third time to clear the sort.
 *
 * Board:   M5Stack Tab5
 * Library: M5GFX (install via Arduino Library Manager)
 ******************************************************************************/

#include <M5GFX.h>
#include "Tab5UI.h"

// ── Icons (PROGMEM PNGs) ────────────────────────────────────────────────────
#include "icons/icon_like.h"
#include "icons/icon_dislike.h"
#include "icons/icon_lightning.h"
#include "icons/icon_bookmark.h"
#include "icons/icon_power.h"
#include "icons/icon_setting.h"
#include "icons/icon_search.h"
#include "icons/icon_refresh.h"
#include "icons/icon_save.h"
#include "icons/icon_delete_two.h"

// ── Display & UI Manager ────────────────────────────────────────────────────
M5GFX display;
UIManager ui(display);

// ── UI Elements ─────────────────────────────────────────────────────────────
UITitleBar  titleBar("Column List Demo");
UIStatusBar statusBar("Tap a header to sort, tap a row to select");

// Main columned list — takes most of the screen
UIColumnList table(TAB5_PADDING, TAB5_TITLE_H + TAB5_PADDING,
                   920,
                   TAB5_SCREEN_H - TAB5_TITLE_H - TAB5_STATUS_H - TAB5_PADDING * 2);

// Selection info
UILabel     selLabel(950, TAB5_TITLE_H + TAB5_PADDING, 310, TAB5_LABEL_H,
                     "Selected: (none)");

UIButton    btnClear(950, TAB5_TITLE_H + TAB5_PADDING + 50, 310, TAB5_BTN_H,
                     "Clear Selection", Tab5Theme::DANGER);

UIButton    btnScroll(950, TAB5_TITLE_H + TAB5_PADDING + 120, 310, TAB5_BTN_H,
                      "Scroll to Bottom", Tab5Theme::PRIMARY);

UIInfoPopup popup("Row Details", "");

// ═════════════════════════════════════════════════════════════════════════════
//  Helper: color text based on status
// ═════════════════════════════════════════════════════════════════════════════
static uint32_t statusColor(const char* status) {
    if (strcmp(status, "Online") == 0)   return Tab5Theme::SECONDARY;  // Green
    if (strcmp(status, "Idle") == 0)     return Tab5Theme::ACCENT;     // Orange
    if (strcmp(status, "Offline") == 0)  return Tab5Theme::DANGER;     // Red
    if (strcmp(status, "Critical") == 0) return Tab5Theme::DANGER;
    if (strcmp(status, "Warning") == 0)  return Tab5Theme::ACCENT;
    if (strcmp(status, "OK") == 0)       return Tab5Theme::SECONDARY;
    return Tab5Theme::TEXT_PRIMARY;
}

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    display.init();
    display.setRotation(1);           // Landscape
    Tab5UI::init(display);
    ui.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // ── Define columns ──
    table.addColumn("Server",   280);
    table.addColumn("Status",   150, textdatum_t::middle_center);
    table.addColumn("Health",   140, textdatum_t::middle_center);
    table.addColumn("Load",     160, textdatum_t::middle_center);
    table.addColumn("Action",   160, textdatum_t::middle_center);

    // ── Row data ──
    // Each row: server name, status text (colored), health text, load %, action icon
    struct ServerInfo {
        const char* name;
        const char* status;
        const char* health;
        const char* load;
        const uint8_t* icon;
        uint32_t iconSize;
    };

    ServerInfo servers[] = {
        { "Alpha-01",    "Online",   "OK",       "23%",  icon_like,      icon_like_size },
        { "Alpha-02",    "Online",   "OK",       "45%",  icon_like,      icon_like_size },
        { "Bravo-01",    "Idle",     "Warning",  "78%",  icon_lightning,  icon_lightning_size },
        { "Bravo-02",    "Online",   "OK",       "12%",  icon_bookmark,  icon_bookmark_size },
        { "Charlie-01",  "Offline",  "Critical", "0%",   icon_power,     icon_power_size },
        { "Charlie-02",  "Online",   "OK",       "56%",  icon_like,      icon_like_size },
        { "Delta-01",    "Idle",     "Warning",  "89%",  icon_refresh,   icon_refresh_size },
        { "Delta-02",    "Online",   "OK",       "34%",  icon_save,      icon_save_size },
        { "Echo-01",     "Offline",  "Critical", "0%",   icon_delete_two, icon_delete_two_size },
        { "Echo-02",     "Online",   "OK",       "67%",  icon_setting,   icon_setting_size },
        { "Foxtrot-01",  "Online",   "OK",       "41%",  icon_search,    icon_search_size },
        { "Foxtrot-02",  "Idle",     "Warning",  "92%",  icon_lightning,  icon_lightning_size },
        { "Golf-01",     "Online",   "OK",       "15%",  icon_like,      icon_like_size },
        { "Golf-02",     "Offline",  "Critical", "0%",   icon_dislike,   icon_dislike_size },
        { "Hotel-01",    "Online",   "OK",       "29%",  icon_bookmark,  icon_bookmark_size },
        { "Hotel-02",    "Online",   "OK",       "53%",  icon_save,      icon_save_size },
        { "India-01",    "Idle",     "Warning",  "81%",  icon_refresh,   icon_refresh_size },
        { "India-02",    "Online",   "OK",       "38%",  icon_like,      icon_like_size },
        { "Juliet-01",   "Online",   "OK",       "22%",  icon_setting,   icon_setting_size },
        { "Juliet-02",   "Offline",  "Critical", "0%",   icon_power,     icon_power_size },
    };

    int serverCount = sizeof(servers) / sizeof(servers[0]);

    for (int i = 0; i < serverCount; i++) {
        int row = table.addRow();
        table.setCellText(row, 0, servers[i].name);
        table.setCellText(row, 1, servers[i].status, statusColor(servers[i].status));
        table.setCellText(row, 2, servers[i].health, statusColor(servers[i].health));
        table.setCellText(row, 3, servers[i].load);
        table.setCellIcon(row, 4, servers[i].icon, servers[i].iconSize);
    }

    // Disable an offline server row
    table.setRowEnabled(4, false);

    // ── Sorting ──
    // All text columns are sortable by default.
    // Disable sorting on the icon-only "Action" column.
    table.setColumnSortable(4, false);

    // ── Style tweaks ──
    table.setTextSize(TAB5_FONT_SIZE_MD);
    table.setHeaderBgColor(Tab5Theme::SURFACE);
    table.setHeaderTextColor(Tab5Theme::TEXT_PRIMARY);
    table.setShowColumnDividers(true);

    // ── Selection callback ──
    table.setOnSelect([](int index, const char* text) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Selected: %s (row %d)", text, index + 1);
        selLabel.setText(buf);
        statusBar.setText(buf);
    });

    // ── Clear Selection button ──
    btnClear.setOnTouchRelease([](TouchEvent e) {
        table.clearSelection();
        selLabel.setText("Selected: (none)");
        statusBar.setText("Selection cleared");
    });

    // ── Scroll to Bottom button ──
    btnScroll.setOnTouchRelease([](TouchEvent e) {
        table.scrollToRow(table.rowCount() - 1);
        statusBar.setText("Scrolled to bottom");
    });

    // ── Style label ──
    selLabel.setTextColor(Tab5Theme::ACCENT);
    selLabel.setTextSize(TAB5_FONT_SIZE_MD);

    statusBar.setLeftText("Tab5");
    statusBar.setRightText("ColumnList Demo");

    // ── Register all elements ──
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&table);
    ui.addElement(&selLabel);
    ui.addElement(&btnClear);
    ui.addElement(&btnScroll);
    ui.addElement(&statusBar);
    ui.addElement(&popup);

    ui.setContentArea(TAB5_TITLE_H, TAB5_SCREEN_H - TAB5_STATUS_H);
    ui.drawAll();
    ui.setSleepTimeout(5);
    ui.setLightSleep(true);
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    ui.update();
    yield();
}
