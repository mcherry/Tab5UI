# Tab5UI Icons

## Attribution

The icons in this directory are sourced from **IconPark** by ByteDance.

- **Project:** [IconPark](https://github.com/bytedance/IconPark)
- **Author:** ByteDance, Inc.
- **License:** Apache License 2.0 (see [LICENSE](LICENSE))

## About IconPark

IconPark provides access to more than 2,000 high-quality icons and introduces an interface for customizing icons. Each icon can be freely adjusted in runtime to meet specific needs (color, size, stroke width, etc.). The original SVG icons were converted to 32×32 pixel PNG images and embedded as `PROGMEM` C byte arrays for use on embedded platforms.

## Usage

Include any icon header in your Arduino sketch:

```cpp
#include "icons/icon_home.h"

// Draw the icon using M5GFX:
display.drawPng(icon_home, icon_home_size, x, y);
```

Each header file provides:

| Symbol | Type | Description |
|--------|------|-------------|
| `icon_<name>[]` | `const uint8_t PROGMEM` | Raw PNG byte array |
| `icon_<name>_size` | `const uint32_t` | Size of the array in bytes |

## Available Icons

| Header | Icon |
|--------|------|
| `icon_aiming.h` | Aiming / crosshair |
| `icon_all_application.h` | All applications / grid |
| `icon_bill.h` | Bill / receipt |
| `icon_bookmark_one.h` | Bookmark (variant) |
| `icon_bookmark.h` | Bookmark |
| `icon_camera.h` | Camera |
| `icon_config.h` | Configuration |
| `icon_delete_two.h` | Delete (variant) |
| `icon_dislike_two.h` | Dislike (variant) |
| `icon_dislike.h` | Dislike / thumbs down |
| `icon_equalizer.h` | Equalizer / sliders |
| `icon_female.h` | Female |
| `icon_hamburger_button.h` | Hamburger menu |
| `icon_home.h` | Home |
| `icon_hourglass_full.h` | Hourglass (full) |
| `icon_hourglass_null.h` | Hourglass (empty) |
| `icon_lightning.h` | Lightning bolt |
| `icon_like.h` | Like / thumbs up |
| `icon_loading_four.h` | Loading (variant) |
| `icon_loading.h` | Loading / spinner |
| `icon_male.h` | Male |
| `icon_more_app.h` | More apps |
| `icon_more_one.h` | More (variant) |
| `icon_more_two.h` | More (variant 2) |
| `icon_more.h` | More / ellipsis |
| `icon_pic.h` | Picture / image |
| `icon_power.h` | Power |
| `icon_preview_close_one.h` | Eye closed (variant) |
| `icon_preview_close.h` | Eye closed |
| `icon_preview_open.h` | Eye open / preview |
| `icon_radar.h` | Radar |
| `icon_refresh.h` | Refresh |
| `icon_rss.h` | RSS feed |
| `icon_save_one.h` | Save (variant) |
| `icon_save.h` | Save / floppy disk |
| `icon_search.h` | Search / magnifying glass |
| `icon_setting_config.h` | Settings config |
| `icon_setting_one.h` | Settings (variant) |
| `icon_setting_three.h` | Settings (variant 3) |
| `icon_setting_two.h` | Settings (variant 2) |
| `icon_setting.h` | Settings / gear |
| `icon_share_three.h` | Share (variant) |
| `icon_share.h` | Share |
| `icon_sleep.h` | Sleep / moon |
| `icon_system.h` | System |
| `icon_tag_one.h` | Tag (variant) |
| `icon_tag.h` | Tag / label |
| `icon_tips.h` | Tips / lightbulb |
| `icon_tool.h` | Tool / wrench |
| `icon_translate.h` | Translate |
| `icon_unlike.h` | Unlike / broken heart |
| `icon_waterfalls_h.h` | Waterfall horizontal |
| `icon_waterfalls_v.h` | Waterfall vertical |
| `icon_zoom_in.h` | Zoom in |
| `icon_zoom_out.h` | Zoom out |
