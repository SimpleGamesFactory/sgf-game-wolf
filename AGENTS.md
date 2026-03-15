# AGENTS.md

## Local collaboration rules
- Change only what the user asks for; do not touch unrelated user edits.
- No side refactors/cleanups beyond the requested scope.
- If a change spans multiple areas of a file, briefly state what will be altered before editing.
- Prefer small, precise patches over sweeping rewrites.
- Keep the project’s style/structure; do not impose your own organization unasked.
- Stop generating fucking bloat code.
- Do not cast without a reason.
- If a cast is truly necessary, use the shortest correct idiom (`std::move`, a compact `static_cast`, or a tiny typed helper) instead of bloated conversion scaffolding.
- Do not replace one bad cast style with another mechanically.
- Keep code compact; do not expand one-line logic into multiple temporaries unless it materially improves correctness or readability.
- If a type already has sane defaults, set only the fields that differ; do not bloat config objects with redundant `0`/`nullptr`/empty/default assignments.
- If a knob is expected to be changed from build `DEFINES`, it must not be implemented as `static constexpr` or a file-local tuning constant. Use `#ifndef/#define` so the build can actually override it.
- Do not bury tunable thresholds/timings/cutoffs inside `.cpp` locals when they are likely to be adjusted during hardware tuning. Keep them overrideable from the build.
- Do not hardcode per-type gameplay differences with `if (kind == ...)` helper logic. Use data-driven archetypes/config passed into the instance.
- Keep navigation/pathfinding outside actor classes. Actors may call a navigation module, but path logic itself does not belong inside the actor implementation.
- When a feature may later be loaded from data files, keep the runtime interface data-driven now so content additions do not require class code changes.
- Follow [CODING_STANDARDS.md](/home/marcin/Arduino/Wolf/CODING_STANDARDS.md) for any C++/Arduino code changes.
- Treat `CODING_STANDARDS.md` as mandatory, not advisory.
- Keep Wolf-specific defaults and overrides in the Wolf project, not in shared `vendor/SGF` profiles or APIs.
- Shared/library code must stay generic; do not introduce `WOLF_*` behavior into SGF.
- When touching shared SGF-facing components, consult [vendor/SGF/README.md](/home/marcin/Arduino/Wolf/vendor/SGF/README.md) for the intended structure.
- For UI/text tweaks, do not alter wording unless necessary.
- When a change affects rendering/perf, verify whether the bottleneck is CPU render, SPI present, static DRAM, or runtime heap before claiming a cause.
- Ask when something is unclear instead of guessing.
- No false statements and no guessing: if unsure, inspect files and verify; double-check before claiming compliance.

## Build and tooling
- Preferred commands:
  - `./vendor/SGF/tools/sgf build board=esp32`
  - `./vendor/SGF/tools/sgf flash board=esp32`
  - `./vendor/SGF/tools/sgf upload board=esp32`
  - `./vendor/SGF/tools/sgf elfinfo -q board=esp32`
- `sgf` semantics:
  - `build` = compile only
  - `upload` = upload existing build artifacts only
  - `flash` = build + upload
  - `elfinfo` = inspect existing ELF unless `--build` is given
- Useful flags:
  - `-q` quiet
  - `-v` raw verbose output from `arduino-cli`
  - `--clean-build` forces clean rebuild
- Common recovery for broken Arduino build artifacts:
  - `rm -rf build/esp32 build-stage/esp32/wolf3d`
  - then `./vendor/SGF/tools/sgf build --clean-build -q board=esp32`

## Current Wolf defaults
- Rendering defaults live in [Game.h](/home/marcin/Arduino/Wolf/Game.h):
  - `WOLF_VIEWPORT_W=240`
  - `WOLF_VIEWPORT_H=180`
  - `WOLF_SIMPLE_FLOOR=1`
  - `WOLF_SIDE_SHADE=0`
  - `WOLF_UPSCALE=2`
  - `WOLF_UPSCALE_BUFFER_COUNT=2`
  - `WOLF_DYNAMIC_FRAMEBUFFER=1`
  - `WOLF_DYNAMIC_FRAMEBUFFER_ALIGNMENT=32`
  - `WOLF_HEAP_COLD_BUFFERS=1`
- Wolf project locally forces DMA bus in [wolf3d.ino](/home/marcin/Arduino/Wolf/wolf3d.ino), not in SGF hardware presets.
- `WOLF_UPSCALE_BUFFER_COUNT=2` is the measured sweet spot. `4` gave no gain and wastes SDRAM.
- Dynamic framebuffer is acceptable only with aligned heap allocation; unaligned heap framebuffer caused large FPS regressions.

## Memory and performance notes
- Static DRAM bottleneck on ESP32 is `dram0_0_seg`, not the full board RAM figure. `elfinfo` shows both.
- For `UPSCALE=1`, a static framebuffer nearly fills `dram0_0_seg`; the dynamic aligned framebuffer avoids that.
- Cold buffers moved to heap:
  - HUD cache
  - map data
  - door open amounts
- BMP textures are not stored in static DRAM; they live in flash blob data and are lazily decoded to heap.

## Texture pipeline
- Texture asset handling must stay outside `Game`; use:
  - [Walls.cpp](/home/marcin/Arduino/Wolf/Walls.cpp)
  - [Door.cpp](/home/marcin/Arduino/Wolf/Door.cpp)
  - [Textures.cpp](/home/marcin/Arduino/Wolf/Textures.cpp)
  - [WeaponRenderer.cpp](/home/marcin/Arduino/Wolf/WeaponRenderer.cpp)
  - [Pickups.cpp](/home/marcin/Arduino/Wolf/Pickups.cpp)
  - [Enemy.cpp](/home/marcin/Arduino/Wolf/Enemy.cpp)
- Build-time generators live in shared SGF tools:
  - [generate_textures.py](/home/marcin/Arduino/Wolf/vendor/SGF/tools/generate_textures.py)
  - [generate_samples.py](/home/marcin/Arduino/Wolf/vendor/SGF/tools/generate_samples.py)
- `sgf build` runs asset prebuild automatically when `textures/` and/or `samples/` are present.
- Palette source of truth:
  - [textures/wolf-128.gpl](/home/marcin/Arduino/Wolf/textures/wolf-128.gpl)
  - generator reads `.gpl`; it is not just an export artifact
- Generated assets:
  - [TexturesGenerated.h](/home/marcin/Arduino/Wolf/TexturesGenerated.h)
  - [TexturesGenerated.cpp](/home/marcin/Arduino/Wolf/TexturesGenerated.cpp)
  - [SamplesGenerated.h](/home/marcin/Arduino/Wolf/SamplesGenerated.h)
  - [SamplesGenerated.cpp](/home/marcin/Arduino/Wolf/SamplesGenerated.cpp)
- Current texture blob format is flash-backed and lazy-decoded by `Textures`.

## Indexed BMP rules
- For `8-bit indexed BMP`, the generator preserves source indices `1:1`.
- For `24/32-bit BMP`, the generator quantizes by RGB to the project palette.
- Sprite/weapon transparency rule:
  - index `0` is transparent
  - this is handled in `Textures`, not by color-keying in game code
- Do not reintroduce transparency checks based on `RGB565 == 0` for sprite/weapon assets.
- For non-sprite cases such as animated doors, treat “shifted out of texture” as a separate opaque/non-opaque state; do not use color `0` as transparency.

## Map symbols and assets
- Full manifest is in [textures.txt](/home/marcin/Arduino/Wolf/textures.txt).
- Important symbols:
  - walls: `# A B C E F G H I`
  - doors: `D 1 2 3`
  - keys: `r g b`
  - pickups: `a` ammo, `h` medkit
  - zombie: `@`
- Door asset names:
  - `door_plain`
  - `door_red`
  - `door_green`
  - `door_blue`
- Sprite asset names:
  - `sprite_key_red`
  - `sprite_key_green`
  - `sprite_key_blue`
  - `sprite_ammo`
  - `sprite_medkit`
  - `sprite_zombie`
- Weapon asset name:
  - `weapon_pistol`

## Rendering architecture
- Sprite rendering is centralized:
  - [Sprite.h](/home/marcin/Arduino/Wolf/Sprite.h)
  - [SpriteRenderer.h](/home/marcin/Arduino/Wolf/SpriteRenderer.h)
  - `renderSprites()`, not separate `renderKeys()` / `renderZombies()`
- `Game` should orchestrate; asset/render specifics belong in dedicated modules.
- Walls and doors use dedicated modules:
  - [Walls.h](/home/marcin/Arduino/Wolf/Walls.h)
  - [Door.h](/home/marcin/Arduino/Wolf/Door.h)

## Door behavior
- Doors are animated via `doorOpenAmounts`, not binary open/closed only.
- Colored doors keep their visual type after first unlock; unlock state is stored separately from the map tile.
- Door raycasting has an alpha-scissor-like behavior:
  - if a shifted door column is outside the texture, DDA continues and can render the wall behind the door.

## Gameplay defaults
- Player starts with `10` ammo.
- Zombie HP and damage logic are not instant-kill:
  - zombies have HP
  - shot damage has light randomness and distance falloff
