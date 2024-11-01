// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "g_statusbar.h"

/*
======================================================================

INTERMISSION

======================================================================
*/

void DeathmatchScoreboard(edict_t *ent);

void MoveClientToIntermission(edict_t *ent)
{
    // [Paril-KEX]
    if (ent->client->ps.pmove.pm_type != PM_FREEZE)
        ent->s.event = EV_OTHER_TELEPORT;
    VectorCopy(level.intermission_origin, ent->s.origin);
    for (int i = 0; i < 3; i++)
        ent->client->ps.pmove.origin[i] = COORD2SHORT(level.intermission_origin[i]);
    VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
    ent->client->ps.pmove.pm_type = PM_FREEZE;
    ent->client->ps.gunindex = 0;
    ent->client->ps.blend[3] = 0;
    ent->client->ps.damage_blend[3] = 0;
    ent->client->ps.rdflags = RDF_NONE;

    // clean up powerup info
    ent->client->quad_time = 0;
    ent->client->invincible_time = 0;
    ent->client->breather_time = 0;
    ent->client->enviro_time = 0;
    ent->client->invisible_time = 0;
    ent->client->grenade_blew_up = false;
    ent->client->grenade_time = 0;

    ent->client->showhelp = false;
    ent->client->showscores = false;

    // RAFAEL
    ent->client->quadfire_time = 0;
    // RAFAEL
    // ROGUE
    ent->client->ir_time = 0;
    ent->client->nuke_time = 0;
    ent->client->double_time = 0;
    ent->client->tracker_pain_time = 0;
    // ROGUE

    ent->viewheight = 0;
    ent->s.modelindex = 0;
    ent->s.modelindex2 = 0;
    ent->s.modelindex3 = 0;
    ent->s.modelindex4 = 0;
    ent->s.effects = EF_NONE;
    ent->s.sound = 0;
    ent->solid = SOLID_NOT;
    ent->movetype = MOVETYPE_NOCLIP;

    gi.linkentity(ent);

    // add the layout

    if (deathmatch->integer) {
        DeathmatchScoreboard(ent);
        ent->client->showscores = true;
    }
}

// [Paril-KEX] update the level entry for end-of-unit screen
void G_UpdateLevelEntry(void)
{
    if (!level.entry)
        return;

    level.entry->found_secrets = level.found_secrets;
    level.entry->total_secrets = level.total_secrets;
    level.entry->killed_monsters = level.killed_monsters;
    level.entry->total_monsters = level.total_monsters;
}

static void G_EndOfUnitEntry(int y, const level_entry_t *entry, int maxlen)
{
    sb_yv(y);

    // we didn't visit this level, so print it as an unknown entry
    if (!entry->pretty_name[0]) {
        sb_string("???");
        return;
    }

    int64_t msec = TO_MSEC(entry->time);
    int minutes = msec / 60000;
    int seconds = (msec / 1000) % 60;
    int tensofsec = (msec / 100) % 10;

    sb_printf("string \"%*s %4d/%-4d %3d/%-3d %02d:%02d.%d\" ",
              maxlen, entry->pretty_name,
              entry->killed_monsters, entry->total_monsters,
              entry->found_secrets, entry->total_secrets,
              minutes, seconds, tensofsec);
}

static int entrycmp(const void *p1, const void *p2)
{
    const level_entry_t *a = (const level_entry_t *)p1;
    const level_entry_t *b = (const level_entry_t *)p2;
    int a_order = a->visit_order;
    int b_order = b->visit_order;

    if (!a_order)
        a_order = a->pretty_name[0] ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2);
    if (!b_order)
        b_order = b->pretty_name[0] ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2);

    return a_order - b_order;
}

void G_EndOfUnitMessage(void)
{
    // [Paril-KEX] update game level entry
    G_UpdateLevelEntry();

    // sort entries
    qsort(game.level_entries, MAX_LEVELS_PER_UNIT, sizeof(game.level_entries[0]), entrycmp);

    int maxlen = 0;
    for (int i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
        const level_entry_t *entry = &game.level_entries[i];
        if (!entry->map_name[0])
            break;
        maxlen = max(maxlen, strlen(entry->pretty_name));
    }

    sb_begin();
    sb_xv(60 - maxlen * 4);
    sb_yv(26);
    sb_printf("string2 \"%*s   Kills   Secrets  Time  \" ", maxlen, "Level");

    level_entry_t totals = { 0 };
    int num_rows = 0;
    int y = 34;

    for (int i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
        const level_entry_t *entry = &game.level_entries[i];
        if (!entry->map_name[0])
            break;

        G_EndOfUnitEntry(y, entry, maxlen);
        y += 8;

        totals.found_secrets += entry->found_secrets;
        totals.killed_monsters += entry->killed_monsters;
        totals.time += entry->time;
        totals.total_monsters += entry->total_monsters;
        totals.total_secrets += entry->total_secrets;

        if (entry->visit_order)
            num_rows++;
    }

    y += 8; // empty row to separate totals

    // make this a space so it prints totals
    if (num_rows > 1) {
        totals.pretty_name[0] = ' ';
        G_EndOfUnitEntry(y, &totals, maxlen);
    }

    sb_yb(-48), sb_xv(0), sb_cstring2("Press any button to continue.");

    gi.WriteByte(svc_layout);
    gi.WriteString(sb_buffer());
    gi.multicast(vec3_origin, MULTICAST_ALL_R);

    for (int i = 0; i < game.maxclients; i++) {
        if (g_edicts[i + 1].inuse)
            game.clients[i].showeou = true;
    }
}

void BeginIntermission(edict_t *targ)
{
    edict_t *ent, *client;

    if (level.intermissiontime)
        return; // already activated

    // ZOID
    if (ctf->integer)
        CTFCalcScores();
    // ZOID

    game.autosaved = false;

    level.intermissiontime = level.time;

    // respawn any dead clients
    for (int i = 0; i < game.maxclients; i++) {
        client = g_edicts + 1 + i;
        if (!client->inuse)
            continue;
        if (client->health <= 0) {
            // give us our max health back since it will reset
            // to pers.health; in instanced items we'd lose the items
            // we touched so we always want to respawn with our max.
            if (P_UseCoopInstancedItems())
                client->client->pers.health = client->client->pers.max_health = client->max_health;

            respawn(client);
        }
    }

    //level.intermission_server_frame = gi.ServerFrame();
    level.changemap = targ->map;
    level.intermission_clear = targ->spawnflags & SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY;
    level.intermission_eou = false;
    level.intermission_fade = targ->spawnflags & SPAWNFLAG_CHANGELEVEL_FADE_OUT;

    // destroy all player trails
    PlayerTrail_Destroy(NULL);

    // [Paril-KEX] update game level entry
    G_UpdateLevelEntry();

    if (strstr(level.changemap, "*")) {
        if (coop->integer) {
            for (int i = 0; i < game.maxclients; i++) {
                client = g_edicts + 1 + i;
                if (!client->inuse)
                    continue;
                // strip players of all keys between units
                for (int n = 0; n < IT_TOTAL; n++)
                    if (itemlist[n].flags & IF_KEY)
                        client->client->pers.inventory[n] = 0;
            }
        }

        level.intermission_eou = true;

        // "no end of unit" maps handle intermission differently
        if (!(targ->spawnflags & SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
            G_EndOfUnitMessage();
        else if ((targ->spawnflags & SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
            level.exitintermission = true; // go immediately to the next level
            return;
        }
    } else {
        if (!deathmatch->integer) {
            level.exitintermission = true; // go immediately to the next level
            return;
        }
    }

    level.exitintermission = false;

    if (!level.level_intermission_set) {
        // find an intermission spot
        ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
        if (!ent) {
            // the map creator forgot to put in an intermission point...
            ent = G_Find(NULL, FOFS(classname), "info_player_start");
            if (!ent)
                ent = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
        } else {
            // choose one of four spots
            int i = irandom1(4);
            while (i--) {
                ent = G_Find(ent, FOFS(classname), "info_player_intermission");
                if (!ent) // wrap around the list
                    ent = G_Find(ent, FOFS(classname), "info_player_intermission");
            }
        }

        if (ent) {
            VectorCopy(ent->s.origin, level.intermission_origin);
            VectorCopy(ent->s.angles, level.intermission_angle);
        }
    }

    // move all clients to the intermission point
    for (int i = 0; i < game.maxclients; i++) {
        client = g_edicts + 1 + i;
        if (!client->inuse)
            continue;
        MoveClientToIntermission(client);
    }
}

#define MAX_SCOREBOARD_SIZE 1024

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(edict_t *ent, edict_t *killer)
{
    char        entry[1024];
    char        string[1400];
    int         stringlength;
    int         i, j, k;
    int         sorted[MAX_CLIENTS];
    int         sortedscores[MAX_CLIENTS];
    int         score, total;
    int         x, y;
    gclient_t  *cl;
    edict_t    *cl_ent;
    const char *tag;

    // ZOID
    if (G_TeamplayEnabled()) {
        CTFScoreboardMessage(ent, killer);
        return;
    }
    // ZOID

    // sort the clients by score
    total = 0;
    for (i = 0; i < game.maxclients; i++) {
        cl_ent = g_edicts + 1 + i;
        if (!cl_ent->inuse || game.clients[i].resp.spectator)
            continue;
        score = game.clients[i].resp.score;
        for (j = 0; j < total; j++) {
            if (score > sortedscores[j])
                break;
        }
        for (k = total; k > j; k--) {
            sorted[k] = sorted[k - 1];
            sortedscores[k] = sortedscores[k - 1];
        }
        sorted[j] = i;
        sortedscores[j] = score;
        total++;
    }

    string[0] = 0;
    stringlength = 0;

    // add the clients in sorted order
    if (total > 16)
        total = 16;

    for (i = 0; i < total; i++) {
        cl = &game.clients[sorted[i]];
        cl_ent = g_edicts + 1 + sorted[i];

        x = (i >= 8) ? 130 : -72;
        y = 0 + 32 * (i % 8);

        // add a dogtag
        if (cl_ent == ent)
            tag = "tag1";
        else if (cl_ent == killer)
            tag = "tag2";
        else
            tag = NULL;

        //===============
        // ROGUE
        // allow new DM games to override the tag picture
        if (gamerules->integer) {
            if (DMGame.DogTag)
                DMGame.DogTag(cl_ent, killer, &tag);
        }
        // ROGUE
        //===============

        if (tag) {
            Q_snprintf(entry, sizeof(entry),
                       "xv %i yv %i picn %s ", x + 32, y, tag);
            j = strlen(entry);
            if (stringlength + j > MAX_SCOREBOARD_SIZE)
                break;
            strcpy(string + stringlength, entry);
            stringlength += j;
        }

        // send the layout
        Q_snprintf(entry, sizeof(entry),
                   "client %i %i %i %i %i %.f ",
                   x, y, sorted[i], cl->resp.score, cl->ping, TO_SEC(level.time - cl->resp.entertime) / 60);
        j = strlen(entry);
        if (stringlength + j > MAX_SCOREBOARD_SIZE)
            break;
        strcpy(string + stringlength, entry);
        stringlength += j;
    }

    gi.WriteByte(svc_layout);
    gi.WriteString(string);
}

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard(edict_t *ent)
{
    DeathmatchScoreboardMessage(ent, ent->enemy);
    gi.unicast(ent, true);
    ent->client->menutime = level.time + SEC(3);
}

/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f(edict_t *ent)
{
    if (level.intermissiontime)
        return;

    ent->client->showinventory = false;
    ent->client->showhelp = false;

    // ZOID
    if (ent->client->menu)
        PMenu_Close(ent);
    // ZOID

    if (!deathmatch->integer && !coop->integer)
        return;

    if (ent->client->showscores) {
        ent->client->showscores = false;
        ent->client->update_chase = true;
        return;
    }

    ent->client->showscores = true;
    DeathmatchScoreboard(ent);
}

/*
==================
HelpComputer

Draw help computer.
==================
*/
static void HelpComputer(edict_t *ent)
{
    const char *sk;

    if (skill->integer == 0)
        sk = "Easy";
    else if (skill->integer == 1)
        sk = "Medium";
    else if (skill->integer == 2)
        sk = "Hard";
    else
        sk = "Nightmare";

    // send the layout
    sb_begin();
    sb_xv(32), sb_yv(8), sb_picn("help"), sb_xv(0), sb_yv(25), sb_cstring2(level.level_name);

    if (level.is_n64) {
        sb_xv(0), sb_yv(54), sb_cstring(game.helpmessage1);
    } else {
        const char *first_message = game.helpmessage1;
        const char *first_title = level.primary_objective_title;

        const char *second_message = game.helpmessage2;
        const char *second_title = level.secondary_objective_title;

        if (level.is_psx) {
            SWAP(const char *, first_message, second_message);
            SWAP(const char *, first_title, second_title);
        }

        int y = 54;
        if (first_message[0]) {
            sb_xv(0), sb_yv(y), sb_cstring2(first_title), sb_yv(y + 11), sb_cstring(first_message);
            y += 58;
        }

        if (second_message[0]) {
            sb_xv(0), sb_yv(y), sb_cstring2(second_title), sb_yv(y + 11), sb_cstring(second_message);
        }
    }

    sb_xv(55), sb_yv(164), sb_string2(sk);
    sb_xv(55), sb_yv(172), sb_string2(va("Kills: %d/%d", level.killed_monsters, level.total_monsters));
    sb_xv(265), sb_yv(164), sb_rstring2(va("Goals: %d/%d", level.found_goals, level.total_goals));
    sb_xv(265), sb_yv(172), sb_rstring2(va("Secrets: %d/%d", level.found_secrets, level.total_secrets));

    gi.WriteByte(svc_layout);
    gi.WriteString(sb_buffer());
    gi.unicast(ent, true);
}

/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f(edict_t *ent)
{
    // this is for backwards compatability
    if (deathmatch->integer) {
        Cmd_Score_f(ent);
        return;
    }

    if (level.intermissiontime)
        return;

    ent->client->showinventory = false;
    ent->client->showscores = false;

    if (ent->client->showhelp &&
        (ent->client->pers.game_help1changed == game.help1changed ||
         ent->client->pers.game_help2changed == game.help2changed)) {
        ent->client->showhelp = false;
        return;
    }

    ent->client->showhelp = true;
    ent->client->pers.helpchanged = 0;
    HelpComputer(ent);
}

//=======================================================================

// [Paril-KEX] for stats we want to always be set in coop
// even if we're spectating
void G_SetCoopStats(edict_t *ent)
{
    if (!coop->integer)
        return;

    if (g_coop_enable_lives->integer)
        ent->client->ps.stats[STAT_LIVES] = ent->client->pers.lives + 1;
    else
        ent->client->ps.stats[STAT_LIVES] = 0;

    // stat for text on what we're doing for respawn
    if (ent->client->coop_respawn_state)
        ent->client->ps.stats[STAT_COOP_RESPAWN] = CONFIG_COOP_RESPAWN_STRING + (ent->client->coop_respawn_state - COOP_RESPAWN_IN_COMBAT);
    else
        ent->client->ps.stats[STAT_COOP_RESPAWN] = 0;
}

static int G_EncodeHealthBar(int bar)
{
    edict_t *ent = level.health_bar_entities[bar];

    if (!ent)
        return 0;

    if (!ent->inuse) {
        level.health_bar_entities[bar] = NULL;
        return 0;
    }

    if (ent->timestamp) {
        if (ent->timestamp < level.time) {
            level.health_bar_entities[bar] = NULL;
            return 0;
        }
        return 1;
    }

    Q_assert(ent->enemy);

    // enemy dead
    if (!ent->enemy->inuse || ent->enemy->health <= 0) {
        // hack for Makron
        if (ent->enemy->monsterinfo.aiflags & AI_DOUBLE_TROUBLE)
            return 1;

        if (ent->delay) {
            ent->timestamp = level.time + SEC(ent->delay);
            return 1;
        }

        level.health_bar_entities[bar] = NULL;
        return 0;
    }

    if (ent->spawnflags & SPAWNFLAG_HEALTHBAR_PVS_ONLY && !gi.inPVS(ent->s.origin, ent->enemy->s.origin))
        return 0;

    float percent = (float)ent->enemy->health / ent->enemy->max_health;
    return 1 + Q_clip(percent * 254 + 0.5f, 1, 254);
}

typedef struct {
    item_id_t item;
    int ofs;
} powerup_info_t;

static const powerup_info_t powerup_table[] = {
    { IT_ITEM_QUAD, CLOFS(quad_time) },
    { IT_ITEM_QUADFIRE, CLOFS(quadfire_time) },
    { IT_ITEM_DOUBLE, CLOFS(double_time) },
    { IT_ITEM_INVULNERABILITY, CLOFS(invincible_time) },
    { IT_ITEM_INVISIBILITY, CLOFS(invisible_time) },
    { IT_ITEM_ENVIROSUIT, CLOFS(enviro_time) },
    { IT_ITEM_REBREATHER, CLOFS(breather_time) },
    { IT_ITEM_IR_GOGGLES, CLOFS(ir_time) },
};

/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t *ent)
{
    const gitem_t *item;
    item_id_t index;
    int       cells = 0;
    item_id_t power_armor_type;

    //
    // health
    //
    if (ent->s.renderfx & RF_USE_DISGUISE)
        ent->client->ps.stats[STAT_HEALTH_ICON] = level.disguise_icon;
    else
        ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
    ent->client->ps.stats[STAT_HEALTH] = ent->health;

    //
    // ammo
    //
    ent->client->ps.stats[STAT_AMMO_ICON] = 0;
    ent->client->ps.stats[STAT_AMMO] = 0;

    if (ent->client->pers.weapon && ent->client->pers.weapon->ammo) {
        item = GetItemByIndex(ent->client->pers.weapon->ammo);

        if (!G_CheckInfiniteAmmo(item)) {
            ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
            ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->pers.weapon->ammo];
        }
    }

    //
    // armor
    //
    power_armor_type = PowerArmorType(ent);
    if (power_armor_type)
        cells = ent->client->pers.inventory[IT_AMMO_CELLS];

    index = ArmorIndex(ent);
    if (power_armor_type && (!index || (TO_MSEC(level.time) % 3000) < 1500)) {
        // flash between power armor and other armor icon
        ent->client->ps.stats[STAT_ARMOR_ICON] = power_armor_type == IT_ITEM_POWER_SHIELD ? gi.imageindex("i_powershield") : gi.imageindex("i_powerscreen");
        ent->client->ps.stats[STAT_ARMOR] = cells;
    } else if (index) {
        item = GetItemByIndex(index);
        ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(item->icon);
        ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
    } else {
        ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
        ent->client->ps.stats[STAT_ARMOR] = 0;
    }

    //
    // pickup message
    //
    if (level.time > ent->client->pickup_msg_time) {
        ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
        ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
    }

    ent->client->ps.stats[STAT_TIMER_ICON] = 0;
    ent->client->ps.stats[STAT_TIMER] = 0;

    //
    // timers
    //
    // PGM
    if (ent->client->owned_sphere) {
        if (ent->client->owned_sphere->spawnflags == SPHERE_DEFENDER) // defender
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_defender");
        else if (ent->client->owned_sphere->spawnflags == SPHERE_HUNTER) // hunter
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_hunter");
        else if (ent->client->owned_sphere->spawnflags == SPHERE_VENGEANCE) // vengeance
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_vengeance");
        else // error case
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("i_fixme");

        ent->client->ps.stats[STAT_TIMER] = ceilf(TO_SEC(ent->client->owned_sphere->timestamp - level.time));
    } else {
        const powerup_info_t *best_powerup = NULL;
        gtime_t best_time = INT_MAX;

        for (int i = 0; i < q_countof(powerup_table); i++) {
            const powerup_info_t *powerup = &powerup_table[i];
            gtime_t powerup_time = *(gtime_t *)((byte *)ent->client + powerup->ofs);

            if (powerup_time <= level.time)
                continue;

            if (powerup_time < best_time) {
                best_powerup = powerup;
                best_time = powerup_time;
            }
        }

        if (best_powerup) {
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex(GetItemByIndex(best_powerup->item)->icon);
            ent->client->ps.stats[STAT_TIMER] = ceilf(TO_SEC(best_time - level.time));
        } else if (ent->client->silencer_shots) {
            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex(GetItemByIndex(IT_ITEM_SILENCER)->icon);
            ent->client->ps.stats[STAT_TIMER] = ent->client->silencer_shots;
        }
    }
    // PGM

    //
    // selected item
    //
    ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

    if (ent->client->pers.selected_item == IT_NULL)
        ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
    else {
        ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(itemlist[ent->client->pers.selected_item].icon);

        if (ent->client->pers.selected_item_time < level.time)
            ent->client->ps.stats[STAT_SELECTED_ITEM_NAME] = 0;
    }

    //
    // layouts
    //
    ent->client->ps.stats[STAT_LAYOUTS] = 0;

    if (deathmatch->integer) {
        if (ent->client->pers.health <= 0 || level.intermissiontime || ent->client->showscores)
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
        if (ent->client->showinventory && ent->client->pers.health > 0)
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;
    } else {
        if (ent->client->showscores || ent->client->showhelp || ent->client->showeou)
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
        if (ent->client->showinventory && ent->client->pers.health > 0)
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

        if (ent->client->showhelp)
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HELP;
    }

    if (level.intermissiontime || ent->client->awaiting_respawn) {
        if (ent->client->awaiting_respawn || (level.intermission_eou || level.is_n64 || (deathmatch->integer && level.intermissiontime)))
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_HUD;

        // N64 always merges into one screen on level ends
        if (level.intermission_eou || level.is_n64 || (deathmatch->integer && level.intermissiontime))
            ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INTERMISSION;
    }

    if (level.story_active)
        ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT | LAYOUTS_HIDE_CROSSHAIR;

    // [Paril-KEX] key display
    if (!deathmatch->integer) {
        int key_offset = 0;

        ent->client->ps.stats[STAT_KEY_A] =
        ent->client->ps.stats[STAT_KEY_B] =
        ent->client->ps.stats[STAT_KEY_C] = 0;

        // there's probably a way to do this in one pass but
        // I'm lazy
        item_id_t keys_held[IT_TOTAL];
        int num_keys_held = 0;

        for (int i = IT_NULL; i < IT_TOTAL; i++) {
            item = &itemlist[i];
            if (!(item->flags & IF_KEY))
                continue;
            if (!ent->client->pers.inventory[item->id])
                continue;
            keys_held[num_keys_held++] = item->id;
        }

        if (num_keys_held > 3)
            key_offset = TO_SEC(level.time) / 5;

        for (int i = 0; i < min(num_keys_held, 3); i++)
            ent->client->ps.stats[STAT_KEY_A + i] = gi.imageindex(GetItemByIndex(keys_held[(i + key_offset) % num_keys_held])->icon);
    }

    //
    // frags
    //
    ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

    //
    // help icon / current weapon if not shown
    //
    if (ent->client->pers.helpchanged >= 1 && ent->client->pers.helpchanged <= 2 && (TO_MSEC(level.time) % 1000) < 500) // haleyjd: time-limited
        ent->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
    else if ((ent->client->pers.hand == CENTER_HANDED) && ent->client->pers.weapon)
        ent->client->ps.stats[STAT_HELPICON] = gi.imageindex(ent->client->pers.weapon->icon);
    else
        ent->client->ps.stats[STAT_HELPICON] = 0;

    ent->client->ps.stats[STAT_SPECTATOR] = 0;

    // set & run the health bar stuff
    ent->client->ps.stats[STAT_HEALTH_BARS] = G_EncodeHealthBar(0) | (G_EncodeHealthBar(1) << 8);

    // ZOID
    SetCTFStats(ent);
    // ZOID
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats(edict_t *ent)
{
    gclient_t *cl;

    for (int i = 1; i <= game.maxclients; i++) {
        cl = g_edicts[i].client;
        if (!g_edicts[i].inuse || cl->chase_target != ent)
            continue;
        memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
        G_SetSpectatorStats(g_edicts + i);
    }
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats(edict_t *ent)
{
    gclient_t *cl = ent->client;

    if (!cl->chase_target)
        G_SetStats(ent);

    cl->ps.stats[STAT_SPECTATOR] = 1;

    // layouts are independant in spectator
    cl->ps.stats[STAT_LAYOUTS] = 0;
    if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
        cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
    if (cl->showinventory && cl->pers.health > 0)
        cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

    if (cl->chase_target && cl->chase_target->inuse)
        cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + (cl->chase_target - g_edicts) - 1;
    else
        cl->ps.stats[STAT_CHASE] = 0;
}
