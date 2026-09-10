#ifndef HELP_VIEW_H
#define HELP_VIEW_H

#include "TextPageView.h"

// Every keyboard shortcut in the firmware, in one place. Compiled by reading the
// input handlers rather than the old README, which had drifted.
class HelpView : public TextPageView {
public:
    explicit HelpView(ConfigManager& config);
};

#endif // HELP_VIEW_H
