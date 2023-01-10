#include "packer.h"

#include "sound_effects_list.h"
#include "game_movie.h"

#include "file_utils.h"


#include <filesystem>

namespace fallout {
    Packer::Packer(const char* outputXbaseFolder, int optimizationFlags)
    {
        this->outputXbaseFolder = outputXbaseFolder;
        this->optimizationFlags = optimizationFlags;
                char path[256];

        // get list of art files
        for (int type = 0; type < OBJ_TYPE_COUNT; type++) {
            for (int i = 0; i < gArtListDescriptions[type].fileNamesLength; i++) {
                snprintf(path, 256, "art\\%s\\%s", gArtListDescriptions[type].name, &gArtListDescriptions[type].fileNames[i * 13]);
                artList.push_back(std::string(path));
            }
        }

        // get list of sound files
        for (int i = 0; i < gSoundEffectsListEntriesLength; i++) {
            snprintf(path, 256, "sound\\%s", gSoundEffectsListEntries[i].name);
            soundList.push_back(std::string(path));
        }

        // get list of movie files
        for (int i = 0; i < MOVIE_COUNT; i++) {
            snprintf(path, 256, "art\\cuts\\%s", gMovieFileNames[i]);
            movieList.push_back(std::string(path));
        }

        // get music files (find all music files in sound/music directory)
        // DIR* dir = opendir("sound/music");
        // if (dir) {
        //     struct dirent* ent;
        //     while ((ent = readdir(dir)) != NULL) {
        //         if (strstr(ent->d_name, ".ACM") != NULL || strstr(ent->d_name, ".acm") != NULL)
        //             musicList.push_back(std::string("sound/music/") + std::string(ent->d_name));
                    
        //     }
        //     closedir(dir);
        // }
        for (const auto& entry : std::filesystem::directory_iterator("sound/music")) {
            if (entry.is_regular_file()) {
                snprintf(path, 256, "sound\\music\\%s", entry.path().filename().string().c_str());
                musicList.push_back(std::string(path));
            }
        }




    }

    Packer::~Packer()
    {
    }

    void Packer::optimize()
    {



        if (optimizationFlags & OPTIM_FLAG_DOWNSCALE_ART_2X) {
            // downscale art 2x
            for (int i = 0; i < artList.size(); i++) {
                std::string path = artList[i];
                if (strstr(path.c_str(), ".FRM") != NULL || strstr(path.c_str(), ".frm") != NULL)
                {
                    std::string outPath = outputXbaseFolder + std::string("\\") + path;
                    int size;
                    dbGetFileSize(path.c_str(), &size);
                    artWrite(outPath.c_str(), (unsigned char*)artDownscale2x(artGet(path.c_str()), size));
                }
            }
        }

        // TODO: the rest
    }

    void Packer::compress_assets(const char* inFolder)
    {
        char path[256];
        char outPath[256];

        // search all files in inFolder recursively, and compress them to outFolder
        for (const auto& entry : std::filesystem::recursive_directory_iterator(inFolder)) {
            if (entry.is_regular_file()) {
                snprintf(path, 256, "%s", entry.path().string().c_str());
                snprintf(outPath, 256, "%s/%s", outputXbaseFolder, path);
                std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
                fileCopyCompressed(path, outPath);
            }
        }
    }
}

