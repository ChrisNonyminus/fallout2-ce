#include "sfall_config.h"

#include <stdio.h>
#include <string.h>

#include "platform_compat.h"

namespace fallout {

#define SFALL_CONFIG_FILE_NAME "ddraw.ini"

#if defined(__WII__)
#define PATH_PREFIX "sd:/fallout/"
#elif defined(__3DS__)
#define PATH_PREFIX "sdmc:/fallout/"
#else
#define PATH_PREFIX ""
#endif

bool gSfallConfigInitialized = false;
Config gSfallConfig;

#if defined (__WII__)
#define SKIP_MOVIES 0 // tmp
#else
#define SKIP_MOVIES 0
#endif

bool sfallConfigInit(int argc, char** argv)
{
    if (gSfallConfigInitialized) {
        return false;
    }

    if (!configInit(&gSfallConfig)) {
        return false;
    }

    // Initialize defaults.
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_DUDE_NATIVE_LOOK_JUMPSUIT_MALE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_DUDE_NATIVE_LOOK_JUMPSUIT_FEMALE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_DUDE_NATIVE_LOOK_TRIBAL_MALE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_DUDE_NATIVE_LOOK_TRIBAL_FEMALE_KEY, "");
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_BIG_FONT_COLOR_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_CREDITS_OFFSET_X_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_CREDITS_OFFSET_Y_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_FONT_COLOR_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_OFFSET_X_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_MAIN_MENU_OFFSET_Y_KEY, 0);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_SKIP_OPENING_MOVIES_KEY, SKIP_MOVIES);
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_STARTING_MAP_KEY, "");
    configSetBool(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_DISPLAY_KARMA_CHANGES_KEY, false);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_OVERRIDE_CRITICALS_MODE_KEY, 2);
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_OVERRIDE_CRITICALS_FILE_KEY, "");
    configSetBool(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_REMOVE_CRITICALS_TIME_LIMITS_KEY, false);
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BOOKS_FILE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_ELEVATORS_FILE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_CONSOLE_OUTPUT_FILE_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_PREMADE_CHARACTERS_FILE_NAMES_KEY, "");
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_PREMADE_CHARACTERS_FACE_FIDS_KEY, "");
    configSetBool(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BURST_MOD_ENABLED_KEY, false);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BURST_MOD_CENTER_MULTIPLIER_KEY, SFALL_CONFIG_BURST_MOD_DEFAULT_CENTER_MULTIPLIER);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BURST_MOD_CENTER_DIVISOR_KEY, SFALL_CONFIG_BURST_MOD_DEFAULT_CENTER_DIVISOR);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BURST_MOD_TARGET_MULTIPLIER_KEY, SFALL_CONFIG_BURST_MOD_DEFAULT_TARGET_MULTIPLIER);
    configSetInt(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_BURST_MOD_TARGET_DIVISOR_KEY, SFALL_CONFIG_BURST_MOD_DEFAULT_TARGET_DIVISOR);
    configSetString(&gSfallConfig, SFALL_CONFIG_MISC_KEY, SFALL_CONFIG_EXTRA_MESSAGE_LISTS_KEY, "");

    char path[COMPAT_MAX_PATH];
    char* executable = argv[0];
    char* ch = strrchr(executable, '\\');
    if (ch != NULL) {
        *ch = '\0';
        snprintf(path, sizeof(path), "%s\\%s", executable, SFALL_CONFIG_FILE_NAME);
        *ch = '\\';
    } else {
        strcpy(path, PATH_PREFIX SFALL_CONFIG_FILE_NAME);
    }

    configRead(&gSfallConfig, path, false);

    configParseCommandLineArguments(&gSfallConfig, argc, argv);

    gSfallConfigInitialized = true;

    return true;
}

void sfallConfigExit()
{
    if (gSfallConfigInitialized) {
        configFree(&gSfallConfig);
        gSfallConfigInitialized = false;
    }
}

} // namespace fallout
