#!/usr/bin/env python3
"""Packages the cross-compiled arm64 `cardmesh` binary (see build_arm64.py)
into a self-contained .deb: the binary and its full transitive runtime
library closure are bundled under /usr/lib/cardmesh, with a thin
/usr/bin/cardmesh wrapper that sets LD_LIBRARY_PATH before exec'ing it.

This avoids depending on the target device's package repository matching
Debian bookworm exactly -- see docs/DEVICE_BUILD.md.

Usage: python3 scripts/package_deb.py <path-to-sysroot-lib-dir> <version>
"""
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from elf_needed_bundled import compute_closure, parse_soname  # noqa: E402

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BINARY = os.path.join(REPO_ROOT, "build-arm64", "cardmesh")


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    libdir = os.path.abspath(sys.argv[1])
    version = sys.argv[2]

    if not os.path.exists(BINARY):
        print(f"Binary not found: {BINARY} -- run scripts/build_arm64.py first")
        sys.exit(1)

    stage = os.path.join(REPO_ROOT, "build-arm64", "deb-stage")
    if os.path.exists(stage):
        shutil.rmtree(stage)

    lib_out = os.path.join(stage, "usr", "lib", "cardmesh")
    bin_out = os.path.join(stage, "usr", "bin")
    debian_out = os.path.join(stage, "DEBIAN")
    desktop_out = os.path.join(stage, "usr", "share", "APPLaunch", "applications")
    pixmaps_out = os.path.join(stage, "usr", "share", "pixmaps")
    os.makedirs(lib_out, exist_ok=True)
    os.makedirs(bin_out, exist_ok=True)
    os.makedirs(debian_out, exist_ok=True)
    os.makedirs(desktop_out, exist_ok=True)
    os.makedirs(pixmaps_out, exist_ok=True)

    # zig objcopy --strip-debug/--strip-all are unimplemented for ELF as of
    # zig 0.16.0, so the shipped binary keeps its debug info (larger, but
    # functionally identical).
    shutil.copy2(BINARY, os.path.join(lib_out, "cardmesh.bin"))
    os.chmod(os.path.join(lib_out, "cardmesh.bin"), 0o755)

    bundled = compute_closure(BINARY, libdir)
    for name, real_path in sorted(bundled.items()):
        # Bundle under the SONAME (e.g. "libcurl.so.4"), not the fully
        # versioned real filename (e.g. "libcurl.so.4.8.0") -- the dynamic
        # linker looks up dependencies by exact SONAME string.
        dest = os.path.join(lib_out, name)
        shutil.copy2(real_path, dest)

        actual_soname = parse_soname(dest)
        if actual_soname is not None and actual_soname != name:
            raise RuntimeError(
                f"SONAME mismatch: bundled {name} as {dest}, but its own "
                f"DT_SONAME is {actual_soname!r} -- the dynamic linker looks "
                "up dependencies by exact SONAME, so this file must be "
                "renamed or it will fail to load at runtime."
            )
    print(f"Bundled {len(bundled)} shared libraries into {lib_out} (SONAME-verified)")

    wrapper_path = os.path.join(bin_out, "cardmesh")
    with open(wrapper_path, "w") as f:
        f.write(
            "#!/bin/sh\n"
            'exec env LD_LIBRARY_PATH="/usr/lib/cardmesh:$LD_LIBRARY_PATH" '
            '/usr/lib/cardmesh/cardmesh.bin "$@"\n'
        )
    os.chmod(wrapper_path, 0o755)

    # dev.cardputer.cc's store validator requires a desktop-entry file at
    # this exact path (confirmed via its "Preliminary check report" --
    # not documented anywhere in the README, discovered by trial upload).
    # Display name is the README's Full Name ("CardMesh for MeshMonitor");
    # Package/Exec stay as the short, space-free identifier ("cardmesh") per
    # the README's Debian metadata spec (Package/Executable: cardmesh).
    desktop_entry = """[Desktop Entry]
Version=1.0
Type=Application
Name=CardMesh for MeshMonitor
Comment=Your pocket console for the mesh.
Exec=cardmesh
Icon=cardmesh
Terminal=false
Categories=Utility;Network;
"""
    with open(os.path.join(desktop_out, "cardmesh.desktop"), "w") as f:
        f.write(desktop_entry)

    icon_src = os.path.join(REPO_ROOT, "assets", "branding", "appIcon.png")
    if os.path.exists(icon_src):
        shutil.copy2(icon_src, os.path.join(pixmaps_out, "cardmesh.png"))

    installed_size_kb = 0
    for dirpath, _dirnames, filenames in os.walk(stage):
        for name in filenames:
            installed_size_kb += os.path.getsize(os.path.join(dirpath, name)) // 1024

    control = f"""Package: cardmesh
Version: {version}
Architecture: arm64
Maintainer: Max Hayim <343602+maxhayim@users.noreply.github.com>
Installed-Size: {installed_size_kb}
Depends: libc6
Section: utils
Priority: optional
Homepage: https://github.com/maxhayim/cardmeshformeshmonitor
Description: Keyboard-first pocket client for MeshMonitor
 CardMesh is a keyboard-first pocket client for MeshMonitor, built for the
 M5Stack CardputerZero. This build ships only the MVP Dashboard screen.
 .
 NOT verified against real CardputerZero hardware or the official online
 emulator -- see docs/DEVICE_BUILD.md in the source repository for the
 assumptions this build makes (framebuffer/input device paths, color depth)
 and what has and hasn't been tested.
"""
    with open(os.path.join(debian_out, "control"), "w") as f:
        f.write(control)

    deb_path = os.path.join(REPO_ROOT, "build-arm64", f"cardmesh_{version}_arm64.deb")
    subprocess.run(["dpkg-deb", "--build", "--root-owner-group", stage, deb_path], check=True)
    print(f"\nBuilt: {deb_path}")


if __name__ == "__main__":
    main()
