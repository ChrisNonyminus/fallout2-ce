#pragma once

// optimizes and repacks game assets

#include "memory.h"
#include "xfile.h"
#include "art.h"
#include "db.h"
#include "dictionary.h"
#include "sound.h"
#include "movie.h"

#include <string>
#include <vector>

namespace fallout {

    #define OPTIM_FLAG_NONE 0x0
    #define OPTIM_FLAG_DOWNSCALE_ART_2X 1 << 1
    #define OPTIM_FLAG_REMOVE_MVES 1 << 2
    #define OPTIM_FLAG_DOWNSAMPLE_SOUND 1 << 3
    #define OPTIM_FLAG_DOWNSCALE_MVES 1 << 4
    #define OPTIM_FLAG_REMOVE_MUSIC 1 << 5
    #define OPTIM_FLAG_REMOVE_VOICE 1 << 6
    #define OPTIM_FLAG_COMPRESS_ASSETS 1 << 7
    #define OPTIM_FLAG_4BIT_COLORINDEX 1 << 8


    struct Packer {
        const char* outputXbaseFolder;
        std::vector<std::string> artList;
        std::vector<std::string> soundList;
        std::vector<std::string> movieList;
        std::vector<std::string> musicList;
        std::vector<std::string> voiceList;


        int optimizationFlags;

        Packer(const char* outputXbaseFolder, int optimizationFlags);
        ~Packer();

        void optimize();
        void compress_assets(const char* inFolder);

    };
}









