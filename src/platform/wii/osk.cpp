#include "osk.h"

#include <SDL.h>

#include "../../color.h"
#include "../../dinput.h"
#include "../../svga.h"
#include "../../text_font.h"
#include "../../window.h"
#include "../../window_manager.h"
#include "../../memory.h"

namespace wii {
Osk::Osk(char* default_text)
{
    text = fallout::internal_strdup(default_text);
    visible = false;
}

Osk::~Osk()
{
    deinit();
}

void Osk::init()
{
    visible = true;
    keys = (u8*)fallout::internal_malloc(256);
    memset(keys, 0, 256);
    window = (u8*)fallout::internal_malloc(640 * 480);
    if (strlen(text) > maxLength) {
        // truncate to zero characters
        memset(text, 0, maxLength);
    }
}

void Osk::deinit()
{
    fallout::internal_free(text);
    fallout::internal_free(keys);
    fallout::internal_free(window);
}

void Osk::update()
{
    if (visible) {
        SDL_PumpEvents();
        SDL_Event event;

        if (hoveredKey < ' ') {
            hoveredKey = ' ';
        }
        if (hoveredKey > '~') {
            hoveredKey = '~';
        }
        while (SDL_PollEvent(&event)) {
            // controls:
            // d-pad (hat): move keyboard cursor
            // a (button 0/9): select key
            // b (button 1/10): backspace
            // start (button 5/7/18): confirm
            // select (button 4/17): cancel

            switch (event.type) {
            case SDL_JOYBUTTONDOWN:
                switch (event.jbutton.button) {
                case 0:
                case 9:
                    if (strlen(text) < maxLength) {
                        keys[hoveredKey] = 1;
                        char c = (char)hoveredKey;
                        char temp[2];
                        sprintf(temp, "%c", c);
                        strcat(text, temp);
                    }

                    break;
                case 1:
                case 10:
                    if (strlen(text) > 0) {
                        text[strlen(text) - 1] = '\0';
                    }
                    break;
                case 5:
                case 7:
                case 18:
                    textComplete = true;
                    visible = false;
                    break;
                case 4:
                case 17:
                    visible = false;
                    break;
                }
                break;
            case SDL_JOYHATMOTION:
                keys[hoveredKey] = 0;
                switch (event.jhat.value) {
                case SDL_HAT_UP:
                    hoveredKey -= 16;
                    break;
                case SDL_HAT_DOWN:
                    hoveredKey += 16;
                    break;
                case SDL_HAT_LEFT:
                    hoveredKey -= 1;
                    break;
                case SDL_HAT_RIGHT:
                    hoveredKey += 1;
                    break;
                }

                // hacky: render again
                draw();
                break;
            }

            if (hoveredKey < ' ') {
                hoveredKey = ' ';
            }
            if (hoveredKey > '~') {
                hoveredKey = '~';
            }

            // hacky: render again
            draw();
        }
    }
}

void Osk::draw()
{
    int white = ((int)(255 * 31.0) << 10) | ((int)(255 * 31.0) << 5) | (int)(255 * 31.0);
    int black = 0;
    int grey = ((int)(127 * 31.0) << 10) | ((int)(127 * 31.0) << 5) | (int)(127 * 31.0);
    if (visible) {
        // draw keyboard
        // clear it first
        for (int i = 0; i < 640 * 480; i++) {
            window[i] = fallout::_colorTable[black];
        }
        for (int i = ' '; i <= '~'; i++) {
            int x = (i % 16) * 20;
            int y = (i / 16) * 20;
            if (i == hoveredKey) {
                for (int j = 0; j < 20; j++) {
                    for (int k = 0; k < 20; k++) {
                        window[(x + j) + (y + k) * 640] = fallout::_colorTable[grey];
                    }
                }
            } else {
                for (int j = 0; j < 20; j++) {
                    for (int k = 0; k < 20; k++) {
                        window[(x + j) + (y + k) * 640] = fallout::_colorTable[black];
                    }
                }
            }
            char c = (char)i;
            int bufferOffsetToPutChar = (x) + (y)*640;
            fallout::fontDrawText(window + bufferOffsetToPutChar, &c, 640, 640, i == hoveredKey ? fallout::_colorTable[992] : fallout::_colorTable[white]);
        }

        int bufferOffsetToPutText = 0 + 240 * 640;
        fallout::fontDrawText(window + bufferOffsetToPutText, text, 640, 640, fallout::_colorTable[992]);
        int bufferOffsetToPutExplanation = 0 + 0 * 640;
        fallout::fontDrawText(window + bufferOffsetToPutExplanation, "Press A to select a key, B to backspace,", 640, 640, fallout::_colorTable[white]);
        bufferOffsetToPutExplanation = 0 + 20 * 640;
        fallout::fontDrawText(window + bufferOffsetToPutExplanation, "START to confirm, SELECT to cancel.", 640, 640, fallout::_colorTable[white]);

        fallout::_scr_blit(window, 640, 480, 0, 0, 640, 480, 0, 0);
    }
}

char* Osk::get_text()
{
    return text;
}

bool Osk::is_visible() const
{
    return visible;
}

}