#!/usr/bin/env python3
from __future__ import annotations

import re
import struct
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent
TEXTURES_DIR = PROJECT_DIR / "textures"
PALETTE_FILE = TEXTURES_DIR / "wolf-128.gpl"
GENERATED_HEADER = PROJECT_DIR / "TexturesGenerated.h"
GENERATED_CPP = PROJECT_DIR / "TexturesGenerated.cpp"
TEX_SIZE = 16


def lerp(a: int, b: int, step: int, count: int) -> int:
    if count <= 1:
        return a
    return int(round(a + (b - a) * (step / float(count - 1))))


def ramp(start: tuple[int, int, int], end: tuple[int, int, int], count: int = 16) -> list[tuple[int, int, int]]:
    return [
        (
            lerp(start[0], end[0], i, count),
            lerp(start[1], end[1], i, count),
            lerp(start[2], end[2], i, count),
        )
        for i in range(count)
    ]


def build_palette() -> list[tuple[int, int, int]]:
    ramps = [
        ((0, 0, 0), (255, 255, 255)),
        ((28, 16, 8), (236, 208, 164)),
        ((36, 0, 0), (255, 164, 148)),
        ((52, 16, 0), (255, 212, 72)),
        ((54, 42, 0), (255, 244, 172)),
        ((0, 26, 8), (176, 255, 196)),
        ((0, 24, 28), (168, 252, 255)),
        ((12, 8, 36), (248, 156, 255)),
    ]
    palette: list[tuple[int, int, int]] = []
    for start, end in ramps:
        palette.extend(ramp(start, end))
    assert len(palette) == 128
    return palette


def write_palette_gpl(palette: list[tuple[int, int, int]]) -> None:
    TEXTURES_DIR.mkdir(parents=True, exist_ok=True)
    lines = [
        "GIMP Palette",
        "Name: Wolf 128",
        "Columns: 8",
        "#",
    ]
    for idx, (r, g, b) in enumerate(palette):
        lines.append(f"{r:3d} {g:3d} {b:3d} C{idx:03d}")
    PALETTE_FILE.write_text("\n".join(lines) + "\n", encoding="ascii")


def read_palette_gpl(path: Path) -> list[tuple[int, int, int]]:
    if not path.exists():
        palette = build_palette()
        write_palette_gpl(palette)
        return palette

    palette: list[tuple[int, int, int]] = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("GIMP Palette"):
            continue
        if line.startswith("Name:"):
            continue
        if line.startswith("Columns:"):
            continue

        parts = line.split()
        if len(parts) < 3:
            continue

        try:
            r = int(parts[0])
            g = int(parts[1])
            b = int(parts[2])
        except ValueError as exc:
            raise ValueError(f"invalid palette entry in {path.name}: {raw_line!r}") from exc

        if not (0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255):
            raise ValueError(f"palette value out of range in {path.name}: {raw_line!r}")
        palette.append((r, g, b))

    if len(palette) != 128:
        raise ValueError(f"{path.name} must contain exactly 128 colors, got {len(palette)}")
    return palette


def read_bmp(path: Path) -> list[tuple[int, int, int]]:
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError("not a BMP file")

    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40:
        raise ValueError("unsupported BMP header")

    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    planes = struct.unpack_from("<H", data, 26)[0]
    bitcount = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    colors_used = struct.unpack_from("<I", data, 46)[0]

    if planes != 1:
      raise ValueError("invalid BMP planes")
    if compression != 0:
      raise ValueError("compressed BMP is unsupported")

    top_down = height < 0
    height = abs(height)
    if width != TEX_SIZE or height != TEX_SIZE:
        raise ValueError(f"expected {TEX_SIZE}x{TEX_SIZE}, got {width}x{height}")

    row_size = ((bitcount * width + 31) // 32) * 4
    pixels: list[tuple[int, int, int]] = []

    if bitcount == 8:
        palette_offset = 14 + dib_size
        if colors_used == 0:
            colors_used = 256
        bmp_palette = []
        for i in range(colors_used):
            b, g, r, _ = struct.unpack_from("<BBBB", data, palette_offset + i * 4)
            bmp_palette.append((r, g, b))
        for y in range(height):
            src_y = y if top_down else (height - 1 - y)
            row = pixel_offset + src_y * row_size
            for x in range(width):
                pixels.append(bmp_palette[data[row + x]])
        return pixels

    if bitcount not in (24, 32):
        raise ValueError("only 8-bit indexed, 24-bit and 32-bit BMP are supported")

    bytes_per_pixel = bitcount // 8
    for y in range(height):
        src_y = y if top_down else (height - 1 - y)
        row = pixel_offset + src_y * row_size
        for x in range(width):
            px = row + x * bytes_per_pixel
            b = data[px + 0]
            g = data[px + 1]
            r = data[px + 2]
            pixels.append((r, g, b))
    return pixels


def nearest_palette_color(rgb: tuple[int, int, int], palette: list[tuple[int, int, int]]) -> tuple[int, int, int]:
    best = palette[0]
    best_dist = 1 << 62
    r0, g0, b0 = rgb
    for cand in palette:
        dr = r0 - cand[0]
        dg = g0 - cand[1]
        db = b0 - cand[2]
        dist = dr * dr * 3 + dg * dg * 4 + db * db * 2
        if dist < best_dist:
            best_dist = dist
            best = cand
    return best


def rgb565(rgb: tuple[int, int, int]) -> int:
    r, g, b = rgb
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def symbol_name(name: str) -> str:
    ident = re.sub(r"[^a-zA-Z0-9_]", "_", name)
    ident = re.sub(r"_+", "_", ident).strip("_")
    if not ident:
        ident = "texture"
    if ident[0].isdigit():
        ident = "t_" + ident
    return ident


def load_assets(palette: list[tuple[int, int, int]]) -> list[tuple[str, list[int]]]:
    TEXTURES_DIR.mkdir(parents=True, exist_ok=True)
    assets: list[tuple[str, list[int]]] = []
    for bmp_path in sorted(TEXTURES_DIR.glob("*.bmp")):
        name = bmp_path.stem
        pixels = read_bmp(bmp_path)
        quantized = [rgb565(nearest_palette_color(px, palette)) for px in pixels]
        assets.append((name, quantized))
    return assets


def write_generated(assets: list[tuple[str, list[int]]]) -> None:
    header = """#pragma once

#include "Textures.h"

namespace TexturesGenerated {

extern const Textures::Asset ASSETS[];
extern const int ASSET_COUNT;

}
"""
    GENERATED_HEADER.write_text(header, encoding="ascii")

    lines: list[str] = [
        '#include "TexturesGenerated.h"',
        "",
        "namespace TexturesGenerated {",
        "",
    ]

    for name, pixels in assets:
        symbol = symbol_name(name)
        lines.append(f"static const uint16_t {symbol}[Textures::TEX_SIZE * Textures::TEX_SIZE] = {{")
        for row in range(TEX_SIZE):
            chunk = pixels[row * TEX_SIZE:(row + 1) * TEX_SIZE]
            lines.append("  " + ", ".join(f"0x{value:04X}" for value in chunk) + ",")
        lines.append("};")
        lines.append("")

    if assets:
        lines.append("const Textures::Asset ASSETS[] = {")
        for name, _ in assets:
            lines.append(f'  {{"{name}", {symbol_name(name)}}},')
        lines.append("};")
        lines.append(f"const int ASSET_COUNT = {len(assets)};")
    else:
        lines.append("const Textures::Asset ASSETS[] = {")
        lines.append('  {"", 0},')
        lines.append("};")
        lines.append("const int ASSET_COUNT = 0;")

    lines.extend(["", "}"])
    GENERATED_CPP.write_text("\n".join(lines) + "\n", encoding="ascii")


def main() -> int:
    palette = read_palette_gpl(PALETTE_FILE)
    assets = load_assets(palette)
    write_generated(assets)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
