#!/usr/bin/env python3
"""
Generate 7 accurate screenshot images of the Tab5UI Demo application.

Screenshot 1: Initial state after first run
Screenshot 2: Menu open
Screenshot 3: On-screen keyboard visible
Screenshot 4: Info popup ("About") shown
Screenshot 5: List demo with icons
Screenshot 6: Tab demo — Controls page
Screenshot 7: Tab demo — Data List page

Target display: M5Stack Tab5 — 1280×720 IPS
"""

from PIL import Image, ImageDraw, ImageFont
import os

# ─── Screen constants ────────────────────────────────────────────────────────
W, H = 1280, 720
PADDING = 12
TITLE_H = 48
STATUS_H = 36
BTN_H = 52
BTN_R = 8
TEXTROW_H = 40
ICON_SIZE = 44
MENU_ITEM_H = 48
INPUT_H = 44

# ─── Theme colors (RGB tuples) ───────────────────────────────────────────────
PRIMARY       = (0x21, 0x96, 0xF3)
PRIMARY_DARK  = (0x15, 0x65, 0xC0)
SECONDARY     = (0x4C, 0xAF, 0x50)
ACCENT        = (0xFF, 0x98, 0x00)
DANGER        = (0xF4, 0x43, 0x36)
BG_DARK       = (0x1A, 0x1A, 0x2E)
BG_MEDIUM     = (0x16, 0x21, 0x3E)
SURFACE       = (0x0F, 0x34, 0x60)
TITLE_BG      = (0x0F, 0x34, 0x60)
STATUS_BG     = (0x1A, 0x1A, 0x2E)
TEXT_PRIMARY   = (0xFF, 0xFF, 0xFF)
TEXT_SECONDARY = (0xB0, 0xBE, 0xC5)
TEXT_DISABLED  = (0x54, 0x6E, 0x7A)
BORDER        = (0x37, 0x47, 0x4F)
DIVIDER       = (0x26, 0x32, 0x38)
SHADOW        = (0x0A, 0x0A, 0x14)

# ─── Font sizes ──────────────────────────────────────────────────────────────
# DejaVu18 base = 18px.  Multipliers: SM=1.4, MD=1.8, LG=2.4
FONT_SM = 25   # 18 * 1.4
FONT_MD = 32   # 18 * 1.8
FONT_LG = 43   # 18 * 2.4

# ─── Load fonts ──────────────────────────────────────────────────────────────
FONT_PATH = "/System/Library/Fonts/Helvetica.ttc"

font_sm = ImageFont.truetype(FONT_PATH, FONT_SM)
font_md = ImageFont.truetype(FONT_PATH, FONT_MD)
font_lg = ImageFont.truetype(FONT_PATH, FONT_LG)

# ─── Helper functions ────────────────────────────────────────────────────────

def darken(color, factor=0.7):
    return tuple(int(c * factor) for c in color)

def rounded_rect(draw, xy, wh, r, fill=None, outline=None, width=1):
    """Draw a rounded rectangle."""
    x, y = xy
    w, h = wh
    draw.rounded_rectangle([x, y, x + w, y + h], radius=r, fill=fill, outline=outline, width=width)

def draw_title_bar(draw, title="Tab5UI Demo", left="< Menu", right="Settings"):
    """Draw the title bar."""
    draw.rectangle([0, 0, W, TITLE_H], fill=TITLE_BG)
    # Title centered
    bbox = font_lg.getbbox(title)
    tw = bbox[2] - bbox[0]
    draw.text(((W - tw) // 2, 6), title, fill=TEXT_PRIMARY, font=font_lg)
    # Left text
    draw.text((PADDING + 4, 12), left, fill=TEXT_SECONDARY, font=font_md)
    # Right text
    bbox = font_md.getbbox(right)
    rw = bbox[2] - bbox[0]
    draw.text((W - rw - PADDING - 4, 12), right, fill=TEXT_SECONDARY, font=font_md)

def draw_status_bar(draw, text="Ready", left="Tab5", right="v1.0"):
    """Draw the status bar."""
    y = H - STATUS_H
    draw.rectangle([0, y, W, H], fill=STATUS_BG)
    # Divider at top
    draw.line([(0, y), (W, y)], fill=DIVIDER, width=1)
    # Center text
    bbox = font_sm.getbbox(text)
    tw = bbox[2] - bbox[0]
    draw.text(((W - tw) // 2, y + 6), text, fill=TEXT_SECONDARY, font=font_sm)
    # Left
    draw.text((PADDING, y + 6), left, fill=TEXT_SECONDARY, font=font_sm)
    # Right
    bbox = font_sm.getbbox(right)
    rw = bbox[2] - bbox[0]
    draw.text((W - rw - PADDING, y + 6), right, fill=TEXT_SECONDARY, font=font_sm)

def draw_button(draw, x, y, w, h, label, bg=PRIMARY, text_color=TEXT_PRIMARY,
                border_color=None, pressed=False):
    """Draw a rounded button."""
    fill = darken(bg) if pressed else bg
    rounded_rect(draw, (x, y), (w, h), BTN_R, fill=fill)
    if border_color:
        rounded_rect(draw, (x, y), (w, h), BTN_R, outline=border_color, width=2)
    bbox = font_md.getbbox(label)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    draw.text((x + (w - tw) // 2, y + (h - th) // 2 - 2), label,
              fill=text_color, font=font_md)

def draw_label(draw, x, y, w, h, text, color=TEXT_PRIMARY, font=None, align="left"):
    """Draw a text label."""
    if font is None:
        font = font_md
    if align == "left":
        draw.text((x, y + (h - font.size) // 2), text, fill=color, font=font)
    elif align == "center":
        bbox = font.getbbox(text)
        tw = bbox[2] - bbox[0]
        draw.text((x + (w - tw) // 2, y + (h - font.size) // 2), text, fill=color, font=font)

def draw_text_row(draw, x, y, w, label, value, show_divider=True):
    """Draw a text row with label on left, value on right."""
    draw.rectangle([x, y, x + w, y + TEXTROW_H], fill=BG_MEDIUM)
    draw.text((x + PADDING, y + 6), label, fill=TEXT_PRIMARY, font=font_md)
    bbox = font_md.getbbox(value)
    vw = bbox[2] - bbox[0]
    draw.text((x + w - vw - PADDING, y + 6), value, fill=TEXT_SECONDARY, font=font_md)
    if show_divider:
        draw.line([(x, y + TEXTROW_H - 1), (x + w, y + TEXTROW_H - 1)], fill=DIVIDER)

def draw_icon_square(draw, x, y, size, fill_color, char="", border_color=BORDER):
    """Draw a square icon."""
    rounded_rect(draw, (x, y), (size, size), 4, fill=fill_color, outline=border_color)
    if char:
        bbox = font_md.getbbox(char)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        draw.text((x + (size - tw) // 2, y + (size - th) // 2 - 2), char,
                  fill=TEXT_PRIMARY, font=font_md)

def draw_icon_circle(draw, cx, cy, r, fill_color, char="", border_color=BORDER):
    """Draw a circle icon."""
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill_color, outline=border_color)
    if char:
        bbox = font_md.getbbox(char)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        draw.text((cx - tw // 2, cy - th // 2 - 2), char, fill=TEXT_PRIMARY, font=font_md)

def draw_text_input(draw, x, y, w, placeholder="", text="", focused=False):
    """Draw a text input field."""
    draw.rectangle([x, y, x + w, y + INPUT_H], fill=BG_MEDIUM)
    bc = PRIMARY if focused else BORDER
    lw = 2 if focused else 1
    draw.rectangle([x, y, x + w, y + INPUT_H], outline=bc, width=lw)
    if text:
        draw.text((x + PADDING, y + 8), text, fill=TEXT_PRIMARY, font=font_md)
    else:
        draw.text((x + PADDING, y + 8), placeholder, fill=TEXT_DISABLED, font=font_md)

def draw_all_base_elements(draw, status_text="Ready"):
    """Draw all base UI elements common to all screenshots."""
    # Background
    # (already filled with BG_DARK)

    # Title bar
    draw_title_bar(draw)

    # Header label
    draw_label(draw, PADDING, 60, 400, 32, "Touch any element below:",
               color=TEXT_SECONDARY, font=font_sm)

    # Buttons row
    draw_button(draw, PADDING, 110, 180, BTN_H, "Action", PRIMARY)
    draw_button(draw, 200, 110, 180, BTN_H, "Toggle", SECONDARY)
    draw_button(draw, 390, 110, 180, BTN_H, "Reset", DANGER)

    # Text rows (left column, width 670)
    draw_text_row(draw, PADDING, 180, 670, "WiFi Status", "Connected")
    draw_text_row(draw, PADDING, 220, 670, "Battery", "87%")
    draw_text_row(draw, PADDING, 260, 670, "Temperature", "24.5 C")

    # Counter label
    draw_label(draw, PADDING, 310, 500, 32, "Button presses: 0", color=TEXT_PRIMARY, font=font_md)

    # Square icons
    draw_icon_square(draw, PADDING,       370, 56, PRIMARY,   "W")
    draw_icon_square(draw, PADDING + 70,  370, 56, SECONDARY, "B")
    draw_icon_square(draw, PADDING + 140, 370, 56, ACCENT,    "T")
    draw_icon_square(draw, PADDING + 210, 370, 56, DANGER,    "!")

    # Circle icons
    draw_icon_circle(draw, PADDING + 28,       450 + 28, 28, PRIMARY,   "1")
    draw_icon_circle(draw, PADDING + 70 + 28,  450 + 28, 28, SECONDARY, "2")
    draw_icon_circle(draw, PADDING + 140 + 28, 450 + 28, 28, ACCENT,    "3")
    draw_icon_circle(draw, PADDING + 210 + 28, 450 + 28, 28, DANGER,    "4")

    # Icon label
    draw_label(draw, PADDING, 520, 400, 32, "Tap icons above to change color",
               color=TEXT_SECONDARY, font=font_sm)

    # Confirm button and result label
    draw_button(draw, PADDING, 560, 280, BTN_H, "Confirm Action", ACCENT)
    draw_label(draw, 300, 560, 370, 32, "Result: (waiting)",
               color=ACCENT, font=font_sm)

    # Right column
    draw_button(draw, 700, 60, 560, 56, "Large Button", SURFACE,
                border_color=PRIMARY)

    draw_label(draw, 700, 130, 560, 32, "Tab5UI Library v1.0",
               color=TEXT_SECONDARY, font=font_sm)

    draw_text_row(draw, 700, 170, 560, "Display", "1280x720")
    draw_text_row(draw, 700, 210, 560, "Touch", "Capacitive")
    draw_text_row(draw, 700, 250, 560, "UI Elements", "11 types", show_divider=False)

    # Text inputs
    draw_text_input(draw, 700, 310, 560, placeholder="Enter your name...")
    draw_text_input(draw, 700, 370, 560, placeholder="Type a message...")

    # Input result label
    draw_label(draw, 700, 430, 560, 32, "Submitted: (none)",
               color=ACCENT, font=font_sm)

    # Status bar
    draw_status_bar(draw, text=status_text)


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 1: Initial state
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot1():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)
    draw_all_base_elements(draw, "Ready")
    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 2: Menu open
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot2():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)
    draw_all_base_elements(draw, "Ready")

    # Draw popup menu
    menu_x = PADDING
    menu_y = TITLE_H + 4
    menu_w = 280
    items = [
        ("New Project", True),
        ("Open File", True),
        ("Save", False),        # disabled
        None,                   # separator
        ("About", True),
    ]

    # Calculate menu height
    menu_h = 0
    for item in items:
        if item is None:
            menu_h += 2  # separator
        else:
            menu_h += MENU_ITEM_H

    # Shadow
    draw.rectangle([menu_x + 4, menu_y + 4, menu_x + menu_w + 4, menu_y + menu_h + 4],
                   fill=SHADOW)

    # Background
    rounded_rect(draw, (menu_x, menu_y), (menu_w, menu_h), 6, fill=SURFACE, outline=BORDER)

    # Items
    cy = menu_y
    for item in items:
        if item is None:
            # Separator
            draw.line([(menu_x + PADDING, cy + 1), (menu_x + menu_w - PADDING, cy + 1)],
                      fill=DIVIDER, width=1)
            cy += 2
        else:
            label, enabled = item
            color = TEXT_PRIMARY if enabled else TEXT_DISABLED
            draw.text((menu_x + PADDING + 4, cy + 10), label, fill=color, font=font_md)
            cy += MENU_ITEM_H

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 3: Keyboard open
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot3():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Draw base elements first  
    draw_all_base_elements(draw, "Ready")

    # Show the first input as focused with some text typed
    # Redraw the input focused with cursor
    draw.rectangle([700, 310, 700 + 560, 310 + INPUT_H], fill=BG_MEDIUM)
    draw.rectangle([700, 310, 700 + 560, 310 + INPUT_H], outline=PRIMARY, width=2)
    typed_text = "Hello"
    draw.text((700 + PADDING, 310 + 8), typed_text, fill=TEXT_PRIMARY, font=font_md)
    # Cursor
    bbox = font_md.getbbox(typed_text)
    tw = bbox[2] - bbox[0]
    cx = 700 + PADDING + tw + 2
    draw.line([(cx, 316), (cx, 348)], fill=TEXT_PRIMARY, width=2)

    # Keyboard panel at bottom of screen
    KB_H = 290
    kb_y = H - KB_H
    draw.rectangle([0, kb_y, W, H], fill=BG_DARK)

    # Key layout
    KB_KEY_W = 88
    KB_KEY_H = 56
    KB_KEY_GAP = 6

    rows = [
        list("qwertyuiop"),
        list("asdfghjkl"),
        ["Shft"] + list("zxcvbnm") + ["Bksp"],
        ["123", ",", "Space", ".", "Done", "Hide"],
    ]

    # Special key widths
    special_widths = {
        "Shft": 1.4,
        "Bksp": 1.4,
        "123": 1.3,
        "Space": 3.5,
        "Done": 1.4,
        "Hide": 1.1,
    }

    # Special key colors
    special_colors = {
        "Shft": BORDER,
        "Bksp": DANGER,
        "123": BORDER,
        "Done": PRIMARY,
        "Hide": BORDER,
    }

    for row_idx, row in enumerate(rows):
        # Calculate total row width
        total_w = 0
        for key in row:
            mult = special_widths.get(key, 1.0)
            total_w += KB_KEY_W * mult + KB_KEY_GAP
        total_w -= KB_KEY_GAP  # remove last gap

        start_x = (W - total_w) / 2
        ky = kb_y + PADDING + row_idx * (KB_KEY_H + KB_KEY_GAP)
        kx = start_x

        for key in row:
            mult = special_widths.get(key, 1.0)
            kw = int(KB_KEY_W * mult)
            bg = special_colors.get(key, SURFACE)

            rounded_rect(draw, (int(kx), int(ky)), (kw, KB_KEY_H), 6, fill=bg)

            # Key label
            f = font_sm if len(key) > 1 else font_md
            bbox = f.getbbox(key)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            tx = int(kx) + (kw - tw) // 2
            ty = int(ky) + (KB_KEY_H - th) // 2 - 2
            draw.text((tx, ty), key, fill=TEXT_PRIMARY, font=f)

            kx += kw + KB_KEY_GAP

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 4: Info popup
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot4():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)
    draw_all_base_elements(draw, "Ready")

    # Info popup — auto-sized to fit "About" title and "Tab5 UI Demo" message
    title = "About"
    message = "Tab5 UI Demo"
    btn_label = "OK"

    # Measure text to auto-size
    title_bbox = font_lg.getbbox(title)
    title_w = title_bbox[2] - title_bbox[0]

    msg_bbox = font_md.getbbox(message)
    msg_w = msg_bbox[2] - msg_bbox[0]

    btn_bbox = font_md.getbbox(btn_label)
    btn_label_w = btn_bbox[2] - btn_bbox[0]
    btn_w = btn_label_w + 60
    if btn_w < 100:
        btn_w = 100

    hpad = PADDING * 2
    popup_w = max(title_w + hpad + 40, msg_w + hpad + 20, btn_w + hpad, 200)
    popup_w = min(popup_w, W - 80)

    # Height
    title_gap = 42
    btn_area_h = 56
    line_h = FONT_MD + 4
    popup_h = PADDING + title_gap + 10 + line_h + 10 + btn_area_h + PADDING
    popup_h = max(popup_h, 140)
    popup_h = min(popup_h, H - 80)

    px = (W - popup_w) // 2
    py = (H - popup_h) // 2

    # Shadow
    draw.rectangle([px + 4, py + 4, px + popup_w + 4, py + popup_h + 4], fill=SHADOW)

    # Background
    rounded_rect(draw, (px, py), (popup_w, popup_h), 8, fill=SURFACE, outline=BORDER)

    # Title
    bbox = font_lg.getbbox(title)
    tw = bbox[2] - bbox[0]
    draw.text((px + (popup_w - tw) // 2, py + PADDING + 4), title,
              fill=TEXT_PRIMARY, font=font_lg)

    # Divider
    div_y = py + PADDING + 38
    draw.line([(px + PADDING, div_y), (px + popup_w - PADDING, div_y)], fill=DIVIDER)

    # Message
    bbox = font_md.getbbox(message)
    mw = bbox[2] - bbox[0]
    msg_y = div_y + 14
    draw.text((px + (popup_w - mw) // 2, msg_y), message,
              fill=TEXT_SECONDARY, font=font_md)

    # OK button
    btn_h = 40
    btn_x = px + (popup_w - btn_w) // 2
    btn_y = py + popup_h - btn_h - PADDING

    rounded_rect(draw, (btn_x, btn_y), (btn_w, btn_h), 6, fill=PRIMARY)
    bbox = font_md.getbbox(btn_label)
    bw = bbox[2] - bbox[0]
    bh = bbox[3] - bbox[1]
    draw.text((btn_x + (btn_w - bw) // 2, btn_y + (btn_h - bh) // 2 - 2), btn_label,
              fill=TEXT_PRIMARY, font=font_md)

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 5: List Demo
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot5():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Title bar
    draw_title_bar(draw, title="List Demo", left="", right="")

    # Status bar
    draw_status_bar(draw, text="Selected: Mango (#13)", left="Tab5", right="List Demo")

    # ── List widget ──
    list_x = PADDING
    list_y = TITLE_H + PADDING
    list_w = 600
    list_h = H - TITLE_H - STATUS_H - PADDING * 2  # 612
    ITEM_H = 48
    SCROLLBAR_W = 6

    # Items with optional icons: (text, iconChar, iconColor, isCircle) or just (text,)
    items = [
        ("Apple",       "A",  DANGER,     False),
        ("Banana",      "B",  ACCENT,     False),
        ("Cherry",      "C",  DANGER,     False),
        ("Date",        "D",  ACCENT,     False),
        ("Elderberry",  "E",  SECONDARY,  False),
        ("Fig",         "F",  SECONDARY,  True),
        ("Grape",       "G",  PRIMARY,    True),
        ("Honeydew",    "H",  SECONDARY,  True),
        ("Indian Fig",  None, None,       False),
        ("Jackfruit",   None, None,       False),
        ("Kiwi",        None, None,       False),
        ("Lemon",       None, None,       False),
        ("Mango",       "M",  ACCENT,     False),   # icon set after creation
        ("Nectarine",   None, None,       False),
        ("Orange",      "O",  PRIMARY,    True),     # icon set after creation
        ("Papaya",      None, None,       False),
        ("Quince",      None, None,       False),
        ("Raspberry",   None, None,       False),
        ("Strawberry",  None, None,       False),
        ("Tangerine",   None, None,       False),
        ("Ugli Fruit",  None, None,       False),
        ("Vanilla Bean",None, None,       False),
        ("Watermelon",  None, None,       False),
        ("Ximenia",     None, None,       False),
        ("Yuzu",        None, None,       False),
        ("Zucchini",    None, None,       False),
    ]

    selected_idx = 12  # "Mango"
    num_items = len(items)
    total_content_h = num_items * ITEM_H
    scroll_offset = 0  # at top

    ICON_PAD = 6
    icon_size = ITEM_H - ICON_PAD * 2   # 36

    # List background
    draw.rectangle([list_x, list_y, list_x + list_w, list_y + list_h], fill=BG_MEDIUM)
    draw.rectangle([list_x, list_y, list_x + list_w, list_y + list_h], outline=BORDER)

    # Draw visible items (clipped to list bounds)
    for i in range(num_items):
        item_y = list_y + (i * ITEM_H) - scroll_offset

        # Skip items outside visible area
        if item_y + ITEM_H <= list_y or item_y >= list_y + list_h:
            continue
        if item_y < list_y:
            continue

        text, icon_char, icon_color, is_circle = items[i]

        # Selected highlight
        if i == selected_idx:
            sel_h = min(ITEM_H, list_y + list_h - item_y)
            draw.rectangle([list_x + 1, item_y, list_x + list_w - SCROLLBAR_W - 2,
                           item_y + sel_h], fill=PRIMARY)

        # Item text
        text_y = item_y + (ITEM_H - FONT_MD) // 2
        if text_y + FONT_MD <= list_y + list_h:
            draw.text((list_x + PADDING, text_y), text,
                      fill=TEXT_PRIMARY, font=font_md)

        # Right-aligned icon
        if icon_char is not None:
            ix = list_x + list_w - SCROLLBAR_W - PADDING - icon_size - 4
            iy = item_y + ICON_PAD

            if is_circle:
                cx = ix + icon_size // 2
                cy = iy + icon_size // 2
                r = icon_size // 2
                draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                             fill=icon_color, outline=BORDER)
                # Character centered
                bbox = font_sm.getbbox(icon_char)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((cx - tw // 2, cy - th // 2 - 1), icon_char,
                          fill=TEXT_PRIMARY, font=font_sm)
            else:
                rounded_rect(draw, (ix, iy), (icon_size, icon_size), 4,
                             fill=icon_color, outline=BORDER)
                # Character centered
                bbox = font_sm.getbbox(icon_char)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((ix + (icon_size - tw) // 2, iy + (icon_size - th) // 2 - 1),
                          icon_char, fill=TEXT_PRIMARY, font=font_sm)

        # Divider
        if i < num_items - 1:
            div_y = item_y + ITEM_H - 1
            if div_y < list_y + list_h:
                draw.line([(list_x + PADDING, div_y),
                           (list_x + list_w - SCROLLBAR_W - PADDING, div_y)],
                          fill=DIVIDER)

    # Scrollbar
    sb_x = list_x + list_w - SCROLLBAR_W - 1
    sb_area_h = list_h - 2

    # Scrollbar track
    draw.rectangle([sb_x, list_y + 1, sb_x + SCROLLBAR_W, list_y + 1 + sb_area_h],
                   fill=darken(BG_MEDIUM, 0.5))

    # Scrollbar thumb
    visible_ratio = list_h / total_content_h
    thumb_h = max(20, int(sb_area_h * visible_ratio))
    max_scroll = total_content_h - list_h
    scroll_ratio = scroll_offset / max_scroll if max_scroll > 0 else 0
    thumb_y = list_y + 1 + int((sb_area_h - thumb_h) * scroll_ratio)

    rounded_rect(draw, (sb_x, thumb_y), (SCROLLBAR_W, thumb_h), 3,
                 fill=TEXT_DISABLED)

    # ── Right-side controls ──

    # Selected label
    draw_label(draw, 640, TITLE_H + PADDING, 620, 32,
               "Selected: Mango (#13)", color=ACCENT, font=font_md)

    # Buttons
    draw_button(draw, 640, TITLE_H + PADDING + 50, 280, BTN_H,
                "Show Selected", PRIMARY)
    draw_button(draw, 940, TITLE_H + PADDING + 50, 280, BTN_H,
                "Clear Selection", DANGER)

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Tab bar helper
# ═════════════════════════════════════════════════════════════════════════════
TAB_BAR_H = 48

def draw_tab_bar(draw, tab_labels, active_index, y,
                 bar_color=SURFACE, active_color=PRIMARY,
                 inactive_color=BG_MEDIUM, border_color=BORDER):
    """Draw a horizontal tab bar at the given y position."""
    num_tabs = len(tab_labels)
    tab_w = W // num_tabs

    # Bar background
    draw.rectangle([0, y, W, y + TAB_BAR_H], fill=bar_color)

    for i, label in enumerate(tab_labels):
        tx = i * tab_w
        tw = tab_w if i < num_tabs - 1 else (W - tx)  # last tab fills remaining

        if i == active_index:
            # Active tab background
            draw.rectangle([tx, y, tx + tw, y + TAB_BAR_H], fill=active_color)
            text_color = TEXT_PRIMARY
        else:
            draw.rectangle([tx, y, tx + tw, y + TAB_BAR_H], fill=inactive_color)
            text_color = TEXT_SECONDARY

        # Tab label centered
        bbox = font_md.getbbox(label)
        lw = bbox[2] - bbox[0]
        lh = bbox[3] - bbox[1]
        draw.text((tx + (tw - lw) // 2, y + (TAB_BAR_H - lh) // 2 - 2),
                  label, fill=text_color, font=font_md)

        # Right border between tabs
        if i < num_tabs - 1:
            draw.line([(tx + tw, y), (tx + tw, y + TAB_BAR_H)], fill=border_color)

    # Bottom border
    draw.line([(0, y + TAB_BAR_H - 1), (W, y + TAB_BAR_H - 1)], fill=border_color)


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 6: Tab Demo — Controls page
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot6():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Title bar
    draw_title_bar(draw, title="Tab Demo", left="< Back", right="v1.0")

    # Tab bar (tabs at top, "Controls" active)
    tab_y = TITLE_H
    draw_tab_bar(draw, ["Controls", "Data List"], 0, tab_y)

    # Content area starts below tab bar
    cy = TITLE_H + TAB_BAR_H

    # ── Left column ──

    # Header label
    draw_label(draw, PADDING, cy + PADDING, 500, 32,
               "Interactive Controls", color=TEXT_SECONDARY, font=font_sm)

    # Buttons
    draw_button(draw, PADDING, cy + 50, 200, BTN_H, "Action", PRIMARY)
    draw_button(draw, 220, cy + 50, 200, BTN_H, "Toggle", SECONDARY)
    draw_button(draw, 440, cy + 50, 200, BTN_H, "Alert", DANGER)

    # Counter label
    draw_label(draw, PADDING, cy + 120, 500, 32,
               "Button presses: 0", color=TEXT_PRIMARY, font=font_md)

    # Text rows
    draw_text_row(draw, PADDING, cy + 165, 620, "Status", "Active")
    draw_text_row(draw, PADDING, cy + 205, 620, "Tab Position", "Top", show_divider=False)

    # Square icons
    draw_icon_square(draw, PADDING,       cy + 260, 52, PRIMARY,   "W")
    draw_icon_square(draw, PADDING + 66,  cy + 260, 52, SECONDARY, "B")
    draw_icon_square(draw, PADDING + 132, cy + 260, 52, ACCENT,    "T")

    # Circle icons
    draw_icon_circle(draw, PADDING + 220 + 26,      cy + 260 + 26, 26, DANGER,  "!")
    draw_icon_circle(draw, PADDING + 220 + 66 + 26, cy + 260 + 26, 26, PRIMARY, "?")

    # Confirm button and result label
    draw_button(draw, PADDING, cy + 330, 300, BTN_H, "Confirm Action", ACCENT)
    draw_label(draw, PADDING + 320, cy + 330, 310, 32,
               "Result: (waiting)", color=ACCENT, font=font_sm)

    # ── Right column ──

    draw_button(draw, 700, cy + PADDING, 540, BTN_H, "Move Tabs to Bottom",
                SURFACE, TEXT_PRIMARY, border_color=PRIMARY)

    draw_label(draw, 700, cy + 80, 540, 32,
               "This is the Controls tab", color=ACCENT, font=font_md)

    draw_text_row(draw, 700, cy + 120, 540, "Display", "1280x720")
    draw_text_row(draw, 700, cy + 160, 540, "Touch", "Capacitive")
    draw_text_row(draw, 700, cy + 200, 540, "Widgets", "14 types", show_divider=False)

    # Text input field
    draw_text_input(draw, 700, cy + 250, 540, placeholder="Type something...")

    # Input result label
    draw_label(draw, 700, cy + 310, 540, 32,
               "Submitted: (none)", color=ACCENT, font=font_sm)

    # Status bar
    draw_status_bar(draw, text="Switch between tabs", left="Tab5", right="Tab Demo")

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 7: Tab Demo — Data List page
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot7():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Title bar
    draw_title_bar(draw, title="Tab Demo", left="< Back", right="v1.0")

    # Tab bar (tabs at top, "Data List" active)
    tab_y = TITLE_H
    draw_tab_bar(draw, ["Controls", "Data List"], 1, tab_y)

    # Content area starts below tab bar
    cy = TITLE_H + TAB_BAR_H

    # Status bar
    draw_status_bar(draw, text="Selected: Messages (#2)", left="Tab5", right="Tab Demo")

    # ── List widget ──
    list_x = PADDING
    list_y = cy + PADDING
    list_w = 600
    list_h = H - TITLE_H - TAB_BAR_H - STATUS_H - PADDING * 2  # content height
    ITEM_H = 48
    SCROLLBAR_W = 6

    items = [
        ("Dashboard",   "D", PRIMARY,   False),
        ("Messages",    "M", ACCENT,    False),
        ("Contacts",    "C", SECONDARY, False),
        ("Calendar",    "C", DANGER,    False),
        ("Tasks",       "T", PRIMARY,   True),
        ("Notes",       "N", SECONDARY, True),
        ("Files",       "F", ACCENT,    True),
        ("Photos",      None, None,     False),
        ("Music",       None, None,     False),
        ("Videos",      None, None,     False),
        ("Downloads",   None, None,     False),
        ("Bookmarks",   None, None,     False),
        ("History",     None, None,     False),
        ("Preferences", None, None,     False),
        ("Network",     None, None,     False),
        ("Bluetooth",   None, None,     False),
        ("Display",     None, None,     False),
        ("Sound",       None, None,     False),
        ("Battery",     None, None,     False),
        ("Storage",     None, None,     False),
    ]

    selected_idx = 1  # "Messages"
    num_items = len(items)
    total_content_h = num_items * ITEM_H
    scroll_offset = 0

    ICON_PAD = 6
    icon_size = ITEM_H - ICON_PAD * 2

    # List background
    draw.rectangle([list_x, list_y, list_x + list_w, list_y + list_h], fill=BG_MEDIUM)
    draw.rectangle([list_x, list_y, list_x + list_w, list_y + list_h], outline=BORDER)

    # Draw visible items
    for i in range(num_items):
        item_y = list_y + (i * ITEM_H) - scroll_offset

        if item_y + ITEM_H <= list_y or item_y >= list_y + list_h:
            continue
        if item_y < list_y:
            continue

        text, icon_char, icon_color, is_circle = items[i]

        # Selected highlight
        if i == selected_idx:
            sel_h = min(ITEM_H, list_y + list_h - item_y)
            draw.rectangle([list_x + 1, item_y, list_x + list_w - SCROLLBAR_W - 2,
                           item_y + sel_h], fill=PRIMARY)

        # Item text
        text_y = item_y + (ITEM_H - FONT_MD) // 2
        if text_y + FONT_MD <= list_y + list_h:
            draw.text((list_x + PADDING, text_y), text,
                      fill=TEXT_PRIMARY, font=font_md)

        # Right-aligned icon
        if icon_char is not None:
            ix = list_x + list_w - SCROLLBAR_W - PADDING - icon_size - 4
            iy = item_y + ICON_PAD

            if is_circle:
                cx = ix + icon_size // 2
                cy_i = iy + icon_size // 2
                r = icon_size // 2
                draw.ellipse([cx - r, cy_i - r, cx + r, cy_i + r],
                             fill=icon_color, outline=BORDER)
                bbox = font_sm.getbbox(icon_char)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((cx - tw // 2, cy_i - th // 2 - 1), icon_char,
                          fill=TEXT_PRIMARY, font=font_sm)
            else:
                rounded_rect(draw, (ix, iy), (icon_size, icon_size), 4,
                             fill=icon_color, outline=BORDER)
                bbox = font_sm.getbbox(icon_char)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((ix + (icon_size - tw) // 2, iy + (icon_size - th) // 2 - 1),
                          icon_char, fill=TEXT_PRIMARY, font=font_sm)

        # Divider
        if i < num_items - 1:
            div_y = item_y + ITEM_H - 1
            if div_y < list_y + list_h:
                draw.line([(list_x + PADDING, div_y),
                           (list_x + list_w - SCROLLBAR_W - PADDING, div_y)],
                          fill=DIVIDER)

    # Scrollbar
    if total_content_h > list_h:
        sb_x = list_x + list_w - SCROLLBAR_W - 1
        sb_area_h = list_h - 2

        draw.rectangle([sb_x, list_y + 1, sb_x + SCROLLBAR_W, list_y + 1 + sb_area_h],
                       fill=darken(BG_MEDIUM, 0.5))

        visible_ratio = list_h / total_content_h
        thumb_h = max(20, int(sb_area_h * visible_ratio))
        max_scroll = total_content_h - list_h
        scroll_ratio = scroll_offset / max_scroll if max_scroll > 0 else 0
        thumb_y = list_y + 1 + int((sb_area_h - thumb_h) * scroll_ratio)

        rounded_rect(draw, (sb_x, thumb_y), (SCROLLBAR_W, thumb_h), 3,
                     fill=TEXT_DISABLED)

    # ── Right-side controls ──

    draw_label(draw, 640, cy + PADDING, 620, 32,
               "Selected: Messages (#2)", color=ACCENT, font=font_md)

    draw_button(draw, 640, cy + PADDING + 50, 280, BTN_H,
                "Show Selected", PRIMARY)
    draw_button(draw, 940, cy + PADDING + 50, 280, BTN_H,
                "Clear Selection", DANGER)

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Screenshot 8: Confirm Popup
# ═════════════════════════════════════════════════════════════════════════════
def generate_screenshot8():
    img = Image.new("RGB", (W, H), BG_DARK)
    draw = ImageDraw.Draw(img)
    draw_all_base_elements(draw, "Ready")

    # Confirm popup — "Delete" / "Are you sure you want to delete this item?"
    title = "Delete"
    message = "Are you sure you want to delete this item?"
    yes_label = "Yes"
    no_label = "No"

    # Measure text to auto-size
    title_bbox = font_lg.getbbox(title)
    title_w = title_bbox[2] - title_bbox[0]

    msg_bbox = font_md.getbbox(message)
    msg_w = msg_bbox[2] - msg_bbox[0]

    yes_bbox = font_md.getbbox(yes_label)
    yes_label_w = yes_bbox[2] - yes_bbox[0]
    yes_btn_w = yes_label_w + 60
    if yes_btn_w < 100:
        yes_btn_w = 100

    no_bbox = font_md.getbbox(no_label)
    no_label_w = no_bbox[2] - no_bbox[0]
    no_btn_w = no_label_w + 60
    if no_btn_w < 100:
        no_btn_w = 100

    btn_gap = 20
    total_btn_w = yes_btn_w + btn_gap + no_btn_w

    hpad = PADDING * 2
    popup_w = max(title_w + hpad + 40, msg_w + hpad + 20, total_btn_w + hpad, 260)
    popup_w = min(popup_w, W - 80)

    # Height
    title_gap = 42
    btn_area_h = 56
    line_h = FONT_MD + 4
    popup_h = PADDING + title_gap + 10 + line_h + 10 + btn_area_h + PADDING
    popup_h = max(popup_h, 140)
    popup_h = min(popup_h, H - 80)

    px = (W - popup_w) // 2
    py = (H - popup_h) // 2

    # Shadow
    draw.rectangle([px + 4, py + 4, px + popup_w + 4, py + popup_h + 4], fill=SHADOW)

    # Background
    rounded_rect(draw, (px, py), (popup_w, popup_h), 8, fill=SURFACE, outline=BORDER)

    # Title
    bbox = font_lg.getbbox(title)
    tw = bbox[2] - bbox[0]
    draw.text((px + (popup_w - tw) // 2, py + PADDING + 4), title,
              fill=TEXT_PRIMARY, font=font_lg)

    # Divider
    div_y = py + PADDING + 38
    draw.line([(px + PADDING, div_y), (px + popup_w - PADDING, div_y)], fill=DIVIDER)

    # Message
    bbox = font_md.getbbox(message)
    mw = bbox[2] - bbox[0]
    msg_y = div_y + 14
    draw.text((px + (popup_w - mw) // 2, msg_y), message,
              fill=TEXT_SECONDARY, font=font_md)

    # Buttons — No (left, red) and Yes (right, green)
    btn_h = 40
    btn_start_x = px + (popup_w - total_btn_w) // 2
    btn_y = py + popup_h - btn_h - PADDING

    # No button
    no_btn_x = btn_start_x
    rounded_rect(draw, (no_btn_x, btn_y), (no_btn_w, btn_h), 6, fill=DANGER)
    bbox = font_md.getbbox(no_label)
    bw = bbox[2] - bbox[0]
    bh = bbox[3] - bbox[1]
    draw.text((no_btn_x + (no_btn_w - bw) // 2, btn_y + (btn_h - bh) // 2 - 2), no_label,
              fill=TEXT_PRIMARY, font=font_md)

    # Yes button
    yes_btn_x = btn_start_x + no_btn_w + btn_gap
    rounded_rect(draw, (yes_btn_x, btn_y), (yes_btn_w, btn_h), 6, fill=SECONDARY)
    bbox = font_md.getbbox(yes_label)
    bw = bbox[2] - bbox[0]
    bh = bbox[3] - bbox[1]
    draw.text((yes_btn_x + (yes_btn_w - bw) // 2, btn_y + (btn_h - bh) // 2 - 2), yes_label,
              fill=TEXT_PRIMARY, font=font_md)

    return img


# ═════════════════════════════════════════════════════════════════════════════
#  Main — Generate all screenshots
# ═════════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "screenshots")
    os.makedirs(out_dir, exist_ok=True)

    generators = [
        ("screenshot1_initial.png",       generate_screenshot1,  "Initial state"),
        ("screenshot2_menu.png",          generate_screenshot2,  "Menu open"),
        ("screenshot3_keyboard.png",      generate_screenshot3,  "Keyboard visible"),
        ("screenshot4_popup.png",         generate_screenshot4,  "Info popup"),
        ("screenshot5_list.png",          generate_screenshot5,  "List demo"),
        ("screenshot6_tab_controls.png",  generate_screenshot6,  "Tab demo — Controls"),
        ("screenshot7_tab_list.png",      generate_screenshot7,  "Tab demo — Data List"),
        ("screenshot8_confirm_popup.png", generate_screenshot8,  "Confirm popup"),
    ]

    for filename, gen_func, desc in generators:
        print(f"Generating {desc}...")
        img = gen_func()
        path = os.path.join(out_dir, filename)
        img.save(path, "PNG")
        print(f"  Saved: {path}")

    print("\nDone! All 8 screenshots generated.")
