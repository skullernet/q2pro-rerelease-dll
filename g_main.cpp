// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

CHECK_GCLIENT_INTEGRITY;
CHECK_EDICT_INTEGRITY;

std::mt19937 mt_rand;

game_locals_t  game;
level_locals_t level;

local_game_import_t  gi;

char local_game_import_t::print_buffer[0x10000];
game_import_ex_t local_game_import_t::ex;

game_export_t  globals;
spawn_temp_t   st;

cached_modelindex   sm_meat_index;
cached_soundindex   snd_fry;

edict_t *g_edicts;

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
cvar_t* g_instagib;
cvar_t* g_coop_player_collision;
cvar_t* g_coop_squad_respawn;
cvar_t* g_coop_enable_lives;
cvar_t* g_coop_num_lives;
cvar_t* g_coop_instanced_items;
cvar_t* g_allow_grapple;
cvar_t* g_grapple_fly_speed;
cvar_t* g_grapple_pull_speed;
cvar_t* g_grapple_damage;
cvar_t* g_coop_health_scaling;
cvar_t* g_weapon_respawn_time;

// dm"flags"
cvar_t* g_no_health;
cvar_t* g_no_items;
cvar_t* g_dm_weapons_stay;
cvar_t* g_dm_no_fall_damage;
cvar_t* g_dm_instant_items;
cvar_t* g_dm_same_level;
cvar_t* g_friendly_fire;
cvar_t* g_dm_force_respawn;
cvar_t* g_dm_force_respawn_time;
cvar_t* g_dm_spawn_farthest;
cvar_t* g_no_armor;
cvar_t* g_dm_allow_exit;
cvar_t* g_infinite_ammo;
cvar_t* g_dm_no_quad_drop;
cvar_t* g_dm_no_quadfire_drop;
cvar_t* g_no_mines;
cvar_t* g_dm_no_stack_double;
cvar_t* g_no_nukes;
cvar_t* g_no_spheres;
cvar_t* g_teamplay_armor_protect;
cvar_t* g_allow_techs;
cvar_t* g_start_items;
cvar_t* g_map_list;
cvar_t* g_map_list_shuffle;
cvar_t *g_lag_compensation;

cvar_t *sv_airaccelerate;
cvar_t *g_damage_scale;
cvar_t *g_disable_player_collision;
cvar_t *ai_damage_scale;
cvar_t *ai_model_scale;
cvar_t *ai_allow_dm_spawn;
cvar_t *ai_movement_disabled;
cvar_t *g_monster_footsteps;

static cvar_t *g_frames_per_frame;

void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint);
void ClientThink(edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect(edict_t *ent, char *userinfo);
void WriteGame(const char *filename, qboolean autosave);
void ReadGame(const char *filename);
void WriteLevel(const char *filename);
void ReadLevel(const char *filename);
qboolean G_CanSave();
void ClientDisconnect(edict_t *ent);
void ClientBegin(edict_t *ent);
void ClientCommand(edict_t *ent);
void G_RunFrame();
void G_PrepFrame();
void InitSave();

#include <chrono>

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void PreInitGame()
{
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

    // set this to match KEX behavior
    gi.cvar_set("map_allsolid_bug", "0");
}

/*
============
InitGame

Called after PreInitGame when the game has set up cvars.
============
*/
void InitGame()
{
    gi.dprintf("==== InitGame ====\n");

    auto cv = gi.cvar("sv_features", NULL, CVAR_NOFLAGS);
    if (!cv || !(cv->flags & CVAR_ROM) || ((int)cv->value & G_FEATURES) != G_FEATURES || gi.ex.apiversion < GAME_API_VERSION_EX)
        gi.error("This game library requires enhanced Q2PRO server");

    gi.cvar_forceset("g_features", G_Fmt("{}", (uint32_t)G_FEATURES).data());

    PreInitGame();

    InitSave();

    // seed RNG
    mt_rand.seed((uint32_t) std::chrono::system_clock::now().time_since_epoch().count());

    gun_x = gi.cvar("gun_x", "0", CVAR_NOFLAGS);
    gun_y = gi.cvar("gun_y", "0", CVAR_NOFLAGS);
    gun_z = gi.cvar("gun_z", "0", CVAR_NOFLAGS);

    // FIXME: sv_ prefix is wrong for these
    sv_rollspeed = gi.cvar("sv_rollspeed", "200", CVAR_NOFLAGS);
    sv_rollangle = gi.cvar("sv_rollangle", "2", CVAR_NOFLAGS);
    sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", CVAR_NOFLAGS);
    sv_gravity = gi.cvar("sv_gravity", "800", CVAR_NOFLAGS);

    g_skipViewModifiers = gi.cvar("g_skipViewModifiers", "0", CVAR_NOSET);

    sv_stopspeed = gi.cvar("sv_stopspeed", "100", CVAR_NOFLAGS); // PGM - was #define in g_phys.c

    // ROGUE
    huntercam = gi.cvar("huntercam", "1", CVAR_SERVERINFO | CVAR_LATCH);
    g_dm_strong_mines = gi.cvar("g_dm_strong_mines", "0", CVAR_NOFLAGS);
    g_dm_random_items = gi.cvar("g_dm_random_items", "0", CVAR_NOFLAGS);
    // ROGUE

    // [Kex] Instagib
    g_instagib = gi.cvar("g_instagib", "0", CVAR_NOFLAGS);

    // [Paril-KEX]
    g_coop_player_collision = gi.cvar("g_coop_player_collision", "0", CVAR_LATCH);
    g_coop_squad_respawn = gi.cvar("g_coop_squad_respawn", "1", CVAR_LATCH);
    g_coop_enable_lives = gi.cvar("g_coop_enable_lives", "0", CVAR_LATCH);
    g_coop_num_lives = gi.cvar("g_coop_num_lives", "2", CVAR_LATCH);
    g_coop_instanced_items = gi.cvar("g_coop_instanced_items", "1", CVAR_LATCH);
    g_allow_grapple = gi.cvar("g_allow_grapple", "auto", CVAR_NOFLAGS);
    g_grapple_fly_speed = gi.cvar("g_grapple_fly_speed", G_Fmt("{}", CTF_DEFAULT_GRAPPLE_SPEED).data(), CVAR_NOFLAGS);
    g_grapple_pull_speed = gi.cvar("g_grapple_pull_speed", G_Fmt("{}", CTF_DEFAULT_GRAPPLE_PULL_SPEED).data(), CVAR_NOFLAGS);
    g_grapple_damage = gi.cvar("g_grapple_damage", "10", CVAR_NOFLAGS);

    g_debug_monster_paths = gi.cvar("g_debug_monster_paths", "0", CVAR_NOFLAGS);
    g_debug_monster_kills = gi.cvar("g_debug_monster_kills", "0", CVAR_LATCH);

    // noset vars
    sv_dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);
    sv_running = gi.cvar("sv_running", nullptr, CVAR_NOFLAGS);
    if (!sv_running)
        gi.error("sv_running cvar doesn't exist");

    // latched vars
    sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
    gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);

    maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
    skill = gi.cvar("skill", "1", CVAR_LATCH);
    maxentities = gi.cvar("maxentities", G_Fmt("{}", MAX_EDICTS).data(), CVAR_LATCH);
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
    filterban = gi.cvar("filterban", "1", CVAR_NOFLAGS);

    g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

    run_pitch = gi.cvar("run_pitch", "0.002", CVAR_NOFLAGS);
    run_roll = gi.cvar("run_roll", "0.005", CVAR_NOFLAGS);
    bob_up = gi.cvar("bob_up", "0.005", CVAR_NOFLAGS);
    bob_pitch = gi.cvar("bob_pitch", "0.002", CVAR_NOFLAGS);
    bob_roll = gi.cvar("bob_roll", "0.002", CVAR_NOFLAGS);

    // flood control
    flood_msgs = gi.cvar("flood_msgs", "4", CVAR_NOFLAGS);
    flood_persecond = gi.cvar("flood_persecond", "4", CVAR_NOFLAGS);
    flood_waitdelay = gi.cvar("flood_waitdelay", "10", CVAR_NOFLAGS);

    g_strict_saves = gi.cvar("g_strict_saves", "1", CVAR_NOFLAGS);

    sv_airaccelerate = gi.cvar("sv_airaccelerate", "0", CVAR_NOFLAGS);

    g_damage_scale = gi.cvar("g_damage_scale", "1", CVAR_NOFLAGS);
    g_disable_player_collision = gi.cvar("g_disable_player_collision", "0", CVAR_NOFLAGS);
    ai_damage_scale = gi.cvar("ai_damage_scale", "1", CVAR_NOFLAGS);
    ai_model_scale = gi.cvar("ai_model_scale", "0", CVAR_NOFLAGS);
    ai_allow_dm_spawn = gi.cvar("ai_allow_dm_spawn", "0", CVAR_NOFLAGS);
    ai_movement_disabled = gi.cvar("ai_movement_disabled", "0", CVAR_NOFLAGS);
    g_monster_footsteps = gi.cvar("g_monster_footsteps", "0", CVAR_NOFLAGS);

    g_frames_per_frame = gi.cvar("g_frames_per_frame", "1", CVAR_NOFLAGS);

    g_coop_health_scaling = gi.cvar("g_coop_health_scaling", "0", CVAR_LATCH);
    g_weapon_respawn_time = gi.cvar("g_weapon_respawn_time", "30", CVAR_NOFLAGS);

    // dm "flags"
    g_no_health = gi.cvar("g_no_health", "0", CVAR_NOFLAGS);
    g_no_items = gi.cvar("g_no_items", "0", CVAR_NOFLAGS);
    g_dm_weapons_stay = gi.cvar("g_dm_weapons_stay", "0", CVAR_LATCH);
    g_dm_no_fall_damage = gi.cvar("g_dm_no_fall_damage", "0", CVAR_NOFLAGS);
    g_dm_instant_items = gi.cvar("g_dm_instant_items", "1", CVAR_NOFLAGS);
    g_dm_same_level = gi.cvar("g_dm_same_level", "0", CVAR_NOFLAGS);
    g_friendly_fire = gi.cvar("g_friendly_fire", "0", CVAR_NOFLAGS);
    g_dm_force_respawn = gi.cvar("g_dm_force_respawn", "0", CVAR_NOFLAGS);
    g_dm_force_respawn_time = gi.cvar("g_dm_force_respawn_time", "0", CVAR_NOFLAGS);
    g_dm_spawn_farthest = gi.cvar("g_dm_spawn_farthest", "1", CVAR_NOFLAGS);
    g_no_armor = gi.cvar("g_no_armor", "0", CVAR_NOFLAGS);
    g_dm_allow_exit = gi.cvar("g_dm_allow_exit", "0", CVAR_NOFLAGS);
    g_infinite_ammo = gi.cvar("g_infinite_ammo", "0", CVAR_LATCH);
    g_dm_no_quad_drop = gi.cvar("g_dm_no_quad_drop", "0", CVAR_NOFLAGS);
    g_dm_no_quadfire_drop = gi.cvar("g_dm_no_quadfire_drop", "0", CVAR_NOFLAGS);
    g_no_mines = gi.cvar("g_no_mines", "0", CVAR_NOFLAGS);
    g_dm_no_stack_double = gi.cvar("g_dm_no_stack_double", "0", CVAR_NOFLAGS);
    g_no_nukes = gi.cvar("g_no_nukes", "0", CVAR_NOFLAGS);
    g_no_spheres = gi.cvar("g_no_spheres", "0", CVAR_NOFLAGS);
    g_teamplay_force_join = gi.cvar("g_teamplay_force_join", "0", CVAR_NOFLAGS);
    g_teamplay_armor_protect = gi.cvar("g_teamplay_armor_protect", "0", CVAR_NOFLAGS);
    g_allow_techs = gi.cvar("g_allow_techs", "auto", CVAR_NOFLAGS);

    g_start_items = gi.cvar("g_start_items", "", CVAR_LATCH);
    g_map_list = gi.cvar("g_map_list", "", CVAR_NOFLAGS);
    g_map_list_shuffle = gi.cvar("g_map_list_shuffle", "0", CVAR_NOFLAGS);
    g_lag_compensation = gi.cvar("g_lag_compensation", "1", CVAR_NOFLAGS);

    // items
    InitItems();

    game = {};

    // initialize all entities for this game
    game.maxentities = clamp(maxentities->integer, maxclients->integer + 1, (int32_t)MAX_EDICTS);
    g_edicts = (edict_t *) gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
    globals.edicts = g_edicts;
    globals.max_edicts = game.maxentities;

    // initialize all clients for this game
    game.maxclients = maxclients->integer;
    game.clients = (gclient_t *) gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
    globals.num_edicts = game.maxclients + 1;

    //======
    // ROGUE
    if (gamerules->integer)
        InitGameRules(); // if there are game rules to set up, do so now.
    // ROGUE
    //======

    G_LoadL10nFile();

    // how far back we should support lag origins for
    game.max_lag_origins = 20 * (0.1f / FRAME_TIME.seconds());
    game.lag_origins = (vec3_t *) gi.TagMalloc(game.maxclients * sizeof(vec3_t) * game.max_lag_origins, TAG_GAME);
}

//===================================================================

void ShutdownGame()
{
    gi.dprintf("==== ShutdownGame ====\n");

    gi.FreeTags(TAG_LEVEL);
    gi.FreeTags(TAG_GAME);
}

static void *G_GetExtension(const char *name)
{
    return nullptr;
}

static void G_RestartFilesystem(void)
{
    G_LoadL10nFile();
}

gtime_t FRAME_TIME;

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2GAME_API game_export_t *GetGameAPI(game_import_t *import)
{
    gi = *import;

    FRAME_TIME = gtime_t::from_ms(1000 / TICK_RATE);

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

Q2GAME_API game_export_ex_t *GetGameAPIEx(game_import_ex_t *import)
{
    static game_export_ex_t gex;

    memcpy(&gi.ex, import, std::min(sizeof(gi.ex), (size_t)import->structsize));

    gex.apiversion = GAME_API_VERSION_EX;
    gex.structsize = sizeof(gex);
    gex.GetExtension = G_GetExtension;
    gex.CanSave = G_CanSave;
    gex.PrepFrame = G_PrepFrame;
    gex.RestartFilesystem = G_RestartFilesystem;

    return &gex;
}

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames()
{
    edict_t *ent;

    // calc the player views now that all pushing
    // and damage has been added
    for (uint32_t i = 0; i < game.maxclients; i++) {
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
edict_t *CreateTargetChangeLevel(const char *map)
{
    edict_t *ent;

    ent = G_Spawn();
    ent->classname = "target_changelevel";
    Q_strlcpy(level.nextmap, map, sizeof(level.nextmap));
    ent->map = level.nextmap;
    return ent;
}

inline std::vector<std::string> str_split(const std::string_view &str, char by)
{
    std::vector<std::string> out;
    size_t start, end = 0;

    while ((start = str.find_first_not_of(by, end)) != std::string_view::npos) {
        end = str.find(by, start);
        out.push_back(std::string{str.substr(start, end - start)});
    }

    return out;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel()
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
        char first_map[MAX_QPATH] { 0 };
        char *map;

        while (1) {
            map = COM_ParseEx(&str, " ");

            if (!*map)
                break;

            if (Q_strcasecmp(map, level.mapname) == 0) {
                // it's in the list, go to the next one
                map = COM_ParseEx(&str, " ");
                if (!*map) {
                    // end of list, go to first one
                    if (!first_map[0]) { // there isn't a first one, same level
                        BeginIntermission(CreateTargetChangeLevel(level.mapname));
                        return;
                    } else {
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
    ent = G_FindByString<&edict_t::classname>(nullptr, "target_changelevel");

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
void CheckNeedPass()
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

        gi.cvar_set("needpass", G_Fmt("{}", need).data());
        password->modified = spectator_password->modified = qfalse;
    }
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules()
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
        if (level.time >= gtime_t::from_min(timelimit->value)) {
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

        for (uint32_t i = 0; i < game.maxclients; i++) {
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
void ExitLevel()
{
    // [Paril-KEX] N64 fade
    if (level.intermission_fade) {
        level.intermission_fade_time = level.time + 1.3_sec;
        level.intermission_fading = true;
        return;
    }

    if (!level.intermission_fading)
        ClientEndServerFrames();

    level.exitintermission = 0;
    level.intermissiontime = 0_ms;

    // [Paril-KEX] support for intermission completely wiping players
    // back to default stuff
    if (level.intermission_clear) {
        level.intermission_clear = false;

        for (uint32_t i = 0; i < game.maxclients; i++) {
            // [Kex] Maintain user info to keep the player skin.
            char userinfo[MAX_INFO_STRING];
            memcpy(userinfo, game.clients[i].pers.userinfo, sizeof(userinfo));

            game.clients[i].pers = game.clients[i].resp.coop_respawn = {};
            g_edicts[i + 1].health = 0; // this should trip the power armor, etc to reset as well

            memcpy(game.clients[i].pers.userinfo, userinfo, sizeof(userinfo));
            memcpy(game.clients[i].resp.coop_respawn.userinfo, userinfo, sizeof(userinfo));
        }
    }

    // [Paril-KEX] end of unit, so clear level trackers
    if (level.intermission_eou) {
        game.level_entries = {};

        // give all players their lives back
        if (g_coop_enable_lives->integer)
            for (auto player : active_players())
                player->client->pers.lives = g_coop_num_lives->integer + 1;
    }

    if (CTFNextMap())
        return;

    if (level.changemap == nullptr) {
        gi.error("Got null changemap when trying to exit level. Was a trigger_changelevel configured correctly?");
        return;
    }

    // for N64 mainly, but if we're directly changing to "victorXXX.pcx" then
    // end game
    size_t start_offset = (level.changemap[0] == '*' ? 1 : 0);

    if (strlen(level.changemap) > (6 + start_offset) &&
        !Q_strncasecmp(level.changemap + start_offset, "victor", 6) &&
        !Q_strncasecmp(level.changemap + strlen(level.changemap) - 4, ".pcx", 4))
        gi.AddCommandString(G_Fmt("gamemap \"!{}\"\n", level.changemap).data());
    else
        gi.AddCommandString(G_Fmt("gamemap \"{}\"\n", level.changemap).data());

    level.changemap = nullptr;
}

static void G_CheckCvars()
{
    if (sv_gravity->modified) {
        level.gravity = sv_gravity->value;
        sv_gravity->modified = qfalse;
    }
}

static bool G_AnyDeadPlayersWithoutLives()
{
    for (auto player : active_players())
        if (player->health <= 0 && !player->client->pers.lives)
            return true;

    return false;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
inline void G_RunFrame_(bool main_loop)
{
    level.in_frame = true;

    G_CheckCvars();

    level.time += FRAME_TIME;

    if (level.intermission_fading) {
        if (level.intermission_fade_time > level.time) {
            float alpha = clamp(1.0f - (level.intermission_fade_time - level.time - 300_ms).seconds(), 0.f, 1.f);

            for (auto player : active_players())
                player->client->ps.screen_blend = { 0, 0, 0, alpha };
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
    if (level.coop_level_restart_time > 0_ms && level.time > level.coop_level_restart_time) {
        ClientEndServerFrames();
        gi.AddCommandString("restart_level\n");
    }

    // clear client coop respawn states; this is done
    // early since it may be set multiple times for different
    // players
    if (coop->integer && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
        for (auto player : active_players()) {
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
    for (uint32_t i = 0; i < globals.num_edicts; i++, ent++) {
        if (!ent->inuse) {
            // defer removing client info so that disconnected, etc works
            if (i > 0 && i <= game.maxclients) {
                if (ent->timestamp && level.time < ent->timestamp) {
                    int32_t playernum = ent - g_edicts - 1;
                    gi.configstring(CS_PLAYERSKINS + playernum, "");
                    ent->timestamp = 0_sec;
                }
            }
            continue;
        }

        level.current_entity = ent;

        // Paril: RF_BEAM entities update their old_origin by hand.
        if (!(ent->s.renderfx & RF_BEAM))
            ent->s.old_origin = ent->s.origin;

        // if the ground entity moved, make sure we are still on it
        if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount)) {
            contents_t mask = G_GetClipMask(ent);

            if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svflags & SVF_MONSTER)) {
                ent->groundentity = nullptr;
                M_CheckGround(ent, mask);
            } else {
                // if it's still 1 point below us, we're good
                trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->gravityVector, ent,
                                      mask);

                if (tr.startsolid || tr.allsolid || tr.ent != ent->groundentity)
                    ent->groundentity = nullptr;
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

        for (auto player : active_players()) {
            if (player->health >= 0) {
                reset_coop_respawn = false;
                break;
            }
        }

        if (reset_coop_respawn) {
            for (auto player : active_players())
                player->client->coop_respawn_state = COOP_RESPAWN_NONE;
        }
    }

    // build the playerstate_t structures for all players
    ClientEndServerFrames();

    // [Paril-KEX] if not in intermission and player 1 is loaded in
    // the game as an entity, increase timer on current entry
    if (level.entry && !level.intermissiontime && g_edicts[1].inuse && g_edicts[1].client->pers.connected)
        level.entry->time += FRAME_TIME;

    // [Paril-KEX] run monster pains now
    for (uint32_t i = 0; i < globals.num_edicts + 1 + game.maxclients + BODY_QUEUE_SIZE; i++) {
        edict_t *e = &g_edicts[i];

        if (!e->inuse || !(e->svflags & SVF_MONSTER))
            continue;

        M_ProcessPain(e);
    }

    level.in_frame = false;
}

inline bool G_AnyPlayerSpawned()
{
    for (auto player : active_players())
        if (player->client && player->client->pers.spawned)
            return true;

    return false;
}

void G_RunFrame()
{
    bool main_loop = sv_running->integer >= SS_GAME;

    if (main_loop && !G_AnyPlayerSpawned())
        return;

    for (int32_t i = 0; i < g_frames_per_frame->integer; i++)
        G_RunFrame_(main_loop);

    // match details.. only bother if there's at least 1 player in-game
    // and not already end of game
    if (G_AnyPlayerSpawned() && !level.intermissiontime) {
        constexpr gtime_t MATCH_REPORT_TIME = 45_sec;

        if (level.time - level.next_match_report > MATCH_REPORT_TIME) {
            level.next_match_report = level.time + MATCH_REPORT_TIME;
            G_ReportMatchDetails(false);
        }
    }
}

/*
================
G_PrepFrame

This has to be done before the world logic, because
player processing happens outside RunFrame
================
*/
void G_PrepFrame()
{
    for (uint32_t i = 0; i < globals.num_edicts; i++)
        g_edicts[i].s.event = EV_NONE;
}
