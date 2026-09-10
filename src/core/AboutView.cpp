#include "AboutView.h"

#ifndef ADVHOME_VERSION
#define ADVHOME_VERSION "dev"
#endif

namespace {

const TextPageLine LINES[] = {
    {"ADVhome " ADVHOME_VERSION, 0},
    {"Home Assistant client for the", 1},
    {"M5Stack Cardputer.", 1},
    {"", 1},
    {"(c) 2026 Tim Pequignot", 1},
    {"MIT License", 1},
    {"peq.me/advhome", 1},
    {"", 1},
    {"Built on the work of:", 0},
    {" M5Unified / M5GFX      MIT", 1},
    {" M5Cardputer           MIT", 1},
    {" LovyanGFX (via M5GFX) FreeBSD", 1},
    {" ArduinoJson           MIT", 1},
    {" arduinoWebSockets  LGPL-2.1", 1},
    {" ESP32 Arduino core LGPL-2.1", 1},
    {" ESP-IDF          Apache-2.0", 1},
    {" Home Assistant   Apache-2.0", 1},
    {"", 1},
    {"The LGPL-2.1 parts are linked", 2},
    {"statically. Source for this", 2},
    {"exact build is at the revision", 2},
    {"above: peq.me/advhome", 2},
    {"", 1},
    {"Full terms: LICENSE and", 2},
    {"THIRD-PARTY-NOTICES.md", 2},
    {"", 1},
    {"Not affiliated with M5Stack,", 2},
    {"Nabu Casa, or the Home", 2},
    {"Assistant project.", 2},
};

constexpr int LINE_COUNT = sizeof(LINES) / sizeof(LINES[0]);

}  // namespace

AboutView::AboutView(ConfigManager& config) : TextPageView(config, LINES, LINE_COUNT) {}
