// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include <fstream>
#include <string>
#include <map>

#define L10N_FILE   "loc_english.txt"

static std::map<std::string, std::string> messages;

void G_LoadL10nFile()
{
    messages.clear();
    auto cv = gi.cvar("fs_gamedir", nullptr, CVAR_NOFLAGS);
    if (!cv) {
        gi.dprintf("fs_gamedir not set\n");
        return;
    }
    auto path = cv->string + std::string("/" L10N_FILE);
    std::ifstream file(path);
    if (!file) {
        gi.dprintf("Couldn't load " L10N_FILE "\n");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        char key[MAX_QPATH];
        char val[MAX_STRING_CHARS];
        auto data = line.c_str();

        COM_Parse(&data, key, sizeof(key));
        if (strcmp(COM_Parse(&data), "="))
            continue;
        COM_Parse(&data, val, sizeof(val));
        if (!data)
            continue;

        messages[key] = val;
    }
    gi.Com_PrintFmt("Loaded {} messages from " L10N_FILE "\n", messages.size());
}

const char *G_GetL10nString(const char *key)
{
    return messages[key].c_str();
}
