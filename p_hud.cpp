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
    if (deathmatch->integer)
        ent->client->showscores = true;
    ent->s.origin = level.intermission_origin;
    for (int i = 0; i < 3; i++)
        ent->client->ps.pmove.origin[i] = COORD2SHORT(level.intermission_origin[i]);
    ent->client->ps.viewangles = level.intermission_angle;
    ent->client->ps.pmove.pm_type = PM_FREEZE;
    ent->client->ps.gunindex = 0;
    /*ent->client->ps.damage_blend[3] =*/ ent->client->ps.screen_blend[3] = 0;
    ent->client->ps.rdflags = RDF_NONE;

    // clean up powerup info
    ent->client->quad_time = 0_ms;
    ent->client->invincible_time = 0_ms;
    ent->client->breather_time = 0_ms;
    ent->client->enviro_time = 0_ms;
    ent->client->invisible_time = 0_ms;
    ent->client->grenade_blew_up = false;
    ent->client->grenade_time = 0_ms;

    ent->client->showhelp = false;
    ent->client->showscores = false;

    // RAFAEL
    ent->client->quadfire_time = 0_ms;
    // RAFAEL
    // ROGUE
    ent->client->ir_time = 0_ms;
    ent->client->nuke_time = 0_ms;
    ent->client->double_time = 0_ms;
    ent->client->tracker_pain_time = 0_ms;
    // ROGUE

    ent->viewheight = 0;
    ent->s.modelindex = 0;
    ent->s.modelindex2 = 0;
    ent->s.modelindex3 = 0;
    ent->s.modelindex = 0;
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
void G_UpdateLevelEntry()
{
    if (!level.entry)
        return;

    level.entry->found_secrets = level.found_secrets;
    level.entry->total_secrets = level.total_secrets;
    level.entry->killed_monsters = level.killed_monsters;
    level.entry->total_monsters = level.total_monsters;
}

inline void G_EndOfUnitEntry(statusbar_t &sb, const int y, const level_entry_t &entry, const size_t maxlen)
{
    sb.yv(y);

    // we didn't visit this level, so print it as an unknown entry
    if (!*entry.pretty_name) {
        sb.string("???");
        return;
    }

    int32_t minutes = entry.time.milliseconds() / 60000;
    int32_t seconds = (entry.time.milliseconds() / 1000) % 60;
    int32_t tensofsec = (entry.time.milliseconds() / 100) % 10;

    sb.string(G_Fmt("{:{}} {:4}/{:<4} {:3}/{:<3} {:02}:{:02}.{}",
                    entry.pretty_name, maxlen,
                    entry.killed_monsters, entry.total_monsters,
                    entry.found_secrets, entry.total_secrets,
                    minutes, seconds, tensofsec).data());
}

void G_EndOfUnitMessage()
{
    // [Paril-KEX] update game level entry
    G_UpdateLevelEntry();

    statusbar_t sb;

    // sort entries
    std::sort(game.level_entries.begin(), game.level_entries.end(), [](const level_entry_t &a, const level_entry_t &b) {
        int32_t a_order = a.visit_order ? a.visit_order : (*a.pretty_name ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2));
        int32_t b_order = b.visit_order ? b.visit_order : (*b.pretty_name ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2));

        return a_order < b_order;
    });

    size_t maxlen = 0;
    for (auto &entry : game.level_entries) {
        if (!*entry.map_name)
            break;
        maxlen = std::max(maxlen, strlen(entry.pretty_name));
    }

    sb.xv(60 - maxlen * 4).yv(26).string2(G_Fmt("{:{}}   Kills   Secrets  Time  ", "Level", maxlen).data());

    int y = 34;
    level_entry_t totals {};
    int32_t num_rows = 0;

    for (auto &entry : game.level_entries) {
        if (!*entry.map_name)
            break;

        G_EndOfUnitEntry(sb, y, entry, maxlen);
        y += 8;

        totals.found_secrets += entry.found_secrets;
        totals.killed_monsters += entry.killed_monsters;
        totals.time += entry.time;
        totals.total_monsters += entry.total_monsters;
        totals.total_secrets += entry.total_secrets;

        if (entry.visit_order)
            num_rows++;
    }

    y += 8; // empty row to separate totals

    // make this a space so it prints totals
    if (num_rows > 1) {
        totals.pretty_name[0] = ' ';
        G_EndOfUnitEntry(sb, y, totals, maxlen);
    }

    sb.yb(-48).xv(0).cstring2("Press any button to continue.");

    gi.WriteByte(svc_layout);
    gi.WriteString(sb.sb.str().c_str());
    gi.multicast(vec3_origin, MULTICAST_ALL_R);

    for (auto player : active_players())
        player->client->showeou = true;
}

// data is binary now.
// u8 num_teams
// u8 num_players
// [ repeat num_teams:
//   string team_name
// ]
// [ repeat num_players:
//   u8 client_index
//   s32 score
//   u8 ranking
//   (if num_teams > 0)
//     u8 team
// ]
void G_ReportMatchDetails(bool is_end)
{
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

    // respawn any dead clients
    for (uint32_t i = 0; i < game.maxclients; i++) {
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

    level.intermissiontime = level.time;
    //level.intermission_server_frame = gi.ServerFrame();
    level.changemap = targ->map;
    level.intermission_clear = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY);
    level.intermission_eou = false;
    level.intermission_fade = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_FADE_OUT);

    // destroy all player trails
    PlayerTrail_Destroy(nullptr);

    // [Paril-KEX] update game level entry
    G_UpdateLevelEntry();

    if (strstr(level.changemap, "*")) {
        if (coop->integer) {
            for (uint32_t i = 0; i < game.maxclients; i++) {
                client = g_edicts + 1 + i;
                if (!client->inuse)
                    continue;
                // strip players of all keys between units
                for (uint32_t n = 0; n < IT_TOTAL; n++)
                    if (itemlist[n].flags & IF_KEY)
                        client->client->pers.inventory[n] = 0;
            }
        }
#if 0
        if (level.achievement && level.achievement[0]) {
            gi.WriteByte(svc_achievement);
            gi.WriteString(level.achievement);
            gi.multicast(vec3_origin, MULTICAST_ALL_R);
        }
#endif

        level.intermission_eou = true;

        // "no end of unit" maps handle intermission differently
        if (!targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
            G_EndOfUnitMessage();
        else if (targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
            // Need to call this now
            G_ReportMatchDetails(true);
            level.exitintermission = 1; // go immediately to the next level
            return;
        }
    } else {
        if (!deathmatch->integer) {
            level.exitintermission = 1; // go immediately to the next level
            return;
        }
    }

    // Call while intermission is running
    G_ReportMatchDetails(true);

    level.exitintermission = 0;

    if (!level.level_intermission_set) {
        // find an intermission spot
        ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_intermission");
        if (!ent) {
            // the map creator forgot to put in an intermission point...
            ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_start");
            if (!ent)
                ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_deathmatch");
        } else {
            // choose one of four spots
            int32_t i = irandom(4);
            while (i--) {
                ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
                if (!ent) // wrap around the list
                    ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
            }
        }

        level.intermission_origin = ent->s.origin;
        level.intermission_angle = ent->s.angles;
    }

    // move all clients to the intermission point
    for (uint32_t i = 0; i < game.maxclients; i++) {
        client = g_edicts + 1 + i;
        if (!client->inuse)
            continue;
        MoveClientToIntermission(client);
    }
}

constexpr size_t MAX_SCOREBOARD_SIZE = 1024;

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(edict_t *ent, edict_t *killer)
{
    static std::string entry, string;
    size_t      j;
    int         sorted[MAX_CLIENTS];
    int         sortedscores[MAX_CLIENTS];
    int         score;
    int         x, y;
    gclient_t  *cl;
    edict_t *cl_ent;
    const char *tag;

    // ZOID
    if (G_TeamplayEnabled()) {
        CTFScoreboardMessage(ent, killer);
        return;
    }
    // ZOID

    entry.clear();
    string.clear();

    //  sort the clients by score
    uint32_t total = 0;
    for (uint32_t i = 0; i < game.maxclients; i++) {
        cl_ent = g_edicts + 1 + i;
        if (!cl_ent->inuse || game.clients[i].resp.spectator)
            continue;
        score = game.clients[i].resp.score;
        for (j = 0; j < total; j++) {
            if (score > sortedscores[j])
                break;
        }
        for (uint32_t k = total; k > j; k--) {
            sorted[k] = sorted[k - 1];
            sortedscores[k] = sortedscores[k - 1];
        }
        sorted[j] = i;
        sortedscores[j] = score;
        total++;
    }

    // add the clients in sorted order
    if (total > 16)
        total = 16;

    for (uint32_t i = 0; i < total; i++) {
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
            tag = nullptr;

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
            fmt::format_to(std::back_inserter(entry), FMT_STRING("xv {} yv {} picn {} "), x + 32, y, tag);

            if (string.length() + entry.length() > MAX_SCOREBOARD_SIZE)
                break;

            string += entry;
        }

        entry.clear();

        fmt::format_to(std::back_inserter(entry),
                       FMT_STRING("client {} {} {} {} {} {} "),
                       x, y, sorted[i], cl->resp.score, cl->ping, (int32_t)(level.time - cl->resp.entertime).minutes());

        if (string.length() + entry.length() > MAX_SCOREBOARD_SIZE)
            break;

        string += entry;

        entry.clear();
    }

#if 0
    // [Paril-KEX] time & frags
    if (fraglimit->integer) {
        fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 string2 \"Frag Limit: {}\" "), fraglimit->integer);
    }
    if (timelimit->value && !level.intermissiontime) {
        //fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 time_limit {} "), gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / FRAME_TIME.milliseconds());
    }

    if (level.intermissiontime)
        fmt::format_to(std::back_inserter(string), FMT_STRING("ifgef {} yb -48 xv 0 cstring2 \"Press any button to continue.\" endif "), (level.intermission_server_frame + (5_sec).frames()));
#endif

    gi.WriteByte(svc_layout);
    gi.WriteString(string.c_str());
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
    ent->client->menutime = level.time + 3_sec;
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
void HelpComputer(edict_t *ent)
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
    statusbar_t sb;

    sb.xv(32).yv(8).picn("help").xv(0).yv(25).cstring2(level.level_name);

    if (level.is_n64) {
        sb.xv(0).yv(54).cstring(game.helpmessage1);
    } else {
        int y = 54;
        if (strlen(game.helpmessage1)) {
            sb.xv(0).yv(y).cstring2("Primary Objective").xv(0).yv(y + 11).cstring(game.helpmessage1);
            y += 58;
        }

        if (strlen(game.helpmessage2)) {
            sb.xv(0).yv(y).cstring2("Secondary Objective").xv(0).yv(y + 11).cstring(game.helpmessage2);
        }
    }

    sb.xv(55).yv(164).string2(sk);
    sb.xv(55).yv(172).string2(G_Fmt("Kills: {}/{}", level.killed_monsters, level.total_monsters).data());
    sb.xv(265).yv(164).rstring2(G_Fmt("Goals: {}/{}", level.found_goals, level.total_goals).data());
    sb.xv(265).yv(172).rstring2(G_Fmt("Secrets: {}/{}", level.found_secrets, level.total_secrets).data());

    gi.WriteByte(svc_layout);
    gi.WriteString(sb.sb.str().c_str());
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

struct powerup_info_t {
    item_id_t item;
    gtime_t gclient_t::*time_ptr = nullptr;
    int32_t gclient_t::*count_ptr = nullptr;
} powerup_table[] = {
    { IT_ITEM_QUAD, &gclient_t::quad_time },
    { IT_ITEM_QUADFIRE, &gclient_t::quadfire_time },
    { IT_ITEM_DOUBLE, &gclient_t::double_time },
    { IT_ITEM_INVULNERABILITY, &gclient_t::invincible_time },
    { IT_ITEM_INVISIBILITY, &gclient_t::invisible_time },
    { IT_ITEM_ENVIROSUIT, &gclient_t::enviro_time },
    { IT_ITEM_REBREATHER, &gclient_t::breather_time },
    { IT_ITEM_IR_GOGGLES, &gclient_t::ir_time },
    { IT_ITEM_SILENCER, nullptr, &gclient_t::silencer_shots }
};

/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t *ent)
{
    gitem_t *item;
    item_id_t index;
    int       cells = 0;
    item_id_t power_armor_type;
    unsigned int invIndex;

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
    if (power_armor_type && (!index || (level.time.milliseconds() % 3000) < 1500)) {
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

        ent->client->ps.stats[STAT_TIMER] = ceil(ent->client->owned_sphere->wait - level.time.seconds());
    } else {
        powerup_info_t *best_powerup = nullptr;

        for (auto &powerup : powerup_table) {
            auto *powerup_time = powerup.time_ptr ? &(ent->client->*powerup.time_ptr) : nullptr;
            auto *powerup_count = powerup.count_ptr ? &(ent->client->*powerup.count_ptr) : nullptr;

            if (powerup_time && *powerup_time <= level.time)
                continue;
            else if (powerup_count && !*powerup_count)
                continue;

            if (!best_powerup) {
                best_powerup = &powerup;
                continue;
            }

            if (powerup_time && *powerup_time < ent->client->*best_powerup->time_ptr) {
                best_powerup = &powerup;
                continue;
            } else if (powerup_count && !best_powerup->time_ptr) {
                best_powerup = &powerup;
                continue;
            }
        }

        if (best_powerup) {
            int16_t value;

            if (best_powerup->count_ptr)
                value = (ent->client->*best_powerup->count_ptr);
            else
                value = ceil((ent->client->*best_powerup->time_ptr - level.time).seconds());

            ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex(GetItemByIndex(best_powerup->item)->icon);
            ent->client->ps.stats[STAT_TIMER] = value;
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
        int32_t key_offset = 0;
        player_stat_t stat = STAT_KEY_A;

        ent->client->ps.stats[STAT_KEY_A] =
        ent->client->ps.stats[STAT_KEY_B] =
        ent->client->ps.stats[STAT_KEY_C] = 0;

        // there's probably a way to do this in one pass but
        // I'm lazy
        std::array<item_id_t, IT_TOTAL> keys_held;
        size_t num_keys_held = 0;

        for (auto &item : itemlist) {
            if (!(item.flags & IF_KEY))
                continue;
            else if (!ent->client->pers.inventory[item.id])
                continue;

            keys_held[num_keys_held++] = item.id;
        }

        if (num_keys_held > 3)
            key_offset = (int32_t)(level.time.seconds() / 5);

        for (int32_t i = 0; i < min(num_keys_held, (size_t) 3); i++, stat = (player_stat_t)(stat + 1))
            ent->client->ps.stats[stat] = gi.imageindex(GetItemByIndex(keys_held[(i + key_offset) % num_keys_held])->icon);
    }

    //
    // frags
    //
    ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

    //
    // help icon / current weapon if not shown
    //
    if (ent->client->pers.helpchanged >= 1 && ent->client->pers.helpchanged <= 2 && (level.time.milliseconds() % 1000) < 500) // haleyjd: time-limited
        ent->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
    else if ((ent->client->pers.hand == CENTER_HANDED) && ent->client->pers.weapon)
        ent->client->ps.stats[STAT_HELPICON] = gi.imageindex(ent->client->pers.weapon->icon);
    else
        ent->client->ps.stats[STAT_HELPICON] = 0;

    ent->client->ps.stats[STAT_SPECTATOR] = 0;

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

    for (uint32_t i = 1; i <= game.maxclients; i++) {
        cl = g_edicts[i].client;
        if (!g_edicts[i].inuse || cl->chase_target != ent)
            continue;
        cl->ps.stats = ent->client->ps.stats;
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
        cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS +
                                   (cl->chase_target - g_edicts) - 1;
    else
        cl->ps.stats[STAT_CHASE] = 0;
}
