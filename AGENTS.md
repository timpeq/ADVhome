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

The partition layout is owned by M5Launcher and changes when firmwares are
installed or removed; `ptable.bin` in the repo is a dated snapshot, not the
current truth. Run `ptable` to see the live layout before assuming an offset.

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
- The `advhom` slot is 1536K. The image currently sits around 81% of it, so
  there is real room, but it is finite: the app's own code is only ~158KB and
  the rest is WiFi/TLS/lwIP/M5GFX/Arduino core, which is fixed cost. Avoid
  bringing in heavy dependencies or overly verbose template classes, and check
  `pio run` output sizes frequently. `advhome_partitions.csv` mirrors the slot
  size so an overflow fails loudly at build time.

**Audio / I2S Peculiarities:**
- Calling `M5.Mic.end()` or interfering with the internal microphone while `M5.Speaker` is running can cause the I2S/codec bus to hang and emit a screeching noise. Ensure that `M5.Mic.begin()` is active and avoid arbitrary `end()` calls during interactions.
- **TTS and a `wss` session do not fit in RAM together.** Measured 2026-09-11
  with heap logging in the TTS path: over `wss` a spoken reply starts at ~43K
  free and has ~33K left once the WAV fetch opens, against 30K of PCM buffers,
  so the socket is torn down for each reply, and that breaks barge-in. Over plain
  `ws` the device idles at ~97K free (TLS was costing ~42K) and bottoms out
  around 36K during playback with the socket up. Plain `ws` is the recommended
  setup; TTS already sends the token over plain HTTP regardless.

**UI Framework (TFT_eSPI):**
- `AppController` manages the state machine and the `TabController`.
- Modal windows (like `EntityDetailView`) should NOT clear the screen. Instead, they should be drawn *over* the `_tabController.drawActiveView(_display)` to create a floating window effect.
- Coordinates are hardcoded for a 240x135 display. Always calculate relative to these bounds.

**Keyboard (M5Cardputer driver):**
When a key "does nothing", suspect the driver before the app code. Three bugs
have come from the same two places, so check these first:

- `Keyboard_Class::KeysState` has **both** `.backspace` and `.del`. The plain
  key sets `.backspace`; Fn maps it to `KEY_DELETE`, which sets `.del`. Checking
  only one makes the key appear to need Fn.
- The arrow keys are aliases on printable keys, and the driver substitutes a
  key's **shifted** character whenever Ctrl, Shift or caps lock is held
  (`Keyboard.cpp`, PASS 3). The table in `Keyboard.h` is
  `{';', ':', KEY_UP}`, `{'.', '>', KEY_DOWN}`, `{',', '<', KEY_LEFT}`,
  `{'/', '?', KEY_RIGHT}`. So any chord built on a direction key must match the
  second column too, or it silently never fires. `KeyboardManager`'s
  `charPressed()` / `charHeldNow()` helpers take both forms; use them for any
  new alias rather than a bare `std::find`.
- The TCA8418 holds its INT line (GPIO11) low until its event FIFO is empty,
  and the reader drains exactly **one** event per `update()`, clearing
  `INT_STAT` only when nothing is left. Waiting for `Keyboard.isPressed()` to go
  false is therefore not enough before arming a level-triggered wake: the release
  event is still queued, the pin is still low, and the device wakes instantly.
  Drain until `digitalRead(11)` reads high before sleeping.
- `getNewChars()` drops `` ` `` and `~`. They are real printable keys, but the
  app treats them as back/escape everywhere, so a text field that appended them
  would insert a character and delete it in the same frame.

Read the pinned library under `.pio/libdeps/m5stack-stamps3/M5Cardputer/` to
confirm behaviour rather than reasoning from the key legends on the case; the
ADV's keyboard is a TCA8418 and does not behave like the original matrix.

**Power and sleep:**
- The ladder is Dim -> Display Off -> Soft Sleep -> the real sleep, one timeout
  each, in `AppController::checkPowerManagement()`. Soft Sleep drops Wi-Fi and
  blanks the screen while still running, so it looks exactly like a failed real
  sleep from the outside. When debugging sleep, confirm which path ran before
  concluding anything.
- `KeyboardManager::hasActivity()` is edge-based by nature. Held keys are handled
  explicitly; anything new that should count as activity has to be added, or the
  idle timers will run while the user is mid-gesture.
- **USB CDC drops when the chip sleeps and the port re-enumerates**, so a serial
  log cannot span a sleep boundary: any reader holds a stale fd and silently
  receives nothing. Diagnose sleep on the screen, not the wire.
- Deep sleep resets the chip, so nothing after `esp_deep_sleep_start()` runs.
  Report a deep-sleep wake from `begin()` via `esp_sleep_get_wakeup_cause()`.
- Wake pins must be RTC-capable. On the ESP32-S3 that is GPIO0-21, which covers
  GO (GPIO0) and the ADV keyboard interrupt (GPIO11).

**Drawing:**
- `DisplayManager` draws into one canvas and `push()` sends it to the panel. Two
  pushes in a frame means the first one is visible: an overlay drawn after the
  active view has already pushed will tear. Either draw the overlay into the same
  frame before the push, or skip the view's update entirely while the overlay is
  up, as the sleep-hold overlay does.

**Versioning:**
- Because Nix overrides `__DATE__` to a deterministic epoch (1980), we use `git_version.py` as a `pre:` script in `platformio.ini` to inject the short git hash into the `ADVHOME_VERSION` macro for version tracking.
