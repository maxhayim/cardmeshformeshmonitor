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

Only the MVP **Dashboard** screen (see the README's "Dashboard" mock) is
implemented:

- Reads `~/.config/cardmesh/config.json` (same format/location as the host
  CLI build — see `src/storage/Settings.cpp`). There is no first-run setup
  UI yet, so this file must exist before launch.
- A background thread (`src/ui/NetworkWorker.cpp`) polls MeshMonitor every
  10 seconds and updates a shared, mutex-guarded model — the LVGL UI thread
  never blocks on network I/O, per the README's "Background Architecture."
- Global keyboard shortcuts only: `R` (refresh now) and `Q`/`Esc` (quit),
  read directly from `/dev/input` (`src/ui/EvdevKeyboard.cpp`) rather than
  through LVGL's focus/group indev system, since there are no focusable
  widgets yet.
- Channels, Chat, Nodes, Node Details, Direct Messages, Sources, Telemetry,
  Traceroute, Field Mode, and Settings screens are **not implemented**.
- `src/storage/Database.cpp` (SQLite cache, unread state, favorites) exists
  and is unit-tested on the host build, but is not yet wired into the device
  UI — the Dashboard's "Unread" tile currently sums `Channel.unreadCount`
  directly from the API response, not from local read-state tracking.
- "Active" node count is a CardMesh-invented heuristic (`lastHeard` within
  the last 15 minutes) — MeshMonitor's API doesn't define an "active" field.

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

## What was NOT verified (the actual risk list)

- **Not run on a physical CardputerZero.**
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
  error.
- **`app-builder.json` schema**: fields (`package`, `executable`,
  `displayName`, `architecture`, etc.) are inferred from what the README
  states the Developer Center expects, not validated against the real
  manifest schema.

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
