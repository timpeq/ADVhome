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

- An M5Stack Cardputer, either the ADV or the original
- USB-C cable for building and flashing
- A 2.4 GHz Wi-Fi network reachable by the Cardputer
- A Home Assistant instance on the same network or otherwise reachable by the device

The PlatformIO target is `m5stack-stamps3`, the StampS3 module inside the
Cardputer, and uses the Arduino framework with an 8 MB flash partition layout.

### Cardputer ADV and original Cardputer

The name is a name, not a hardware requirement. ADVhome was written on and is
tested on a **Cardputer ADV**, but almost nothing in it is specific to that
board, and one binary is meant to run on both.

There is no separate build. `M5Unified` and `M5Cardputer` detect the board at
run time, so the same `firmware.bin` picks the right display, keyboard reader,
speaker, microphone and battery ADC on either machine. The two keyboards differ
completely in hardware — the ADV uses a TCA8418 controller, the original a
scanned matrix — but both feed the same key map, so every shortcut and Fn
combination described below is identical on the two.

Only the sleep code asks which board it is on, in `AppController.cpp`, and only
to arm the right wake source.

**One feature is genuinely missing on the original: motion as activity.** The
ADV has an IMU; the original Cardputer has none. On the ADV, picking the device
up counts as activity and so holds off, or reverses, the `Dim`, `Display Off`
and `Soft Sleep` steps of the power ladder. On the original there is nothing to
poll — the code checks `M5.Imu.isEnabled()` first, so it degrades to keypresses
only rather than misbehaving — and the ladder advances purely on its timers.
Note that this is not wake-on-pick-up even on the ADV: no interrupt line is
configured, so once the device is really asleep the CPU is not running to notice
movement, and only a key or GO brings it back.

What that means in practice, honestly stated: on the original Cardputer this is
**untested rather than unsupported**. Nobody working on it has one. The parts
that are pure software — Wi-Fi, the Home Assistant connection, entities,
favorites, chat, the setup portal, the whole UI — carry no board-specific code
at all and should behave the same. Three things are worth checking if you are
the first to run it on an original, and reporting back:

- **Keyboard wake from `DEEP` sleep.** The ADV wakes on one interrupt line. The
  original has to latch its matrix rows low through deep sleep so a keypress can
  still pull a column down; that path was written from the ESP32-S3 reference
  and never run. `Wake On GO Only` avoids it entirely.
- **Speaker volume.** The default was tuned against the ADV's ES8311 codec,
  which squares its gain. The original drives its amplifier directly and may be
  louder or quieter at the same setting.
- **Microphone gain** for voice input, for the same reason.

Bug reports from an original Cardputer are welcome and are the only way these
stop being caveats.

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
than the slot, or when the device has no readable partition table.

M5Launcher owns the partition table on a shared device and rewrites it when
firmwares are installed or removed, so slots move and the set of slots changes.
That is why the target is resolved by label at flash time. If the `advhom` slot
is missing entirely, reinstall ADVhome through M5Launcher so it allocates one;
`deploy` will then find it by name.

`ptable.bin` in the repository is a **historical snapshot** taken at one point in
time, kept for reference. It is not authoritative and will not match a device
whose firmware set has changed since. Do not write it back to `0x8000` to
"repair" a layout: it would point the table at firmwares that are no longer at
those offsets. Use `ptable` to see what is actually on the device.

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

**Use the plain `http://…:8123` form if you want voice replies.** An HTTPS URL
works for everything else, but its TLS session costs about 42 KB of RAM, and a
spoken reply needs about 30 KB of audio buffers on top of the download itself.
Over HTTPS the firmware has to close the connection for every reply and reopen it
afterwards, so a reply can't be interrupted with a new question, and state
changes made while it is talking are missed until they change again. Over HTTP
the connection stays up, and holding GO cuts a reply off and starts listening
straight away. Spoken replies are fetched over plain HTTP on port 8123 either way,
token included, so HTTPS protects less than it appears to.

Create the long-lived access token in Home Assistant from your user profile page. The URL and token are stored in the device's local Preferences storage.

## Controls

The full list lives on the device under **Menu > Help & Shortcuts**. In summary:

- Up / Down, or `;` and `.`: move through lists
- Left / Right, or `,` and `/`: change the Entities sub-tab (domain)
- `Enter`: open the selected entity, or run the action in a detail window
- `Space`: toggle the highlighted entity without opening it. A playing or paused
  media player pauses or resumes instead of powering off
- `+` / `-`: adjust brightness or volume from the list
- In a detail window, Up / Down or `+` / `-` set a light's brightness or a media
  player's volume; held, they keep sending as the value moves
- Typing letters: jump to the first entity whose name starts with what you typed
- `Ctrl-F`: add or remove the selected entity from Favorites
- `Ctrl` + Up / Down: carry the highlighted favorite up or down the list
- `Tab`: next tab. Inside a detail window it closes the window instead.
- `Backspace`, `Esc`, `` ` ``, or `~`: delete text, close a detail window, or
  return from a Menu page to the menu list

`W`/`S` and `A`/`D` do **not** navigate entity lists. Earlier versions of this
document said they did. They are ordinary letters and feed the type-ahead search
like any other; only the initial Wi-Fi network picker, which has no type-ahead,
treats `W` and `S` as up and down.

Favorites are shown in the order you arrange them when **Favorites Sort** is set
to `ORDER`. To change that order, open the **Entities** tab, switch to the
**Favorites** sub-tab, and hold `Ctrl` while pressing Up or Down: the highlighted
row is carried rather than passed over, and the highlight turns cyan while `Ctrl`
is held. Reordering is deliberately unavailable from the Home screen, which is
for using favorites rather than arranging them, and is inert while the sort is
set to `NAME` because the list would immediately re-sort. The new order is held
in memory and written to storage 1.5 seconds after the last move, so carrying a
row several places costs one write rather than one per step.

The `Space` toggle and the `+`/`-` adjustment act on a highlighted row without
opening it, which is quick but easy to trigger by accident. Both can be turned
off under **Menu > Settings** ("SPACE Toggles in List" and "+/- Adjusts in
List").

The Chat tab sends text through Home Assistant's authenticated WebSocket
`conversation/process` command and keeps the returned conversation ID for
follow-up messages. This is a text-first path and does not require audio
processing on the Cardputer. On supported Cardputer hardware, hold the top GO
button in Chat to stream a voice request to the Assist pipeline; release it to
finish recording. Voice input is experimental and currently displays the
transcript and agent response as text.

### Voice roadmap

**This shipped on 2026-09-07.** Push-to-talk capture and spoken replies both
work; the estimate below is left in place because it was wrong by a wide margin
and that is worth remembering.

One detail from building it is worth recording: the Assist pipeline's own TTS
stage returns MP3, which is not practical to decode in the RAM and flash left on
this device. ADVhome therefore stops the pipeline at `intent` and calls
`/api/tts_get_url` itself with `preferred_format=wav`, which `M5.Speaker` can
play directly. Holding GO during a reply interrupts it and starts a new request,
as long as the Home Assistant URL is plain HTTP; see First boot for why.

The original estimate, from before any of it was attempted:

> The Cardputer hardware includes a microphone and speaker, so push-to-talk voice
> interaction is feasible, but it is a separate project from text chat. The
> firmware would need to capture and buffer microphone audio, negotiate an Assist
> pipeline session with Home Assistant, stream audio in the format that pipeline
> expects, decode the returned audio, and handle speaker timing.
>
> Estimated effort: microphone capture and a push-to-talk UI are medium effort
> (about 1-2 weeks); end-to-end Assist pipeline streaming and playback are high
> effort (about 2-4 additional weeks), mainly because of audio buffering,
> encoding/decoding, and limited RAM.

A text Chat fallback remains available and can be used without voice.

## Menu

The rightmost tab is the Menu, drawn as a hamburger icon. It shows the app name
and the build's git revision, and opens three pages:

- **Settings** — the former Config list: brightness and TTS volume, the Wi-Fi and
  Home Assistant pages, battery visibility, Chat tab visibility, reconnect
  interval, scroll and seek repeat timing, the optional list shortcuts, the voice
  pipeline, power timeouts and sleep depth, and the Diagnostics page. Chat is
  enabled by default and can be hidden without disabling the rest of the Home
  Assistant connection or entity controls.
- **Help & Shortcuts** — every keyboard shortcut in the firmware.
- **About & License** — version, copyright, license, and the attribution for
  every library compiled into the firmware.

Most rows change a value in place with Left/Right or `+`/`-`. Rows marked `>`
open a full-screen page instead, on Enter, and Backspace returns to the list:

- **Wi-Fi** — the current SSID, IP address, and signal strength, with an action
  that forgets the saved network.
- **Home Assistant** — the configured URL and the server's version, with an
  action that clears the saved URL and token.
- **Diagnostics** — heap, RSSI, device IP, and the Home Assistant version.

Both destructive actions require pressing Enter twice; the confirmation disarms
itself after a few seconds. Because credentials are read only at startup, either
action clears the relevant keys and restarts the device, which then re-enters
Wi-Fi selection or the browser setup portal as appropriate.

## Power management

Four timeouts in **Menu > Settings** form a ladder, each handing off to the next
when the device stays idle:

| Setting | What happens |
| :--- | :--- |
| `Dim T/O` | The backlight dims. |
| `Disp Off T/O` | The panel is put to sleep and the Wi-Fi radio drops to `WIFI_PS_MAX_MODEM`. The connection stays up; state pushes just arrive less promptly. |
| `Soft Sleep T/O` | Wi-Fi is switched off, the audio codec released and the CPU dropped to 80 MHz. The firmware is still running, so a keypress resumes it immediately — as does moving the device, on a Cardputer ADV, which has an IMU. |
| `Deep Sleep T/O` | The device actually sleeps, in the manner chosen by `Deep Sleep`. |

`Deep Sleep` selects the depth of that last step:

- `OFF` — never sleeps automatically; the device stops at Soft Sleep.
- `LIGHT` — ESP32 light sleep. RAM is retained and execution resumes in place,
  so the Home Assistant connection and entity cache survive.
- `DEEP` — true deep sleep. Draws the least, but the chip resets, so waking
  costs a full boot, Wi-Fi association and Home Assistant reconnect.

`Wake On GO Only` restricts waking to the GO button instead of any key. GO always
wakes the device regardless.

Holding `ESC` sleeps immediately when `ESC for Sleep` is enabled, at whatever
depth `Deep Sleep` is set to; when that is `OFF` the held key still performs a
light sleep, so the gesture is never a no-op. A progress bar appears after about
300 ms and fills over one second, then reads "Release to sleep", because a held
key produces no other feedback and the device cannot sleep until every key is
up.

Both depths have been exercised on a Cardputer ADV, including `DEEP`, which uses
`ext1` rather than light sleep's GPIO wake and reboots on wake. On the original
Cardputer, waking from `DEEP` with the keyboard uses a different mechanism that
has never been run on hardware; see the compatibility notes above. `Wake On GO
Only` sidesteps it.

None of these levels have been measured with a meter; they are reasoned from the
ESP32-S3 datasheet and the M5Unified driver source. See `TODO.md`.

## Recovering a stranded device

If the saved network has gone away or the Home Assistant URL is wrong, the device
would otherwise retry forever with no way into its own settings. The connecting
and reconnecting screens accept:

- `ESC` — forget the Wi-Fi network and restart into the scan list
- `H` — clear the Home Assistant URL and token and restart into the setup portal

On the initial Wi-Fi connecting screen, `ESC` returns to the network list
directly without a restart.

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
src/core/MenuView.*           Menu tab; hosts the pages below as sub-views
src/core/TextPageView.*       Shared scrolling page of static text
src/core/AboutView.*          On-device about, license, and attribution page
src/core/HelpView.*           On-device keyboard shortcut reference
src/core/NetworkView.*        Connection summary with a confirmed reset action
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

## License

ADVhome is released under the [MIT License](LICENSE).

The firmware statically links `arduinoWebSockets` and the Arduino ESP32 core, both
LGPL-2.1. If you redistribute a prebuilt binary on its own, pair it with a link to
the source at that revision. See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## Credits

ADVhome is a thin layer on top of work done by other people:

- **M5Stack** for [M5Unified](https://github.com/m5stack/M5Unified),
  [M5GFX](https://github.com/m5stack/M5GFX) and
  [M5Cardputer](https://github.com/m5stack/M5Cardputer), and for building hardware
  worth writing firmware for
- **lovyan03**, whose [LovyanGFX](https://github.com/lovyan03/LovyanGFX) M5GFX
  derives from, and which is the reason anything renders at all
- **Benoit Blanchon** for [ArduinoJson](https://arduinojson.org/), which does the
  unglamorous work in every single message this firmware handles
- **Markus Sattler** for
  [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- **Espressif** for the ESP32 Arduino core and ESP-IDF
- **The Home Assistant project** and its contributors, for an API good enough that
  a $30 handheld can be a real client, and for the Assist pipeline that makes the
  voice features possible
- **Bruno Morcelli** for [M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher),
  which is how most people will actually install this

Development was AI-assisted, primarily with Google Antigravity and Claude Code.
See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for the full picture.
