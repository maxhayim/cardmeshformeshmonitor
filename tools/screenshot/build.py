#!/usr/bin/env python3
"""Builds the screenshot tool natively (host macOS/Linux, no cross-compiler
needed) -- see docs/DEVICE_BUILD.md for why this tool exists.

Usage: python3 tools/screenshot/build.py
"""
import glob
import os
import subprocess

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOL_DIR = os.path.join(REPO_ROOT, "tools", "screenshot")
OUT_DIR = os.path.join(TOOL_DIR, "build")
OBJ_DIR = os.path.join(OUT_DIR, "obj")

INCLUDES = [
    TOOL_DIR,  # tools/screenshot/lv_conf.h must win over src/ui/lv_conf.h
    os.path.join(REPO_ROOT, "src"),
    os.path.join(REPO_ROOT, "src", "ui"),
    os.path.join(REPO_ROOT, "third_party"),
    os.path.join(REPO_ROOT, "third_party", "lvgl"),
]


def compile_unit(src, is_cpp):
    rel = os.path.relpath(src, REPO_ROOT)
    obj = os.path.join(OBJ_DIR, rel.replace(os.sep, "_") + ".o")
    os.makedirs(os.path.dirname(obj), exist_ok=True)

    cmd = ["c++" if is_cpp else "cc", "-c", src, "-o", obj, "-O1"]
    for inc in INCLUDES:
        cmd += ["-I", inc]
    cmd += ["-std=c++17"] if is_cpp else ["-std=gnu11", "-Wno-implicit-function-declaration"]

    print(("CXX " if is_cpp else "CC  ") + rel)
    subprocess.run(cmd, check=True)
    return obj


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    cpp_sources = [
        os.path.join(TOOL_DIR, "main.cpp"),
        os.path.join(REPO_ROOT, "src", "ui", "DashboardScreen.cpp"),
        os.path.join(REPO_ROOT, "src", "ui", "MapScreen.cpp"),
        os.path.join(REPO_ROOT, "src", "ui", "EmojiFont.cpp"),
        os.path.join(REPO_ROOT, "src", "api", "HttpClient.cpp"),
        os.path.join(REPO_ROOT, "src", "map", "TileCache.cpp"),
        os.path.join(REPO_ROOT, "src", "map", "TileFetcher.cpp"),
    ]
    lvgl_sources = sorted(
        glob.glob(os.path.join(REPO_ROOT, "third_party", "lvgl", "src", "**", "*.c"), recursive=True)
    )

    objects = [compile_unit(s, is_cpp=True) for s in cpp_sources]
    objects += [compile_unit(s, is_cpp=False) for s in lvgl_sources]

    binary = os.path.join(OUT_DIR, "screenshot")
    subprocess.run(["c++", "-O1", "-o", binary, *objects, "-lcurl"], check=True)
    print(f"\nBuilt: {binary}")


if __name__ == "__main__":
    main()
