# Device Build: how `cardmesh_<version>_arm64.deb` is produced

This document describes the CardputerZero device build — the LVGL-based
`cardmesh` binary and the `.deb` that packages it — and, more importantly,
**exactly what has and hasn't been verified**. Read this before submitting a
build to the CardputerZero Developer Center.

## Why this exists

The README describes a `.deb` release built with the official CardputerZero
AppBuilder toolchain. That toolchain was not available in the environment
this build was produced in — no CardputerZero SDK, no Docker image, no ARM64
Linux cross-compiler, and no way to run the online emulator
(`https://cardputer.cc/emulator/`) or physical hardware.

Rather than skip the device build entirely, this is a **best-effort
alternative path**: a real ARM64 Linux ELF binary, cross-compiled and linked
against real (not fabricated) shared libraries, packaged into a real `.deb`.
It has not been run on a CardputerZero or in its emulator. Treat it as a
release candidate to test, not a verified release.

## What's actually implemented

The MVP **Dashboard** screen (see the README's "Dashboard" mock) and a
**Map** screen showing node positions on an offline-capable OpenStreetMap
view are implemented:

- Reads `~/.config/cardmesh/config.json` (same format/location as the host
  CLI build — see `src/storage/Settings.cpp`). There is no first-run setup
  UI yet, so this file must exist before launch.
- A background thread (`src/ui/NetworkWorker.cpp`) polls MeshMonitor every
  10 seconds and updates shared, mutex-guarded models (`DashboardModel`,
  `MapModel`) — the LVGL UI thread never blocks on network I/O, per the
  README's "Background Architecture."
- Global keyboard shortcuts: `R` (refresh now), `M` (open Map from
  Dashboard), `B`/`Esc` (back to Dashboard from Map), arrow keys (pan on
  Map), `+`/`-` (zoom on Map), `Q` (quit) — read directly from
  `/dev/input` (`src/ui/EvdevKeyboard.cpp`) rather than through LVGL's
  focus/group indev system, since neither screen has focusable widgets.
- Channels, Chat, Nodes, Node Details, Direct Messages, Sources, Telemetry,
  Traceroute, Field Mode, and Settings screens are **not implemented**.
- `src/storage/Database.cpp` (SQLite cache, unread state, favorites) exists
  and is unit-tested on the host build, but is not yet wired into the device
  UI — the Dashboard's "Unread" tile currently sums `Channel.unreadCount`
  directly from the API response, not from local read-state tracking.
- "Active" node count is a CardMesh-invented heuristic (`lastHeard` within
  the last 15 minutes) — MeshMonitor's API doesn't define an "active" field.
- **Emoji in the source name render as small inline images**, not tofu
  boxes — LVGL's bundled Montserrat font has no emoji glyphs at all, so
  `src/ui/EmojiFont.{h,cpp}` substitutes small embedded Twemoji-derived
  images for every single-codepoint emoji and every two-codepoint
  regional-indicator flag pair (1651 codepoints total; see
  `tools/gen_emoji_asset.py`). **Not covered**: multi-codepoint ZWJ
  sequences — family groups, skin-tone modifiers, profession+gender
  combos — since LVGL's imgfont callback only gets a 1-codepoint
  lookahead, not full text shaping like a browser has. Those still render
  as whatever the fallback font does with each individual codepoint
  (typically nothing, since Montserrat doesn't have them either).

## Map screen: offline OpenStreetMap tiles

`M` from the Dashboard opens a node-position map. This is a real runtime
tile cache, not a fixed pre-baked region: `src/map/TileCache.cpp` fetches
OSM raster tiles over HTTP (`AppSettings::tileServerTemplate`, default
`tile.openstreetmap.org`) and writes them to
`~/.local/state/cardmesh/tiles/{z}/{x}/{y}.png`; once a tile has been
fetched once it displays with no connection, for whatever areas have
actually been viewed. `src/map/TileFetcher.cpp` runs this on a background
thread (mirroring `NetworkWorker`'s pattern) so a cache miss never blocks
the UI. `src/ui/MapScreen.cpp` renders a 2×2 grid of tiles covering the
320×170 viewport at any pan offset, plus small marker dots (yellow for
favorites) at each node's projected position, computed via standard OSM
"slippy map" Web Mercator math (`src/map/WebMercator.h`).

The initial view centers on and auto-zooms to fit the bounding box of
whatever nodes currently have a known GPS position (falls back to nothing
drawn if none do) — not a hardcoded zoom level, since real mesh node
spreads vary enormously. After that, `M`/arrows/`+`/`-` are yours; nothing
re-centers automatically once you've panned or zoomed manually.

**OSM tile server usage policy**: `tile.openstreetmap.org`'s policy
(https://operations.osmfoundation.org/policies/tiles/) prohibits heavy or
bulk automated use and requires a descriptive `User-Agent` (CardMesh sends
one identifying itself and this repo). Fine for light personal use with
this disk cache in front of it, but a widely-distributed build should
point `tileServerTemplate` in `~/.config/cardmesh/config.json` at a
self-hosted or commercial tile server instead.

**A real bug this surfaced, worth knowing about**: the first working build
of this screen showed node markers correctly but a completely black map —
no tiles, no error visible. `LV_USE_LOG` and `LV_LOG_PRINTF` turned out to
both be `0` in both `lv_conf.h` variants (only `LV_LOG_LEVEL` had been set,
which does nothing while the gate itself is off), so LVGL's own warnings
about *why* were silently discarded. With logging actually enabled, the
real error was `lv_realloc: couldn't reallocate memory` — `LV_MEM_SIZE`
was 128KB, but a single decoded 256×256 OSM tile needs ~256KB
(256×256×4 bytes ARGB8888) to decode into, so the allocator couldn't even
fit one tile. The 16×16 emoji images (§ above) never hit this because
they're 1024 bytes each. Fixed by raising `LV_MEM_SIZE` to 4MB in both
`src/ui/lv_conf.h` and `tools/screenshot/lv_conf.h`, and logging is now
left on (`LV_LOG_LEVEL_WARN`) rather than re-silenced, since it's what
actually surfaced this. If a future feature adds more/larger decoded
images and hits this again, the same “enable logging first, don’t guess”
approach is the fastest way back to the real error.

Verified via `tools/screenshot/`: `./tools/screenshot/build/screenshot map
out.raw` does a real HTTP fetch against the real tile server (mock is only
the node position list — this dev machine isn't near real mesh nodes) and
renders the actual `MapScreen` code; `screenshots/map.png` in the README
is that real output, not a mockup.

## Toolchain: why zig instead of a native cross-compiler

Homebrew's `aarch64-unknown-linux-gnu` cross-toolchain formula
(`messense/macos-cross-toolchains`) refused to install because this
machine's Xcode Command Line Tools are older than the bottle requires, and
updating CLT requires either a GUI Software Update flow or
`sudo rm -rf /Library/Developer/CommandLineTools` — a system-wide change out
of scope for this task.

[Zig](https://ziglang.org) (`brew install zig`) bundles its own Clang and
ships glibc ABI stub data for a wide range of glibc versions, so
`zig cc -target aarch64-linux-gnu.2.36` cross-compiles and links real
aarch64 glibc binaries with no system cross-toolchain and no CLT dependency.
This is the mechanism `scripts/build_arm64.py` uses.

## Sysroot: why Debian bookworm packages instead of a "real" SDK

CardMesh links against libcurl and SQLite. Building OpenSSL/libcurl from
source for a cross target is slow and failure-prone within this task's
scope, so instead `scripts/fetch_sysroot.py` downloads the actual `.deb`
packages Debian bookworm (12) ships for `arm64` — `libcurl4`,
`libcurl4-openssl-dev`, `libssl3`/`libssl-dev`, `libsqlite3-0`/`-dev`, and
the full transitive chain curl pulls in (GnuTLS, Kerberos, LDAP, SASL,
brotli, zstd, etc. — 31 packages total) — and extracts them into a local
sysroot with real headers and shared libraries.

**This is the central assumption of this entire build**: that
CardputerZero's actual OS is close enough to Debian bookworm (glibc 2.36,
same library ABI) for this to work. The README says "Debian ARM64
packaging," which is why bookworm was chosen, but this has not been
confirmed against the actual device.

To reproduce or refresh the sysroot:

```sh
python3 scripts/fetch_sysroot.py /path/to/sysroot-workdir
```

If a package URL in `fetch_sysroot.py` 404s (Debian rotates old point
releases out of the pool sometimes), re-resolve the current filename from
`https://deb.debian.org/debian/dists/bookworm/main/binary-arm64/Packages.xz`.

## Build pipeline

```sh
brew install zig dpkg          # cross-compiler + .deb tooling
python3 scripts/fetch_sysroot.py /tmp/cardmesh-sysroot
python3 scripts/build_arm64.py /tmp/cardmesh-sysroot/root
python3 scripts/package_deb.py /tmp/cardmesh-sysroot/root/usr/lib/aarch64-linux-gnu 0.1.0
```

`build_arm64.py` compiles the core (`src/api`, `src/storage`), the UI layer
(`src/ui`, `src/ui_main.cpp`), and all of vendored LVGL (`third_party/lvgl`)
with `zig c++`/`zig cc`, then links dynamically against the sysroot's
`libcurl`/`libsqlite3`.

`package_deb.py` computes the **full transitive closure** of shared
libraries the binary actually needs (`scripts/elf_needed_bundled.py` parses
the ELF `.dynamic` section directly — ~230 lines of pure Python, no
`readelf`/`objdump` available on this macOS host) and bundles all 29 of them
under `/usr/lib/cardmesh/`, each renamed to its own `DT_SONAME` (this was
caught as a bug during development: the natural file to copy is something
like `libcurl.so.4.8.0`, but the dynamic linker looks up dependencies by the
exact SONAME string `libcurl.so.4` — bundling under the versioned filename
would silently fail at runtime). The package installs a thin
`/usr/bin/cardmesh` wrapper that sets `LD_LIBRARY_PATH=/usr/lib/cardmesh`
before exec'ing the real binary, so the bundled libraries resolve for the
whole dependency chain, not just cardmesh's direct dependencies. `libc6` is
the only declared `Depends:` — everything else ships in the package.

## What was verified, and how

- The binary is a valid `ELF 64-bit LSB executable, ARM aarch64 ... for
  GNU/Linux`, confirmed by direct inspection of the ELF header (no
  `file`/`readelf` needed, but `file` on this Mac does report this
  correctly for foreign-arch ELFs).
- Every `DT_NEEDED` entry, for the binary and for every bundled library
  transitively, resolves to a file that's either bundled or expected to be
  part of any base glibc install (`libc.so.6`, `libm.so.6`,
  `ld-linux-aarch64.so.1`, etc.) — `package_deb.py` raises an error if
  anything is missing rather than silently shipping a broken package.
- Every bundled library's own `DT_SONAME` was checked to match the filename
  it's bundled under (see the bug note above).
- The full pipeline (fetch sysroot → build → package) was re-run from a
  clean directory using only the committed scripts, to confirm it's
  reproducible and not dependent on hand-fixups made during development.

## Confirmed against the real dev.cardputer.cc portal

`cardmesh_0.1.0_arm64.deb` was actually uploaded to
`https://dev.cardputer.cc/` (2026-08-13). This confirmed several things
this document previously listed as unverified assumptions:

- **The upload itself succeeded** and the portal's "Preliminary check
  report" parsed the package correctly, reporting "包含 30 个 ELF 二进制
  （架构均为 arm64）" — "contains 30 ELF binaries (all arm64
  architecture)" — matching this build's binary + 29 bundled libraries
  exactly. This independently confirms the arm64/ELF validity claimed in
  "What was verified, and how" above, from a source other than this build
  pipeline's own checks.
- **A desktop-entry file is required** at
  `usr/share/APPLaunch/applications/*.desktop` — the portal rejected v0.1.0
  with "缺少 usr/share/APPLaunch/applications/*.desktop（商店应用必须提供）"
  ("missing ...; required for store apps"). This is not documented anywhere
  in the README and was only discovered by this trial upload.
  `scripts/package_deb.py` now generates a standard freedesktop-style
  `.desktop` entry (`Name`, `Exec=cardmesh`, `Icon=cardmesh`, `Categories`)
  at that path, and bundles `assets/branding/appIcon.png` at
  `usr/share/pixmaps/cardmesh.png` to match `Icon=cardmesh`.
- **The store submission form's real fields** are now known: app name,
  one-line summary (≤80 chars), full description, up to 6 comma-separated
  categories, an optional square-PNG icon (falls back to the icon inside
  the `.deb` if left empty), and up to 6 screenshots at 320×170. A
  `store` section in `app-builder.json` can reportedly auto-fill these from
  a **public** source repository — not applicable here since this repo is
  private, per the README's "Notes for AI Coding Assistants."

Still unconfirmed: whether the package actually *launches and renders*
correctly once installed — the portal's check is a packaging/metadata
validator, not a functional test on real hardware or the emulator.

## What was NOT verified (the actual risk list)

- **Not run on a physical CardputerZero**, and not confirmed to launch
  successfully even after passing the store portal's packaging checks
  (see above — that check validates structure, not runtime behavior).
- **Not run in the online emulator** (`https://cardputer.cc/emulator/`) —
  unknown whether that emulator even executes a plain Linux
  fbdev/evdev ELF binary, or expects integration through the AppBuilder's
  own runtime/manifest format instead.
- **Framebuffer device path**: assumed `/dev/fb0` (override via the
  `CARDMESH_FB_DEVICE` environment variable — edit
  `/usr/bin/cardmesh` after install, or set it in whatever launches the app).
- **Input device path**: assumed `/dev/input/event0` (override via
  `CARDMESH_INPUT_DEVICE`).
- **Color depth**: `lv_conf.h` assumes 16bpp (RGB565), a common depth for
  small embedded IPS panels, but not confirmed for this specific display.
  LVGL's fbdev driver reads the real depth from the framebuffer at runtime,
  so a mismatch may cause incorrect colors rather than a crash — but this
  is unconfirmed either way.
- **glibc baseline**: the bundled libraries require glibc ≥ 2.36 on the
  device itself (`libc6` is not bundled — bundling libc is far riskier than
  depending on the system's own). If CardputerZero's OS ships an older
  glibc, the process will fail to start with a `GLIBC_2.36 not found`-style
  error. The store portal's packaging check does not validate this.
- **Map screen performance**: repeated tile decode/redraw while panning
  (each tile is decoded fresh, not pre-cached as a bitmap — LVGL's own
  image cache may or may not help here) has not been measured on anything
  resembling CardputerZero's actual CPU. The host screenshot tool renders
  fast, but that's an Apple Silicon Mac, not representative.
- **Map screen network behavior on the real device is unconfirmed**: tile
  fetches use the same `HttpClient`/libcurl path as MeshMonitor API calls
  and were verified from this dev machine's network, not from whatever
  connectivity a real CardputerZero has in the field.

## If it doesn't run

Given the above, the most likely failure modes, roughly in order of
likelihood, are:

1. The online emulator or device doesn't expect a raw Linux
   fbdev/evdev binary at all, and needs AppBuilder-specific integration —
   in which case the real CardputerZero SDK is required, and this whole
   approach needs to be revisited with that toolchain instead.
2. Wrong framebuffer/input device paths — fixable via the environment
   variables above.
3. `GLIBC_2.36 not found` — the device's glibc is older than Debian
   bookworm's; the sysroot would need to be rebuilt from an older Debian
   suite (e.g. bullseye/11) via `fetch_sysroot.py` with adjusted package
   URLs, and `build_arm64.py`'s `TARGET` lowered to match.
4. Wrong color depth — adjust `LV_COLOR_DEPTH` in `src/ui/lv_conf.h` and
   rebuild.
