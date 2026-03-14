#!/usr/bin/env python3
from __future__ import annotations

import wave
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent
SAMPLES_DIR = PROJECT_DIR / "samples"
MANIFEST_FILE = PROJECT_DIR / "samples.txt"
GENERATED_HEADER = PROJECT_DIR / "AudioSamplesGenerated.h"
GENERATED_CPP = PROJECT_DIR / "AudioSamplesGenerated.cpp"


def write_text_if_changed(path: Path, content: str) -> None:
    if path.exists():
        existing = path.read_text(encoding="ascii")
        if existing == content:
            return
    path.write_text(content, encoding="ascii")


def symbol_name(name: str) -> str:
    cleaned = "".join(ch if ch.isalnum() or ch == "_" else "_" for ch in name)
    cleaned = cleaned.strip("_") or "sample"
    if cleaned[0].isdigit():
        cleaned = "s_" + cleaned
    return cleaned


def parse_manifest(path: Path) -> dict[str, dict[str, object]]:
    if not path.exists():
        default_manifest = """# code [root=Hz]
# To jest wykaz obsługiwanych kodów. Sam generator bierze pliki z samples/*.wav.
# Jeśli plik samples/<code>.wav istnieje, zostanie użyty.
"""
        write_text_if_changed(path, default_manifest)
        return {}

    entries: dict[str, dict[str, object]] = {}
    for line_no, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) < 1:
            raise ValueError(f"{path.name}:{line_no}: expected 'code [root=Hz]'")
        code = parts[0]
        root_hz = 1.0
        for token in parts[1:]:
            if token.startswith("root="):
                root_hz = float(token.split("=", 1)[1])
            else:
                raise ValueError(f"{path.name}:{line_no}: unsupported token {token!r}")
        entries[code] = {"root_hz": root_hz}
    return entries


def load_wav_as_int8(path: Path) -> tuple[list[int], int]:
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        sample_rate = wav.getframerate()
        frame_count = wav.getnframes()
        raw = wav.readframes(frame_count)

    if channels != 1:
        raise ValueError(f"{path.name}: only mono WAV is supported")
    if sample_width not in (1, 2):
        raise ValueError(f"{path.name}: only 8-bit or 16-bit PCM WAV is supported")

    data: list[int] = []
    if sample_width == 1:
        for value in raw:
            data.append(value - 128)
    else:
        for i in range(0, len(raw), 2):
            sample = int.from_bytes(raw[i:i + 2], byteorder="little", signed=True)
            data.append(max(-128, min(127, sample >> 8)))
    peak = max((abs(value) for value in data), default=0)
    if peak > 0 and peak < 127:
        scale = 127.0 / float(peak)
        normalized: list[int] = []
        for value in data:
            scaled = int(round(value * scale))
            normalized.append(max(-128, min(127, scaled)))
        data = normalized
    return data, sample_rate


def collect_entries(metadata: dict[str, dict[str, object]]) -> list[dict[str, object]]:
    SAMPLES_DIR.mkdir(parents=True, exist_ok=True)
    entries: list[dict[str, object]] = []
    for wav_path in sorted(SAMPLES_DIR.glob("*.wav")):
        code = wav_path.stem
        meta = metadata.get(code, {})
        entries.append({
            "code": code,
            "path": wav_path,
            "root_hz": float(meta.get("root_hz", 1.0)),
        })
    return entries


def generate(entries: list[dict[str, object]]) -> None:
    header = """#pragma once

#include <stdint.h>

#include "SGF/Synth.h"

namespace AudioSamplesGenerated {

struct Entry {
  const char* code;
  const SGFAudio::AudioSample* sample;
};

extern const Entry ENTRIES[];
extern const unsigned int ENTRY_COUNT;

}
"""
    write_text_if_changed(GENERATED_HEADER, header)

    lines: list[str] = [
        '#include "AudioSamplesGenerated.h"',
        "",
        "namespace AudioSamplesGenerated {",
        "",
    ]
    entry_lines: list[str] = []

    for entry in entries:
        code = str(entry["code"])
        path = Path(entry["path"])
        root_hz = float(entry["root_hz"])
        if not path.exists():
          raise ValueError(f"missing sample file: {path}")
        pcm, sample_rate = load_wav_as_int8(path)
        ident = symbol_name(code)
        lines.append(f"static const int8_t pcm_{ident}[] = {{")
        row: list[str] = []
        for i, value in enumerate(pcm):
            row.append(f"{value}")
            if len(row) == 16 or i == len(pcm) - 1:
                lines.append("  " + ", ".join(row) + ",")
                row = []
        lines.append("};")
        lines.append(
            f"static const SGFAudio::AudioSample sample_{ident}{{"
            f" pcm_{ident}, {len(pcm)}u, {sample_rate}u, {root_hz}f, false, 0u, 0u }};")
        lines.append("")
        entry_lines.append(f'  {{"{code}", &sample_{ident}}},')

    lines.append("const Entry ENTRIES[] = {")
    lines.extend(entry_lines or ["  {}," ])
    lines.append("};")
    lines.append(f"const unsigned int ENTRY_COUNT = {len(entries)}u;")
    lines.append("")
    lines.append("}")
    write_text_if_changed(GENERATED_CPP, "\n".join(lines) + "\n")


def main() -> int:
    metadata = parse_manifest(MANIFEST_FILE)
    entries = collect_entries(metadata)
    generate(entries)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
