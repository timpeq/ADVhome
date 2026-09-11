#ifndef BATTERY_H
#define BATTERY_H

namespace Battery {

// Battery percentage for the status bar. The Cardputer reads its cell through
// a divider on a bare ADC pin, and M5.Power takes a single sample per call, so
// the raw figure swings several percent from one frame to the next. This
// samples at most once a second, averages, and only moves the number once the
// average has clearly moved.
int level();

}  // namespace Battery

#endif  // BATTERY_H
