#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include "DiagnosticView.h"
#include "HomeAssistantManager.h"
#include "ScrollRepeater.h"
#include <vector>
#include <functional>

class ConfigView : public View {
public:
    ConfigView(ConfigManager& config, DiagnosticView& diagnosticView, HomeAssistantManager& haManager, std::function<void()> onSettingsChanged = nullptr);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    ConfigManager& _config;
    DiagnosticView& _diagnosticView;
    HomeAssistantManager& _haManager;
    std::function<void()> _onSettingsChanged;
    
    struct Setting {
        String name;
        int type; // 0 = bool, 1 = int (interval), 2 = int (enum)
    };
    
    std::vector<Setting> _settings;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    
    // Temporary variables for drawing
    bool _showBattery;
    bool _hideUnavailable;
    bool _showChat;
    bool _showDiagnostics = false;
    int _reconInt;
    int _scrollDelay;
    int _scrollSpeed;
    int _tempStep;
    int _seekStep;
    int _seekStepMax;
    int _favoritesSort;
    int _brightness;
    int _dimTO;
    int _dispOffTO;
    int _softSleepTO;
    int _deepSleepTO;
    bool _escDeepSleep;
    bool _ttsEnabled;
    int _ttsVolume;
    bool _ttsDebug;
    bool _goToChat;
    String _voicePipelineName;
    ScrollRepeater _scrollRepeater;
    ScrollRepeater _valueRepeater;
    
    void refreshValues();
    void toggleCurrent(int direction = 1);
};

#endif // CONFIG_VIEW_H
