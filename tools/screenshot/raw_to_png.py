#!/usr/bin/env python3
"""Converts a 320x170 raw RGB888 dump (from the screenshot tool) to PNG."""
import sys

from PIL import Image

WIDTH = 320
HEIGHT = 170


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <in.raw> <out.png>")
        sys.exit(1)

    with open(sys.argv[1], "rb") as f:
        data = f.read()

    expected = WIDTH * HEIGHT * 3
    if len(data) != expected:
        print(f"Expected {expected} bytes, got {len(data)}")
        sys.exit(1)

    img = Image.frombytes("RGB", (WIDTH, HEIGHT), data)
    img.save(sys.argv[2])
    print(f"Wrote {sys.argv[2]}")


if __name__ == "__main__":
    main()
