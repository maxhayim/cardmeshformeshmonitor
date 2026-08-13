#pragma once

#include "lvgl.h"

namespace cardmesh::ui {

// Creates an LVGL font that renders emoji from the embedded Twemoji-derived
// atlas (src/ui/assets/EmojiAtlas.h) as small inline images -- covering
// every single-codepoint emoji plus every two-codepoint regional-indicator
// flag pair (~1650 total) -- and falls back to `fallback` for everything
// else, including multi-codepoint ZWJ sequences (family groups, skin-tone
// modifiers, profession+gender combos), which LVGL's imgfont can't express
// since it only gets a 1-codepoint lookahead. See tools/gen_emoji_asset.py.
//
// The caller owns the returned font and must pass it to destroyEmojiFont()
// when done (mirrors lv_imgfont_create/lv_imgfont_destroy).
lv_font_t* createEmojiFont(uint16_t pixelSize, const lv_font_t* fallback);
void destroyEmojiFont(lv_font_t* font);

}  // namespace cardmesh::ui
