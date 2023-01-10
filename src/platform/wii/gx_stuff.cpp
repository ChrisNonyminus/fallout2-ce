#include "gx_stuff.h"

#include "../../svga.h"

#include <SDL.h>

namespace wii {
    bool GXDrawing = false;

    void HandleGX() {
        // SDL_QuitSubSystem(SDL_INIT_VIDEO);
        // GXDrawing = true;
        
        // InitVideo();
        
    }

    void GXDraw() {
        if (!GXDrawing) {
            HandleGX();
        }
    }

    void GXFinish() {
        // GXDrawing = false;
        // SDL_InitSubSystem(SDL_INIT_VIDEO);

        // fallout::gSdlWindow = SDL_SetVideoMode(640, 480, 8, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF);

        // if (!fallout::gSdlWindow) {
        //     printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        //     exit(1);
        // }


    }

}