// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "q_files.h"

#define MIN_MESSAGES    1024

#define L10N_FILE   "localization/loc_english.txt"

typedef struct {
    const char *key;
    const char *value;
} message_t;

static message_t *messages;
static int nb_messages;

static int messagecmp(const void *p1, const void *p2)
{
    return strcmp(((const message_t *)p1)->key, ((const message_t *)p2)->key);
}

static void parse_line(const char *data)
{
    char *key, *val, *p, *s;

    key = COM_Parse(&data);
    p = COM_Parse(&data);
    val = COM_Parse(&data);
    if (!data || strcmp(p, "="))
        return;

    if (!messages) {
        messages = gi.TagMalloc(MIN_MESSAGES * sizeof(messages[0]), TAG_L10N);
    } else if (!(nb_messages & (MIN_MESSAGES - 1))) {
        Q_assert(nb_messages < INT_MAX / sizeof(messages[0]) - MIN_MESSAGES);
        messages = gix.TagRealloc(messages, (nb_messages + MIN_MESSAGES) * sizeof(messages[0]));
    }

    // remove %junk%%junk% prefixes
    while (*val == '%') {
        if (!(p = strchr(val + 1, '%')))
            break;
        val = p + 1;
    }

    // unescape linefeeds
    s = p = val;
    while (*s) {
        if (*s == '\\' && s[1] == 'n') {
            *p++ = '\n';
            s += 2;
        } else {
            *p++ = *s++;
        }
    }
    *p = 0;

    message_t *m = &messages[nb_messages++];
    m->key = G_CopyString(key, TAG_L10N);
    m->value = G_CopyString(val, TAG_L10N);
}

void G_FreeL10nFile(void)
{
    gi.FreeTags(TAG_L10N);
    messages = NULL;
    nb_messages = 0;
}

void G_LoadL10nFile(void)
{
    G_FreeL10nFile();

    filesystem_api_v1_t *fs = gix.GetExtension("FILESYSTEM_API_V1");
    if (!fs) {
        gi.dprintf("FILESYSTEM_API_V1 not available\n");
        return;
    }

    char *data;
    int ret = fs->LoadFile(L10N_FILE, (void **)&data, 0, TAG_L10N);
    if (!data) {
        gi.dprintf("Couldn't load %s: %s\n", L10N_FILE, fs->ErrorString(ret));
        return;
    }

    char *s = data;
    while (*s) {
        char *p = strchr(s, '\n');
        if (p)
            *p = 0;

        parse_line(s);

        if (!p)
            break;
        s = p + 1;
    }

    gi.TagFree(data);

    qsort(messages, nb_messages, sizeof(messages[0]), messagecmp);

    gi.dprintf("Loaded %d messages from %s\n", nb_messages, L10N_FILE);
}

const char *G_GetL10nString(const char *key)
{
    message_t k = { key };
    message_t *m = bsearch(&k, messages, nb_messages, sizeof(messages[0]), messagecmp);
    return m ? m->value : NULL;
}
