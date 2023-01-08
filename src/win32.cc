#include "win32.h"

#include <stdlib.h>

#include <SDL.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "main.h"
#include "svga.h"
#include "window_manager.h"

#if __3DS__
#include <3ds.h>
#endif

#if __WII__
#include <fat.h>
#include <ogcsys.h>
#include <wiiuse/wiiuse.h>
#include <ogc/usbmouse.h>
#include <wiikeyboard/keyboard.h>
#endif

#if __APPLE__ && TARGET_OS_IOS
#include "platform/ios/paths.h"
#endif

#if defined(__WII__) || defined(__3DS__)
#undef main // SDL_main.h defines main() as SDL_main()
#endif

namespace fallout {

#ifdef _WIN32
// 0x51E444
bool gProgramIsActive = false;

// GNW95MUTEX
HANDLE _GNW95_mutex = NULL;

// 0x4DE700
int main(int argc, char* argv[])
{
    _GNW95_mutex = CreateMutexA(0, TRUE, "GNW95MUTEX");
    if (GetLastError() == ERROR_SUCCESS) {
        SDL_ShowCursor(SDL_DISABLE);

        gProgramIsActive = true;
        falloutMain(argc, argv);

        CloseHandle(_GNW95_mutex);
    }
    return 0;
}
#else
bool gProgramIsActive = false;

int main(int argc, char* argv[])
{
#if __APPLE__ && TARGET_OS_IOS
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    chdir(iOSGetDocumentsPath());
#endif

#if __APPLE__ && TARGET_OS_OSX
    char* basePath = SDL_GetBasePath();
    chdir(basePath);
    SDL_free(basePath);
#endif

#if __ANDROID__
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    chdir(SDL_AndroidGetExternalStoragePath());
#endif

#if defined(__WII__)
    fatInitDefault();

#endif

    SDL_ShowCursor(SDL_DISABLE);
    gProgramIsActive = true;
    return falloutMain(argc, argv);
}
#endif

} // namespace fallout

#if defined(__WII__)
extern "C" void Terminate();
void resetCallback(u32 irq, void* ctx)
{
    Terminate();
}
#endif


int main(int argc, char* argv[])
{
#if defined(__WII__)    
    L2Enhance();
    u32 version = IOS_GetVersion();
    s32 preferred = IOS_GetPreferredVersion();
    if (preferred > 0 && version != preferred) {
        IOS_ReloadIOS(preferred);
    }
    WPAD_Init();
    PAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    SYS_SetPowerCallback(Terminate);
    SYS_SetResetCallback(resetCallback);
    WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);
    MOUSE_Init();
    KEYBOARD_Init(NULL);
#endif

    return fallout::main(argc, argv);
}

extern "C" {
#if __WII__ // lmao
bool TerminateRequested = false;
void Terminate()
{
    SDL_Quit();
    printf("Terminate requested\n");
    exit(1);
}
#endif
}
