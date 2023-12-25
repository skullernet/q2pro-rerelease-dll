// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

enum {
    PMENU_ALIGN_LEFT,
    PMENU_ALIGN_CENTER,
    PMENU_ALIGN_RIGHT
};

typedef struct pmenu_s pmenu_t;

typedef void (*UpdateFunc_t)(edict_t *ent);

typedef struct {
    pmenu_t     *entries;
    int          cur;
    int          num;
    void        *arg;
    UpdateFunc_t UpdateFunc;
} pmenuhnd_t;

typedef void (*SelectFunc_t)(edict_t *ent, pmenuhnd_t *hnd);

struct pmenu_s {
    char         text[64];
    int          align;
    SelectFunc_t SelectFunc;
};

pmenuhnd_t *PMenu_Open(edict_t *ent, const pmenu_t *entries, int cur, int num, void *arg, UpdateFunc_t UpdateFunc);
void        PMenu_Close(edict_t *ent);
void        PMenu_UpdateEntry(pmenu_t *entry, const char *text, int align, SelectFunc_t SelectFunc);
void        PMenu_Do_Update(edict_t *ent);
void        PMenu_Update(edict_t *ent);
void        PMenu_Next(edict_t *ent);
void        PMenu_Prev(edict_t *ent);
void        PMenu_Select(edict_t *ent);
