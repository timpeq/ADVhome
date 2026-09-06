#ifndef VIEW_H
#define VIEW_H

#include "DisplayManager.h"
#include "KeyboardManager.h"

class View {
public:
    virtual ~View() = default;
    
    virtual void onEnter() {}
    virtual void onExit() {}
    
    virtual void draw(DisplayManager& display) = 0;
    virtual bool handleInput(KeyboardManager& keyboard) = 0;
};

#endif // VIEW_H
