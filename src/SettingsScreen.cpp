#include "SettingsScreen.h"
#include <TFT_eSPI.h>
#include <string.h>
#include "Theme.h"

extern TFT_eSPI tft;

namespace {
constexpr uint32_t fastSpinMs = 80;
constexpr int fastStep = 5;
constexpr uint32_t feedbackMs = 2000;
constexpr uint32_t refreshMs = 250;
constexpr int barClearX = 309;
}

SettingsScreen::SettingsScreen(const SettingsContext& context, const SettingsActions& actions)
    : _context(context), _actions(actions) {
    openRoot();
}

void SettingsScreen::openRoot() {
    _depth = 0;
    _stack[0] = {&SettingsMenu::root(), 0, 0};
    _mode = Mode::Browse;
    _feedbackIndex = -1;
    _full = true;
}

void SettingsScreen::resetKeys() {
    for (RowKey& key : _rowKeys) {
        key.state = -1;
        key.value[0] = '\0';
    }
    _headerMenu = nullptr;
    _headerKey = -1;
    _barKey = -1;
    _hintShown = nullptr;
    _pageShown[0] = '\0';
    _barCleared = false;
    _refreshAt = 0;
}

void SettingsScreen::init() {
    if (_mode == Mode::Confirm) _mode = Mode::Browse;
    _full = true;
}

void SettingsScreen::update() {}

bool SettingsScreen::takeNavigation(NavTarget& target) {
    if (!_navRequest) return false;
    _navRequest = false;
    target = {"Main", 0};
    return true;
}

void SettingsScreen::reveal() {
    Frame& f = frame();
    if (f.cursor < 0) {
        f.first = 0;
        return;
    }
    if (f.cursor < f.first) f.first = f.cursor;
    if (f.cursor >= f.first + SettingsUi::visibleRows) f.first = f.cursor - SettingsUi::visibleRows + 1;
}

void SettingsScreen::moveCursor(int step) {
    Frame& f = frame();
    int next = f.cursor + step;
    if (next < -1) next = -1;
    if (next > count() - 1) next = count() - 1;
    f.cursor = next;
    reveal();
}

void SettingsScreen::back() {
    if (_depth > 0) {
        _depth--;
        _feedbackIndex = -1;
        resetKeys();
    } else {
        _navRequest = true;
    }
}

void SettingsScreen::showFeedback(const char* text) {
    strlcpy(_feedback, text, sizeof(_feedback));
    _feedbackIndex = frame().cursor;
    _feedbackUntil = millis() + feedbackMs;
}

void SettingsScreen::enterConfirm(ActionId action) {
    _mode = Mode::Confirm;
    _confirmAction = action;
    _confirmFocused = false;
    _full = true;
    _buttonsDirty = true;
}

void SettingsScreen::runConfirmed() {
    if (_confirmAction == ActionId::FactoryReset) {
        _context.config.factoryReset();
        if (_actions.reboot) _actions.reboot();
        return;
    }
    _context.config.resetSettings();
    showFeedback("Done");
}

void SettingsScreen::activate(const MenuItem& selected, int index) {
    switch (selected.kind) {
        case ItemKind::Setting: {
            const SettingDef& def = Settings::def(selected.setting);
            if (def.kind == SettingKind::Toggle) {
                _context.config.setValue(selected.setting, !_context.config.value(selected.setting));
            } else {
                _mode = Mode::Edit;
                _lastAdjustMs = 0;
            }
            break;
        }
        case ItemKind::Submenu:
            if (_depth + 1 < maxDepth) {
                _depth++;
                _stack[_depth] = {selected.submenu, 0, 0};
                _feedbackIndex = -1;
                resetKeys();
            }
            break;
        case ItemKind::Action:
            switch (selected.action) {
                case ActionId::Reboot:
                    if (_actions.reboot) _actions.reboot();
                    break;
                case ActionId::ResetSettings:
                case ActionId::FactoryReset:
                    enterConfirm(selected.action);
                    break;
                case ActionId::WifiReconnect:
                    if (_actions.wifiReconnect) _actions.wifiReconnect();
                    showFeedback("Reconnecting");
                    break;
                case ActionId::SensorReinit:
                    if (_actions.sensorReinit) _actions.sensorReinit();
                    showFeedback("Reading");
                    break;
                case ActionId::PhoneSetup: showFeedback("Not built yet"); break;
                case ActionId::BuzzerTest: showFeedback("No buzzer yet"); break;
                case ActionId::CalendarTest:
                    if (_actions.calendarRefresh) _actions.calendarRefresh();
                    showFeedback("Fetching");
                    break;
                case ActionId::WeatherTest:
                    if (_actions.weatherRefresh) _actions.weatherRefresh();
                    showFeedback("Fetching");
                    break;
                case ActionId::SpotifyTest:
                    if (_actions.spotifyRefresh) _actions.spotifyRefresh();
                    showFeedback("Fetching");
                    break;
                case ActionId::Relink: showFeedback("Run PC helper"); break;
                case ActionId::RunChecks: showFeedback("Coming soon"); break;
            }
            break;
        case ItemKind::Info:
            break;
    }
}

void SettingsScreen::adjust(int direction) {
    const MenuItem& entry = item(frame().cursor);
    const SettingDef& def = Settings::def(entry.setting);
    int step = def.step;
    if (def.kind == SettingKind::Number) {
        uint32_t now = millis();
        if (now - _lastAdjustMs < fastSpinMs) step *= fastStep;
        _lastAdjustMs = now;
    }
    int next = _context.config.value(entry.setting) + direction * step;
    _context.config.setValue(entry.setting, Settings::clamp(entry.setting, next));
}

void SettingsScreen::handleInput(Input input) {
    _refreshAt = 0;

    if (_mode == Mode::Confirm) {
        switch (input) {
            case Input::FocusNext:
            case Input::FocusPrev:
                _confirmFocused = input == Input::FocusNext;
                _buttonsDirty = true;
                break;
            case Input::Select: {
                bool confirmed = _confirmFocused;
                _mode = Mode::Browse;
                _full = true;
                if (confirmed) runConfirmed();
                break;
            }
            case Input::SubPagePrev:
                _mode = Mode::Browse;
                _full = true;
                break;
            default:
                break;
        }
        return;
    }

    if (_mode == Mode::Edit) {
        switch (input) {
            case Input::FocusNext: adjust(1); break;
            case Input::FocusPrev: adjust(-1); break;
            case Input::Select: _mode = Mode::Browse; break;
            case Input::SubPagePrev:
                _mode = Mode::Browse;
                back();
                break;
            default: break;
        }
        return;
    }

    switch (input) {
        case Input::FocusNext: moveCursor(1); break;
        case Input::FocusPrev: moveCursor(-1); break;
        case Input::Select:
            if (frame().cursor < 0) {
                back();
            } else {
                activate(item(frame().cursor), frame().cursor);
            }
            break;
        case Input::SubPagePrev: back(); break;
        default: break;
    }
}

Health SettingsScreen::valueText(const MenuItem& entry, char* out, size_t size) {
    out[0] = '\0';
    if (entry.kind == ItemKind::Setting) {
        Settings::format(entry.setting, _context.config.value(entry.setting), out, size);
        return Health::None;
    }
    if (entry.info) return entry.info(_context, out, size);
    return Health::None;
}

void SettingsScreen::draw() {
    if (_mode == Mode::Confirm) {
        drawConfirm();
        return;
    }
    drawBrowse();
}

void SettingsScreen::drawBrowse() {
    if (_full) {
        tft.fillScreen(Theme::background);
        resetKeys();
        _full = false;
    }

    Frame& f = frame();
    if (_headerMenu != f.menu || _headerKey != (f.cursor < 0)) {
        _headerMenu = f.menu;
        _headerKey = f.cursor < 0;
        SettingsUi::header(f.menu->title, f.cursor < 0);
    }

    uint32_t now = millis();
    if ((int32_t)(now - _refreshAt) < 0) return;
    _refreshAt = now + refreshMs;

    drawRows();

    int bar = f.cursor < 0 ? 0 : f.cursor;
    if (count() > SettingsUi::visibleRows) {
        if (bar != _barKey) {
            _barKey = bar;
            SettingsUi::scrollBar(bar, count());
        }
    } else if (!_barCleared) {
        _barCleared = true;
        tft.fillRect(barClearX, SettingsUi::rowTop, 320 - barClearX, SettingsUi::visibleRows * SettingsUi::rowHeight,
                     Theme::background);
    }

    drawFooter();
}

void SettingsScreen::drawRows() {
    Frame& f = frame();
    uint32_t now = millis();
    if (_feedbackIndex >= 0 && (int32_t)(now - _feedbackUntil) >= 0) _feedbackIndex = -1;

    for (int slot = 0; slot < SettingsUi::visibleRows; slot++) {
        int index = f.first + slot;
        RowView view = {RowState::Empty, "", "", Health::None, false};
        char value[valueSize];
        value[0] = '\0';
        int32_t state = -2;

        if (index < count()) {
            const MenuItem& entry = item(index);
            bool focused = index == f.cursor;
            view.label = entry.label;
            view.health = valueText(entry, value, sizeof(value));
            if (entry.kind == ItemKind::Action && index == _feedbackIndex) {
                strlcpy(value, _feedback, sizeof(value));
                view.health = Health::None;
            }
            if (entry.kind == ItemKind::Info) {
                view.state = focused ? RowState::InfoSelected : RowState::Info;
            } else {
                view.state = focused ? (_mode == Mode::Edit ? RowState::Editing : RowState::Selected) : RowState::Normal;
                view.chevron = true;
            }
            state = index * 16 + static_cast<int32_t>(view.state);
        }

        RowKey& key = _rowKeys[slot];
        if (key.state == state && key.health == view.health && strcmp(key.value, value) == 0) continue;
        key.state = state;
        key.health = view.health;
        strlcpy(key.value, value, sizeof(key.value));
        view.value = value;
        SettingsUi::row(slot, view);
    }
}

void SettingsScreen::drawFooter() {
    Frame& f = frame();
    const char* hint = "";
    char page[8] = "";
    if (_mode == Mode::Edit) {
        hint = "Turn to change, click to save";
    } else if (f.cursor < 0) {
        hint = "Click to go back";
    } else {
        const MenuItem& entry = item(f.cursor);
        switch (entry.kind) {
            case ItemKind::Setting:
                hint = Settings::def(entry.setting).kind == SettingKind::Toggle ? "Click to switch" : "Click to edit";
                break;
            case ItemKind::Submenu: hint = "Click to open"; break;
            case ItemKind::Action: hint = "Click to run"; break;
            case ItemKind::Info: hint = "Read only"; break;
        }
        snprintf(page, sizeof(page), "%d/%d", f.cursor + 1, count());
    }

    if (hint == _hintShown && strcmp(page, _pageShown) == 0) return;
    _hintShown = hint;
    strlcpy(_pageShown, page, sizeof(_pageShown));
    SettingsUi::footer(hint, page);
}

void SettingsScreen::drawConfirm() {
    bool factory = _confirmAction == ActionId::FactoryReset;
    if (_full) {
        _full = false;
        _buttonsDirty = true;
        if (factory) {
            SettingsUi::dialog("Factory reset?", "Erases all settings", "and the saved Wi-Fi.",
                               "This cannot be undone.", Theme::alert);
        } else {
            SettingsUi::dialog("Reset settings?", "Restores every setting", "to its default value.",
                               "Wi-Fi is kept.", Theme::accent);
        }
    }
    if (!_buttonsDirty) return;
    _buttonsDirty = false;
    SettingsUi::dialogButtons("Cancel", factory ? "Erase" : "Reset", _confirmFocused);
}
