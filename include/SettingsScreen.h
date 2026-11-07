#pragma once
#include "screens.h"
#include "SettingsMenu.h"

class SettingsScreen : public Screen {
public:
    SettingsScreen(const SettingsContext& context, const SettingsActions& actions);

    void openRoot();

    void init() override;
    void update() override;
    void draw() override;
    void handleInput(Input input) override;
    bool takeNavigation(NavTarget& target) override;
    bool usesSideButtons() const override { return true; }

private:
    static constexpr int maxDepth = 6;
    static constexpr int valueSize = 32;

    enum class Mode : uint8_t { Browse, Edit, Confirm };

    struct Frame {
        const Menu* menu;
        int8_t cursor;
        uint8_t first;
    };

    struct RowKey {
        int32_t state;
        Health health;
        char value[valueSize];
    };

    SettingsContext _context;
    SettingsActions _actions;
    Frame _stack[maxDepth];
    int _depth = 0;
    Mode _mode = Mode::Browse;
    ActionId _confirmAction = ActionId::ResetSettings;
    bool _confirmFocused = false;
    bool _navRequest = false;
    bool _full = true;
    bool _buttonsDirty = true;
    bool _barCleared = false;
    uint32_t _lastAdjustMs = 0;
    uint32_t _refreshAt = 0;
    uint32_t _feedbackUntil = 0;
    int _feedbackIndex = -1;
    char _feedback[valueSize] = "";

    RowKey _rowKeys[SettingsUi::visibleRows];
    const Menu* _headerMenu = nullptr;
    int32_t _headerKey = -1;
    int32_t _barKey = -1;
    const char* _hintShown = nullptr;
    char _pageShown[8] = "";

    Frame& frame() { return _stack[_depth]; }
    const MenuItem& item(int index) { return frame().menu->items[index]; }
    int count() { return frame().menu->count; }

    void resetKeys();
    void moveCursor(int step);
    void reveal();
    void activate(const MenuItem& selected, int index);
    void back();
    void adjust(int direction);
    void showFeedback(const char* text);
    void runConfirmed();
    void enterConfirm(ActionId action);

    Health valueText(const MenuItem& entry, char* out, size_t size);
    void drawBrowse();
    void drawRows();
    void drawFooter();
    void drawConfirm();
};
