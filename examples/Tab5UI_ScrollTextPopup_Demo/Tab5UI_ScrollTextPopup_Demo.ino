/*******************************************************************************
 * Tab5UI_ScrollTextPopup_Demo.ino — UIScrollTextPopup demo
 *
 * Demonstrates the large scrollable-text popup with full Markdown support.
 * Three buttons trigger different content: Help, Release Notes, and License.
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
UITitleBar  titleBar("ScrollTextPopup Demo");
UIStatusBar statusBar("Ready");

UILabel     headerLabel(TAB5_PADDING, 70, 600, TAB5_LABEL_H,
                        "Tap a button to open a scrollable popup:");

// Buttons to trigger different popups
UIButton    btnHelp(TAB5_PADDING, 130, 380, TAB5_BTN_H,
                    "Help / Guide", Tab5Theme::PRIMARY);
UIButton    btnRelease(TAB5_PADDING, 200, 380, TAB5_BTN_H,
                       "Release Notes", Tab5Theme::SECONDARY);
UIButton    btnLicense(TAB5_PADDING, 270, 380, TAB5_BTN_H,
                       "License", Tab5Theme::ACCENT);

UILabel     statusLabel(TAB5_PADDING, 350, 600, TAB5_LABEL_H,
                        "Last action: (none)");

// Right-side info
UITextRow   row1(500, 130, 760, "Widget", "UIScrollTextPopup");
UITextRow   row2(500, 170, 760, "Markdown", "Headings, bold, italic, code");
UITextRow   row3(500, 210, 760, "Scrolling", "Touch-drag to scroll");
UITextRow   row4(500, 250, 760, "Dismiss", "Close button or tap outside");

// ── Scrollable Text Popups ──────────────────────────────────────────────────

// Help popup with rich Markdown content
static const char* helpText =
    "# Getting Started\n"
    "\n"
    "Welcome to the **Tab5UI** library! This guide covers the basics "
    "of building user interfaces for the *M5Stack Tab5*.\n"
    "\n"
    "---\n"
    "\n"
    "## Creating Elements\n"
    "\n"
    "All UI elements inherit from `UIElement`. Common widgets include:\n"
    "\n"
    "- **UIButton** — Tappable buttons with color themes\n"
    "- **UILabel** — Static or dynamic text labels\n"
    "- **UITextRow** — Key-value rows for displaying info\n"
    "- **UIList** — Scrollable lists with selection\n"
    "- **UIScrollText** — Scrollable Markdown text areas\n"
    "- **UIColumnList** — Multi-column sortable lists\n"
    "\n"
    "## Adding to UIManager\n"
    "\n"
    "Register elements with `ui.add(&element)` in `setup()`. "
    "The UIManager handles drawing and touch routing automatically.\n"
    "\n"
    "### Touch Events\n"
    "\n"
    "Use `setOnTouch()` and `setOnRelease()` callbacks:\n"
    "\n"
    "- `TOUCH` fires on finger down\n"
    "- `TOUCH_RELEASE` fires on finger up\n"
    "\n"
    "### Layout Tips\n"
    "\n"
    "The Tab5 display is **1280x720** pixels. The title bar uses "
    "the top `48px` and the status bar uses the bottom `36px`, "
    "leaving **636px** of vertical space for your content.\n"
    "\n"
    "---\n"
    "\n"
    "## Popups\n"
    "\n"
    "Three popup types are available:\n"
    "\n"
    "- `UIInfoPopup` — Simple message with an *OK* button\n"
    "- `UIConfirmPopup` — Yes/No confirmation dialog\n"
    "- `UIScrollTextPopup` — Large scrollable text with **Markdown**\n"
    "\n"
    "All popups are **modal** and can be dismissed by tapping the "
    "close button or tapping outside the popup area.\n"
    "\n"
    "---\n"
    "\n"
    "### Markdown Support\n"
    "\n"
    "The following Markdown is rendered:\n"
    "\n"
    "- `# Heading 1` — Large heading with underline\n"
    "- `## Heading 2` — Medium heading\n"
    "- `### Heading 3` — Small heading\n"
    "- `**bold**` — Bold text in accent color\n"
    "- `*italic*` — Italic text in secondary color\n"
    "- Backtick for `inline code` with background\n"
    "- `- item` — Bullet lists\n"
    "- `---` — Horizontal rules\n";

UIScrollTextPopup helpPopup("Help & Guide", helpText);

// Release notes popup
static const char* releaseText =
    "# Release Notes\n"
    "\n"
    "## Version 1.2.0\n"
    "*February 13, 2026*\n"
    "\n"
    "### New Features\n"
    "\n"
    "- **UIColumnList** — Multi-column list widget with sortable headers\n"
    "- **UIScrollTextPopup** — Large scrollable popup with Markdown\n"
    "- Click-to-sort on column headers with sort indicators\n"
    "\n"
    "### Improvements\n"
    "\n"
    "- Added `ScrollTextLine` struct for efficient text reflow\n"
    "- Sprite-buffered rendering for all new widgets\n"
    "- Comprehensive wiki documentation\n"
    "\n"
    "---\n"
    "\n"
    "## Version 1.1.0\n"
    "*January 2026*\n"
    "\n"
    "### New Features\n"
    "\n"
    "- **UIScrollText** — Scrollable text with Markdown rendering\n"
    "- **UICheckbox** — Toggle checkboxes\n"
    "- **UIRadioButton** and **UIRadioGroup** — Radio selection\n"
    "- **UITextInput** and **UIKeyboard** — On-screen text input\n"
    "- **UITabBar** — Tab navigation between screens\n"
    "\n"
    "### Improvements\n"
    "\n"
    "- Touch-drag scrolling with momentum\n"
    "- Flicker-free sprite-based rendering\n"
    "- Dark theme with `Tab5Theme` color constants\n"
    "\n"
    "---\n"
    "\n"
    "## Version 1.0.0\n"
    "*December 2025*\n"
    "\n"
    "- Initial release\n"
    "- Core widgets: `UIButton`, `UILabel`, `UITextRow`\n"
    "- `UITitleBar` and `UIStatusBar`\n"
    "- `UIMenu` overlay popup\n"
    "- `UIInfoPopup` and `UIConfirmPopup`\n"
    "- `UIList` with scrollable selection\n"
    "- Icon widgets: `UIIconSquare`, `UIIconCircle`\n";

UIScrollTextPopup releasePopup("Release Notes", releaseText);

// License popup
static const char* licenseText =
    "# MIT License\n"
    "\n"
    "Copyright (c) 2025-2026 Tab5UI Contributors\n"
    "\n"
    "---\n"
    "\n"
    "Permission is hereby granted, free of charge, to any person "
    "obtaining a copy of this software and associated documentation "
    "files (the **Software**), to deal in the Software without "
    "restriction, including without limitation the rights to *use*, "
    "*copy*, *modify*, *merge*, *publish*, *distribute*, *sublicense*, "
    "and/or *sell* copies of the Software, and to permit persons to "
    "whom the Software is furnished to do so, subject to the "
    "following conditions:\n"
    "\n"
    "---\n"
    "\n"
    "## Conditions\n"
    "\n"
    "The above copyright notice and this permission notice shall be "
    "included in **all copies** or substantial portions of the Software.\n"
    "\n"
    "---\n"
    "\n"
    "## Disclaimer\n"
    "\n"
    "THE SOFTWARE IS PROVIDED `AS IS`, WITHOUT WARRANTY OF ANY KIND, "
    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
    "OF **MERCHANTABILITY**, **FITNESS FOR A PARTICULAR PURPOSE** AND "
    "**NONINFRINGEMENT**.\n"
    "\n"
    "In no event shall the authors or copyright holders be liable for "
    "any *claim*, *damages* or other liability, whether in an action "
    "of contract, tort or otherwise, arising from, out of or in "
    "connection with the software or the use or other dealings in "
    "the Software.\n";

UIScrollTextPopup licensePopup("License", licenseText);

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    display.init();
    display.setRotation(1);           // Landscape 1280×720
    Tab5UI::init(display);
    ui.setBrightness(128);
    display.setFont(&fonts::DejaVu18);

    // Button callbacks
    btnHelp.setOnTouchRelease([](TouchEvent) {
        helpPopup.show();
        statusBar.setText("Showing help");
    });

    btnRelease.setOnTouchRelease([](TouchEvent) {
        releasePopup.show();
        statusBar.setText("Showing release notes");
    });

    btnLicense.setOnTouchRelease([](TouchEvent) {
        licensePopup.show();
        statusBar.setText("Showing license");
    });

    // Dismiss callbacks
    helpPopup.setOnDismiss([](TouchEvent) {
        statusLabel.setText("Last action: Closed Help popup");
        statusBar.setText("Ready");
    });

    releasePopup.setOnDismiss([](TouchEvent) {
        statusLabel.setText("Last action: Closed Release Notes popup");
        statusBar.setText("Ready");
    });

    licensePopup.setOnDismiss([](TouchEvent) {
        statusLabel.setText("Last action: Closed License popup");
        statusBar.setText("Ready");
    });

    // Status bar config
    statusBar.setLeftText("Tab5");
    statusBar.setRightText("v1.2");

    // Register all elements with the UI manager
    ui.setBackground(Tab5Theme::BG_DARK);
    ui.clearScreen();

    ui.addElement(&titleBar);
    ui.addElement(&headerLabel);
    ui.addElement(&btnHelp);
    ui.addElement(&btnRelease);
    ui.addElement(&btnLicense);
    ui.addElement(&statusLabel);
    ui.addElement(&row1);
    ui.addElement(&row2);
    ui.addElement(&row3);
    ui.addElement(&row4);
    ui.addElement(&statusBar);

    // Add popups last (they render on top when visible)
    ui.addElement(&helpPopup);
    ui.addElement(&releasePopup);
    ui.addElement(&licensePopup);

    ui.setContentArea(TAB5_TITLE_H, Tab5UI::screenH() - TAB5_STATUS_H);
    ui.drawAll();
    ui.setSleepTimeout(5);
    ui.setLightSleep(true);
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    ui.update();
}
