#pragma once

struct ListFocus {
    int count = 0;
    int visible = 1;
    int index = 0;
    int first = 0;

    void move(int step) {
        if (count <= 0) return;
        index = (index + step + count) % count;
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
