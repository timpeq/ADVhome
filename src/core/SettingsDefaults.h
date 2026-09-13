#ifndef SETTINGS_DEFAULTS_H
#define SETTINGS_DEFAULTS_H

// Every setting's first-boot value, and the range Menu > Settings allows.
// Edit a row and rebuild. A value saved on the device always wins, so a new
// default only shows where that setting was never changed, or once the device's
// saved settings are cleared.

// A number the Settings screen steps through with Left/Right or +/-.
struct IntSetting {
    int def;
    int min;
    int max;
    int step;

    int clamp(int v) const { return v < min ? min : (v > max ? max : v); }
    int adjust(int v, int direction) const { return clamp(v + step * direction); }
};

namespace Defaults {

// ---- Display and power -----------------------------------------------------
// The timeouts are seconds without input before each step of the idle ladder:
// dim, screen off, soft sleep, then sleep at the depth Sleep Depth picks.
//                                            default    min    max   step
constexpr IntSetting Brightness        = {      200,     25,   255,    25 };  // backlight, out of 255
constexpr IntSetting DimTimeout        = {       30,     10,   120,    10 };  // s
constexpr IntSetting DisplayOffTimeout = {       60,     30,   300,    30 };  // s
constexpr IntSetting SoftSleepTimeout  = {      120,     60,   600,    30 };  // s
constexpr IntSetting SleepTimeout      = {      600,    300,  7200,   300 };  // s

constexpr int  SleepDepth   = 1;       // 0 = OFF (stop at Soft Sleep), 1 = LIGHT (resumes in place),
                                       // 2 = DEEP (reboots on wake, then reconnects)
constexpr bool WakeOnGoOnly = false;   // true: only GO wakes the device; false: any key
constexpr bool EscForSleep  = true;    // holding ESC sleeps on demand (light sleep if Sleep Depth is OFF)

// Not a setting: the backlight level Dim drops to, out of 255.
constexpr int  DimmedBrightness = 10;

// ---- Voice and Chat --------------------------------------------------------
//                                            default    min    max   step
constexpr IntSetting TtsVolume         = {       80,      0,   100,    10 };  // %

constexpr bool TtsPlayback    = true;   // speak Assist replies aloud
constexpr bool TtsDebug       = false;  // also show TTS diagnostics in the Chat log
constexpr bool ShowChatTab    = true;
constexpr bool GoButtonToChat = true;   // GO jumps to Chat; false: GO acts as Enter
// Voice Pipeline has no default here: until one is picked, Home Assistant's
// preferred pipeline is used.

// ---- Home and lists --------------------------------------------------------
constexpr bool ShowBatteryInTab       = true;
constexpr bool HideUnavailable        = false;  // hide unavailable favorites on Home
constexpr bool SpaceTogglesInList     = true;   // SPACE toggles the highlighted row
constexpr bool PlusMinusAdjustsInList = true;   // +/- adjusts the highlighted row
constexpr int  FavoritesSort          = 0;      // 0 = ORDER (as arranged), 1 = NAME

// ---- Controls --------------------------------------------------------------
//                                            default    min    max   step
constexpr IntSetting ScrollStartDelay  = {      500,    200,  1000,   100 };  // ms a key is held before it repeats
constexpr IntSetting ScrollRepeat      = {      100,     40,   200,    20 };  // ms between repeats

constexpr int  TempStep = 0;  // thermostat step: 0 = AUTO (the entity's own), 1 = 0.5 deg, 2 = 1 deg

// Media seek. Seek Step (Min) and (Max) are picked from these lists.
constexpr int  SeekStepMin = 5;   // s
constexpr int  SeekStepMax = 30;  // s
constexpr int  SeekStepMinChoices[] = { 5, 10, 15, 30 };
constexpr int  SeekStepMaxChoices[] = { 5, 10, 15, 30, 60 };

// ---- Connection ------------------------------------------------------------
//                                            default    min    max   step
constexpr IntSetting ReconnectInterval = {     5000,   1000, 30000,  1000 };  // ms between Home Assistant reconnect attempts

}  // namespace Defaults

#endif  // SETTINGS_DEFAULTS_H
