#include "ScreenManager.h"
#include "ListFocus.h"

ScreenManager::ScreenManager(InputRouter& input, NavState& nav, const Page* pages, uint8_t pageCount, ScreensMenu& menu)
    : _input(input), _nav(nav), _pages(pages), _pageCount(pageCount), _menu(menu) {}

void ScreenManager::begin() {
    _input.begin();
    show(0, 0);
}

Screen* ScreenManager::active() {
    if (_menuOpen) return &_menu;
    if (_detail) return _detail;
    return _pages[_page].subPages[_subPage];
}

void ScreenManager::closeDetail() {
    _detail = nullptr;
    active()->init();
}

void ScreenManager::show(uint8_t page, uint8_t subPage) {
    _detail = nullptr;
    _page = page;
    _subPage = subPage;
    _nav.subPage = subPage;
    _nav.subPageCount = _pages[page].count;
    active()->init();
}

void ScreenManager::stepSubPage(int step) {
    int next = stepIndex(_subPage, step, _pages[_page].count);
    if (next == _subPage) return;
    show(_page, next);
}

void ScreenManager::dispatch(Input input) {
    switch (input) {
        case Input::Home:
            if (!_menuOpen && !_detail && _page == 0 && _subPage == 0) return;
            _menuOpen = false;
            show(0, 0);
            return;
        case Input::Menu:
            _menuOpen = !_menuOpen;
            if (_menuOpen) {
                _menu.open(_page);
            } else {
                active()->init();
            }
            return;
        case Input::SubPagePrev:
        case Input::SubPageNext:
            if (_menuOpen) return;
            if (_detail) {
                closeDetail();
                return;
            }
            if (_pages[_page].count < 2) return;
            stepSubPage(input == Input::SubPageNext ? 1 : -1);
            return;
        case Input::Select:
            if (_menuOpen) {
                _menuOpen = false;
                show(_menu.focusedPage(), 0);
                return;
            }
            if (_detail) {
                closeDetail();
                return;
            }
            break;
        default:
            break;
    }
    Screen* screen = active();
    screen->handleInput(input);
    if (Screen* detail = screen->takeDetail()) {
        _detail = detail;
        _detail->init();
    }
}

void ScreenManager::update() {
    _input.update();
    Input input;
    while (_input.next(input)) dispatch(input);
    active()->update();
}

void ScreenManager::draw() {
    active()->draw();
}
