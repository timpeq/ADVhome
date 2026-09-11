#include "Battery.h"
#include <M5Unified.h>
#include <math.h>

namespace {

constexpr uint32_t SAMPLE_MS = 1000;
// No sample for this long means the screen was off or the device slept, and
// the cell has moved on; start over rather than easing in from a stale value.
constexpr uint32_t RESEED_MS = 60000;
constexpr int READS_PER_SAMPLE = 8;
constexpr float SMOOTHING = 0.1f;  // ~10 s to settle at one sample a second
constexpr float DEADBAND = 1.5f;   // % the average must move to change the number

float average = 0.0f;
int shown = -1;
uint32_t lastSample = 0;

// Mean of several back-to-back reads, or -1 if none succeeded.
float sampleLevel() {
    int sum = 0;
    int count = 0;
    for (int i = 0; i < READS_PER_SAMPLE; i++) {
        int level = M5.Power.getBatteryLevel();
        if (level >= 0) {
            sum += level;
            count++;
        }
    }
    return count ? (float)sum / count : -1.0f;
}

}  // namespace

int Battery::level() {
    uint32_t now = millis();
    if (shown >= 0 && now - lastSample < SAMPLE_MS) return shown;

    float sample = sampleLevel();
    if (sample < 0.0f) return shown < 0 ? 0 : shown;

    bool reseed = shown < 0 || now - lastSample > RESEED_MS;
    lastSample = now;
    if (reseed) {
        average = sample;
        shown = lroundf(sample);
    } else {
        average += SMOOTHING * (sample - average);
        if (fabsf(average - shown) >= DEADBAND) shown = lroundf(average);
    }
    return shown;
}
