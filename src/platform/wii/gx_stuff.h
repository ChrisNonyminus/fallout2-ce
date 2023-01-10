#pragma once

// handles temporary drawing using GX, and hands control of GX back to SDL when done

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <vector>


namespace wii {
    void HandleGX();
    void GXDraw();
    void GXDrawDone();

    extern bool GXDrawing;


}



