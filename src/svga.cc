#include "svga.h"

#include <limits.h>
#include <string.h>

#include <SDL.h>

#include "config.h"
#include "draw.h"
#include "interface.h"
#include "memory.h"
#include "mmx.h"
#include "mouse.h"
#include "win32.h"
#include "window_manager.h"
#include "window_manager_private.h"

namespace fallout {

static bool createRenderer(int width, int height);
static void destroyRenderer();

// 0x51E2C8
bool gMmxEnabled = true;

// screen rect
Rect _scr_size;

// 0x6ACA18
void (*_scr_blit)(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y) = _GNW95_ShowRect;

// 0x6ACA1C
void (*_zero_mem)() = NULL;

SDL_Surface* gSdlSurface = NULL;
SDL_Surface* gSdlTextureSurface = NULL;

#if !defined(__3DS__) && !defined(__WII__)
SDL_Window* gSdlWindow = NULL;
SDL_Renderer* gSdlRenderer = NULL;
SDL_Texture* gSdlTexture = NULL;
#else
SDL_Surface* gSdlWindow = NULL;
#endif

#if defined(__3DS__)
SDL_Surface* gSdlTopScreen = NULL;
SDL_Surface* gSdlBottomScreen = NULL;
#endif

// TODO: Remove once migration to update-render cycle is completed.
FpsLimiter sharedFpsLimiter;

// 0x4CACD0
void mmxSetEnabled(bool a1)
{
    // 0x51E2CC
    static bool probed = false;

    // 0x6ACA20
    static bool supported;

    if (!probed) {
        supported = mmxIsSupported();
        probed = true;
    }

    if (supported) {
        gMmxEnabled = a1;
    }
}

// 0x4CAD08
int _init_mode_320_200()
{
    return _GNW95_init_mode_ex(320, 200, 8);
}

// 0x4CAD40
int _init_mode_320_400()
{
    return _GNW95_init_mode_ex(320, 400, 8);
}

// 0x4CAD5C
int _init_mode_640_480_16()
{
    return -1;
}

// 0x4CAD64
int _init_mode_640_480()
{
    return _init_vesa_mode(640, 480);
}

// 0x4CAD94
int _init_mode_640_400()
{
    return _init_vesa_mode(640, 400);
}

// 0x4CADA8
int _init_mode_800_600()
{
    return _init_vesa_mode(800, 600);
}

// 0x4CADBC
int _init_mode_1024_768()
{
    return _init_vesa_mode(1024, 768);
}

// 0x4CADD0
int _init_mode_1280_1024()
{
    return _init_vesa_mode(1280, 1024);
}

// 0x4CADF8
void _get_start_mode_()
{
}

// 0x4CADFC
void _zero_vid_mem()
{
    if (_zero_mem) {
        _zero_mem();
    }
}

// 0x4CAE1C
int _GNW95_init_mode_ex(int width, int height, int bpp)
{
    bool fullscreen = true;

    Config resolutionConfig;
    if (configInit(&resolutionConfig)) {
        if (configRead(&resolutionConfig, "f2_res.ini", false)) {
            int screenWidth;
            if (configGetInt(&resolutionConfig, "MAIN", "SCR_WIDTH", &screenWidth)) {
                width = screenWidth;
            }

            int screenHeight;
            if (configGetInt(&resolutionConfig, "MAIN", "SCR_HEIGHT", &screenHeight)) {
                height = screenHeight;
            }

            bool windowed;
            if (configGetBool(&resolutionConfig, "MAIN", "WINDOWED", &windowed)) {
                fullscreen = !windowed;
            }

            configGetBool(&resolutionConfig, "IFACE", "IFACE_BAR_MODE", &gInterfaceBarMode);
            configGetInt(&resolutionConfig, "IFACE", "IFACE_BAR_WIDTH", &gInterfaceBarWidth);
            configGetInt(&resolutionConfig, "IFACE", "IFACE_BAR_SIDE_ART", &gInterfaceSidePanelsImageId);
            configGetBool(&resolutionConfig, "IFACE", "IFACE_BAR_SIDES_ORI", &gInterfaceSidePanelsExtendFromScreenEdge);
        }
        configFree(&resolutionConfig);
    }

    if (_GNW95_init_window(width, height, fullscreen) == -1) {
        return -1;
    }

    if (directDrawInit(width, height, bpp) == -1) {
        return -1;
    }

    _scr_size.left = 0;
    _scr_size.top = 0;
    _scr_size.right = width - 1;
    _scr_size.bottom = height - 1;

    mmxSetEnabled(true);

    _mouse_blit_trans = NULL;
    _scr_blit = _GNW95_ShowRect;
    _zero_mem = _GNW95_zero_vid_mem;
    _mouse_blit = _GNW95_ShowRect;

    return 0;
}

// 0x4CAECC
int _init_vesa_mode(int width, int height)
{
    return _GNW95_init_mode_ex(width, height, 8);
}

// 0x4CAEDC
int _GNW95_init_window(int width, int height, bool fullscreen)
{
    if (gSdlWindow == NULL) {
#if !defined(__3DS__) && !defined(__WII__)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
            return -1;
        }
        SDL_ShowCursor(SDL_DISABLE);

#if !defined(__3DS__) && !defined(__WII__)

        Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

        if (fullscreen) {
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        }
        gSdlWindow = SDL_CreateWindow(gProgramWindowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);
#else
#if defined(__3DS__)
        gSdlWindow = SDL_SetVideoMode(400, 480, 8, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DUALSCR | SDL_DOUBLEBUF);
        gSdlTopScreen = SDL_CreateRGBSurface(0, 400, 240, 8, 0, 0, 0, 0);
        gSdlBottomScreen = SDL_CreateRGBSurface(0, 320, 240, 8, 0, 0, 0, 0);

#else
        gSdlWindow = SDL_SetVideoMode(640, 480, 8, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF);
#endif
#endif
        if (gSdlWindow == NULL) {
            return -1;
        }

        if (!createRenderer(width, height)) {
            destroyRenderer();

#if !defined(__3DS__) && !defined(__WII__)

            SDL_DestroyWindow(gSdlWindow);
#else
            SDL_Quit();
#endif
            gSdlWindow = NULL;

            return -1;
        }
    }

    return 0;
}

// 0x4CAF9C
int directDrawInit(int width, int height, int bpp)
{
    if (gSdlSurface != NULL) {
        unsigned char* palette = directDrawGetPalette();
        directDrawFree();

        if (directDrawInit(width, height, bpp) == -1) {
            return -1;
        }

        directDrawSetPalette(palette);

        return 0;
    }

    gSdlSurface = SDL_CreateRGBSurface(0, width, height, bpp, 0, 0, 0, 0);

    SDL_Color colors[256];
    for (int index = 0; index < 256; index++) {
        colors[index].r = index;
        colors[index].g = index;
        colors[index].b = index;
#if !defined(__3DS__) && !defined(__WII__)
        colors[index].a = 255;
#endif
    }
#if !defined(__3DS__) && !defined(__WII__)

    SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);
#else
    SDL_SetColors(gSdlSurface, colors, 0, 256);
    SDL_SetColors(gSdlWindow, colors, 0, 256);
#if defined(__3DS__)
    SDL_SetColors(gSdlTopScreen, colors, 0, 256);
    SDL_SetColors(gSdlBottomScreen, colors, 0, 256);
#endif
#endif

    return 0;
}

// 0x4CB1B0
void directDrawFree()
{
    if (gSdlSurface != NULL) {
        SDL_FreeSurface(gSdlSurface);
#if defined(__3DS__)
        SDL_FreeSurface(gSdlTopScreen);
        SDL_FreeSurface(gSdlBottomScreen);
#endif
        gSdlSurface = NULL;
    }
}

SDL_Rect BTM_OFFSET = { 0, 240 };
// 0x4CB310
void directDrawSetPaletteInRange(unsigned char* palette, int start, int count)
{
    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color colors[256];

        if (count != 0) {
            for (int index = 0; index < count; index++) {
                colors[index].r = palette[index * 3] << 2;
                colors[index].g = palette[index * 3 + 1] << 2;
                colors[index].b = palette[index * 3 + 2] << 2;
#if !defined(__3DS__) && !defined(__WII__)
                colors[index].a = 255;
#endif
            }
        }

#if !defined(__3DS__) && !defined(__WII__)
        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, start, count);
#else
        SDL_SetColors(gSdlSurface, colors, start, count);
        SDL_SetColors(gSdlWindow, colors, start, count);
#if defined(__3DS__)
        SDL_SetColors(gSdlTopScreen, colors, start, count);
        SDL_SetColors(gSdlBottomScreen, colors, start, count);
#endif
#endif
#if defined(__3DS__)
        SDL_BlitSurface(gSdlTopScreen, NULL, gSdlTextureSurface, NULL);
        SDL_BlitSurface(gSdlBottomScreen, NULL, gSdlTextureSurface, &BTM_OFFSET);
#else

        SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);

#endif
    }
}

// 0x4CB568
void directDrawSetPalette(unsigned char* palette)
{
    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color colors[256];

        for (int index = 0; index < 256; index++) {
            colors[index].r = palette[index * 3] << 2;
            colors[index].g = palette[index * 3 + 1] << 2;
            colors[index].b = palette[index * 3 + 2] << 2;
#if !defined(__3DS__) && !defined(__WII__)
            colors[index].a = 255;
#endif
        }

#if !defined(__3DS__) && !defined(__WII__)
        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);
#else
        SDL_SetColors(gSdlSurface, colors, 0, 256);
        SDL_SetColors(gSdlWindow, colors, 0, 256);
#if defined(__3DS__)
        SDL_SetColors(gSdlTopScreen, colors, 0, 256);
        SDL_SetColors(gSdlBottomScreen, colors, 0, 256);
#endif
#endif
#if defined(__3DS__)
        SDL_BlitSurface(gSdlTopScreen, NULL, gSdlTextureSurface, NULL);
        SDL_BlitSurface(gSdlBottomScreen, NULL, gSdlTextureSurface, &BTM_OFFSET);
#else
        SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);
#endif
    }
}

// 0x4CB68C
unsigned char* directDrawGetPalette()
{
    // 0x6ACA24
    static unsigned char palette[768];

    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color* colors = gSdlSurface->format->palette->colors;

        for (int index = 0; index < 256; index++) {
            SDL_Color* color = &(colors[index]);
            palette[index * 3] = color->r >> 2;
            palette[index * 3 + 1] = color->g >> 2;
            palette[index * 3 + 2] = color->b >> 2;
        }
    }

    return palette;
}

// 0x4CB850
void _GNW95_ShowRect(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY)
{
#if !defined(__3DS__)

    blitBufferToBuffer(src + srcPitch * srcY + srcX, srcWidth, srcHeight, srcPitch, (unsigned char*)gSdlSurface->pixels + gSdlSurface->pitch * destY + destX, gSdlSurface->pitch);
#else
    blitBufferToBuffer(src + srcPitch * srcY + srcX, srcWidth, srcHeight, srcPitch, (unsigned char*)gSdlTopScreen->pixels + gSdlTopScreen->pitch * destY + destX, gSdlTopScreen->pitch);
#endif

    SDL_Rect srcRect;
    srcRect.x = destX;
    srcRect.y = destY;
    srcRect.w = srcWidth;
    srcRect.h = srcHeight;

    SDL_Rect destRect;
    destRect.x = destX;
    destRect.y = destY;


#if defined(__3DS__)
    SDL_BlitSurface(gSdlTopScreen, &srcRect, gSdlTextureSurface, &destRect);
    SDL_BlitSurface(gSdlBottomScreen, &srcRect, gSdlTextureSurface, &destRect);
#else

    SDL_BlitSurface(gSdlSurface, &srcRect, gSdlTextureSurface, &destRect);

#endif

}

// Clears drawing surface.
//
// 0x4CBBC8
void _GNW95_zero_vid_mem()
{
    if (!gProgramIsActive) {
        return;
    }

#if defined(__3DS__)
    for (int y = 0; y < gSdlTopScreen->h; y++) {
        memset(gSdlTopScreen->pixels + gSdlTopScreen->pitch * y, 0, gSdlTopScreen->w);
    }
    for (int y = 0; y < gSdlBottomScreen->h; y++) {
        memset(gSdlBottomScreen->pixels + gSdlBottomScreen->pitch * y, 0, gSdlBottomScreen->w);
    }
    SDL_BlitSurface(gSdlTopScreen, NULL, gSdlTextureSurface, NULL);
    SDL_BlitSurface(gSdlBottomScreen, NULL, gSdlTextureSurface, &BTM_OFFSET);
#else

    unsigned char* surface = (unsigned char*)gSdlSurface->pixels;
    for (int y = 0; y < gSdlSurface->h; y++) {
        memset(surface, 0, gSdlSurface->w);
        surface += gSdlSurface->pitch;
    }


    SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);

#endif
}

int screenGetWidth()
{
    // TODO: Make it on par with _xres;
    return rectGetWidth(&_scr_size);
}

int screenGetHeight()
{
    // TODO: Make it on par with _yres.
    return rectGetHeight(&_scr_size);
}

int screenGetVisibleHeight()
{
    int windowBottomMargin = 0;

    if (!gInterfaceBarMode) {
        windowBottomMargin = INTERFACE_BAR_HEIGHT;
    }
    return screenGetHeight() - windowBottomMargin;
}

static bool createRenderer(int width, int height)
{
#if !defined(__3DS__) && !defined(__WII__)
    gSdlRenderer = SDL_CreateRenderer(gSdlWindow, -1, 0);
    if (gSdlRenderer == NULL) {
        return false;
    }

    if (SDL_RenderSetLogicalSize(gSdlRenderer, width, height) != 0) {
        return false;
    }

    gSdlTexture = SDL_CreateTexture(gSdlRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (gSdlTexture == NULL) {
        return false;
    }

    Uint32 format;
    if (SDL_QueryTexture(gSdlTexture, &format, NULL, NULL, NULL) != 0) {
        return false;
    }

    gSdlTextureSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(format), format);
    if (gSdlTextureSurface == NULL) {
        return false;
    }
#else
    gSdlTextureSurface = gSdlWindow;
    if (gSdlTextureSurface == NULL) {
        return false;
    }
#endif

    return true;
}

static void destroyRenderer()
{

#if !defined(__3DS__) && !defined(__WII__)
    if (gSdlTextureSurface != NULL) {
        SDL_FreeSurface(gSdlTextureSurface);
        gSdlTextureSurface = NULL;
    }
    if (gSdlTexture != NULL) {
        SDL_DestroyTexture(gSdlTexture);
        gSdlTexture = NULL;
    }

    if (gSdlRenderer != NULL) {
        SDL_DestroyRenderer(gSdlRenderer);
        gSdlRenderer = NULL;
    }
#endif
}

void handleWindowSizeChanged()
{
    destroyRenderer();
    createRenderer(screenGetWidth(), screenGetHeight());
}

void renderPresent()
{
#if !defined(__3DS__) && !defined(__WII__)
    SDL_UpdateTexture(gSdlTexture, NULL, gSdlTextureSurface->pixels, gSdlTextureSurface->pitch);
    SDL_RenderClear(gSdlRenderer);
    SDL_RenderCopy(gSdlRenderer, gSdlTexture, NULL, NULL);
    SDL_RenderPresent(gSdlRenderer);
#else
#if defined(__WII__)
    // // debug: blit gSDLSurface to gSdlTextureSurface
    // SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);

    // debug: save bmps
    SDL_SaveBMP(gSdlTextureSurface, "sd:/gSdlTextureSurface.bmp");
    SDL_SaveBMP(gSdlWindow, "sd:/gSdlWindow.bmp");
    SDL_SaveBMP(gSdlSurface, "sd:/gSdlSurface.bmp");

    SDL_Flip(gSdlWindow);

    // // copy from texture surface to screen surface, convert to 16-bit
    // SDL_LockSurface(gSdlWindow);
    // SDL_Surface* converted = SDL_DisplayFormat(gSdlTextureSurface);
    // SDL_BlitSurface(converted, NULL, gSdlWindow, NULL);
    // SDL_FreeSurface(converted);
    // SDL_UnlockSurface(gSdlWindow);
#endif
#endif
}

} // namespace fallout
