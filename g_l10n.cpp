// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "filesystem.h"
#include <map>

#define L10N_FILE   "localization/loc_english.txt"

static std::map<std::string, std::string> messages;

static void parse_line(const char *data)
{
    char key[MAX_QPATH];
    char val[MAX_STRING_CHARS];
    char *p, *q;

    COM_Parse(&data, key, sizeof(key));
    if (strcmp(COM_Parse(&data), "="))
        return;
    COM_Parse(&data, val, sizeof(val));
    if (!data)
        return;

    // remove %junk%%junk% prefixes
    p = val;
    while (*p == '%') {
        if (!(q = strchr(p + 1, '%')))
            break;
        p = q + 1;
    }
    messages[key] = p;
}

void G_LoadL10nFile()
{
    messages.clear();

    filesystem_api_v1_t *fs = (filesystem_api_v1_t *)gi.ex.GetExtension("FILESYSTEM_API_V1");
    if (!fs) {
        gi.dprintf("FILESYSTEM_API_V1 not available\n");
        return;
    }

    char *data;
    int ret = fs->LoadFile(L10N_FILE, (void **)&data, 0, TAG_GAME);
    if (!data) {
        gi.Com_PrintFmt("Couldn't load {}: {}\n", L10N_FILE, fs->ErrorString(ret));
        return;
    }

    char *base = data;
    while (*data) {
        char *p = strchr(data, '\n');
        if (p)
            *p = 0;

        parse_line(data);

        if (!p)
            break;
        data = p + 1;
    }

    gi.TagFree(base);
    gi.Com_PrintFmt("Loaded {} messages from {}\n", messages.size(), L10N_FILE);
}

const char *G_GetL10nString(const char *key)
{
    return messages[key].c_str();
}
