# Agent Instructions

## Development environment

Use the repository's Nix flake for development tools and dependencies whenever possible.

- Enter the environment with `nix develop`.
- Use the provided PlatformIO from that shell for builds, tests, and device operations.
- Prefer project configuration in `flake.nix` and `platformio.ini` over installing global tools or manually managing dependency versions.
- Keep dependency changes declared in the repository so another developer or agent can reproduce the environment.

## Git workflow

Use Git to track new features as development progresses.

- Check `git status` before starting work and before finishing.
- Make focused commits as coherent features or fixes are completed.
- Use clear commit messages that describe the behavior added or fixed.
- Do not commit secrets, device credentials, or unrelated generated files (e.g., `.pio/`).
- Do not rewrite or discard existing user changes unless explicitly requested.
- Include relevant documentation and tests in the same feature commit when practical.

## Validation & Deployment

Before committing a change, run the narrowest relevant checks from the Nix shell. 
To build the firmware:
```sh
nix develop --command pio run
```

To build AND flash the firmware automatically, use the provided deploy script:
```sh
nix develop --command deploy
```
The `deploy` script waits for the device, then reads the device's own partition
table and writes the firmware into the app slot labelled `advhom`. It never uses
a hardcoded flash offset, and it refuses to write when the slot is missing or
the image does not fit. `ptable` prints the connected device's layout.

**Never flash `firmware_merged.bin` to this device.** It carries a partition
table generated from `advhome_partitions.csv` (a single app filling the flash);
writing it to `0x0` destroys the M5Launcher multi-slot layout. That merged image
is only for a Cardputer dedicated to ADVhome.

## System Architecture & Constraints

**Memory Management (ESP32-S3):**
The device has ~320KB of usable RAM. Home Assistant instances can have thousands of entities. 
- *DO NOT* ingest all entities. Use `EntityManager::isSupportedDomain` to filter by domain.
- For high-volume domains like `sensor`, always filter by `device_class` (e.g., `temperature`, `humidity`) inside the HTTP chunker and WebSocket listener in `HomeAssistantManager.cpp`.
- *DO NOT* add large attributes to the base `Entity` struct, this causes massive heap fragmentation. For domains that need large rich attributes (like `climate` or `media_player`), store their states in a separate decoupled `std::map` inside `EntityManager`.

**Flash Limit:**
- Flash memory is extremely tight (~97% full). Avoid bringing in heavy dependencies or writing overly verbose template classes. Check `pio run` output sizes frequently.

**Audio / I2S Peculiarities:**
- Calling `M5.Mic.end()` or interfering with the internal microphone while `M5.Speaker` is running can cause the I2S/codec bus to hang and emit a screeching noise. Ensure that `M5.Mic.begin()` is active and avoid arbitrary `end()` calls during interactions.

**UI Framework (TFT_eSPI):**
- `AppController` manages the state machine and the `TabController`.
- Modal windows (like `EntityDetailView`) should NOT clear the screen. Instead, they should be drawn *over* the `_tabController.drawActiveView(_display)` to create a floating window effect.
- Coordinates are hardcoded for a 240x135 display. Always calculate relative to these bounds.

**Versioning:**
- Because Nix overrides `__DATE__` to a deterministic epoch (1980), we use `git_version.py` as a `pre:` script in `platformio.ini` to inject the short git hash into the `ADVHOME_VERSION` macro for version tracking.
