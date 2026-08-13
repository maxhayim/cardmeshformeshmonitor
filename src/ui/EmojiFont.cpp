#include "EmojiFont.h"

#include <array>
#include <mutex>

#include "assets/EmojiAtlas.h"

namespace cardmesh::ui {

namespace {

using assets::EmojiIndexEntry;
using assets::kBlankGlyphLength;
using assets::kBlankGlyphOffset;
using assets::kEmojiAtlas;
using assets::kEmojiIndex;
using assets::kEmojiIndexCount;

lv_image_dsc_t makeDsc(uint32_t offset, uint32_t length) {
    lv_image_dsc_t dsc{};
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_RAW;
    dsc.data_size = length;
    dsc.data = &kEmojiAtlas[offset];
    return dsc;
}

// One permanent lv_image_dsc_t per atlas entry (1:1 with kEmojiIndex, same
// order) plus one for the blank/invisible glyph. Built once and never
// mutated again, so returned pointers stay valid for the process lifetime
// regardless of whether LVGL's font glyph cache holds onto them across
// frames -- a per-call reusable scratch struct would risk a cached pointer
// silently pointing at a *different* glyph's data after being overwritten.
struct GlyphTable {
    std::array<lv_image_dsc_t, kEmojiIndexCount> perEntry;
    lv_image_dsc_t blank;

    GlyphTable() {
        for (size_t i = 0; i < kEmojiIndexCount; i++) {
            perEntry[i] = makeDsc(kEmojiIndex[i].offset, kEmojiIndex[i].length);
        }
        blank = makeDsc(kBlankGlyphOffset, kBlankGlyphLength);
    }
};

const GlyphTable& glyphTable() {
    static const GlyphTable table;
    return table;
}

bool isRegionalIndicator(uint32_t codepoint) { return codepoint >= 0x1F1E6 && codepoint <= 0x1F1FF; }

// Binary search kEmojiIndex (sorted by codepoint1, then codepoint2) for an
// exact (cp1, cp2) match. Returns -1 if not found.
int findIndex(uint32_t cp1, uint32_t cp2) {
    size_t lo = 0;
    size_t hi = kEmojiIndexCount;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const EmojiIndexEntry& e = kEmojiIndex[mid];
        if (e.codepoint1 < cp1 || (e.codepoint1 == cp1 && e.codepoint2 < cp2)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < kEmojiIndexCount && kEmojiIndex[lo].codepoint1 == cp1 && kEmojiIndex[lo].codepoint2 == cp2) {
        return static_cast<int>(lo);
    }
    return -1;
}

const void* emojiPathCb(const lv_font_t* /*font*/, uint32_t unicode, uint32_t unicode_next,
                         int32_t* offset_y, void* /*user_data*/) {
    *offset_y = -2;
    const GlyphTable& table = glyphTable();

    // A flag pair starting here takes priority (its second codepoint is
    // handled below, on the *next* path_cb call, via isRegionalIndicator).
    const int flagIndex = findIndex(unicode, unicode_next);
    if (flagIndex >= 0 && kEmojiIndex[flagIndex].codepoint2 != 0) {
        return &table.perEntry[flagIndex];
    }

    const int singleIndex = findIndex(unicode, 0);
    if (singleIndex >= 0) {
        return &table.perEntry[singleIndex];
    }

    // Second half of a flag pair we already rendered, or a stray/incomplete
    // regional indicator on its own -- render nothing instead of falling
    // back to the normal font, which has no glyph for this range either
    // and would show a tofu box.
    if (isRegionalIndicator(unicode)) {
        return &table.blank;
    }

    return nullptr;  // not an emoji CardMesh has an image for -- use fallback font.
}

}  // namespace

lv_font_t* createEmojiFont(uint16_t pixelSize, const lv_font_t* fallback) {
    lv_font_t* font = lv_imgfont_create(pixelSize, emojiPathCb, nullptr);
    if (font != nullptr) {
        font->fallback = fallback;
    }
    return font;
}

void destroyEmojiFont(lv_font_t* font) {
    if (font != nullptr) {
        lv_imgfont_destroy(font);
    }
}

}  // namespace cardmesh::ui
