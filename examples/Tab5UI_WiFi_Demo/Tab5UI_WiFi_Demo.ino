/*******************************************************************************
 * Tab5UI_WiFi_Demo.ino — Portrait WiFi Scanner demo for the Tab5UI library
 *
 * Demonstrates portrait orientation (720×1280) with a scrollable UIList and
 * a "Scan" button that discovers nearby WiFi networks and populates the list
 * with SSID names and signal‑strength icons.
 *
 * Board:   M5Stack Tab5
 * Library: M5GFX (install via Arduino Library Manager)
 ******************************************************************************/

#include <M5GFX.h>
#include <WiFi.h>
#include "Tab5UI.h"

// ── Display & UI Manager ────────────────────────────────────────────────────
M5GFX display;
UIManager ui(display);

// ── Portrait layout constants ───────────────────────────────────────────────
// These are evaluated at runtime after Tab5UI::init(), so we use helper
// variables set in setup().
static int16_t screenW;
static int16_t screenH;

// ── UI Elements ─────────────────────────────────────────────────────────────
UITitleBar  titleBar("WiFi Scanner");
UIStatusBar statusBar("Tap Scan to find networks");

// Placeholder geometry — repositioned in setup() after rotation is known
UIList   wifiList(0, 0, 100, 100);
UIButton btnScan(0, 0, 100, TAB5_BTN_H, "Scan", Tab5Theme::PRIMARY);

UIInfoPopup infoPopup("Network Info", "");

// ── Signal strength icon character ──────────────────────────────────────────
// Returns a single‑char label for the icon based on RSSI
static const char* rssiIcon(int rssi) {
    if (rssi >= -55) return "4";   // Excellent
    if (rssi >= -67) return "3";   // Good
    if (rssi >= -75) return "2";   // Fair
    return "1";                     // Weak
}

// Returns an icon colour based on RSSI
static uint32_t rssiColor(int rssi) {
    if (rssi >= -55) return Tab5Theme::SECONDARY;   // Green — excellent
    if (rssi >= -67) return Tab5Theme::PRIMARY;    // Blue  — good
    if (rssi >= -75) return Tab5Theme::ACCENT;     // Amber — fair
    return Tab5Theme::DANGER;                       // Red   — weak
}

// ── Scan WiFi networks ─────────────────────────────────────────────────────
static bool scanning = false;

void doWiFiScan() {
    if (scanning) return;
    scanning = true;

    statusBar.setText("Scanning...");
    btnScan.setEnabled(false);
    ui.drawAll();

    int n = WiFi.scanNetworks();

    wifiList.clearItems();

    if (n <= 0) {
        wifiList.addItem("(no networks found)");
        statusBar.setText("No networks found");
    } else {
        for (int i = 0; i < n && i < TAB5_LIST_MAX_ITEMS; i++) {
            char label[64];
            snprintf(label, sizeof(label), "%s  (%d dBm)",
                     WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            int rssi = WiFi.RSSI(i);
            wifiList.addItem(label,
                             rssiIcon(rssi),
                             rssiColor(rssi),
                             true);             // circle icon
        }

        char msg[48];
        snprintf(msg, sizeof(msg), "Found %d network%s", n, n == 1 ? "" : "s");
        statusBar.setText(msg);
    }

    WiFi.scanDelete();
    btnScan.setEnabled(true);
    scanning = false;
    ui.drawAll();
}

// ═════════════════════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    display.init();
    display.setRotation(0);                // ◀ Portrait (720 × 1280)
    Tab5UI::init(display);
    display.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // ── Resolve runtime screen size ─────────────────────────────────────
    screenW = Tab5UI::screenW();           // 720
    screenH = Tab5UI::screenH();           // 1280

    // ── WiFi in station mode (no connection, just scanning) ─────────────
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // ── Reposition widgets for portrait ─────────────────────────────────
    int16_t pad    = TAB5_PADDING;
    int16_t listX  = pad;
    int16_t listY  = TAB5_TITLE_H + pad;
    int16_t listW  = screenW - pad * 2;
    int16_t btnH   = TAB5_BTN_H;
    int16_t listH  = screenH - TAB5_TITLE_H - TAB5_STATUS_H
                     - btnH - pad * 4;

    wifiList.setPosition(listX, listY);
    wifiList.setSize(listW, listH);
    wifiList.setTextSize(TAB5_FONT_SIZE_MD);

    int16_t btnY = listY + listH + pad;
    btnScan.setPosition(listX, btnY);
    btnScan.setSize(listW, btnH);

    // ── List selection callback — show details popup ────────────────────
    wifiList.setOnSelect([](int index, const char* text) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Network #%d\n%s", index + 1, text);
        infoPopup.setTitle("Network Info");
        infoPopup.setMessage(buf);
        infoPopup.show();
    });

    // ── Scan button callback ────────────────────────────────────────────
    btnScan.setOnTouchRelease([](TouchEvent e) {
        doWiFiScan();
    });

    // ── Status bar extras ───────────────────────────────────────────────
    statusBar.setLeftText("Tab5");
    statusBar.setRightText("WiFi");

    // ── Register elements ───────────────────────────────────────────────
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&wifiList);
    ui.addElement(&btnScan);
    ui.addElement(&statusBar);
    ui.addElement(&infoPopup);

    ui.setContentArea(TAB5_TITLE_H, screenH - TAB5_STATUS_H);
    ui.drawAll();
}

// ═════════════════════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    ui.update();
    yield();
}
