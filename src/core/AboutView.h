#ifndef ABOUT_VIEW_H
#define ABOUT_VIEW_H

#include "TextPageView.h"

// Attribution has to be readable on the device itself, not only in the
// repository, because most people will meet ADVhome as a firmware image
// installed through M5Launcher and never see the source tree.
class AboutView : public TextPageView {
public:
    explicit AboutView(ConfigManager& config);
};

#endif // ABOUT_VIEW_H
