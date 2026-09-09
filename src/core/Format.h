#ifndef FORMAT_H
#define FORMAT_H

#include <Arduino.h>
#include <math.h>

// Small numeric formatters built from integer math. printf's "%f" family drags
// newlib's float-capable vfprintf into the image, which is expensive on a
// board this close to its flash ceiling.
namespace Format {

// Temperature label: 21.5 -> "21.5", 22.0 -> "22". Home Assistant only shows a
// decimal when there is one, so a whole-degree thermostat reads "72", not
// "72.0". NAN renders as "--".
inline String temperature(float value) {
    if (isnan(value)) return "--";
    int tenths = (int)lroundf(value * 10.0f);
    bool negative = tenths < 0;
    if (negative) tenths = -tenths;
    String out = negative ? "-" : "";
    out += String(tenths / 10);
    if (tenths % 10 != 0) {
        out += '.';
        out += (char)('0' + tenths % 10);
    }
    return out;
}

// Nearest whole number: 47.2 -> "47". NAN renders as "--".
inline String rounded(float value) {
    if (isnan(value)) return "--";
    return String((int)lroundf(value));
}

// "1:05" / "12:04" elapsed-time label from a whole number of seconds.
inline String clock(int seconds) {
    if (seconds < 0) seconds = 0;
    String out = String(seconds / 60);
    out += ':';
    int secs = seconds % 60;
    if (secs < 10) out += '0';
    out += String(secs);
    return out;
}

} // namespace Format

#endif // FORMAT_H
