#ifndef ABOUT_VIEW_H
#define ABOUT_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"

// Static "About & License" page. Attribution has to be readable on the device
// itself, not only in the repository, because most people will meet ADVhome as a
// firmware image installed through M5Launcher and never see the source tree.
class AboutView : public View {
public:
    explicit AboutView(ConfigManager& config);

    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    int _scrollOffset = 0;
    ScrollRepeater _scrollRepeater;
};

#endif // ABOUT_VIEW_H
