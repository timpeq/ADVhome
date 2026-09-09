#ifndef FORMAT_H
#define FORMAT_H

#include <Arduino.h>
#include <math.h>

// Small numeric formatters built from integer math. printf's "%f" family drags
// newlib's float-capable vfprintf into the image, which is expensive on a
// board this close to its flash ceiling.
namespace Format {

// One decimal place: 21.5 -> "21.5". NAN renders as "--".
inline String oneDecimal(float value) {
    if (isnan(value)) return "--";
    int tenths = (int)lroundf(value * 10.0f);
    String out = String(tenths / 10);
    out += '.';
    out += (char)('0' + abs(tenths % 10));
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
