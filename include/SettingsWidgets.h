#pragma once
#include <stddef.h>
#include <stdint.h>

enum class Health : uint8_t { None, Ok, Warn, Error, Off };
enum class RowState : uint8_t { Normal, Selected, Editing, Info, InfoSelected, Empty };

struct RowView {
    RowState state;
    const char* label;
    const char* value;
    Health health;
    bool chevron;
};

namespace SettingsUi {
constexpr int visibleRows = 8;
constexpr int rowTop = 32, rowHeight = 24, rowWidth = 308;
constexpr int footerY = 224;

void header(const char* title, bool backFocused);
void row(int slot, const RowView& view);
void scrollBar(int index, int count);
void footer(const char* hint, const char* page);
void dialog(const char* title, const char* line1, const char* line2, const char* line3, uint16_t panelColor);
void dialogButtons(const char* cancelLabel, const char* confirmLabel, bool confirmFocused);
}
