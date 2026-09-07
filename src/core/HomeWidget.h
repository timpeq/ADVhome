#ifndef HOME_WIDGET_H
#define HOME_WIDGET_H

#include "DisplayManager.h"
#include "KeyboardManager.h"

class HomeWidget {
public:
    virtual ~HomeWidget() = default;
    virtual void draw(DisplayManager& display) = 0;
    virtual bool handleInput(KeyboardManager& keyboard) = 0;
};

#endif // HOME_WIDGET_H
