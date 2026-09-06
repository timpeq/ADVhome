#ifndef DIAGNOSTIC_VIEW_H
#define DIAGNOSTIC_VIEW_H

#include "View.h"
#include "WifiConnectionManager.h"
#include "HomeAssistantManager.h"

class DiagnosticView : public View {
public:
    DiagnosticView(WifiConnectionManager& wifi, HomeAssistantManager& ha);
    
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    WifiConnectionManager& _wifi;
    HomeAssistantManager& _ha;
};

#endif // DIAGNOSTIC_VIEW_H
