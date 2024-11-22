// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

game_locals_t  game;
level_locals_t level;

game_import_t       gi;
game_import_ex_t    gix;

filesystem_api_v1_t *fs;
debug_draw_api_v1_t *draw;

bool use_psx_assets;

game_export_t  globals;
spawn_temp_t   st;

const trace_t null_trace;

edict_t *g_edicts;

cvar_t *developer;
cvar_t *deathmatch;
cvar_t *coop;
cvar_t *skill;
cvar_t *fraglimit;
cvar_t *timelimit;
// ZOID
cvar_t *capturelimit;
cvar_t *g_quick_weapon_switch;
cvar_t *g_instant_weapon_switch;
// ZOID
cvar_t      *password;
cvar_t      *spectator_password;
cvar_t      *needpass;
static cvar_t *maxclients;
cvar_t      *maxspectators;
static cvar_t *maxentities;
cvar_t      *g_select_empty;
cvar_t      *sv_dedicated;
cvar_t      *sv_running;

cvar_t *filterban;

cvar_t *sv_maxvelocity;
cvar_t *sv_gravity;

cvar_t *g_skipViewModifiers;

cvar_t *sv_rollspeed;
cvar_t *sv_rollangle;
cvar_t *gun_x;
cvar_t *gun_y;
cvar_t *gun_z;

cvar_t *run_pitch;
cvar_t *run_roll;
cvar_t *bob_up;
cvar_t *bob_pitch;
cvar_t *bob_roll;

cvar_t *sv_cheats;

cvar_t *g_debug_monster_paths;
cvar_t *g_debug_monster_kills;

cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;

cvar_t *sv_stopspeed; // PGM     (this was a define in g_phys.c)

cvar_t *g_strict_saves;

// ROGUE cvars
cvar_t *gamerules;
cvar_t *huntercam;
cvar_t *g_dm_strong_mines;
cvar_t *g_dm_random_items;
// ROGUE

// [Kex]
cvar_t *g_instagib;
cvar_t *g_coop_player_collision;
cvar_t *g_coop_squad_respawn;
cvar_t *g_coop_enable_lives;
cvar_t *g_coop_num_lives;
cvar_t *g_coop_instanced_items;
cvar_t *g_allow_grapple;
cvar_t *g_grapple_fly_speed;
cvar_t *g_grapple_pull_speed;
cvar_t *g_grapple_damage;
cvar_t *g_coop_health_scaling;
cvar_t *g_weapon_respawn_time;

// dm"flags"
cvar_t *g_no_health;
cvar_t *g_no_items;
cvar_t *g_dm_weapons_stay;
cvar_t *g_dm_no_fall_damage;
cvar_t *g_dm_instant_items;
cvar_t *g_dm_same_level;
cvar_t *g_friendly_fire;
cvar_t *g_dm_force_respawn;
cvar_t *g_dm_force_respawn_time;
cvar_t *g_dm_spawn_farthest;
cvar_t *g_no_armor;
cvar_t *g_dm_allow_exit;
cvar_t *g_infinite_ammo;
cvar_t *g_dm_no_quad_drop;
cvar_t *g_dm_no_quadfire_drop;
cvar_t *g_no_mines;
cvar_t *g_dm_no_stack_double;
cvar_t *g_no_nukes;
cvar_t *g_no_spheres;
cvar_t *g_teamplay_armor_protect;
cvar_t *g_allow_techs;
cvar_t *g_start_items;
cvar_t *g_map_list;
cvar_t *g_map_list_shuffle;

cvar_t *sv_airaccelerate;
cvar_t *g_damage_scale;
cvar_t *g_disable_player_collision;
cvar_t *ai_damage_scale;
cvar_t *ai_model_scale;
cvar_t *ai_allow_dm_spawn;
cvar_t *ai_movement_disabled;
cvar_t *g_monster_footsteps;

static cvar_t *g_frames_per_frame;

static void G_RunFrame(void);
static void G_PrepFrame(void);

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
static void PreInitGame(void)
{
    developer = gi.cvar("developer", "0", 0);
    maxclients = gi.cvar("maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
    deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
    coop = gi.cvar("coop", "0", CVAR_LATCH);
    teamplay = gi.cvar("teamplay", "0", CVAR_LATCH);

    // ZOID
    CTFInit();
    // ZOID

    // ZOID
    // This gamemode only supports deathmatch
    if (ctf->integer) {
        if (!deathmatch->integer) {
            gi.dprintf("Forcing deathmatch.\n");
            gi.cvar_set("deathmatch", "1");
        }
        // force coop off
        if (coop->integer)
            gi.cvar_set("coop", "0");
        // force tdm off
        if (teamplay->integer)
            gi.cvar_set("teamplay", "0");
    }
    if (teamplay->integer) {
        if (!deathmatch->integer) {
            gi.dprintf("Forcing deathmatch.\n");
            gi.cvar_set("deathmatch", "1");
        }
        // force coop off
        if (coop->integer)
            gi.cvar_set("coop", "0");
    }
    // ZOID
}

/*
============
InitGame

Called after PreInitGame when the game has set up cvars.
============
*/
static void InitGame(void)
{
    gi.dprintf("==== InitGame ====\n");

    cvar_t *cv = gi.cvar("sv_features", NULL, 0);
    if (!cv || !(cv->flags & BIT(7)) || ((int)cv->value & G_FEATURES_REQUIRED) != G_FEATURES_REQUIRED || gix.apiversion < GAME_API_VERSION_EX_MINIMUM)
        gi.error("This game library requires enhanced Q2PRO server");

    gi.cvar_forceset("g_features", va("%d", G_FEATURES));

    PreInitGame();

    // seed RNG
    Q_srand(time(NULL));

    fs = gix.GetExtension(FILESYSTEM_API_V1);
    draw = gix.GetExtension(DEBUG_DRAW_API_V1);

    gun_x = gi.cvar("gun_x", "0", 0);
    gun_y = gi.cvar("gun_y", "0", 0);
    gun_z = gi.cvar("gun_z", "0", 0);

    // FIXME: sv_ prefix is wrong for these
    sv_rollspeed = gi.cvar("sv_rollspeed", "200", 0);
    sv_rollangle = gi.cvar("sv_rollangle", "2", 0);
    sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
    sv_gravity = gi.cvar("sv_gravity", "800", 0);

    g_skipViewModifiers = gi.cvar("g_skipViewModifiers", "0", 0);

    sv_stopspeed = gi.cvar("sv_stopspeed", "100", 0); // PGM - was #define in g_phys.c

    // ROGUE
    huntercam = gi.cvar("huntercam", "1", CVAR_SERVERINFO | CVAR_LATCH);
    g_dm_strong_mines = gi.cvar("g_dm_strong_mines", "0", 0);
    g_dm_random_items = gi.cvar("g_dm_random_items", "0", 0);
    // ROGUE

    // [Kex] Instagib
    g_instagib = gi.cvar("g_instagib", "0", 0);

    // [Paril-KEX]
    g_coop_player_collision = gi.cvar("g_coop_player_collision", "0", CVAR_LATCH);
    g_coop_squad_respawn = gi.cvar("g_coop_squad_respawn", "1", CVAR_LATCH);
    g_coop_enable_lives = gi.cvar("g_coop_enable_lives", "0", CVAR_LATCH);
    g_coop_num_lives = gi.cvar("g_coop_num_lives", "2", CVAR_LATCH);
    g_coop_instanced_items = gi.cvar("g_coop_instanced_items", "1", CVAR_LATCH);
    g_allow_grapple = gi.cvar("g_allow_grapple", "auto", 0);
    g_grapple_fly_speed = gi.cvar("g_grapple_fly_speed", va("%d", CTF_DEFAULT_GRAPPLE_SPEED), 0);
    g_grapple_pull_speed = gi.cvar("g_grapple_pull_speed", va("%d", CTF_DEFAULT_GRAPPLE_PULL_SPEED), 0);
    g_grapple_damage = gi.cvar("g_grapple_damage", "10", 0);

    g_debug_monster_paths = gi.cvar("g_debug_monster_paths", "0", 0);
    g_debug_monster_kills = gi.cvar("g_debug_monster_kills", "0", CVAR_LATCH);

    // noset vars
    sv_dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);
    sv_running = gi.cvar("sv_running", NULL, 0);
    if (!sv_running)
        gi.error("sv_running cvar doesn't exist");

    // latched vars
    sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
    gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);

    maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
    skill = gi.cvar("skill", "1", CVAR_LATCH);
    maxentities = gi.cvar("maxentities", va("%d", MAX_EDICTS), CVAR_LATCH);
    gamerules = gi.cvar("gamerules", "0", CVAR_LATCH); // PGM

    // change anytime vars
    fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
    timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
    // ZOID
    capturelimit = gi.cvar("capturelimit", "0", CVAR_SERVERINFO);
    g_quick_weapon_switch = gi.cvar("g_quick_weapon_switch", "1", CVAR_LATCH);
    g_instant_weapon_switch = gi.cvar("g_instant_weapon_switch", "0", CVAR_LATCH);
    // ZOID
    password = gi.cvar("password", "", CVAR_USERINFO);
    spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
    needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
    filterban = gi.cvar("filterban", "1", 0);

    g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

    run_pitch = gi.cvar("run_pitch", "0.002", 0);
    run_roll = gi.cvar("run_roll", "0.005", 0);
    bob_up = gi.cvar("bob_up", "0.005", 0);
    bob_pitch = gi.cvar("bob_pitch", "0.002", 0);
    bob_roll = gi.cvar("bob_roll", "0.002", 0);

    // flood control
    flood_msgs = gi.cvar("flood_msgs", "4", 0);
    flood_persecond = gi.cvar("flood_persecond", "4", 0);
    flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);

    g_strict_saves = gi.cvar("g_strict_saves", "1", 0);

    sv_airaccelerate = gi.cvar("sv_airaccelerate", "0", 0);

    g_damage_scale = gi.cvar("g_damage_scale", "1", 0);
    g_disable_player_collision = gi.cvar("g_disable_player_collision", "0", 0);
    ai_damage_scale = gi.cvar("ai_damage_scale", "1", 0);
    ai_model_scale = gi.cvar("ai_model_scale", "0", 0);
    ai_allow_dm_spawn = gi.cvar("ai_allow_dm_spawn", "0", 0);
    ai_movement_disabled = gi.cvar("ai_movement_disabled", "0", 0);
    g_monster_footsteps = gi.cvar("g_monster_footsteps", "0", 0);

    g_frames_per_frame = gi.cvar("g_frames_per_frame", "1", 0);

    g_coop_health_scaling = gi.cvar("g_coop_health_scaling", "0", CVAR_LATCH);
    g_weapon_respawn_time = gi.cvar("g_weapon_respawn_time", "30", 0);

    // dm "flags"
    g_no_health = gi.cvar("g_no_health", "0", 0);
    g_no_items = gi.cvar("g_no_items", "0", 0);
    g_dm_weapons_stay = gi.cvar("g_dm_weapons_stay", "0", CVAR_LATCH);
    g_dm_no_fall_damage = gi.cvar("g_dm_no_fall_damage", "0", 0);
    g_dm_instant_items = gi.cvar("g_dm_instant_items", "1", 0);
    g_dm_same_level = gi.cvar("g_dm_same_level", "0", 0);
    g_friendly_fire = gi.cvar("g_friendly_fire", "0", 0);
    g_dm_force_respawn = gi.cvar("g_dm_force_respawn", "0", 0);
    g_dm_force_respawn_time = gi.cvar("g_dm_force_respawn_time", "0", 0);
    g_dm_spawn_farthest = gi.cvar("g_dm_spawn_farthest", "1", 0);
    g_no_armor = gi.cvar("g_no_armor", "0", 0);
    g_dm_allow_exit = gi.cvar("g_dm_allow_exit", "0", 0);
    g_infinite_ammo = gi.cvar("g_infinite_ammo", "0", CVAR_LATCH);
    g_dm_no_quad_drop = gi.cvar("g_dm_no_quad_drop", "0", 0);
    g_dm_no_quadfire_drop = gi.cvar("g_dm_no_quadfire_drop", "0", 0);
    g_no_mines = gi.cvar("g_no_mines", "0", 0);
    g_dm_no_stack_double = gi.cvar("g_dm_no_stack_double", "0", 0);
    g_no_nukes = gi.cvar("g_no_nukes", "0", 0);
    g_no_spheres = gi.cvar("g_no_spheres", "0", 0);
    g_teamplay_force_join = gi.cvar("g_teamplay_force_join", "0", 0);
    g_teamplay_armor_protect = gi.cvar("g_teamplay_armor_protect", "0", 0);
    g_allow_techs = gi.cvar("g_allow_techs", "auto", 0);

    g_start_items = gi.cvar("g_start_items", "", CVAR_LATCH);
    g_map_list = gi.cvar("g_map_list", "", 0);
    g_map_list_shuffle = gi.cvar("g_map_list_shuffle", "0", 0);

    // items
    InitItems();

    // initialize all entities for this game
    game.maxentities = Q_clip(maxentities->integer, maxclients->integer + 1, MAX_EDICTS);
    g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
    globals.edicts = g_edicts;
    globals.max_edicts = game.maxentities;

    // initialize all clients for this game
    game.maxclients = maxclients->integer;
    game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
    globals.num_edicts = game.maxclients + 1;

#if USE_FPS
    // variable FPS support
    if (cv->integer & GMF_VARIABLE_FPS) {
        cv = gi.cvar("sv_fps", NULL, 0);
        if (!cv || !cv->integer)
            gi.error("GMF_VARIABLE_FPS exported but no 'sv_fps' cvar");
        game.tick_rate = cv->integer;
        game.frame_time = 1000 / game.tick_rate;
        game.frame_time_sec = 1.0f / game.tick_rate;
    } else {
        game.tick_rate = BASE_FRAMERATE;
        game.frame_time = BASE_FRAMETIME;
        game.frame_time_sec = BASE_FRAMETIME_1000;
    }
#endif

    //======
    // ROGUE
    if (gamerules->integer)
        InitGameRules(); // if there are game rules to set up, do so now.
    // ROGUE
    //======

    G_LoadL10nFile();

    Nav_Init();

    cv = gi.cvar("game", NULL, 0);
    use_psx_assets = cv && !strncmp(cv->string, "psx", 3);
}

//===================================================================

static void ShutdownGame(void)
{
    gi.dprintf("==== ShutdownGame ====\n");

    memset(&game, 0, sizeof(game));

    gi.FreeTags(TAG_LEVEL);
    gi.FreeTags(TAG_GAME);

    Nav_Shutdown();
    G_FreeL10nFile();
    G_CleanupSaves();
}

static void *G_GetExtension(const char *name)
{
    return NULL;
}

static void G_RestartFilesystem(void)
{
    G_LoadL10nFile();

    Nav_Unload();
    Nav_Load(level.mapname);
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
q_exported game_export_t *GetGameAPI(game_import_t *import)
{
    gi = *import;

    globals.apiversion = GAME_API_VERSION;
    globals.Init = InitGame;
    globals.Shutdown = ShutdownGame;
    globals.SpawnEntities = SpawnEntities;

    globals.WriteGame = WriteGame;
    globals.ReadGame = ReadGame;
    globals.WriteLevel = WriteLevel;
    globals.ReadLevel = ReadLevel;

    globals.ClientThink = ClientThink;
    globals.ClientConnect = ClientConnect;
    globals.ClientUserinfoChanged = ClientUserinfoChanged;
    globals.ClientDisconnect = ClientDisconnect;
    globals.ClientBegin = ClientBegin;
    globals.ClientCommand = ClientCommand;

    globals.RunFrame = G_RunFrame;

    globals.ServerCommand = ServerCommand;

    globals.edict_size = sizeof(edict_t);

    return &globals;
}

q_exported const game_export_ex_t *GetGameAPIEx(const game_import_ex_t *import)
{
    memcpy(&gix, import, min(sizeof(gix), import->structsize));

    static const game_export_ex_t gex = {
        .apiversion = GAME_API_VERSION_EX,
        .structsize = sizeof(gex),
        .GetExtension = G_GetExtension,
        .CanSave = G_CanSave,
        .PrepFrame = G_PrepFrame,
        .RestartFilesystem = G_RestartFilesystem,
        .EntityVisibleToClient = G_EntityVisibleToClient,
    };

    return &gex;
}

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
static void ClientEndServerFrames(void)
{
    edict_t *ent;

    // calc the player views now that all pushing
    // and damage has been added
    for (int i = 0; i < game.maxclients; i++) {
        ent = g_edicts + 1 + i;
        if (!ent->inuse || !ent->client)
            continue;
        ClientEndServerFrame(ent);
    }
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
static edict_t *CreateTargetChangeLevel(const char *map)
{
    edict_t *ent;

    ent = G_Spawn();
    ent->classname = "target_changelevel";
    if (map != level.nextmap)
        Q_strlcpy(level.nextmap, map, sizeof(level.nextmap));
    ent->map = level.nextmap;
    return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel(void)
{
    edict_t *ent;

    // stay on same level flag
    if (g_dm_same_level->integer) {
        BeginIntermission(CreateTargetChangeLevel(level.mapname));
        return;
    }

    if (*level.forcemap) {
        BeginIntermission(CreateTargetChangeLevel(level.forcemap));
        return;
    }

    // see if it's in the map list
    if (*g_map_list->string) {
        const char *str = g_map_list->string;
        char first_map[MAX_QPATH];
        char *map;

        first_map[0] = 0;
        while (1) {
            map = COM_Parse(&str);
            if (!*map)
                break;

            if (Q_strcasecmp(map, level.mapname) == 0) {
                // it's in the list, go to the next one
                map = COM_Parse(&str);
                if (!*map) {
                    // end of list, go to first one
                    if (!first_map[0]) { // there isn't a first one, same level
                        BeginIntermission(CreateTargetChangeLevel(level.mapname));
                        return;
                    } else {
#if 0
                        // [Paril-KEX] re-shuffle if necessary
                        if (g_map_list_shuffle->integer) {
                            auto values = str_split(g_map_list->string, ' ');

                            if (values.size() == 1) {
                                // meh
                                BeginIntermission(CreateTargetChangeLevel(level.mapname));
                                return;
                            }

                            std::shuffle(values.begin(), values.end(), mt_rand);

                            // if the current map is the map at the front, push it to the end
                            if (values[0] == level.mapname)
                                std::swap(values[0], values[values.size() - 1]);

                            gi.cvar_forceset("g_map_list", fmt::format("{}", join_strings(values, " ")).data());

                            BeginIntermission(CreateTargetChangeLevel(values[0].c_str()));
                            return;
                        }
#endif

                        BeginIntermission(CreateTargetChangeLevel(first_map));
                        return;
                    }
                } else {
                    BeginIntermission(CreateTargetChangeLevel(map));
                    return;
                }
            }
            if (!first_map[0])
                Q_strlcpy(first_map, map, sizeof(first_map));
        }
    }

    if (level.nextmap[0]) { // go to a specific map
        BeginIntermission(CreateTargetChangeLevel(level.nextmap));
        return;
    }

    // search for a changelevel
    ent = G_Find(NULL, FOFS(classname), "target_changelevel");

    if (!ent) {
        // the map designer didn't include a changelevel,
        // so create a fake ent that goes back to the same level
        BeginIntermission(CreateTargetChangeLevel(level.mapname));
        return;
    }

    BeginIntermission(ent);
}

/*
=================
CheckNeedPass
=================
*/
static void CheckNeedPass(void)
{
    int need;

    // if password or spectator_password has changed, update needpass
    // as needed
    if (password->modified || spectator_password->modified) {
        need = 0;

        if (*password->string && Q_strcasecmp(password->string, "none"))
            need |= 1;
        if (*spectator_password->string && Q_strcasecmp(spectator_password->string, "none"))
            need |= 2;

        gi.cvar_set("needpass", va("%d", need));
        password->modified = spectator_password->modified = qfalse;
    }
}

/*
=================
CheckDMRules
=================
*/
static void CheckDMRules(void)
{
    gclient_t *cl;

    if (level.intermissiontime)
        return;

    if (!deathmatch->integer)
        return;

    // ZOID
    if (ctf->integer && CTFCheckRules()) {
        EndDMLevel();
        return;
    }
    if (CTFInMatch())
        return; // no checking in match mode
    // ZOID

    //=======
    // ROGUE
    if (gamerules->integer && DMGame.CheckDMRules) {
        if (DMGame.CheckDMRules())
            return;
    }
    // ROGUE
    //=======

    if (timelimit->value) {
        if (level.time >= SEC(timelimit->value * 60)) {
            gi.bprintf(PRINT_HIGH, "Time limit hit.\n");
            EndDMLevel();
            return;
        }
    }

    if (fraglimit->integer) {
        // [Paril-KEX]
        if (teamplay->integer) {
            CheckEndTDMLevel();
            return;
        }

        for (int i = 0; i < game.maxclients; i++) {
            cl = game.clients + i;
            if (!g_edicts[i + 1].inuse)
                continue;

            if (cl->resp.score >= fraglimit->integer) {
                gi.bprintf(PRINT_HIGH, "Frag limit hit.\n");
                EndDMLevel();
                return;
            }
        }
    }
}

/*
=============
ExitLevel
=============
*/
static void ExitLevel(void)
{
    // [Paril-KEX] N64 fade
    if (level.intermission_fade) {
        level.intermission_fade_time = level.time + SEC(1.3f);
        level.intermission_fading = true;
        return;
    }

    if (!level.intermission_fading)
        ClientEndServerFrames();

    level.exitintermission = 0;
    level.intermissiontime = 0;

    // [Paril-KEX] support for intermission completely wiping players
    // back to default stuff
    if (level.intermission_clear) {
        level.intermission_clear = false;

        for (int i = 0; i < game.maxclients; i++) {
            gclient_t *client = &game.clients[i];

            // [Kex] Maintain user info to keep the player skin.
            char userinfo[MAX_INFO_STRING];
            Q_strlcpy(userinfo, client->pers.userinfo, sizeof(userinfo));

            memset(&client->pers, 0, sizeof(client->pers));
            memset(&client->resp.coop_respawn, 0, sizeof(client->resp.coop_respawn));
            g_edicts[i + 1].health = 0; // this should trip the power armor, etc to reset as well

            Q_strlcpy(client->pers.userinfo, userinfo, sizeof(client->pers.userinfo));
            Q_strlcpy(client->resp.coop_respawn.userinfo, userinfo, sizeof(client->resp.coop_respawn.userinfo));
        }
    }

    // [Paril-KEX] end of unit, so clear level trackers
    if (level.intermission_eou) {
        memset(game.level_entries, 0, sizeof(game.level_entries));

        // give all players their lives back
        if (g_coop_enable_lives->integer) {
            for (int i = 0; i < game.maxclients; i++) {
                if (g_edicts[i + 1].inuse)
                    game.clients[i].pers.lives = g_coop_num_lives->integer + 1;
            }
        }
    }

    if (CTFNextMap())
        return;

    if (level.changemap == NULL) {
        gi.error("Got null changemap when trying to exit level. Was a trigger_changelevel configured correctly?");
        return;
    }

    // for N64 mainly, but if we're directly changing to "victorXXX.pcx" then
    // end game
    int start_offset = (level.changemap[0] == '*' ? 1 : 0);

    if (strlen(level.changemap) > (6 + start_offset) &&
        !Q_strncasecmp(level.changemap + start_offset, "victor", 6) &&
        !Q_strncasecmp(level.changemap + strlen(level.changemap) - 4, ".pcx", 4))
        gi.AddCommandString(va("gamemap \"!%s\"\n", level.changemap));
    else
        gi.AddCommandString(va("gamemap \"%s\"\n", level.changemap));

    level.changemap = NULL;
}

static void G_CheckCvars(void)
{
    if (sv_gravity->modified) {
        level.gravity = sv_gravity->value;
        sv_gravity->modified = qfalse;
    }
}

static bool G_AnyDeadPlayersWithoutLives(void)
{
    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        if (player->inuse && player->health <= 0 && !player->client->pers.lives)
            return true;
    }

    return false;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
static void G_RunFrame_(bool main_loop)
{
    level.in_frame = true;

    G_CheckCvars();

    level.time += FRAME_TIME;

    Nav_Frame();

    if (level.intermission_fading) {
        if (level.intermission_fade_time > level.time) {
            float alpha = Q_clipf(1.3f - TO_SEC(level.intermission_fade_time - level.time), 0, 1);

            for (int i = 0; i < game.maxclients; i++) {
                if (g_edicts[i + 1].inuse)
                    Vector4Set(game.clients[i].ps.blend, 0, 0, 0, alpha);
            }
        } else {
            level.intermission_fade = false;
            ExitLevel();
            level.intermission_fading = false;
        }

        level.in_frame = false;

        return;
    }

    edict_t *ent;

    // exit intermissions

    if (level.exitintermission) {
        ExitLevel();
        level.in_frame = false;
        return;
    }

    // reload the map start save if restart time is set (all players are dead)
    if (level.coop_level_restart_time > 0 && level.time > level.coop_level_restart_time) {
        ClientEndServerFrames();
        gi.AddCommandString("restart_level\n");
    }

    // clear client coop respawn states; this is done
    // early since it may be set multiple times for different
    // players
    if (coop->integer && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (!player->inuse)
                continue;
            if (player->client->respawn_time >= level.time)
                player->client->coop_respawn_state = COOP_RESPAWN_WAITING;
            else if (g_coop_enable_lives->integer && player->health <= 0 && player->client->pers.lives == 0)
                player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
            else if (g_coop_enable_lives->integer && G_AnyDeadPlayersWithoutLives())
                player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
            else
                player->client->coop_respawn_state = COOP_RESPAWN_NONE;
        }
    }

    //
    // treat each object in turn
    // even the world gets a chance to think
    //
    ent = &g_edicts[0];
    for (int i = 0; i < globals.num_edicts; i++, ent++) {
        if (!ent->inuse) {
            // defer removing client info so that disconnected, etc works
            if (i > 0 && i <= game.maxclients) {
                if (ent->timestamp && level.time < ent->timestamp) {
                    int playernum = ent - g_edicts - 1;
                    gi.configstring(CS_PLAYERSKINS + playernum, "");
                    ent->timestamp = 0;
                }
            }
            continue;
        }

        level.current_entity = ent;

        // Paril: RF_BEAM entities update their old_origin by hand.
        if (!(ent->s.renderfx & RF_BEAM))
            VectorCopy(ent->s.origin, ent->s.old_origin);

        // if the ground entity moved, make sure we are still on it
        if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount)) {
            contents_t mask = G_GetClipMask(ent);

            if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svflags & SVF_MONSTER)) {
                ent->groundentity = NULL;
                M_CheckGround(ent, mask);
            } else {
                // if it's still 1 point below us, we're good
                vec3_t end;
                VectorAdd(ent->s.origin, ent->gravityVector, end);
                trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, mask);

                if (tr.startsolid || tr.allsolid || tr.ent != ent->groundentity)
                    ent->groundentity = NULL;
                else
                    ent->groundentity_linkcount = ent->groundentity->linkcount;
            }
        }

        if (i > 0 && i <= game.maxclients) {
            ClientBeginServerFrame(ent);
            continue;
        }

        G_RunEntity(ent);
    }

    // see if it is time to end a deathmatch
    CheckDMRules();

    // see if needpass needs updated
    CheckNeedPass();

    if (coop->integer && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
        // rarely, we can see a flash of text if all players respawned
        // on some other player, so if everybody is now alive we'll reset
        // back to empty
        bool reset_coop_respawn = true;

        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (player->inuse && player->health > 0) {
                reset_coop_respawn = false;
                break;
            }
        }

        if (reset_coop_respawn) {
            for (int i = 1; i <= game.maxclients; i++) {
                edict_t *player = &g_edicts[i];
                if (player->inuse)
                    player->client->coop_respawn_state = COOP_RESPAWN_NONE;
            }
        }
    }

    // build the playerstate_t structures for all players
    ClientEndServerFrames();

    // [Paril-KEX] if not in intermission and player 1 is loaded in
    // the game as an entity, increase timer on current entry
    if (level.entry && !level.intermissiontime && g_edicts[1].inuse && g_edicts[1].client->pers.connected)
        level.entry->time += FRAME_TIME;

    // [Paril-KEX] run monster pains now
    for (int i = game.maxclients + BODY_QUEUE_SIZE + 1; i < globals.num_edicts; i++) {
        edict_t *e = &g_edicts[i];

        if (!e->inuse || !(e->svflags & SVF_MONSTER))
            continue;

        M_ProcessPain(e);
    }

    level.in_frame = false;
}

static bool G_AnyPlayerSpawned(void)
{
    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        if (player->inuse && player->client && player->client->pers.spawned)
            return true;
    }

    return false;
}

static void G_RunFrame(void)
{
    bool main_loop = sv_running->integer >= 2;

    if (main_loop && !G_AnyPlayerSpawned())
        return;

    for (int i = 0; i < g_frames_per_frame->integer; i++)
        G_RunFrame_(main_loop);
}

/*
================
G_PrepFrame

This has to be done before the world logic, because
player processing happens outside RunFrame
================
*/
static void G_PrepFrame(void)
{
    for (int i = 0; i < globals.num_edicts; i++)
        g_edicts[i].s.event = EV_NONE;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c can link
void Com_LPrintf(print_type_t type, const char *fmt, ...)
{
    va_list     argptr;
    char        text[MAX_STRING_CHARS];

    if (type == PRINT_DEVELOPER)
        return;

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    gi.dprintf("%s", text);
}

void Com_Error(error_type_t type, const char *fmt, ...)
{
    va_list     argptr;
    char        text[MAX_STRING_CHARS];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    gi.error("%s", text);
}
#endif
