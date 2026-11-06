#pragma once

constexpr bool wrapFocus = false;

inline int stepIndex(int index, int step, int count) {
    int next = index + step;
    if (wrapFocus) return (next % count + count) % count;
    return next < 0 ? 0 : (next >= count ? count - 1 : next);
}

struct ListFocus {
    int count = 0;
    int visible = 1;
    int index = 0;
    int first = 0;

    void move(int step) {
        if (count <= 0) return;
        index = stepIndex(index, step, count);
        reveal();
    }

    void set(int newIndex) {
        index = newIndex;
        reveal();
    }

private:
    void reveal() {
        if (index < first) first = index;
        if (index >= first + visible) first = index - visible + 1;
    }
};
