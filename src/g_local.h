// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_local.h -- local definitions for game module
#pragma once

#include "q_shared.h"
#include "m_flash.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "game.h"

#if USE_FPS
#define G_GMF_VARIABLE_FPS  GMF_VARIABLE_FPS
#else
#define G_GMF_VARIABLE_FPS  0
#endif

// features this game supports
#define G_FEATURES_REQUIRED (GMF_PROPERINUSE | GMF_WANT_ALL_DISCONNECTS | GMF_ENHANCED_SAVEGAMES | GMF_PROTOCOL_EXTENSIONS)
#define G_FEATURES          (G_FEATURES_REQUIRED | G_GMF_VARIABLE_FPS)

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION "baseq2"

// protocol bytes that can be directly added to messages
enum {
    svc_muzzleflash = 1,
    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_inventory,
    svc_stufftext = 11
};

#define STEPSIZE    18.0f

//==================================================================

extern const vec3_t player_mins;
extern const vec3_t player_maxs;

extern game_import_t    gi;
extern game_import_ex_t gix;

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor.
// the following 8 are reserved and should never be used by any entity.
// (power cubes in coop use these after spawning as well)

// these spawnflags affect every entity. note that items are a bit special
// because these 8 bits are instead used for power cube bits.
#define SPAWNFLAG_NONE              0
#define SPAWNFLAG_NOT_EASY          0x00000100
#define SPAWNFLAG_NOT_MEDIUM        0x00000200
#define SPAWNFLAG_NOT_HARD          0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH    0x00000800
#define SPAWNFLAG_NOT_COOP          0x00001000
#define SPAWNFLAG_RESERVED1         0x00002000
#define SPAWNFLAG_COOP_ONLY         0x00004000
#define SPAWNFLAG_RESERVED2         0x00008000
#define SPAWNFLAG_EDITOR_MASK       0x0000ff00

// stores a level time; most newer engines use int64_t for
// time storage, but seconds are handy for compatibility
// with Quake and older mods.

typedef int64_t gtime_t;
typedef int effects_t;
typedef int contents_t;
typedef int spawnflags_t;
typedef int button_t;
typedef int svflags_t;

#define SEC(n)  ((gtime_t)((n) * 1000LL))
#define MSEC(n) ((gtime_t)(n))
#define HZ(n)   ((gtime_t)(1000LL / (n)))

#define TO_SEC(f)   ((f) * 0.001f)
#define TO_MSEC(f)  (f)

#if USE_FPS
#define TICK_RATE       game.tick_rate
#define FRAME_TIME      game.frame_time
#define FRAME_TIME_SEC  game.frame_time_sec
#else
#define TICK_RATE       BASE_FRAMERATE
#define FRAME_TIME      ((gtime_t)BASE_FRAMETIME)
#define FRAME_TIME_SEC  BASE_FRAMETIME_1000
#endif

// view pitching times
#define DAMAGE_TIME SEC(0.5f)
#define FALL_TIME   SEC(0.3f)
#define KICK_TIME   SEC(0.2f)

// memory tags to allow dynamic memory to be cleaned up
enum {
    TAG_GAME  = 765, // clear when unloading the dll
    TAG_LEVEL = 766, // clear when loading a new level
    TAG_L10N  = 767, // for localization strings
};

#define MELEE_DISTANCE  50

#define BODY_QUEUE_SIZE 8

// null trace used when touches don't need a trace
extern const trace_t null_trace;

typedef enum {
    WEAPON_READY,
    WEAPON_ACTIVATING,
    WEAPON_DROPPING,
    WEAPON_FIRING
} weaponstate_t;

// gib flags
typedef enum {
    GIB_NONE        = 0,      // no flags (organic)
    GIB_METALLIC    = BIT(0), // bouncier
    GIB_ACID        = BIT(1), // acidic (gekk)
    GIB_HEAD        = BIT(2), // head gib; the input entity will transform into this
    GIB_DEBRIS      = BIT(3), // explode outwards rather than in velocity, no blood
    GIB_SKINNED     = BIT(4), // use skinnum
    GIB_UPRIGHT     = BIT(5), // stay upright on ground
} gib_type_t;

// monster ai flags
#define AI_NONE                 0ULL
#define AI_STAND_GROUND         BIT_ULL(0)
#define AI_TEMP_STAND_GROUND    BIT_ULL(1)
#define AI_SOUND_TARGET         BIT_ULL(2)
#define AI_LOST_SIGHT           BIT_ULL(3)
#define AI_PURSUIT_LAST_SEEN    BIT_ULL(4)
#define AI_PURSUE_NEXT          BIT_ULL(5)
#define AI_PURSUE_TEMP          BIT_ULL(6)
#define AI_HOLD_FRAME           BIT_ULL(7)
#define AI_GOOD_GUY             BIT_ULL(8)
#define AI_BRUTAL               BIT_ULL(9)
#define AI_NOSTEP               BIT_ULL(10)
#define AI_DUCKED               BIT_ULL(11)
#define AI_COMBAT_POINT         BIT_ULL(12)
#define AI_MEDIC                BIT_ULL(13)
#define AI_RESURRECTING         BIT_ULL(14)

// ROGUE
#define AI_MANUAL_STEERING      BIT_ULL(15)
#define AI_TARGET_ANGER         BIT_ULL(16)
#define AI_DODGING              BIT_ULL(17)
#define AI_CHARGING             BIT_ULL(18)
#define AI_HINT_PATH            BIT_ULL(19)
#define AI_IGNORE_SHOTS         BIT_ULL(20)
// PMM - FIXME - last second added for E3 .. there's probably a better way to do this but
// this works
#define AI_DO_NOT_COUNT         BIT_ULL(21) // set for healed monsters
#define AI_SPAWNED_CARRIER      BIT_ULL(22) // both do_not_count and spawned are set for spawned monsters
#define AI_SPAWNED_MEDIC_C      BIT_ULL(23) // both do_not_count and spawned are set for spawned monsters
#define AI_SPAWNED_WIDOW        BIT_ULL(24) // both do_not_count and spawned are set for spawned monsters
#define AI_BLOCKED              BIT_ULL(25) // used by blocked_checkattack: set to say I'm attacking while blocked
                                            // (prevents run-attacks)
// ROGUE

#define AI_SPAWNED_ALIVE        BIT_ULL(26) // [Paril-KEX] for spawning dead
#define AI_SPAWNED_DEAD         BIT_ULL(27)
#define AI_HIGH_TICK_RATE       BIT_ULL(28) // not limited by 10hz actions
#define AI_NO_PATH_FINDING      BIT_ULL(29) // don't try nav nodes for path finding
#define AI_PATHING              BIT_ULL(30) // using nav nodes currently
#define AI_STINKY               BIT_ULL(31) // spawn flies
#define AI_STUNK                BIT_ULL(32) // already spawned files

#define AI_ALTERNATE_FLY        BIT_ULL(33) // use alternate flying mechanics; see monsterinfo.fly_xxx
#define AI_TEMP_MELEE_COMBAT    BIT_ULL(34) // temporarily switch to the melee combat style
#define AI_FORGET_ENEMY         BIT_ULL(35) // forget the current enemy
#define AI_DOUBLE_TROUBLE       BIT_ULL(36) // JORG only
#define AI_REACHED_HOLD_COMBAT  BIT_ULL(37)
#define AI_THIRD_EYE            BIT_ULL(38)

// mask to catch all three flavors of spawned
#define AI_SPAWNED_MASK         (AI_SPAWNED_CARRIER | AI_SPAWNED_MEDIC_C | AI_SPAWNED_WIDOW)

// monster attack state
typedef enum {
    AS_NONE,
    AS_STRAIGHT,
    AS_SLIDING,
    AS_MELEE,
    AS_MISSILE,
    AS_BLIND // PMM - used by boss code to do nasty things even if it can't see you
} monster_attack_state_t;

// handedness values
typedef enum {
    RIGHT_HANDED,
    LEFT_HANDED,
    CENTER_HANDED
} handedness_t;

typedef enum {
    AUTOSW_SMART,
    AUTOSW_ALWAYS,
    AUTOSW_ALWAYS_NO_AMMO,
    AUTOSW_NEVER
} auto_switch_t;

#define SFL_CROSS_TRIGGER_MASK  (0xffffffffu & ~SPAWNFLAG_EDITOR_MASK)

// noise types for PlayerNoise
typedef enum {
    PNOISE_SELF,
    PNOISE_WEAPON,
    PNOISE_IMPACT
} player_noise_t;

typedef struct {
    int     base_count;
    int     max_count;
    float   normal_protection;
    float   energy_protection;
} gitem_armor_t;

// edict->movetype values
typedef enum {
    MOVETYPE_NONE,   // never moves
    MOVETYPE_NOCLIP, // origin and angles change with no interaction
    MOVETYPE_PUSH,   // no clip to world, push on box contact
    MOVETYPE_STOP,   // no clip to world, stops on box contact

    MOVETYPE_WALK, // gravity
    MOVETYPE_STEP, // gravity, special edge handling
    MOVETYPE_FLY,
    MOVETYPE_TOSS,       // gravity
    MOVETYPE_FLYMISSILE, // extra size to monsters
    MOVETYPE_BOUNCE,
    // RAFAEL
    MOVETYPE_WALLBOUNCE,
    // RAFAEL
    // ROGUE
    MOVETYPE_NEWTOSS // PGM - for deathball
    // ROGUE
} movetype_t;

// edict->flags
#define FL_NONE                 0ULL
#define FL_FLY                  BIT_ULL(0)
#define FL_SWIM                 BIT_ULL(1)  // implied immunity to drowning
#define FL_IMMUNE_LASER         BIT_ULL(2)
#define FL_INWATER              BIT_ULL(3)
#define FL_GODMODE              BIT_ULL(4)
#define FL_NOTARGET             BIT_ULL(5)
#define FL_IMMUNE_SLIME         BIT_ULL(6)
#define FL_IMMUNE_LAVA          BIT_ULL(7)
#define FL_PARTIALGROUND        BIT_ULL(8)  // not all corners are valid
#define FL_WATERJUMP            BIT_ULL(9)  // player jumping out of water
#define FL_TEAMSLAVE            BIT_ULL(10) // not the first on the team
#define FL_NO_KNOCKBACK         BIT_ULL(11)
#define FL_POWER_ARMOR          BIT_ULL(12) // power armor (if any) is active

// ROGUE
#define FL_MECHANICAL           BIT_ULL(13) // entity is mechanical, use sparks not blood
#define FL_SAM_RAIMI            BIT_ULL(14) // entity is in sam raimi cam mode
#define FL_DISGUISED            BIT_ULL(15) // entity is in disguise, monsters will not recognize.
#define FL_NOGIB                BIT_ULL(16) // player has been vaporized by a nuke, drop no gibs
#define FL_DAMAGEABLE           BIT_ULL(17)
#define FL_STATIONARY           BIT_ULL(18)
// ROGUE

#define FL_ALIVE_KNOCKBACK_ONLY BIT_ULL(19) // only apply knockback if alive or on same frame as death
#define FL_NO_DAMAGE_EFFECTS    BIT_ULL(20)

#define FL_COOP_HEALTH_SCALE    BIT_ULL(21) // [Paril-KEX] gets scaled by coop health scaling
#define FL_FLASHLIGHT           BIT_ULL(22) // enable flashlight
#define FL_KILL_VELOCITY        BIT_ULL(23) // for berserker slam
#define FL_NOVISIBLE            BIT_ULL(24) // super invisibility
#define FL_DODGE                BIT_ULL(25) // monster should try to dodge this
#define FL_TEAMMASTER           BIT_ULL(26) // is a team master (only here so that entities abusing teammaster/teamchain for stuff don't break)
#define FL_LOCKED               BIT_ULL(27) // entity is locked for the purposes of navigation
#define FL_ALWAYS_TOUCH         BIT_ULL(28) // always touch, even if we normally wouldn't
#define FL_NO_STANDING          BIT_ULL(29) // don't allow "standing" on non-brush entities
#define FL_WANTS_POWER_ARMOR    BIT_ULL(30) // for players, auto-shield

#define FL_RESPAWN              BIT_ULL(31) // used for item respawning
#define FL_TRAP                 BIT_ULL(32) // entity is a trap of some kind
#define FL_TRAP_LASER_FIELD     BIT_ULL(33) // enough of a special case to get it's own flag...
#define FL_IMMORTAL             BIT_ULL(34) // never go below 1hp

// gitem_t->flags
typedef enum {
    IF_ANY          = -1,
    IF_NONE         = 0,
    IF_WEAPON       = BIT(0), // use makes active weapon
    IF_AMMO         = BIT(1),
    IF_ARMOR        = BIT(2),
    IF_STAY_COOP    = BIT(3),
    IF_KEY          = BIT(4),
    IF_POWERUP      = BIT(5),
    // ROGUE
    IF_NOT_GIVEABLE = BIT(6), // item can not be given
    // ROGUE
    IF_HEALTH       = BIT(7),
    // ZOID
    IF_TECH         = BIT(8),
    IF_NO_HASTE     = BIT(9),
    // ZOID

    IF_NO_INFINITE_AMMO = BIT(10), // [Paril-KEX] don't allow infinite ammo to affect
    IF_POWERUP_WHEEL    = BIT(11), // [Paril-KEX] item should be in powerup wheel
    IF_POWERUP_ONOFF    = BIT(12), // [Paril-KEX] for wheel; can't store more than one, show on/off state
    IF_NOT_RANDOM       = BIT(13), // [Paril-KEX] item never shows up in randomizations
} item_flags_t;

// health edict_t->style
enum {
    HEALTH_IGNORE_MAX = 1,
    HEALTH_TIMED = 2
};

// item IDs; must match itemlist order
typedef enum {
    IT_NULL, // must always be zero

    IT_ARMOR_BODY,
    IT_ARMOR_COMBAT,
    IT_ARMOR_JACKET,
    IT_ARMOR_SHARD,

    IT_ITEM_POWER_SCREEN,
    IT_ITEM_POWER_SHIELD,

    IT_WEAPON_GRAPPLE,
    IT_WEAPON_BLASTER,
    IT_WEAPON_CHAINFIST,
    IT_WEAPON_SHOTGUN,
    IT_WEAPON_SSHOTGUN,
    IT_WEAPON_MACHINEGUN,
    IT_WEAPON_ETF_RIFLE,
    IT_WEAPON_CHAINGUN,
    IT_AMMO_GRENADES,
    IT_AMMO_TRAP,
    IT_AMMO_TESLA,
    IT_WEAPON_GLAUNCHER,
    IT_WEAPON_PROXLAUNCHER,
    IT_WEAPON_RLAUNCHER,
    IT_WEAPON_HYPERBLASTER,
    IT_WEAPON_IONRIPPER,
    IT_WEAPON_PLASMABEAM,
    IT_WEAPON_RAILGUN,
    IT_WEAPON_PHALANX,
    IT_WEAPON_BFG,
    IT_WEAPON_DISRUPTOR,

    IT_AMMO_SHELLS,
    IT_AMMO_BULLETS,
    IT_AMMO_CELLS,
    IT_AMMO_ROCKETS,
    IT_AMMO_SLUGS,
    IT_AMMO_MAGSLUG,
    IT_AMMO_FLECHETTES,
    IT_AMMO_PROX,
    IT_AMMO_NUKE,
    IT_AMMO_ROUNDS,

    IT_ITEM_QUAD,
    IT_ITEM_QUADFIRE,
    IT_ITEM_INVULNERABILITY,
    IT_ITEM_INVISIBILITY,
    IT_ITEM_SILENCER,
    IT_ITEM_REBREATHER,
    IT_ITEM_ENVIROSUIT,
    IT_ITEM_ANCIENT_HEAD,
    IT_ITEM_LEGACY_HEAD,
    IT_ITEM_ADRENALINE,
    IT_ITEM_BANDOLIER,
    IT_ITEM_PACK,
    IT_ITEM_IR_GOGGLES,
    IT_ITEM_DOUBLE,
    IT_ITEM_SPHERE_VENGEANCE,
    IT_ITEM_SPHERE_HUNTER,
    IT_ITEM_SPHERE_DEFENDER,
    IT_ITEM_DOPPELGANGER,
    IT_ITEM_TAG_TOKEN,

    IT_KEY_DATA_CD,
    IT_KEY_POWER_CUBE,
    IT_KEY_EXPLOSIVE_CHARGES,
    IT_KEY_YELLOW,
    IT_KEY_POWER_CORE,
    IT_KEY_PYRAMID,
    IT_KEY_DATA_SPINNER,
    IT_KEY_PASS,
    IT_KEY_BLUE_KEY,
    IT_KEY_RED_KEY,
    IT_KEY_GREEN_KEY,
    IT_KEY_COMMANDER_HEAD,
    IT_KEY_AIRSTRIKE,
    IT_KEY_NUKE_CONTAINER,
    IT_KEY_NUKE,

    IT_HEALTH_SMALL,
    IT_HEALTH_MEDIUM,
    IT_HEALTH_LARGE,
    IT_HEALTH_MEGA,

    IT_FLAG1,
    IT_FLAG2,

    IT_TECH_RESISTANCE,
    IT_TECH_STRENGTH,
    IT_TECH_HASTE,
    IT_TECH_REGENERATION,

    IT_ITEM_FLASHLIGHT,

    IT_TOTAL
} item_id_t;

typedef struct gitem_s {
    item_id_t   id;        // matches item list index
    const char *classname; // spawning name
    bool (*pickup)(edict_t *ent, edict_t *other);
    void (*use)(edict_t *ent, const struct gitem_s *item);
    void (*drop)(edict_t *ent, const struct gitem_s *item);
    void (*weaponthink)(edict_t *ent);
    const char *pickup_sound;
    const char *world_model;
    effects_t   world_model_flags;
    const char *view_model;

    // client side info
    const char *icon;
    const char *use_name; // for use command, english only
    const char *pickup_name; // for printing on pickup
    const char *pickup_name_definite; // definite article version for languages that need it

    int          quantity;  // for ammo how much, for weapons how much is used per shot
    item_id_t    ammo;      // for weapons
    item_id_t    chain;     // weapon chain root
    item_flags_t flags;     // IT_* flags

    const char *vwep_model; // vwep model string (for weapons)

    const gitem_armor_t *armor_info;
    int                  tag;

    const char *precaches; // string of all models, sounds, and images this item will use

    int         quantity_warn;  // when to warn on low ammo
} gitem_t;

// ammo IDs
typedef enum {
    AMMO_BULLETS,
    AMMO_SHELLS,
    AMMO_ROCKETS,
    AMMO_GRENADES,
    AMMO_CELLS,
    AMMO_SLUGS,
    // RAFAEL
    AMMO_MAGSLUG,
    AMMO_TRAP,
    // RAFAEL
    // ROGUE
    AMMO_FLECHETTES,
    AMMO_TESLA,
    AMMO_DISRUPTOR,
    AMMO_PROX,
    // ROGUE
    AMMO_MAX
} ammo_t;

// powerup IDs
typedef enum {
    POWERUP_SCREEN,
    POWERUP_SHIELD,

    POWERUP_AM_BOMB,

    POWERUP_QUAD,
    POWERUP_QUADFIRE,
    POWERUP_INVULNERABILITY,
    POWERUP_INVISIBILITY,
    POWERUP_SILENCER,
    POWERUP_REBREATHER,
    POWERUP_ENVIROSUIT,
    POWERUP_ADRENALINE,
    POWERUP_IR_GOGGLES,
    POWERUP_DOUBLE,
    POWERUP_SPHERE_VENGEANCE,
    POWERUP_SPHERE_HUNTER,
    POWERUP_SPHERE_DEFENDER,
    POWERUP_DOPPELGANGER,

    POWERUP_FLASHLIGHT,
    POWERUP_COMPASS,
    POWERUP_TECH1,
    POWERUP_TECH2,
    POWERUP_TECH3,
    POWERUP_TECH4,
    POWERUP_MAX
} powerup_t;

// means of death
typedef enum {
    MOD_UNKNOWN,
    MOD_BLASTER,
    MOD_SHOTGUN,
    MOD_SSHOTGUN,
    MOD_MACHINEGUN,
    MOD_CHAINGUN,
    MOD_GRENADE,
    MOD_G_SPLASH,
    MOD_ROCKET,
    MOD_R_SPLASH,
    MOD_HYPERBLASTER,
    MOD_RAILGUN,
    MOD_BFG_LASER,
    MOD_BFG_BLAST,
    MOD_BFG_EFFECT,
    MOD_HANDGRENADE,
    MOD_HG_SPLASH,
    MOD_WATER,
    MOD_SLIME,
    MOD_LAVA,
    MOD_CRUSH,
    MOD_TELEFRAG,
    MOD_TELEFRAG_SPAWN,
    MOD_FALLING,
    MOD_SUICIDE,
    MOD_HELD_GRENADE,
    MOD_EXPLOSIVE,
    MOD_BARREL,
    MOD_BOMB,
    MOD_EXIT,
    MOD_SPLASH,
    MOD_TARGET_LASER,
    MOD_TRIGGER_HURT,
    MOD_HIT,
    MOD_TARGET_BLASTER,
    // RAFAEL 14-APR-98
    MOD_RIPPER,
    MOD_PHALANX,
    MOD_BRAINTENTACLE,
    MOD_BLASTOFF,
    MOD_GEKK,
    MOD_TRAP,
    // END 14-APR-98
    //========
    // ROGUE
    MOD_CHAINFIST,
    MOD_DISINTEGRATOR,
    MOD_ETF_RIFLE,
    MOD_BLASTER2,
    MOD_HEATBEAM,
    MOD_TESLA,
    MOD_PROX,
    MOD_NUKE,
    MOD_VENGEANCE_SPHERE,
    MOD_HUNTER_SPHERE,
    MOD_DEFENDER_SPHERE,
    MOD_TRACKER,
    MOD_DBALL_CRUSH,
    MOD_DOPPLE_EXPLODE,
    MOD_DOPPLE_VENGEANCE,
    MOD_DOPPLE_HUNTER,
    // ROGUE
    //========
    MOD_GRAPPLE,
    MOD_BLUEBLASTER
} mod_id_t;

typedef struct {
    mod_id_t    id: 16;
    bool        friendly_fire;
    bool        no_point_loss;
} mod_t;

#define FOFS(x) q_offsetof(edict_t, x)
#define STOFS(x) q_offsetof(spawn_temp_t, x)
#define CLOFS(x) q_offsetof(gclient_t, x)

// the total number of levels we'll track for the
// end of unit screen.
#define MAX_LEVELS_PER_UNIT 8

typedef struct {
    // bsp name
    char map_name[MAX_QPATH];
    // map name
    char pretty_name[MAX_QPATH];
    // these are set when we leave the level
    int total_secrets;
    int found_secrets;
    int total_monsters;
    int killed_monsters;
    // total time spent in the level, for end screen
    gtime_t time;
    // the order we visited levels in
    int visit_order;
} level_entry_t;

typedef struct precache_s {
    struct precache_s   *next;
    void                (*func)(void);
} precache_t;

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct {
    char    helpmessage1[MAX_TOKEN_CHARS];
    char    helpmessage2[MAX_TOKEN_CHARS];
    int     help1changed, help2changed;

    gclient_t *clients; // [maxclients]

    // can't store spawnpoint in level, because
    // it would get overwritten by the savegame restore
    char spawnpoint[MAX_TOKEN_CHARS]; // needed for coop respawns

    // store latched cvars here that we want to get at often
    int maxclients;
    int maxentities;

    // cross level triggers
    int cross_level_flags, cross_unit_flags;

    bool autosaved;

    // [Paril-KEX]
    level_entry_t level_entries[MAX_LEVELS_PER_UNIT];

    precache_t *precaches;

#if USE_FPS
    int tick_rate;
    gtime_t frame_time;
    float frame_time_sec;
#endif
} game_locals_t;

#define MAX_HEALTH_BARS 2

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct {
    bool in_frame;
    gtime_t time;

    char level_name[MAX_QPATH]; // the descriptive name (Outer Base, etc)
    char mapname[MAX_QPATH];    // the server name (base1, etc)
    char nextmap[MAX_QPATH];    // go here when fraglimit is hit
    char forcemap[MAX_QPATH];   // go here

    // intermission state
    gtime_t     intermissiontime; // time the intermission was started
    const char *changemap;
    const char *achievement;
    bool        exitintermission;
    bool        intermission_eou;
    bool        intermission_clear; // [Paril-KEX] clear inventory on switch
    bool        level_intermission_set; // [Paril-KEX] for target_camera switches; don't find intermission point
    bool        intermission_fade, intermission_fading; // [Paril-KEX] fade on exit instead of immediately leaving
    gtime_t     intermission_fade_time;
    vec3_t      intermission_origin;
    vec3_t      intermission_angle;
    bool        respawn_intermission; // only set once for respawning players

    int pic_health;
    int snd_fry;

    int total_secrets;
    int found_secrets;

    int total_goals;
    int found_goals;

    int total_monsters;
    edict_t *monsters_registered[MAX_EDICTS]; // only for debug
    int killed_monsters;

    edict_t *current_entity; // entity running from G_RunFrame
    int body_que;   // dead bodies

    int power_cubes; // ugly necessity for coop

    // ROGUE
    edict_t *disguise_violator;
    gtime_t  disguise_violation_time;
    int      disguise_icon; // [Paril-KEX]
    // ROGUE

    bool is_n64;
    gtime_t coop_level_restart_time; // restart the level after this time
    bool instantitems; // instantitems 1 set in worldspawn

    // N64 goal stuff
    const char *goals; // NULL if no goals in world
    int goal_num; // current relative goal number, increased with each target_goal

    // offset for the first vwep model, for
    // skinnum encoding
    int vwep_offset;

    // coop health scaling factor;
    // this percentage of health is added
    // to the monster's health per player.
    float coop_health_scaling;
    // this isn't saved in the save file, but stores
    // the amount of players currently active in the
    // level, compared against monsters' individual
    // scale #
    int coop_scale_players;

    // [Paril-KEX] current level entry
    level_entry_t *entry;

    // start items
    const char *start_items;
    // disable grappling hook
    bool no_grapple;

    // saved gravity
    float gravity;
    // level is a hub map, and shouldn't be included in EOU stuff
    bool hub_map;
    // active health bar entities
    edict_t *health_bar_entities[MAX_HEALTH_BARS];
    bool deadly_kill_box;
    bool story_active;
    gtime_t next_auto_save;

    float skyrotate;
    int skyautorotate;
} level_locals_t;

// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay.
// defaults can/should be set in the struct.
typedef struct {
    // world vars
    const char *sky;
    float       skyrotate;
    vec3_t      skyaxis;
    int         skyautorotate;
    const char *nextmap;

    int         lip;
    int         distance;
    int         height;
    const char *noise;
    float       pausetime;
    const char *item;
    const char *gravity;

    float minyaw;
    float maxyaw;
    float minpitch;
    float maxpitch;

    const char *music; // [Edward-KEX]
    int instantitems;
    float radius; // [Paril-KEX]
    bool hub_map; // [Paril-KEX]
    const char *achievement; // [Paril-KEX]

    // [Paril-KEX]
    const char *goals;

    // [Paril-KEX]
    const char *image;

    int fade_start_dist;
    int fade_end_dist;
    const char *start_items;
    int no_grapple;
    float health_multiplier;

    const char *reinforcements; // [Paril-KEX]
    const char *noise_start, *noise_middle, *noise_end; // [Paril-KEX]
    int loop_count; // [Paril-KEX]
} spawn_temp_t;

typedef enum {
    STATE_TOP,
    STATE_BOTTOM,
    STATE_UP,
    STATE_DOWN
} move_state_t;

#define MOVEINFO_ENDFUNC(n) n
#define MOVEINFO_BLOCKED(n) n

typedef struct {
    // fixed data
    vec3_t start_origin;
    vec3_t start_angles;
    vec3_t end_origin;
    vec3_t end_angles, end_angles_reversed;

    int sound_start;
    int sound_middle;
    int sound_end;

    float accel;
    float speed;
    float decel;
    float distance;

    float wait;

    // state data
    move_state_t state;
    bool         reversing;
    vec3_t       dir;
    vec3_t       dest;
    float        current_speed;
    float        move_speed;
    float        next_speed;
    float        remaining_distance;
    float        decel_distance;
    void (*endfunc)(edict_t *self);
    void (*blocked)(edict_t *self, edict_t *other);
} moveinfo_t;

typedef struct {
    void (*aifunc)(edict_t *self, float dist);
    float dist;
    void (*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct {
    int   firstframe;
    int   lastframe;
    const mframe_t *frame;
    void (*endfunc)(edict_t *self);
    float sidestep_scale;
} mmove_t;

#define MMOVE_T(n) n

#define MONSTERINFO_STAND(n) n
#define MONSTERINFO_IDLE(n) n
#define MONSTERINFO_SEARCH(n) n
#define MONSTERINFO_WALK(n) n
#define MONSTERINFO_RUN(n) n
#define MONSTERINFO_DODGE(n) n
#define MONSTERINFO_ATTACK(n) n
#define MONSTERINFO_MELEE(n) n
#define MONSTERINFO_SIGHT(n) n
#define MONSTERINFO_CHECKATTACK(n) n
#define MONSTERINFO_SETSKIN(n) n
#define MONSTERINFO_BLOCKED(n) n
#define MONSTERINFO_PHYSCHANGED(n) n
#define MONSTERINFO_DUCK(n) n
#define MONSTERINFO_UNDUCK(n) n
#define MONSTERINFO_SIDESTEP(n) n

// combat styles, for navigation
typedef enum {
    COMBAT_UNKNOWN, // automatically choose based on attack functions
    COMBAT_MELEE, // should attempt to get up close for melee
    COMBAT_MIXED, // has mixed melee/ranged; runs to get up close if far enough away
    COMBAT_RANGED // don't bother pathing if we can see the player
} combat_style_t;

typedef struct {
    const char *classname;
    int strength;
    float radius;
    vec3_t mins, maxs;
} reinforcement_t;

typedef struct {
    reinforcement_t *reinforcements;
    int              num_reinforcements;
} reinforcement_list_t;

#define MAX_REINFORCEMENTS_TOTAL 255 // because 255 acts as "none chosen"
#define MAX_REINFORCEMENTS 5 // max number of spawns we can do at once.

#define HOLD_FOREVER INT64_MAX

typedef struct {
    // [Paril-KEX] allow some moves to be done instantaneously, but
    // others can wait the full frame.
    // NB: always use `M_SetAnimation` as it handles edge cases.
    const mmove_t     *active_move, *next_move;
    uint64_t           aiflags; // PGM - unsigned, since we're close to the max
    int                nextframe; // if next_move is set, this is ignored until a frame is ran
    float              scale;

    void (*stand)(edict_t *self);
    void (*idle)(edict_t *self);
    void (*search)(edict_t *self);
    void (*walk)(edict_t *self);
    void (*run)(edict_t *self);
    void (*dodge)(edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity);
    void (*attack)(edict_t *self);
    void (*melee)(edict_t *self);
    void (*sight)(edict_t *self, edict_t *other);
    bool (*checkattack)(edict_t *self);
    void (*setskin)(edict_t *self);
    void (*physics_change)(edict_t *self);

    gtime_t pausetime;
    gtime_t attack_finished;
    gtime_t fire_wait;

    vec3_t                 saved_goal;
    gtime_t                search_time;
    gtime_t                trail_time;
    vec3_t                 last_sighting;
    monster_attack_state_t attack_state;
    bool                   lefty;
    gtime_t                idle_time;
    int                    linkcount;

    item_id_t power_armor_type;
    int       power_armor_power;

    // for monster revive
    item_id_t initial_power_armor_type;
    int       max_power_armor_power;
    int       weapon_sound, engine_sound;

    // ROGUE
    bool (*blocked)(edict_t *self, float dist);
    gtime_t  last_hint_time; // last time the monster checked for hintpaths.
    edict_t *goal_hint;      // which hint_path we're trying to get to
    int      medicTries;
    edict_t *badMedic1, *badMedic2; // these medics have declared this monster "unhealable"
    edict_t *healer;                // this is who is healing this monster
    bool (*duck)(edict_t *self, gtime_t eta);
    void (*unduck)(edict_t *self);
    bool (*sidestep)(edict_t *self);
    float    base_height;
    gtime_t  next_duck_time;
    gtime_t  duck_wait_time;
    edict_t *last_player_enemy;
    // blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
    // (set in the monster) of the next shot
    bool    blindfire;      // will the monster blindfire?
    bool    can_jump;       // will the monster jump?
    bool    had_visibility; // Paril: used for blindfire
    float   drop_height, jump_height;
    gtime_t blind_fire_delay;
    vec3_t  blind_fire_target;
    // used by the spawners to not spawn too much and keep track of #s of monsters spawned
    int      monster_slots; // nb: for spawned monsters, this is how many slots we took from our commander
    int      monster_used;
    edict_t *commander;
    // powerup timers, used by widow, our friend
    gtime_t quad_time;
    gtime_t invincible_time;
    gtime_t double_time;
    // ROGUE

    // Paril
    gtime_t   surprise_time;
    item_id_t armor_type;
    int       armor_power;
    bool      close_sight_tripped;
    gtime_t   melee_debounce_time; // don't melee until this time has passed
    gtime_t   strafe_check_time; // time until we should reconsider strafing
    int       base_health; // health that we had on spawn, before any co-op adjustments
    int       health_scaling; // number of players we've been scaled up to
    gtime_t   next_move_time; // high tick rate
    gtime_t   bad_move_time; // don't try straight moves until this is over
    gtime_t   bump_time; // don't slide against walls for a bit
    gtime_t   random_change_time; // high tickrate
    gtime_t   path_blocked_counter; // break out of paths when > a certain time
    gtime_t   path_wait_time; // don't try nav nodes until this is over
    combat_style_t combat_style; // pathing style

    edict_t *damage_attacker;
    edict_t *damage_inflictor;
    int      damage_blood, damage_knockback;
    vec3_t   damage_from;
    mod_t    damage_mod;

    // alternate flying mechanics
    float fly_max_distance, fly_min_distance; // how far we should try to stay
    float fly_acceleration; // accel/decel speed
    float fly_speed; // max speed from flying
    vec3_t fly_ideal_position; // ideally where we want to end up to hover, relative to our target if not pinned
    gtime_t fly_position_time; // if <= level.time, we can try changing positions
    bool fly_buzzard, fly_above; // orbit around all sides of their enemy, not just the sides
    bool fly_pinned; // whether we're currently pinned to ideal position (made absolute)
    bool fly_thrusters; // slightly different flight mechanics, for melee attacks
    gtime_t fly_recovery_time; // time to try a new dir to get away from hazards
    vec3_t fly_recovery_dir;

    gtime_t checkattack_time;
    int start_frame;
    gtime_t dodge_time;
    int move_block_counter;
    gtime_t move_block_change_time;
    gtime_t react_to_damage_time;

    reinforcement_list_t    reinforcements;
    byte                    chosen_reinforcements[MAX_REINFORCEMENTS]; // readied for spawn; 255 is value for none

    gtime_t jump_time;

    // NOTE: if adding new elements, make sure to add them
    // in g_save.cpp too!
} monsterinfo_t;

// non-monsterinfo save stuff
#define PRETHINK(n) n
#define THINK(n) n
#define TOUCH(n) n
#define USE(n) n
#define PAIN(n) n
#define DIE(n) n

// ROGUE
// this determines how long to wait after a duck to duck again.
// if we finish a duck-up, this gets cut in half.
#define DUCK_INTERVAL   SEC(5)
// ROGUE

extern game_locals_t  game;
extern level_locals_t level;
extern game_export_t  globals;
extern spawn_temp_t   st;

extern edict_t *g_edicts;

static inline float lerp(float a, float b, float f)
{
    return a * (1.0f - f) + b * f;
}

// uniform float [0, 1)
static inline float frandom(void)
{
    return frand();
}

// uniform float [min_inclusive, max_exclusive)
static inline float frandom2(float min_inclusive, float max_exclusive)
{
    return lerp(min_inclusive, max_exclusive, frandom());
}

// uniform float [0, max_exclusive)
static inline float frandom1(float max_exclusive)
{
    return frandom() * max_exclusive;
}

static inline void frandom_vec(vec3_t v, float max_exclusive)
{
    v[0] = frandom1(max_exclusive);
    v[1] = frandom1(max_exclusive);
    v[2] = frandom1(max_exclusive);
}

// uniform time [min_inclusive, max_exclusive)
static inline gtime_t random_time_sec(float min_inclusive, float max_exclusive)
{
    gtime_t a = SEC(min_inclusive);
    gtime_t b = SEC(max_exclusive);
    return a + Q_rand_uniform(b - a);
}

// uniform float [-1, 1)
// note: closed on min but not max
// to match vanilla behavior
static inline float crandom(void)
{
    return crand();
}

// uniform float (-1, 1)
static inline float crandom_open(void)
{
    float f;
    while (q_unlikely((f = crandom()) == -1.0f))
        ;
    return f;
}

static inline void crandom_vec(vec3_t v, float f)
{
    v[0] = crandom() * f;
    v[1] = crandom() * f;
    v[2] = crandom() * f;
}

// raw unsigned int32 value from random
static inline uint32_t irandom(void)
{
    return Q_rand();
}

// uniform int [min, max)
// always returns min if min == (max - 1)
// undefined behavior if min > (max - 1)
static inline int irandom2(int min_inclusive, int max_exclusive)
{
    return min_inclusive + Q_rand_uniform(max_exclusive - min_inclusive);
}

// uniform int [0, max)
// always returns 0 if max <= 0
// note for Q2 code:
// - to fix rand()%x, do irandom(x)
// - to fix rand()&x, do irandom(x + 1)
static inline int irandom1(int max_exclusive)
{
    return Q_rand_uniform(max_exclusive);
}

// flip a coin
static inline bool brandom(void)
{
    return irandom() & 1;
}

#define random_element(array)   ((array)[irandom1(q_countof(array))])

extern cvar_t *deathmatch;
extern cvar_t *coop;
extern cvar_t *skill;
extern cvar_t *fraglimit;
extern cvar_t *timelimit;
// ZOID
extern cvar_t *capturelimit;
extern cvar_t *g_quick_weapon_switch;
extern cvar_t *g_instant_weapon_switch;
// ZOID
extern cvar_t *password;
extern cvar_t *spectator_password;
extern cvar_t *needpass;
extern cvar_t *g_select_empty;
extern cvar_t *sv_dedicated;

extern cvar_t *filterban;

extern cvar_t *sv_gravity;
extern cvar_t *sv_maxvelocity;

extern cvar_t *gun_x, *gun_y, *gun_z;
extern cvar_t *sv_rollspeed;
extern cvar_t *sv_rollangle;

extern cvar_t *run_pitch;
extern cvar_t *run_roll;
extern cvar_t *bob_up;
extern cvar_t *bob_pitch;
extern cvar_t *bob_roll;

extern cvar_t *sv_cheats;
extern cvar_t *g_debug_monster_paths;
extern cvar_t *g_debug_monster_kills;
extern cvar_t *maxspectators;

extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;

extern cvar_t *sv_maplist;

extern cvar_t *g_skipViewModifiers;

extern cvar_t *sv_stopspeed; // PGM - this was a define in g_phys.c

extern cvar_t *g_strict_saves;
extern cvar_t *g_coop_health_scaling;
extern cvar_t *g_weapon_respawn_time;

extern cvar_t *g_no_health;
extern cvar_t *g_no_items;
extern cvar_t *g_dm_weapons_stay;
extern cvar_t *g_dm_no_fall_damage;
extern cvar_t *g_dm_instant_items;
extern cvar_t *g_dm_same_level;
extern cvar_t *g_friendly_fire;
extern cvar_t *g_dm_force_respawn;
extern cvar_t *g_dm_force_respawn_time;
extern cvar_t *g_dm_spawn_farthest;
extern cvar_t *g_no_armor;
extern cvar_t *g_dm_allow_exit;
extern cvar_t *g_infinite_ammo;
extern cvar_t *g_dm_no_quad_drop;
extern cvar_t *g_dm_no_quadfire_drop;
extern cvar_t *g_no_mines;
extern cvar_t *g_dm_no_stack_double;
extern cvar_t *g_no_nukes;
extern cvar_t *g_no_spheres;
extern cvar_t *g_teamplay_armor_protect;
extern cvar_t *g_allow_techs;

extern cvar_t *g_start_items;
extern cvar_t *g_map_list;
extern cvar_t *g_map_list_shuffle;

// ROGUE
extern cvar_t *gamerules;
extern cvar_t *huntercam;
extern cvar_t *g_dm_strong_mines;
extern cvar_t *g_dm_random_items;
// ROGUE

// [Kex]
extern cvar_t *g_instagib;
extern cvar_t *g_coop_player_collision;
extern cvar_t *g_coop_squad_respawn;
extern cvar_t *g_coop_enable_lives;
extern cvar_t *g_coop_num_lives;
extern cvar_t *g_coop_instanced_items;
extern cvar_t *g_allow_grapple;
extern cvar_t *g_grapple_fly_speed;
extern cvar_t *g_grapple_pull_speed;
extern cvar_t *g_grapple_damage;

extern cvar_t *sv_airaccelerate;

extern cvar_t *g_damage_scale;
extern cvar_t *g_disable_player_collision;
extern cvar_t *ai_damage_scale;
extern cvar_t *ai_model_scale;
extern cvar_t *ai_allow_dm_spawn;
extern cvar_t *ai_movement_disabled;
extern cvar_t *g_monster_footsteps;

#define world (&g_edicts[0])

// item spawnflags
#define SPAWNFLAG_ITEM_TRIGGER_SPAWN    0x00000001
#define SPAWNFLAG_ITEM_NO_TOUCH         0x00000002
#define SPAWNFLAG_ITEM_TOSS_SPAWN       0x00000004
#define SPAWNFLAG_ITEM_MAX              0x00000008
// 8 bits reserved for editor flags & power cube bits
// (see SPAWNFLAG_NOT_EASY above)
#define SPAWNFLAG_ITEM_DROPPED          0x00010000
#define SPAWNFLAG_ITEM_DROPPED_PLAYER   0x00020000
#define SPAWNFLAG_ITEM_TARGETS_USED     0x00040000

extern const gitem_t itemlist[IT_TOTAL];

//
// g_cmds.c
//
bool CheckFlood(edict_t *ent);
void Cmd_Help_f(edict_t *ent);
void Cmd_Score_f(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);
void ClientCommand(edict_t *ent);

//
// g_items.c
//
void      PrecacheItem(const gitem_t *it);
void      InitItems(void);
void      SetItemNames(void);
const gitem_t *FindItem(const char *pickup_name);
const gitem_t *FindItemByClassname(const char *classname);
edict_t *Drop_Item(edict_t *ent, const gitem_t *item);
void      SetRespawnEx(edict_t *ent, gtime_t delay, bool hide_self);
void      SetRespawn(edict_t *ent, gtime_t delay);
void      ChangeWeapon(edict_t *ent);
void      SpawnItem(edict_t *ent, const gitem_t *item);
void      Think_Weapon(edict_t *ent);
item_id_t ArmorIndex(edict_t *ent);
item_id_t PowerArmorType(edict_t *ent);
const gitem_t *GetItemByIndex(item_id_t index);
const gitem_t *GetItemByAmmo(ammo_t ammo);
const gitem_t *GetItemByPowerup(powerup_t powerup);
bool      Add_Ammo(edict_t *ent, const gitem_t *item, int count);
void      G_CheckPowerArmor(edict_t *ent);
void      G_CheckAutoSwitch(edict_t *ent, const gitem_t *item, bool is_new);
bool      G_CanDropItem(const gitem_t *item);
void      Touch_Item(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self);
void      droptofloor(edict_t *ent);
void      P_ToggleFlashlight(edict_t *ent, bool state);
qboolean  G_EntityVisibleToClient(edict_t *client, edict_t *ent);

//
// g_utils.c
//
void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result);
void G_ProjectSource2(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t result);
void closest_point_to_box(const vec3_t from, const vec3_t mins, const vec3_t maxs, vec3_t point);
float distance_between_boxes(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
bool boxes_intersect(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
edict_t *G_Find(edict_t *from, int fieldofs, const char *match);
edict_t *findradius(edict_t *from, const vec3_t org, float rad);
edict_t *G_PickTarget(const char *targetname);
void     G_UseTargets(edict_t *ent, edict_t *activator);
void     G_PrintActivationMessage(edict_t *ent, edict_t *activator, bool coop_global);
void     G_SetMovedir(vec3_t angles, vec3_t movedir);
float    vectoyaw(const vec3_t vec);
void     vectoangles(const vec3_t value1, vec3_t angles);
char    *etos(edict_t *ent);

void     G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
void     G_FreeEdict(edict_t *e);

void G_TouchTriggers(edict_t *ent);
void G_TouchProjectiles(edict_t *ent, const vec3_t previous_origin);

char *G_CopyString(const char *in, int tag);

// ROGUE
edict_t *findradius2(edict_t *from, const vec3_t org, float rad);
// ROGUE

void G_PlayerNotifyGoal(edict_t *player);

bool KillBox(edict_t *ent, bool from_spawning, mod_id_t mod, bool bsp_clipping);

//
// g_spawn.c
//
void ED_ParseField(const char *key, const char *value, edict_t *ent);
void ED_CallSpawn(edict_t *ent);
char *ED_NewString(const char *string);
bool ED_WasKeySpecified(const char *key);
void ED_SetKeySpecified(const char *key);
void ED_InitSpawnVars(void);
const char *G_GetLightStyle(int style);
void G_AddPrecache(void (*func)(void));
void G_RefreshPrecaches(void);
void G_PrecacheInventoryItems(void);
void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint);

//
// g_save.c
//
void WriteGame(const char *filename, qboolean autosave);
void ReadGame(const char *filename);
void WriteLevel(const char *filename);
void ReadLevel(const char *filename);
void G_CleanupSaves(void);
qboolean G_CanSave(void);

//
// g_target.c
//
void target_laser_think(edict_t *self);
void target_laser_off(edict_t *self);

#define SPAWNFLAG_LASER_ON          0x0001
#define SPAWNFLAG_LASER_RED         0x0002
#define SPAWNFLAG_LASER_GREEN       0x0004
#define SPAWNFLAG_LASER_BLUE        0x0008
#define SPAWNFLAG_LASER_YELLOW      0x0010
#define SPAWNFLAG_LASER_ORANGE      0x0020
#define SPAWNFLAG_LASER_FAT         0x0040
#define SPAWNFLAG_LASER_ZAP         0x80000000
#define SPAWNFLAG_LASER_LIGHTNING   0x00010000

#define SPAWNFLAG_HEALTHBAR_PVS_ONLY    1

// damage flags
typedef enum {
    DAMAGE_NONE             = 0,      // no damage flags
    DAMAGE_RADIUS           = BIT(0), // damage was indirect
    DAMAGE_NO_ARMOR         = BIT(1), // armour does not protect from this damage
    DAMAGE_ENERGY           = BIT(2), // damage is from an energy based weapon
    DAMAGE_NO_KNOCKBACK     = BIT(3), // do not affect velocity, just view angles
    DAMAGE_BULLET           = BIT(4), // damage is from a bullet (used for ricochets)
    DAMAGE_NO_PROTECTION    = BIT(5), // armor, shields, invulnerability, and godmode have no effect
    // ROGUE
    DAMAGE_DESTROY_ARMOR    = BIT(6), // damage is done to armor and health.
    DAMAGE_NO_REG_ARMOR     = BIT(7), // damage skips regular armor
    DAMAGE_NO_POWER_ARMOR   = BIT(8), // damage skips power armor
    // ROGUE
    DAMAGE_NO_INDICATOR     = BIT(9), // for clients: no damage indicators
} damageflags_t;

//
// g_combat.c
//
bool OnSameTeam(edict_t *ent1, edict_t *ent2);
bool CanDamage(edict_t *targ, edict_t *inflictor);
bool CheckTeamDamage(edict_t *targ, edict_t *attacker);
void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, const vec3_t dir, const vec3_t point,
              const vec3_t normal, int damage, int knockback, damageflags_t dflags, mod_t mod);
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius,
                    damageflags_t dflags, mod_t mod);
void Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);

// ROGUE
void T_RadiusNukeDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, mod_t mod);
void T_RadiusClassDamage(edict_t *inflictor, edict_t *attacker, float damage, char *ignoreClass, float radius, mod_t mod);
void cleanupHealTarget(edict_t *ent);
// ROGUE

#define DEFAULT_BULLET_HSPREAD              300
#define DEFAULT_BULLET_VSPREAD              500
#define DEFAULT_SHOTGUN_HSPREAD             1000
#define DEFAULT_SHOTGUN_VSPREAD             500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT    12
#define DEFAULT_SHOTGUN_COUNT               12
#define DEFAULT_SSHOTGUN_COUNT              20

//
// g_func.c
//
void train_use(edict_t *self, edict_t *other, edict_t *activator);
void func_train_find(edict_t *self);
edict_t *plat_spawn_inside_trigger(edict_t *ent);
void Move_Calc(edict_t *ent, const vec3_t dest, void (*endfunc)(edict_t *self));
void G_SetMoveinfoSounds(edict_t *self, const char *default_start, const char *default_mid, const char *default_end);

#define SPAWNFLAG_TRAIN_START_ON        1
#define SPAWNFLAG_WATER_SMART           2
#define SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN  8
#define SPAWNFLAG_DOOR_REVERSE          2

//
// g_monster.c
//

typedef enum {
    WATER_NONE,
    WATER_FEET,
    WATER_WAIST,
    WATER_UNDER
} water_level_t;

void monster_muzzleflash(edict_t *self, const vec3_t start, monster_muzzleflash_id_t id);
void monster_fire_bullet(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int kick, int hspread,
                         int vspread, monster_muzzleflash_id_t flashtype);
void monster_fire_shotgun(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick, int hspread,
                          int vspread, int count, monster_muzzleflash_id_t flashtype);
void monster_fire_blaster(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                          monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_flechette(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                            monster_muzzleflash_id_t flashtype);
void monster_fire_grenade(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed,
                          monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust);
void monster_fire_rocket(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                         monster_muzzleflash_id_t flashtype);
void monster_fire_railgun(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick,
                          monster_muzzleflash_id_t flashtype);
void monster_fire_bfg(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, int kick,
                      float damage_radius, monster_muzzleflash_id_t flashtype);
bool M_CheckClearShot(edict_t *self, const vec3_t offset);
bool M_CheckClearShotEx(edict_t *self, const vec3_t offset, vec3_t start);
void M_ProjectFlashSource(edict_t *self, const vec3_t offset, const vec3_t forward, const vec3_t right, vec3_t start);
bool M_droptofloor_generic(vec3_t origin, const vec3_t mins, const vec3_t maxs, bool ceiling,
                           edict_t *ignore, contents_t mask, bool allow_partial);
bool M_droptofloor(edict_t *ent);
void monster_think(edict_t *self);
void monster_dead_think(edict_t *self);
void monster_dead(edict_t *self);
void monster_footstep(edict_t *self);
void walkmonster_start(edict_t *self);
void swimmonster_start(edict_t *self);
void flymonster_start(edict_t *self);
void monster_death_use(edict_t *self);
void M_CategorizePosition(edict_t *self, const vec3_t in_point, water_level_t *waterlevel, contents_t *watertype);
void M_WorldEffects(edict_t *ent);
bool M_CheckAttack(edict_t *self);
void M_CheckGround(edict_t *ent, contents_t mask);
void monster_use(edict_t *self, edict_t *other, edict_t *activator);
void M_ProcessPain(edict_t *e);
bool M_ShouldReactToPain(edict_t *self, mod_t mod);
void M_SetAnimation(edict_t *self, const mmove_t *move);
void M_SetAnimationEx(edict_t *self, const mmove_t *move, bool instant);
bool M_AllowSpawn(edict_t * self);
void M_SetEffects(edict_t *ent);
void G_MonsterKilled(edict_t *self);

// Paril: used in N64. causes them to be mad at the player
// regardless of circumstance.
#define HACKFLAG_ATTACK_PLAYER  1
// used in N64, appears to change their behavior for the end scene.
#define HACKFLAG_END_CUTSCENE   4

bool monster_start(edict_t *self);
void monster_start_go(edict_t *self);
// RAFAEL
void monster_fire_ionripper(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                            monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_heat(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                       monster_muzzleflash_id_t flashtype, float lerp_factor);
void monster_fire_dabeam(edict_t *self, int damage, bool secondary, void (*update_func)(edict_t *self));
void dabeam_update(edict_t *self, bool damage);
void monster_fire_blueblaster(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                              monster_muzzleflash_id_t flashtype, effects_t effect);
void G_Monster_CheckCoopHealthScaling(void);
// RAFAEL
// ROGUE
void monster_fire_blaster2(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                           monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_tracker(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, edict_t *enemy,
                          monster_muzzleflash_id_t flashtype);
void monster_fire_heatbeam(edict_t *self, const vec3_t start, const vec3_t dir, const vec3_t offset, int damage,
                           int kick, monster_muzzleflash_id_t flashtype);
void stationarymonster_start(edict_t *self);
void monster_done_dodge(edict_t *self);
// ROGUE

typedef enum {
    GOOD_POSITION,
    STUCK_FIXED,
    NO_GOOD_POSITION
} stuck_result_t;

stuck_result_t G_FixStuckObject_Generic(vec3_t origin, const vec3_t own_mins, const vec3_t own_maxs, edict_t *ignore, contents_t mask);

stuck_result_t G_FixStuckObject(edict_t *self, vec3_t check);

// this is for the count of monsters
int M_SlotsLeft(edict_t *self);
void M_SetupReinforcements(const char *reinforcements, reinforcement_list_t *list);
int M_PickReinforcements(edict_t *self, int max_slots);

// shared with monsters
#define SPAWNFLAG_MONSTER_AMBUSH        1
#define SPAWNFLAG_MONSTER_TRIGGER_SPAWN 2
#define SPAWNFLAG_MONSTER_DEAD          BIT(16)
#define SPAWNFLAG_MONSTER_SUPER_STEP    BIT(17)
#define SPAWNFLAG_MONSTER_NO_DROP       BIT(18)
#define SPAWNFLAG_MONSTER_SCENIC        BIT(19)

// fixbot spawnflags
#define SPAWNFLAG_FIXBOT_FIXIT      4
#define SPAWNFLAG_FIXBOT_TAKEOFF    8
#define SPAWNFLAG_FIXBOT_LANDING    16
#define SPAWNFLAG_FIXBOT_WORKING    32

//
// g_misc.c
//
typedef struct {
    const char *gibname;
    uint16_t count;
    uint16_t type;
    float scale;
} gib_def_t;

void ThrowClientHead(edict_t *self, int damage);
void gib_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);
edict_t *ThrowGib(edict_t *self, const char *gibname, int damage, gib_type_t type, float scale);
void ThrowGibs(edict_t *self, int damage, const gib_def_t *gibs);
void PrecacheGibs(const gib_def_t *gibs);
void BecomeExplosion1(edict_t *self);
void misc_viper_use(edict_t *self, edict_t *other, edict_t *activator);
void misc_strogg_ship_use(edict_t *self, edict_t *other, edict_t *activator);
void VelocityForDamage(int damage, vec3_t v);
void ClipGibVelocity(edict_t *ent);

#define SPAWNFLAG_PATH_CORNER_TELEPORT  1
#define SPAWNFLAG_POINT_COMBAT_HOLD     1

// max chars for a clock string;
// " 0:00:00" is the longest string possible
// plus null terminator.
#define CLOCK_MESSAGE_SIZE  9

//
// g_ai.c
//
edict_t *AI_GetSightClient(edict_t *self);

void ai_stand(edict_t *self, float dist);
void ai_move(edict_t *self, float dist);
void ai_walk(edict_t *self, float dist);
void ai_turn(edict_t *self, float dist);
void ai_run(edict_t *self, float dist);
void ai_charge(edict_t *self, float dist);

#define RANGE_MELEE 20 // bboxes basically touching
#define RANGE_NEAR  440
#define RANGE_MID   940

// [Paril-KEX] adjusted to return an actual distance, measured
// in a way that is consistent regardless of what is fighting what
float range_to(edict_t *self, edict_t *other);

bool FindTarget(edict_t *self);
void FoundTarget(edict_t *self);
void HuntTarget(edict_t *self, bool animate_state);
bool infront(edict_t *self, edict_t *other);
bool visible_ex(edict_t *self, edict_t *other, contents_t mask);
bool visible(edict_t *self, edict_t *other);
bool FacingIdeal(edict_t *self);

// [Paril-KEX] generic function
bool M_CheckAttack_Base(edict_t *self, float stand_ground_chance, float melee_chance, float near_chance,
                        float mid_chance, float far_chance, float strafe_scalar);

//
// g_weapon.c
//
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick,
                 int hspread, int vspread, mod_t mod);
void fire_shotgun(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick,
                  int hspread, int vspread, int count, mod_t mod);
void blaster_touch(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self);
void fire_blaster(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed,
                  effects_t effect, mod_t mod);
void Grenade_Explode(edict_t *ent);
void fire_grenade(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, gtime_t timer,
                  float damage_radius, float right_adjust, float up_adjust, bool monster);
void fire_grenade2(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, gtime_t timer,
                   float damage_radius, bool held);
void rocket_touch(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self);
edict_t *fire_rocket(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                     float damage_radius, int radius_damage);
void fire_rail(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick);
void fire_bfg(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius);
// RAFAEL
void fire_ionripper(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, effects_t effect);
void fire_heat(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius,
               int radius_damage, float turn_fraction);
void fire_blueblaster(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, effects_t effect);
void fire_plasma(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed,
                 float damage_radius, int radius_damage);
void fire_trap(edict_t *self, const vec3_t start, const vec3_t aimdir, int speed);
// RAFAEL
void fire_disintegrator(edict_t *self, const vec3_t start, const vec3_t dir, int speed);
void P_AddWeaponKick(edict_t *ent, float scale, float pitch);
void P_GetThrowAngles(edict_t *ent, vec3_t angles);

//
// g_ptrail.c
//
void PlayerTrail_Add(edict_t *player);
void PlayerTrail_Destroy(edict_t *player);
edict_t *PlayerTrail_Pick(edict_t *self, bool next);

//
// g_client.c
//
#define SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY   8
#define SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT    16
#define SPAWNFLAG_CHANGELEVEL_FADE_OUT          32
#define SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE   64

void respawn(edict_t *ent);
void BeginIntermission(edict_t *targ);
void PutClientInServer(edict_t *ent);
void InitClientPersistant(edict_t *ent, gclient_t *client);
void InitClientResp(gclient_t *client);
void InitBodyQue(void);
void ClientBeginServerFrame(edict_t *ent);
void ClientThink(edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect(edict_t *ent, char *userinfo);
void ClientDisconnect(edict_t *ent);
void ClientBegin(edict_t *ent);
void ClientUserinfoChanged(edict_t *ent, char *userinfo);
void P_AssignClientSkinnum(edict_t *ent);
float PlayersRangeFromSpot(edict_t *spot);
bool SpawnPointClear(edict_t *spot);
bool SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles, bool force_spawn);
edict_t *SelectDeathmatchSpawnPoint(bool farthest, bool force_spawn, bool fallback_to_ctf_or_start, bool *any_valid);
void G_PostRespawn(edict_t *self);
void LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker);

//
// g_player.c
//
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);

//
// g_svcmds.c
//
void ServerCommand(void);
bool SV_FilterPacket(const char *from);

//
// p_view.c
//
void ClientEndServerFrame(edict_t *ent);

//
// p_hud.c
//
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void G_SetCoopStats(edict_t *ent);
void G_SetSpectatorStats(edict_t *ent);
void G_CheckChaseStats(edict_t *ent);
void DeathmatchScoreboardMessage(edict_t *client, edict_t *killer);
void G_UpdateLevelEntry(void);
void G_EndOfUnitMessage(void);

//
// p_weapon.c
//
void PlayerNoise(edict_t *who, const vec3_t where, player_noise_t type);
void P_ProjectSource(edict_t *ent, const vec3_t angles, const vec3_t distance, vec3_t result_start, vec3_t result_dir);
void NoAmmoWeaponChange(edict_t *ent, bool sound);
void G_RemoveAmmo(edict_t *ent);
void G_RemoveAmmoEx(edict_t *ent, int quantity);
void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
                    int FRAME_DEACTIVATE_LAST, const int *pause_frames, const int *fire_frames,
                    void (*fire)(edict_t *ent));
void Weapon_Repeating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
                      int FRAME_DEACTIVATE_LAST, const int *pause_frames, void (*fire)(edict_t *ent));
void Throw_Generic(edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_PRIME_SOUND,
                   const char *prime_sound, int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, const int *pause_frames,
                   bool explode, const char *primed_sound, void (*fire)(edict_t *ent, bool held), bool extra_idle_frame);
int P_DamageModifier(edict_t *ent);
bool G_CheckInfiniteAmmo(const gitem_t *item);
void Weapon_PowerupSound(edict_t *ent);

#define GRENADE_TIMER_SEC   3.0f
#define GRENADE_MINSPEED    400
#define GRENADE_MAXSPEED    800

extern bool is_quad;
// RAFAEL
extern bool is_quadfire;
// RAFAEL
extern player_muzzle_t is_silenced;
// ROGUE
extern int damage_multiplier;
// ROGUE

//
// m_move.c
//
bool M_CheckBottom_Fast_Generic(const vec3_t absmins, const vec3_t absmaxs, bool ceiling);
bool M_CheckBottom_Slow_Generic(const vec3_t origin, const vec3_t absmins, const vec3_t absmaxs,
                                edict_t *ignore, contents_t mask, bool ceiling, bool allow_any_step_height);
bool M_CheckBottom(edict_t *ent);
bool SV_CloseEnough(edict_t *ent, edict_t *goal, float dist);
bool M_walkmove(edict_t *ent, float yaw, float dist);
void M_MoveToGoal(edict_t *ent, float dist);
void M_ChangeYaw(edict_t *ent);
bool ai_check_move(edict_t *self, float dist);
void slerp(const vec3_t from, const vec3_t to, float t, vec3_t out);

//
// g_phys.c
//
#define sv_friction         6
#define sv_waterfriction    1

void G_RunEntity(edict_t *ent);
bool SV_RunThink(edict_t *ent);
void SV_AddRotationalFriction(edict_t *ent);
void SV_AddGravity(edict_t *ent);
void SV_CheckVelocity(edict_t *ent);
void SV_FlyMove(edict_t *ent, float time, contents_t mask);
contents_t G_GetClipMask(edict_t *ent);
void G_Impact(edict_t *e1, const trace_t *trace);
void ClipVelocity(const vec3_t in, const vec3_t normal, vec3_t out, float overbounce);
void SlideClipVelocity(const vec3_t in, const vec3_t normal, vec3_t out, float overbounce);

//
// g_main.c
//
void SaveClientData(void);
void FetchClientEntData(edict_t *ent);
void EndDMLevel(void);

//
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

//====================
// ROGUE PROTOTYPES
//
// g_newweap.c
//
void fire_flechette(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, int kick);
void fire_prox(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed);
void fire_nuke(edict_t *self, const vec3_t start, const vec3_t aimdir, int speed);
bool fire_player_melee(edict_t *self, const vec3_t start, const vec3_t aim, int reach, int damage, int kick, mod_t mod);
void fire_tesla(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed);
void fire_blaster2(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, effects_t effect, bool hyper);
void fire_heatbeam(edict_t *self, const vec3_t start, const vec3_t aimdir, const vec3_t offset, int damage, int kick, bool monster);
void fire_tracker(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, edict_t *enemy);

//
// g_newai.c
//
bool blocked_checkplat(edict_t *self, float dist);

typedef enum {
    NO_JUMP,
    JUMP_TURN,
    JUMP_JUMP_UP,
    JUMP_JUMP_DOWN
} blocked_jump_result_t;

blocked_jump_result_t blocked_checkjump(edict_t *self, float dist);
bool monsterlost_checkhint(edict_t *self);
bool inback(edict_t *self, edict_t *other);
float realrange(edict_t *self, edict_t *other);
edict_t *SpawnBadArea(const vec3_t mins, const vec3_t maxs, gtime_t lifespan, edict_t *owner);
edict_t *CheckForBadArea(edict_t *ent);
bool MarkTeslaArea(edict_t *self, edict_t *tesla);
void InitHintPaths(void);
void PredictAim(edict_t *self, edict_t *target, const vec3_t start, float bolt_speed, bool eye_height, float offset, vec3_t aimdir, vec3_t aimpoint);
bool M_CalculatePitchToFire(edict_t *self, const vec3_t target, const vec3_t start, vec3_t aim, float speed, float time_remaining, bool mortar, bool destroy_on_touch);
bool below(edict_t *self, edict_t *other);
void drawbbox(edict_t *self);
void M_MonsterDodge(edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity);
void monster_duck_down(edict_t *self);
void monster_duck_hold(edict_t *self);
void monster_duck_up(edict_t *self);
bool has_valid_enemy(edict_t *self);
void TargetTesla(edict_t *self, edict_t *tesla);
void hintpath_stop(edict_t *self);
edict_t *PickCoopTarget(edict_t *self);
int CountPlayers(void);
bool monster_jump_finished(edict_t *self);
void BossExplode(edict_t *self);

// g_rogue_func
void plat2_spawn_danger_area(edict_t *ent);
void plat2_kill_danger_area(edict_t *ent);

// g_rogue_spawn
edict_t *CreateMonster(const vec3_t origin, const vec3_t angles, const char *classname);
edict_t *CreateFlyMonster(const vec3_t origin, const vec3_t angles, const vec3_t mins, const vec3_t maxs,
                          const char *classname);
edict_t *CreateGroundMonster(const vec3_t origin, const vec3_t angles, const vec3_t mins, const vec3_t maxs,
                             const char *classname, float height);
bool     FindSpawnPoint(const vec3_t startpoint, const vec3_t mins, const vec3_t maxs, vec3_t spawnpoint,
                        float maxMoveUp, bool drop);
bool     CheckSpawnPoint(const vec3_t origin, const vec3_t mins, const vec3_t maxs);
bool     CheckGroundSpawnPoint(const vec3_t origin, const vec3_t entMins, const vec3_t entMaxs,
                               float height, float gravity);
void     SpawnGrow_Spawn(const vec3_t startpos, float start_size, float end_size);
void     Widowlegs_Spawn(const vec3_t startpos, const vec3_t angles);

// g_rogue_items
bool Pickup_Nuke(edict_t *ent, edict_t *other);
void Use_IR(edict_t *ent, const gitem_t *item);
void Use_Double(edict_t *ent, const gitem_t *item);
void Use_Nuke(edict_t *ent, const gitem_t *item);
void Use_Doppleganger(edict_t *ent, const gitem_t *item);
bool Pickup_Doppleganger(edict_t *ent, edict_t *other);
bool Pickup_Sphere(edict_t *ent, edict_t *other);
void Use_Defender(edict_t *ent, const gitem_t *item);
void Use_Hunter(edict_t *ent, const gitem_t *item);
void Use_Vengeance(edict_t *ent, const gitem_t *item);
void Item_TriggeredSpawn(edict_t *self, edict_t *other, edict_t *activator);
void SetTriggeredSpawn(edict_t *ent);

//
// g_sphere.c
//
void Defender_Launch(edict_t *self);
void Vengeance_Launch(edict_t *self);
void Hunter_Launch(edict_t *self);

//
// g_newdm.c
//
void     InitGameRules(void);
item_id_t DoRandomRespawn(edict_t *ent);
void     PrecacheForRandomRespawn(void);
bool     Tag_PickupToken(edict_t *ent, edict_t *other);
void     fire_doppleganger(edict_t *ent, const vec3_t start, const vec3_t aimdir);

//
// p_client.c
//
void RemoveAttackingPainDaemons(edict_t *self);
bool G_ShouldPlayersCollide(bool weaponry);
bool P_UseCoopInstancedItems(void);

#define SPAWNFLAG_LANDMARK_KEEP_Z   1

// [Paril-KEX] convenience functions that returns true
// if the powerup should be 'active' (false to disable,
// will flash at 500ms intervals after 3 sec)
static inline bool G_PowerUpExpiringRelative(gtime_t left)
{
    return TO_MSEC(left) > 3000 || (TO_MSEC(left) % 1000) < 500;
}

static inline bool G_PowerUpExpiring(gtime_t time)
{
    return G_PowerUpExpiringRelative(time - level.time);
}

//
// g_l10n.c
//
void G_LoadL10nFile(void);
void G_FreeL10nFile(void);
const char *G_GetL10nString(const char *key);

#define MAX_NETNAME 16

// ZOID
#include "ctf/g_ctf.h"
#include "ctf/p_ctf_menu.h"
// ZOID

//============================================================================

// client_t->anim_priority
typedef enum {
    ANIM_BASIC, // stand / run
    ANIM_WAVE,
    ANIM_JUMP,
    ANIM_PAIN,
    ANIM_ATTACK,
    ANIM_DEATH,

    // flags
    ANIM_REVERSED = BIT(8)
} anim_priority_t;

enum {
    GESTURE_NONE = -1,
    GESTURE_FLIP_OFF,
    GESTURE_SALUTE,
    GESTURE_TAUNT,
    GESTURE_WAVE,
    GESTURE_POINT,
    GESTURE_POINT_NO_PING,
    GESTURE_MAX
};

// player_state->stats[] indexes
enum {
    STAT_CTF_TEAM1_PIC = 18,
    STAT_CTF_TEAM1_CAPS = 19,
    STAT_CTF_TEAM2_PIC = 20,
    STAT_CTF_TEAM2_CAPS = 21,
    STAT_CTF_FLAG_PIC = 22,
    STAT_CTF_JOINED_TEAM1_PIC = 23,
    STAT_CTF_JOINED_TEAM2_PIC = 24,
    STAT_CTF_TEAM1_HEADER = 25,
    STAT_CTF_TEAM2_HEADER = 26,
    STAT_CTF_TECH = 27,
    STAT_CTF_ID_VIEW = 28,
    STAT_CTF_MATCH = 29,
    STAT_CTF_ID_VIEW_COLOR = 30,
    STAT_CTF_TEAMINFO = 31,

    // [Paril-KEX] Key display
    STAT_KEY_A = 18,
    STAT_KEY_B = 19,
    STAT_KEY_C = 20,

    // [Paril-KEX] top of screen coop respawn state
    STAT_COOP_RESPAWN = 21,

    // [Paril-KEX] respawns remaining
    STAT_LIVES = 22,

    // [Paril-KEX]
    STAT_HEALTH_BARS = 23, // two health bar values (0 - inactive, 1 - dead, 2-255 - alive)

    // [Paril-KEX]
    STAT_SELECTED_ITEM_NAME = 31,
};

#define SELECTED_ITEM_TIME SEC(3)

// state for coop respawning; used to select which
// message to print for the player this is set on.
typedef enum {
    COOP_RESPAWN_NONE, // no messagee
    COOP_RESPAWN_IN_COMBAT, // player is in combat
    COOP_RESPAWN_BAD_AREA, // player not in a good spot
    COOP_RESPAWN_BLOCKED, // spawning was blocked by something
    COOP_RESPAWN_WAITING, // for players that are waiting to respawn
    COOP_RESPAWN_NO_LIVES, // out of lives, so need to wait until level switch
    COOP_RESPAWN_TOTAL
} coop_respawn_t;

// reserved general CS ranges
enum {
    CONFIG_CTF_MATCH = CS_GENERAL,
    CONFIG_CTF_TEAMINFO,
    CONFIG_CTF_PLAYER_NAME,
    CONFIG_CTF_PLAYER_NAME_END = CONFIG_CTF_PLAYER_NAME + MAX_CLIENTS,

    // nb: offset by 1 since NONE is zero
    CONFIG_COOP_RESPAWN_STRING,
    CONFIG_COOP_RESPAWN_STRING_END = CONFIG_COOP_RESPAWN_STRING + (COOP_RESPAWN_TOTAL - 1),

    CONFIG_HEALTH_BAR_NAME, // active health bar name

    CONFIG_LAST
};

typedef enum {
    BMODEL_ANIM_FORWARDS,
    BMODEL_ANIM_BACKWARDS,
    BMODEL_ANIM_RANDOM
} bmodel_anim_style_t;

typedef struct {
    int start, end; // range, inclusive
    bmodel_anim_style_t style;
    int speed; // in milliseconds
    bool nowrap;
} bmodel_anim_param_t;

typedef struct {
    bmodel_anim_param_t params[2];

    // game-only
    bool                enabled;
    bool                alternate, currently_alternate;
    gtime_t             next_tick;
} bmodel_anim_t;

// never turn back shield on automatically; this is
// the legacy behavior.
#define AUTO_SHIELD_MANUAL  -1
// when it is >= 0, the shield will turn back on
// when we have that many cells in our inventory
// if possible.
#define AUTO_SHIELD_AUTO    0

// client data that stays across multiple level loads
typedef struct {
    char          userinfo[MAX_INFO_STRING];
    char          social_id[MAX_INFO_VALUE];
    char          netname[MAX_NETNAME];
    handedness_t  hand;
    auto_switch_t autoswitch;
    int           autoshield; // see AUTO_SHIELD_*

    bool connected, spawned; // a loadgame will leave valid entities that
                             // just don't have a connection yet

    // values saved and restored from edicts when changing levels
    int         health;
    int         max_health;
    uint64_t    savedFlags;

    item_id_t selected_item;
    gtime_t   selected_item_time;
    int       inventory[IT_TOTAL];

    // ammo capacities
    int16_t max_ammo[AMMO_MAX];

    const gitem_t *weapon;
    const gitem_t *lastweapon;

    int power_cubes; // used for tracking the cubes in coop games
    int score;       // for calculating total unit score in coop games

    int game_help1changed, game_help2changed;
    int helpchanged; // flash F1 icon if non 0, play sound
                     // and increment only if 1, 2, or 3
    gtime_t help_time;

    bool spectator; // client wants to be a spectator
    bool bob_skip; // [Paril-KEX] client wants no movement bob

    gtime_t megahealth_time; // relative megahealth time value
    int lives; // player lives left (1 = no respawns remaining)
} client_persistent_t;

// client data that stays across deathmatch respawns
typedef struct {
    client_persistent_t coop_respawn; // what to set client->pers to on a respawn
    gtime_t             entertime;    // level.time the client entered the game
    int                 score;        // frags, etc
    vec3_t              cmd_angles;   // angles sent over in the last command

    bool spectator; // client is a spectator

    // ZOID
    ctfteam_t ctf_team; // CTF team
    int       ctf_state;
    gtime_t   ctf_lasthurtcarrier;
    gtime_t   ctf_lastreturnedflag;
    gtime_t   ctf_flagsince;
    gtime_t   ctf_lastfraggedcarrier;
    bool      id_state;
    gtime_t   lastidtime;
    bool      voted; // for elections
    bool      ready;
    bool      admin;
    ghost_t  *ghost; // for ghost codes
    // ZOID
} client_respawn_t;

// [Paril-KEX] seconds until we are fully invisible after
// making a racket
#define INVISIBILITY_TIME   SEC(2)

// time between ladder sounds
#define LADDER_SOUND_TIME   SEC(0.3f)

// time after damage that we can't respawn on a player for
#define COOP_DAMAGE_RESPAWN_TIME    SEC(2)

// time after firing that we can't respawn on a player for
#define COOP_DAMAGE_FIRING_TIME     SEC(2.5f)

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s {
    // shared with server; do not touch members until the "private" section
    player_state_t ps; // communicated by server to clients
    int            ping;

    // private to game
    client_persistent_t pers;
    client_respawn_t    resp;
    pmove_state_t       old_pmove; // for detecting out-of-pmove changes

    bool showscores;    // set layout stat
    bool showeou;       // end of unit screen
    bool showinventory; // set layout stat
    bool showhelp;

    button_t  buttons;
    button_t  oldbuttons;
    button_t  latched_buttons;
    usercmd_t cmd; // last CMD send

    // weapon cannot fire until this time is up
    gtime_t weapon_fire_finished;
    // time between processing individual animation frames
    gtime_t weapon_think_time;
    // if we latched fire between server frames but before
    // the weapon fire finish has elapsed, we'll "press" it
    // automatically when we have a chance
    bool weapon_fire_buffered;
    bool weapon_thunk;

    const gitem_t *newweapon;

    // sum up damage over an entire frame, so
    // shotgun blasts give a single big kick
    int    damage_armor;     // damage absorbed by armor
    int    damage_parmor;    // damage absorbed by power armor
    int    damage_blood;     // damage taken out of health
    int    damage_knockback; // impact damage
    vec3_t damage_from;      // origin for vector calculation

    float killer_yaw; // when dead, look at killer

    weaponstate_t weaponstate;
    struct {
        vec3_t  angles, origin;
        gtime_t time, total;
    } kick;
    gtime_t       quake_time;
    float         v_dmg_roll, v_dmg_pitch;
    gtime_t       v_dmg_time; // damage kicks
    gtime_t       fall_time;
    float         fall_value; // for view drop on fall
    float         damage_alpha;
    float         bonus_alpha;
    vec3_t        damage_blend;
    vec3_t        v_angle, v_forward; // aiming direction
    float         bobtime;            // so off-ground doesn't change it
    vec3_t        oldviewangles;
    vec3_t        oldvelocity;
    edict_t      *oldgroundentity; // [Paril-KEX]
    gtime_t       flash_time; // [Paril-KEX] for high tickrate

    gtime_t       next_drown_time;
    water_level_t old_waterlevel;
    int           breather_sound;

    // animation vars
    int             anim_end;
    anim_priority_t anim_priority;
    bool            anim_duck;
    bool            anim_run;
    gtime_t         anim_time;

    // powerup timers
    gtime_t quad_time;
    gtime_t invincible_time;
    gtime_t breather_time;
    gtime_t enviro_time;
    gtime_t invisible_time;

    bool    grenade_blew_up;
    gtime_t grenade_time, grenade_finished_time;
    // RAFAEL
    gtime_t quadfire_time;
    // RAFAEL
    int silencer_shots;
    int weapon_sound;

    gtime_t pickup_msg_time;

    gtime_t flood_locktill; // locked from talking
    gtime_t flood_when[10]; // when messages were said
    int     flood_whenhead; // head pointer for when said

    gtime_t respawn_time; // can respawn when time > this

    edict_t *chase_target; // player we are chasing
    bool     update_chase; // need to update chase info?

    //=======
    // ROGUE
    gtime_t double_time;
    gtime_t ir_time;
    gtime_t nuke_time;
    gtime_t tracker_pain_time;

    edict_t *owned_sphere; // this points to the player's sphere
    // ROGUE
    //=======

    gtime_t empty_click_sound;

    // ZOID
    bool        inmenu;   // in menu
    pmenuhnd_t *menu;     // current menu
    gtime_t     menutime; // time to update menu
    bool        menudirty;
    edict_t    *ctf_grapple;            // entity of grapple
    int         ctf_grapplestate;       // true if pulling
    gtime_t     ctf_grapplereleasetime; // time of grapple release
    gtime_t     ctf_regentime;          // regen tech
    gtime_t     ctf_techsndtime;
    gtime_t     ctf_lasttechmsg;
    // ZOID

    // used for player trails.
    edict_t *trail_head, *trail_tail;
    // whether to use weapon chains
    bool no_weapon_chains;

    // seamless level transitions
    bool landmark_free_fall;
    char *landmark_name;
    vec3_t landmark_rel_pos; // position relative to landmark, un-rotated from landmark angle
    gtime_t landmark_noise_time;

    gtime_t invisibility_fade_time; // [Paril-KEX] at this time, the player will be mostly fully cloaked
    gtime_t chase_msg_time; // to prevent CTF message spamming
    int menu_sign; // menu sign
    vec3_t last_ladder_pos; // for ladder step sounds
    gtime_t last_ladder_sound;
    coop_respawn_t coop_respawn_state;
    gtime_t last_damage_time;

    // [Paril-KEX] these are now per-player, to work better in coop
    edict_t *sight_entity;
    gtime_t  sight_entity_time;
    edict_t *sound_entity;
    gtime_t  sound_entity_time;
    edict_t *sound2_entity;
    gtime_t  sound2_entity_time;
    // for high tickrate weapon angles
    vec3_t   slow_view_angles;
    gtime_t  slow_view_angle_time;

    // only set temporarily
    bool awaiting_respawn;
    gtime_t respawn_timeout; // after this time, force a respawn

    gtime_t  last_attacker_time;
    // saved - for coop; last time we were in a firing state
    gtime_t  last_firing_time;
};

// ==========================================
// PLAT 2
// ==========================================
typedef enum {
    PLAT2_NONE    = 0,
    PLAT2_CALLED  = BIT(0),
    PLAT2_MOVING  = BIT(1),
    PLAT2_WAITING = BIT(2)
} plat2flags_t;

struct edict_s {
    // shared with server; do not touch members until the "private" section
    entity_state_t s;
    gclient_t     *client; // NULL if not a player
    // the server expects the first part
    // of gclient_t to be a player_state_t
    // but the rest of it is opaque

    qboolean     inuse;

    // world linkage data
    int     linkcount;
    list_t  area; // linked to a division node or leaf
    int     num_clusters; // if -1, use headnode instead
    int     clusternums[MAX_ENT_CLUSTERS];
    int     headnode; // unused if num_clusters != -1
    int     areanum, areanum2;

    svflags_t  svflags;
    vec3_t     mins, maxs;
    vec3_t     absmin, absmax, size;
    solid_t    solid;
    contents_t clipmask;
    edict_t    *owner;

    entity_state_extension_t    x;

    //================================

    // private to game
    int spawn_count; // [Paril-KEX] used to differentiate different entities that may be in the same slot
    movetype_t  movetype;
    uint64_t    flags;

    const char *model;
    gtime_t     freetime; // sv.time when the object was freed

    //
    // only used locally in game, not by server
    //
    const char      *message;
    const char      *classname;
    spawnflags_t    spawnflags;

    gtime_t timestamp;

    float       angle; // set in qe3, -1 = up, -2 = down
    const char *target;
    const char *targetname;
    const char *killtarget;
    const char *team;
    const char *pathtarget;
    const char *deathtarget;
    const char *healthtarget;
    const char *itemtarget; // [Paril-KEX]
    const char *combattarget;
    edict_t *target_ent;

    float  speed, accel, decel;
    vec3_t movedir;
    vec3_t pos1, pos2, pos3;

    vec3_t  velocity;
    vec3_t  avelocity;
    int     mass;
    gtime_t air_finished;
    float   gravity; // per entity gravity multiplier (1.0 is normal)
                     // use for lowgrav artifact, flares

    edict_t *goalentity;
    edict_t *movetarget;
    float    yaw_speed;
    float    ideal_yaw;

    gtime_t nextthink;
    void (*prethink)(edict_t *self);
    void (*postthink)(edict_t *self);
    void (*think)(edict_t *self);
    void (*touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self);
    void (*use)(edict_t *self, edict_t *other, edict_t *activator);
    void (*pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod);
    void (*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);

    gtime_t touch_debounce_time; // are all these legit?  do we need more/less of them?
    gtime_t pain_debounce_time;
    gtime_t damage_debounce_time;
    gtime_t fly_sound_debounce_time; // move to clientinfo
    gtime_t last_move_time;

    int     health;
    int     max_health;
    int     gib_health;
    gtime_t show_hostile;

    gtime_t powerarmor_time;

    const char *map; // target_changelevel

    int     viewheight; // height above origin where eyesight is determined
    bool    deadflag;
    bool    takedamage;
    int     dmg;
    int     radius_dmg;
    float   dmg_radius;
    int     sounds; // make this a spawntemp var?
    int     count;

    edict_t *chain;
    edict_t *enemy;
    edict_t *oldenemy;
    edict_t *activator;
    edict_t *groundentity;
    int      groundentity_linkcount;
    edict_t *teamchain;
    edict_t *teammaster;

    edict_t *mynoise; // can go in client only
    edict_t *mynoise2;

    int     noise_index;
    int     noise_index2;
    float   volume;
    float   attenuation;

    // timing variables
    float wait;
    float delay; // before firing targets
    float random;

    gtime_t teleport_time;

    contents_t    watertype;
    water_level_t waterlevel;

    vec3_t move_origin;
    vec3_t move_angles;

    int style; // also used as areaportal number

    const gitem_t *item; // for bonus items

    // common data blocks
    moveinfo_t    moveinfo;
    monsterinfo_t monsterinfo;

    //=========
    // ROGUE
    plat2flags_t plat2flags;
    vec3_t       offset;
    vec3_t       gravityVector;
    edict_t     *bad_area;
    edict_t     *hint_chain;
    edict_t     *monster_hint_chain;
    edict_t     *target_hint_chain;
    int          hint_chain_id;
    // ROGUE
    //=========

    char clock_message[CLOCK_MESSAGE_SIZE];

    // Paril: we died on this frame, apply knockback even if we're dead
    gtime_t dead_time;
    // used for dabeam monsters
    edict_t *beam, *beam2;
    // proboscus for Parasite
    edict_t *proboscus;
    // for vooping things
    edict_t *disintegrator;
    gtime_t disintegrator_time;
    int hackflags; // n64

    // instanced coop items
    byte    item_picked_up_by[MAX_CLIENTS / 8];
    gtime_t slime_debounce_time;

    // [Paril-KEX]
    bmodel_anim_t bmodel_anim;

    const char *style_on, *style_off;
    int crosslevel_flags;
    // NOTE: if adding new elements, make sure to add them
    // in g_save.cpp too!
};

//=============
// ROGUE
#define SPHERE_DEFENDER     0x0001
#define SPHERE_HUNTER       0x0002
#define SPHERE_VENGEANCE    0x0004
#define SPHERE_DOPPLEGANGER 0x10000

#define SPHERE_TYPE     (SPHERE_DEFENDER | SPHERE_HUNTER | SPHERE_VENGEANCE)
#define SPHERE_FLAGS    SPHERE_DOPPLEGANGER

//
// deathmatch games
//
enum {
    RDM_TAG = 2,
    RDM_DEATHBALL = 3
};

typedef struct {
    void (*GameInit)(void);
    void (*PostInitSetup)(void);
    void (*ClientBegin)(edict_t *ent);
    bool (*SelectSpawnPoint)(edict_t *ent, vec3_t origin, vec3_t angles, bool force_spawn);
    void (*PlayerDeath)(edict_t *targ, edict_t *inflictor, edict_t *attacker);
    void (*Score)(edict_t *attacker, edict_t *victim, int scoreChange, mod_t mod);
    void (*PlayerEffects)(edict_t *ent);
    void (*DogTag)(edict_t *ent, edict_t *killer, const char **pic);
    void (*PlayerDisconnect)(edict_t *ent);
    int (*ChangeDamage)(edict_t *targ, edict_t *attacker, int damage, mod_t mod);
    int (*ChangeKnockback)(edict_t *targ, edict_t *attacker, int knockback, mod_t mod);
    int (*CheckDMRules)(void);
} dm_game_rt;

extern dm_game_rt DMGame;

// ROGUE
//============

// we won't ever pierce more than this many entities for a single trace.
#define MAX_PIERCE  16

// base class for pierce args; this stores
// the stuff we are piercing.
typedef struct {
    edict_t *ents[MAX_PIERCE];
    solid_t solids[MAX_PIERCE];
    int count;
} pierce_t;

static inline void pierce_begin(pierce_t *p)
{
    p->count = 0;
}

static inline bool pierce_mark(pierce_t *p, edict_t *ent)
{
    // ran out of pierces
    if (p->count == MAX_PIERCE)
        return false;

    p->ents[p->count] = ent;
    p->solids[p->count] = ent->solid;
    p->count++;

    ent->solid = SOLID_NOT;
    gi.linkentity(ent);

    return true;
}

static inline void pierce_end(pierce_t *p)
{
    for (int i = 0; i < p->count; i++) {
        edict_t *ent = p->ents[i];
        ent->solid = p->solids[i];
        gi.linkentity(ent);
    }
}

static inline bool M_CheckGib(edict_t *self, mod_t mod)
{
    if (self->deadflag && mod.id == MOD_CRUSH)
        return true;

    return self->health <= self->gib_health;
}
