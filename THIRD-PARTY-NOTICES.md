# Third-Party Notices

ADVhome is built on other people's work. This file lists every library that is
compiled into the firmware, who wrote it, and the terms it is offered under.

## Libraries linked into the firmware

| Library | Author | License | Notes |
| :--- | :--- | :--- | :--- |
| [M5Unified](https://github.com/m5stack/M5Unified) | M5Stack | MIT | Board abstraction, display, speaker, microphone |
| [M5GFX](https://github.com/m5stack/M5GFX) | M5Stack | MIT | Graphics, derived from LovyanGFX by lovyan03 |
| [M5Cardputer](https://github.com/m5stack/M5Cardputer) | M5Stack, Sean | MIT | Cardputer keyboard and board support |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | Benoit Blanchon | MIT | All Home Assistant JSON parsing and serialization |
| [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) | Markus Sattler | **LGPL-2.1** | Home Assistant WebSocket transport — see note below |
| [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) | Ken Shirriff, Rafi Khan, Armin Joachimsmeyer et al. | MIT | Pulled in transitively by M5Cardputer |
| [Arduino core for ESP32](https://github.com/espressif/arduino-esp32) | Espressif Systems | **LGPL-2.1** | Arduino framework, Wi-Fi, Preferences |
| [ESP-IDF](https://github.com/espressif/esp-idf) | Espressif Systems | Apache-2.0 | lwIP, mbedTLS, FreeRTOS and drivers beneath the core |

M5Cardputer ships without a LICENSE file in its repository. It is treated here as
MIT, consistent with every other M5Stack Arduino library, but that is an inference
rather than a stated grant.

## LGPL note — important if you redistribute binaries

`arduinoWebSockets` and the Arduino ESP32 core are LGPL-2.1, and they are
**statically linked** into the ADVhome firmware image. LGPL-2.1 section 6 requires
that anyone receiving the binary be able to relink it against a modified version of
those libraries.

Distributing ADVhome as complete, buildable source satisfies this: the repository
pins every dependency in `platformio.ini`, and `pio run` reproduces the image.

If you redistribute a **prebuilt `.bin` on its own** — an M5Burner entry, a release
asset, a firmware bundle — ship it alongside a link to the corresponding source at
that exact revision. The `ADVHOME_VERSION` git hash compiled into the Diagnostic
view exists partly so that link can be exact.

## Services and protocols

ADVhome talks to [Home Assistant](https://www.home-assistant.io/) (Apache-2.0)
over its WebSocket and REST APIs, including the `assist_pipeline` and
`conversation` interfaces. ADVhome is an independent client and is not affiliated
with or endorsed by the Home Assistant project or Nabu Casa.

Deployment to a shared device targets a slot managed by
[M5Launcher](https://github.com/bmorcelli/M5Stick-Launcher) by Bruno Morcelli.
ADVhome does not include or link any M5Launcher code; it only reads the
partition table M5Launcher wrote and writes into the slot M5Launcher allocated.

"M5Stack", "Cardputer" and "StampS3" are trademarks of M5Stack Technology Co., Ltd.
ADVhome is a third-party firmware and is not an M5Stack product.

## Development tooling

The firmware was developed with AI assistance, primarily
[Google Antigravity](https://antigravity.google/) and
[Claude Code](https://claude.com/claude-code). Tooling that generates code is not a
copyright holder and is not listed above as a dependency; it is noted here because
it is a material fact about how this project was built.
