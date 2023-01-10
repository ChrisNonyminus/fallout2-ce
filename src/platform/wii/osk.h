#pragma once

// On screen keyboard

#include <SDL.h>

#include "../../dinput.h"
#include "../../svga.h"

namespace wii {
struct Osk {
    Osk(char* default_text);
    ~Osk();

    void init();
    void deinit();

    void update();
    void draw();

    char* get_text();

    bool is_visible() const;

    bool textComplete = false;

    int maxLength = 8;

private:
    char* text;
    bool visible;

    u8* keys;
    int hoveredKey = 'A';

    u8* window;

};
}
