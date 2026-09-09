# ADVhome

ADVhome is an M5Stack Cardputer firmware for monitoring and controlling Home Assistant entities from a compact, keyboard-driven interface.

## Features

- Wi-Fi network scanning and on-device credential entry
- Home Assistant connection over the WebSocket API
- Long-lived access token setup through a browser portal
- Live entity state updates
- Favorites, entity browsing, and entity detail views
- Service controls for supported entities
- Media player information and controls
- Text chat with the Home Assistant conversation agent
- Connection diagnostics, including device IP and Home Assistant version
- Persistent Wi-Fi, Home Assistant, favorites, and UI settings in ESP32 Preferences

## Hardware

- M5Stack Cardputer
- M5Stack StampS3
- USB-C cable for building and flashing
- A 2.4 GHz Wi-Fi network reachable by the Cardputer
- A Home Assistant instance on the same network or otherwise reachable by the device

The PlatformIO target is `m5stack-stamps3` and uses the Arduino framework with an 8 MB flash partition layout.

## Development setup

### Nix

The repository includes a Nix flake that provides PlatformIO, Python with `pyserial`, and a `deploy` helper.

```sh
nix develop
pio run
```

The shell prints the available build and deployment commands when it starts.

### PlatformIO directly

Install PlatformIO and build the project with:

```sh
pio run
```

Dependencies are declared in `platformio.ini` and are installed by PlatformIO, including:

- M5Unified
- M5Cardputer
- ArduinoJson
- WebSockets

## Flashing

### Full firmware image

A normal build produces the application firmware and, through the post-build script, a merged image:

```text
.pio/build/m5stack-stamps3/firmware_merged.bin
```

Use the merged image with a tool such as M5Burner when the complete flash image is required. The merged image contains the bootloader, partitions, and application firmware at their ESP32-S3 flash offsets.

**Do not flash the merged image to a shared M5Launcher device.** It writes a
partition table built from `advhome_partitions.csv`, which describes a single
app filling the flash. Writing it to offset `0x0` replaces the launcher's
multi-slot table and makes every other firmware on the device unreachable. The
merged image is for a Cardputer dedicated to ADVhome. For a shared device, use
`deploy`, which only writes the app slot.

### M5Launcher deployment

The Nix shell provides a deployment helper for the configured M5Launcher slot:

```sh
deploy
```

This command builds the firmware, reads the partition table from the connected
device, resolves the app slot labelled `advhom`, checks that the image fits, and
writes only that slot. The flash offset is never hardcoded: on a shared
M5Launcher install the slot layout is whatever the launcher set up, and writing
to a stale offset silently overwrites a neighbouring firmware instead of
failing.

To see the connected device's partition table without building or writing:

```sh
ptable
```

Useful options:

```sh
deploy --dry-run          # resolve the slot and check the fit, write nothing
deploy --slot cardpu      # target a different app slot by label
ADVHOME_PORT=/dev/ttyACM1 deploy
```

`deploy` refuses to write when the slot is missing, when the image is larger
than the slot, or when the device has no readable partition table. If the table
itself is gone, `ptable.bin` in the repository is a known-good copy of this
device's layout; restoring it is destructive and deliberate:

```sh
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash 0x8000 ptable.bin
```

## First boot

1. Flash the firmware and reset the Cardputer.
2. ADVhome scans for nearby Wi-Fi networks.
3. Select a network with `W`/`S` or the arrow keys, then press Enter or Space.
4. Enter the Wi-Fi password and submit it with Enter or Space.
5. After connecting, the display shows the Cardputer's local IP address and a QR code.
6. Open `http://<device-ip>/` from a device on the same network.
7. Enter the Home Assistant URL and a long-lived access token, then choose **Connect**.
8. ADVhome connects to Home Assistant and opens the main interface.

The Home Assistant URL may use either HTTP or HTTPS, for example:

```text
http://homeassistant.local:8123
https://home.example.com
```

Create the long-lived access token in Home Assistant from your user profile page. The URL and token are stored in the device's local Preferences storage.

## Controls

- `W` / `S`, or Up / Down: move through lists
- `Enter`, `Space`, or Cardputer button A: select or submit
- `Backspace`, `Esc`, `` ` ``, or `~`: delete text or go back, depending on the active view
- `A` / `D`, or Left / Right: change the Entities sub-tab (domain)
- `Tab`: switch between interface sections where supported
- Typing letters: jump to the first entity whose name starts with what you typed
- `Ctrl-F`: add or remove the selected entity from Favorites

The Chat tab sends text through Home Assistant's authenticated WebSocket
`conversation/process` command and keeps the returned conversation ID for
follow-up messages. This is a text-first path and does not require audio
processing on the Cardputer. On supported Cardputer hardware, hold the top GO
button in Chat to stream a voice request to the Assist pipeline; release it to
finish recording. Voice input is experimental and currently displays the
transcript and agent response as text.

### Voice roadmap

The Cardputer hardware includes a microphone and speaker, so push-to-talk voice
interaction is feasible, but it is a separate project from text chat. The
firmware would need to capture and buffer microphone audio, negotiate an Assist
pipeline session with Home Assistant, stream audio in the format that pipeline
expects, decode the returned audio, and handle speaker timing. The existing
authenticated WebSocket is the likely control and event transport, but voice
audio should not be assumed to be identical to the JSON entity messages; the
Assist pipeline's audio framing and session lifecycle need to be verified.

Estimated effort: microphone capture and a push-to-talk UI are medium effort
(about 1-2 weeks); end-to-end Assist pipeline streaming and playback are high
effort (about 2-4 additional weeks), mainly because of audio buffering,
encoding/decoding, and limited RAM. A text Chat fallback should remain even
after voice support is added.

The Config view exposes UI options such as battery visibility, Chat tab visibility,
reconnect interval, back-button behavior, and scroll repeat timing. Chat is enabled by
default and can be hidden without disabling the rest of the Home Assistant
connection or entity controls.

## Project layout

```text
src/main.cpp                 Arduino setup and main loop
src/core/AppController.*     Application state machine
src/core/DisplayManager.*    Cardputer display rendering
src/core/KeyboardManager.*   Keyboard input handling
src/core/WifiConnectionManager.*
                             Wi-Fi scanning and connection management
src/core/SetupPortal.*        Browser-based Home Assistant setup
src/core/HomeAssistantManager.*
                             Home Assistant WebSocket and API integration
src/core/EntityManager.*      Cached Home Assistant entities
src/core/EntityList.*         Shared scrolling entity list (rows, scrollbar, type-ahead)
src/core/*View.*              Main, detail, configuration, favorites, and diagnostic views
tools/flash_slot.py           Resolves the target app slot from the device's partition table
merge_firmware.py             Post-build merged ESP32-S3 image generation
platformio.ini                PlatformIO target and dependencies
flake.nix                     Reproducible Nix development shell and deploy helper
```

## Troubleshooting

### No Wi-Fi networks appear

Confirm that the device is within range and that the network is visible to the Cardputer. Reset the device to retry the scan.

### Wi-Fi credentials are rejected

ADVhome clears the saved Wi-Fi credentials after a failed connection. Check the password and select the network again.

### The setup page does not open

Use the IP address shown on the Cardputer, and ensure the browser device is on the same network. The setup server listens on HTTP port 80.

### Home Assistant does not connect

Check that the URL includes the correct host and port, that Home Assistant is reachable from the device, and that the token is valid. An invalid token is cleared and must be entered again.

### Serial output

USB CDC is enabled at boot by the PlatformIO configuration. Use PlatformIO's monitor command while the device is connected:

```sh
pio device monitor
```

Serial logs include Wi-Fi and Home Assistant connection status.
