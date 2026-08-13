#!/usr/bin/env python3
"""Generates src/ui/assets/EmojiAtlas.h: embeds every single-codepoint emoji
plus every two-codepoint regional-indicator flag pair from Twemoji as small
resized PNGs, packed into one atlas byte array plus a lookup index, for
LVGL's imgfont (see EmojiFont.cpp).

This is real coverage, not "all emoji": multi-codepoint ZWJ sequences
(family groups, skin-tone-modified emoji, profession+gender combos,
subdivision flags like Scotland/Wales) are Twitter's compound glyphs and are
NOT included -- LVGL's imgfont only gets a 2-codepoint lookahead
(unicode, unicode_next), not full text-shaping, so those sequences would
need a different rendering approach entirely. Unsupported codepoints fall
back to the normal font, which typically renders nothing (LVGL's Montserrat
has no fallback glyph for them either) rather than a random emoji image.

Source: Twemoji (https://github.com/twitter/twemoji), CC-BY 4.0.
Attribution: Twitter, Inc and other contributors.

Usage:
  git clone --depth 1 --filter=blob:none --sparse \
      https://github.com/twitter/twemoji.git /tmp/twemoji
  (cd /tmp/twemoji && git sparse-checkout set assets/72x72)
  python3 tools/gen_emoji_asset.py /tmp/twemoji/assets/72x72
"""
import io
import os
import sys

try:
    from PIL import Image
except ImportError:
    raise SystemExit("This script requires Pillow: pip3 install pillow")

OUT_SIZE = (16, 16)
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_HEADER = os.path.join(REPO_ROOT, "src", "ui", "assets", "EmojiAtlas.h")


def load_entries(assets_dir):
    """Returns [(codepoints_tuple, filepath), ...] for single-codepoint
    emoji and two-codepoint regional-indicator (flag) pairs only."""
    entries = []
    for name in sorted(os.listdir(assets_dir)):
        if not name.endswith(".png"):
            continue
        stem = name[:-4]
        try:
            codepoints = tuple(int(p, 16) for p in stem.split("-"))
        except ValueError:
            continue

        if len(codepoints) == 1:
            entries.append((codepoints, os.path.join(assets_dir, name)))
        elif len(codepoints) == 2 and all(0x1F1E6 <= c <= 0x1F1FF for c in codepoints):
            entries.append((codepoints, os.path.join(assets_dir, name)))
        # else: ZWJ sequence / skin-tone modifier / tag sequence -- skipped, see module docstring.

    return entries


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    assets_dir = sys.argv[1]

    entries = load_entries(assets_dir)
    print(f"Found {len(entries)} supported emoji (single-codepoint + flag pairs)")

    atlas = bytearray()
    index_entries = []  # (cp1, cp2_or_0, offset, length)

    # A 1x1 fully-transparent glyph. Used for regional-indicator codepoints
    # that don't form a recognized flag pair with their neighbor (e.g. the
    # second half of a flag we already rendered as one wide glyph on the
    # first half, or a stray/incomplete regional indicator) -- rendering
    # this instead of falling back to the normal font avoids a tofu box for
    # a codepoint that font has no glyph for either. See EmojiFont.cpp.
    blank = Image.new("RGBA", (1, 1), (0, 0, 0, 0))
    buf = io.BytesIO()
    blank.save(buf, format="PNG", optimize=True)
    blank_png_bytes = buf.getvalue()
    blank_offset = len(atlas)
    atlas.extend(blank_png_bytes)
    blank_length = len(blank_png_bytes)

    for codepoints, path in entries:
        img = Image.open(path).convert("RGBA").resize(OUT_SIZE, Image.LANCZOS)
        buf = io.BytesIO()
        img.save(buf, format="PNG", optimize=True)
        png_bytes = buf.getvalue()

        offset = len(atlas)
        atlas.extend(png_bytes)

        cp1 = codepoints[0]
        cp2 = codepoints[1] if len(codepoints) > 1 else 0
        index_entries.append((cp1, cp2, offset, len(png_bytes)))

    # Sorted by (cp1, cp2) so EmojiFont.cpp can binary-search.
    index_entries.sort(key=lambda e: (e[0], e[1]))

    print(f"Atlas size: {len(atlas)} bytes ({len(atlas) / 1024:.1f} KiB)")

    atlas_lines = []
    for i in range(0, len(atlas), 20):
        chunk = atlas[i : i + 20]
        atlas_lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")

    index_lines = [
        f"    {{0x{cp1:x}, 0x{cp2:x}, {offset}u, {length}u}}," for cp1, cp2, offset, length in index_entries
    ]

    header = f"""#pragma once

// Twemoji single-codepoint emoji + regional-indicator flag pairs, packed
// into one atlas of {OUT_SIZE[0]}x{OUT_SIZE[1]}px PNGs for LVGL's imgfont.
// Source: https://github.com/twitter/twemoji (CC-BY 4.0).
// Regenerate via tools/gen_emoji_asset.py; do not hand-edit.
// {len(index_entries)} entries, {len(atlas)} atlas bytes.

#include <cstddef>
#include <cstdint>

namespace cardmesh::ui::assets {{

struct EmojiIndexEntry {{
    uint32_t codepoint1;
    uint32_t codepoint2;  // 0 if this entry is a single codepoint, not a flag pair.
    uint32_t offset;
    uint32_t length;
}};

inline const uint8_t kEmojiAtlas[] = {{
{chr(10).join(atlas_lines)}
}};

inline constexpr size_t kEmojiAtlasSize = sizeof(kEmojiAtlas);

inline constexpr uint32_t kBlankGlyphOffset = {blank_offset}u;
inline constexpr uint32_t kBlankGlyphLength = {blank_length}u;

// Sorted by (codepoint1, codepoint2) for binary search.
inline const EmojiIndexEntry kEmojiIndex[] = {{
{chr(10).join(index_lines)}
}};

inline constexpr size_t kEmojiIndexCount = sizeof(kEmojiIndex) / sizeof(kEmojiIndex[0]);

}}  // namespace cardmesh::ui::assets
"""

    os.makedirs(os.path.dirname(OUT_HEADER), exist_ok=True)
    with open(OUT_HEADER, "w") as f:
        f.write(header)

    print(f"Wrote {OUT_HEADER}")


if __name__ == "__main__":
    main()
