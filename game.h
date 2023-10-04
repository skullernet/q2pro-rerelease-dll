// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// game.h - game API stuff
#pragma once

#include <array>
#include <limits.h>

// compatibility with legacy float[3] stuff for engine
#ifdef GAME_INCLUDE
using gvec3_t = vec3_t;
using gvec3_ptr_t = vec3_t *;
using gvec3_cptr_t = const vec3_t *;
using gvec3_ref_t = vec3_t &;
using gvec3_cref_t = const vec3_t &;
using gvec4_t = std::array<float, 4>;
#else
using gvec3_t = float[3];
using gvec3_ptr_t = gvec3_t;
using gvec3_ref_t = gvec3_t;
using gvec3_cref_t = const gvec3_t;
using gvec3_cptr_t = const gvec3_t;
using gvec4_t = float[4];
#endif

constexpr size_t MAX_SPLIT_PLAYERS = 8;

constexpr size_t MAX_NETNAME = 16;

constexpr float STEPSIZE = 18.0f;

// ugly hack to support bitflags on enums
// and use enable_if to prevent confusing cascading
// errors if you use the operators wrongly
#define MAKE_ENUM_BITFLAGS(T)                                                                                          \
    constexpr T operator~(const T &v)                                                                                  \
    {                                                                                                                  \
        return static_cast<T>(~static_cast<std::underlying_type_t<T>>(v));                                             \
    }                                                                                                                  \
    constexpr T operator|(const T &v, const T &v2)                                                                     \
    {                                                                                                                  \
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) | static_cast<std::underlying_type_t<T>>(v2)); \
    }                                                                                                                  \
    constexpr T operator&(const T &v, const T &v2)                                                                     \
    {                                                                                                                  \
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) & static_cast<std::underlying_type_t<T>>(v2)); \
    }                                                                                                                  \
    constexpr T operator^(const T &v, const T &v2)                                                                     \
    {                                                                                                                  \
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) ^ static_cast<std::underlying_type_t<T>>(v2)); \
    }                                                                                                                  \
    template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
    constexpr T &operator|=(T &v, const T &v2)                                                                         \
    {                                                                                                                  \
        v = v | v2;                                                                                                    \
        return v;                                                                                                      \
    }                                                                                                                  \
    template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
    constexpr T &operator&=(T &v, const T &v2)                                                                         \
    {                                                                                                                  \
        v = v & v2;                                                                                                    \
        return v;                                                                                                      \
    }                                                                                                                  \
    template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
    constexpr T &operator^=(T &v, const T &v2)                                                                         \
    {                                                                                                                  \
        v = v ^ v2;                                                                                                    \
        return v;                                                                                                      \
    }

using byte = uint8_t;

// bit simplification
template<size_t n>
using bit_t = std::conditional_t<n >= 32, uint64_t, uint32_t >;

// template is better for this because you can see
// it in the hover-over preview
template<size_t n>
constexpr bit_t<n> bit_v = 1ull << n;

#ifdef _WIN32
#define q_exported __declspec(dllexport)
#define q_imported __declspec(dllimport)
#else
#define q_exported __attribute__((visibility("default")))
#define q_imported
#endif

#if defined(KEX_Q2GAME_EXPORTS)
#define Q2GAME_API extern "C" q_exported
#elif defined(KEX_Q2GAME_IMPORTS)
#define Q2GAME_API extern "C" q_imported
#else
#define Q2GAME_API
#endif

// game.h -- game dll information visible to server
constexpr int32_t GAME_API_VERSION = 3;
constexpr int32_t GAME_API_VERSION_EX = 1;

// forward declarations
struct edict_t;
struct gclient_t;

constexpr size_t MAX_STRING_CHARS = 1024; // max length of a string passed to Cmd_TokenizeString
constexpr size_t MAX_STRING_TOKENS = 80;  // max tokens resulting from Cmd_TokenizeString
constexpr size_t MAX_TOKEN_CHARS = 512;   // max length of an individual token

constexpr size_t MAX_QPATH = 64;   // max length of a quake game pathname
constexpr size_t MAX_OSPATH = 128; // max length of a filesystem pathname

//
// per-level limits
//
constexpr size_t MAX_CLIENTS = 256; // absolute limit
constexpr size_t MAX_EDICTS = 8192; // upper limit, due to svc_sound encoding as 13 bits
constexpr size_t MAX_LIGHTSTYLES = 256;
constexpr size_t MAX_MODELS = 8192; // these are sent over the net as shorts
constexpr size_t MAX_SOUNDS = 2048; // so they cannot be blindly increased
constexpr size_t MAX_IMAGES = 2048;
constexpr size_t MAX_ITEMS = 256;
constexpr size_t MAX_GENERAL = (MAX_CLIENTS * 2); // general config strings

constexpr size_t ENTITYNUM_BITS = 13;

// game print flags
enum print_type_t {
    PRINT_LOW = 0,    // pickup messages
    PRINT_MEDIUM = 1, // death messages
    PRINT_HIGH = 2,   // critical messages
    PRINT_CHAT = 3,   // chat messages
    PRINT_TYPEWRITER = 4, // centerprint but typed out one char at a time
    PRINT_CENTER = 5, // centerprint without a separate function (loc variants only)
    PRINT_TTS = 6, // PRINT_HIGH but will speak for players with narration on

    PRINT_BROADCAST = (1 << 3), // Bitflag, add to message to broadcast print to all clients.
    PRINT_NO_NOTIFY = (1 << 4) // Bitflag, don't put on notify
};

MAKE_ENUM_BITFLAGS(print_type_t);

// destination class for gi.multicast()
enum multicast_t {
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
};

enum qboolean { qfalse, qtrue };

enum game_features_t {
    GMF_NONE = 0,

    // R1Q2 and Q2PRO specific
    GMF_CLIENTNUM = bit_v<0>,            // game sets clientNum gclient_s field
    GMF_PROPERINUSE = bit_v<1>,          // game maintains edict_s inuse field properly
    GMF_MVDSPEC = bit_v<2>,              // game is dummy MVD client aware
    GMF_WANT_ALL_DISCONNECTS = bit_v<3>, // game wants ClientDisconnect() for non-spawned clients

    // Q2PRO specific
    GMF_ENHANCED_SAVEGAMES = bit_v<10>,   // game supports safe/portable savegames
    GMF_VARIABLE_FPS = bit_v<11>,         // game supports variable server FPS
    GMF_EXTRA_USERINFO = bit_v<12>,       // game wants extra userinfo after normal userinfo
    GMF_IPV6_ADDRESS_AWARE = bit_v<13>,   // game supports IPv6 addresses
    GMF_ALLOW_INDEX_OVERFLOW = bit_v<14>, // game wants PF_FindIndex() to return 0 on overflow
    GMF_PROTOCOL_EXTENSIONS = bit_v<15>,  // game supports protocol extensions
};

MAKE_ENUM_BITFLAGS(game_features_t);

constexpr game_features_t G_FEATURES = GMF_PROPERINUSE | GMF_WANT_ALL_DISCONNECTS | GMF_ENHANCED_SAVEGAMES | GMF_PROTOCOL_EXTENSIONS;

/*
==========================================================

CVARS (console variables)

==========================================================
*/

enum cvar_flags_t : uint32_t {
    CVAR_NOFLAGS = 0,
    CVAR_ARCHIVE = bit_v<0>,     // set to cause it to be saved to config
    CVAR_USERINFO = bit_v<1>,    // added to userinfo  when changed
    CVAR_SERVERINFO = bit_v<2>,  // added to serverinfo when changed
    CVAR_NOSET = bit_v<3>,       // don't allow change from console at all,
                                 // but can be set from the command line
    CVAR_LATCH = bit_v<4>,       // save changes until server restart
    CVAR_ROM = bit_v<7>,         // can't be changed even from cmdline
};
MAKE_ENUM_BITFLAGS(cvar_flags_t);

// nothing outside the Cvar_*() functions should modify these fields!
struct cvar_t {
    char         *name;
    char         *string;
    char         *latched_string; // for CVAR_LATCH vars
    cvar_flags_t flags;
    qboolean     modified;
    float        value;
    cvar_t      *next;
    int32_t      integer; // integral value
};

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
enum contents_t : uint32_t {
    CONTENTS_NONE = 0,
    CONTENTS_SOLID = bit_v<0>,   // an eye is never valid in a solid
    CONTENTS_WINDOW = bit_v<1>, // translucent, but not watery
    CONTENTS_AUX = bit_v<2>,
    CONTENTS_LAVA = bit_v<3>,
    CONTENTS_SLIME = bit_v<4>,
    CONTENTS_WATER = bit_v<5>,
    CONTENTS_MIST = bit_v<6>,

    // remaining contents are non-visible, and don't eat brushes

    CONTENTS_PROJECTILECLIP = bit_v<14>, // [Paril-KEX] projectiles will collide with this

    CONTENTS_AREAPORTAL = bit_v<15>,

    CONTENTS_PLAYERCLIP = bit_v<16>,
    CONTENTS_MONSTERCLIP = bit_v<17>,

    // currents can be added to any other contents, and may be mixed
    CONTENTS_CURRENT_0 = bit_v<18>,
    CONTENTS_CURRENT_90 = bit_v<19>,
    CONTENTS_CURRENT_180 = bit_v<20>,
    CONTENTS_CURRENT_270 = bit_v<21>,
    CONTENTS_CURRENT_UP = bit_v<22>,
    CONTENTS_CURRENT_DOWN = bit_v<23>,

    CONTENTS_ORIGIN = bit_v<24>, // removed before bsping an entity

    CONTENTS_MONSTER = bit_v<25>, // should never be on a brush, only in game
    CONTENTS_DEADMONSTER = bit_v<26>,

    CONTENTS_DETAIL = bit_v<27>,       // brushes to be added after vis leafs
    CONTENTS_TRANSLUCENT = bit_v<28>, // auto set if any surface has trans
    CONTENTS_LADDER = bit_v<29>,

    CONTENTS_PLAYER = bit_v<30>, // [Paril-KEX] should never be on a brush, only in game; player
    CONTENTS_PROJECTILE = bit_v<31>  // [Paril-KEX] should never be on a brush, only in game; projectiles.
                          // used to solve deadmonster collision issues.
};

MAKE_ENUM_BITFLAGS(contents_t);

constexpr contents_t LAST_VISIBLE_CONTENTS = CONTENTS_MIST;

enum surfflags_t : uint32_t {
    SURF_NONE = 0,
    SURF_LIGHT = bit_v<0>, // value will hold the light strength
    SURF_SLICK = bit_v<1>, // effects game physics
    SURF_SKY = bit_v<2>,      // don't draw, but add to skybox
    SURF_WARP = bit_v<3>,  // turbulent water warp
    SURF_TRANS33 = bit_v<4>,
    SURF_TRANS66 = bit_v<5>,
    SURF_FLOWING = bit_v<6>, // scroll towards angle
    SURF_NODRAW = bit_v<7>,  // don't bother referencing the texture
    SURF_ALPHATEST = bit_v<25>,   // [Paril-KEX] alpha test using widely supported flag
    SURF_N64_UV = bit_v<28>,  // [Sam-KEX] Stretches texture UVs
    SURF_N64_SCROLL_X = bit_v<29>,  // [Sam-KEX] Texture scroll X-axis
    SURF_N64_SCROLL_Y = bit_v<30>,  // [Sam-KEX] Texture scroll Y-axis
    SURF_N64_SCROLL_FLIP = bit_v<31>  // [Sam-KEX] Flip direction of texture scroll
};

MAKE_ENUM_BITFLAGS(surfflags_t);

// content masks
constexpr contents_t MASK_ALL = static_cast<contents_t>(-1);
constexpr contents_t MASK_SOLID = (CONTENTS_SOLID | CONTENTS_WINDOW);
constexpr contents_t MASK_PLAYERSOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_PLAYER);
constexpr contents_t MASK_DEADSOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW);
constexpr contents_t MASK_MONSTERSOLID = (CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_PLAYER);
constexpr contents_t MASK_WATER = (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME);
constexpr contents_t MASK_OPAQUE = (CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA);
constexpr contents_t MASK_SHOT = (CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_WINDOW | CONTENTS_DEADMONSTER);
constexpr contents_t MASK_CURRENT = (CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN);
constexpr contents_t MASK_BLOCK_SIGHT = (CONTENTS_SOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_MONSTER | CONTENTS_PLAYER);
constexpr contents_t MASK_NAV_SOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW);
constexpr contents_t MASK_LADDER_NAV_SOLID = (CONTENTS_SOLID | CONTENTS_WINDOW);
constexpr contents_t MASK_WALK_NAV_SOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTERCLIP);
constexpr contents_t MASK_PROJECTILE = MASK_SHOT | CONTENTS_PROJECTILECLIP;

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
enum solidity_area_t {
    AREA_SOLID = 1,
    AREA_TRIGGERS = 2
};

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
struct cplane_t {
    gvec3_t normal;
    float   dist;
    byte    type;    // for fast side tests
    byte    signbits; // signx + (signy<<1) + (signz<<1)
    byte    pad[2];
};

// [Paril-KEX]
constexpr size_t MAX_MATERIAL_NAME = 16;

struct csurface_t {
    char        name[16];
    surfflags_t flags;
    int32_t     value;
};

// a trace is returned when a box is swept through the world
struct trace_t {
    qboolean    allsolid;   // if true, plane is not valid
    qboolean    startsolid; // if true, the initial point was in a solid area
    float       fraction;   // time completed, 1.0 = didn't hit anything
    gvec3_t     endpos;     // final position
    cplane_t    plane;      // surface normal at impact
    csurface_t *surface;    // surface hit
    contents_t  contents;   // contents on other side of surface hit
    edict_t     *ent;       // not set by CM_*() functions
};

// pmove_state_t is the information necessary for client side movement
// prediction
enum pmtype_t {
    // can accelerate and turn
    PM_NORMAL,
    PM_SPECTATOR, // [Paril-KEX] clip against walls, but not entities
    // no acceleration or turning
    PM_DEAD,
    PM_GIB, // different bounding box
    PM_FREEZE
};

// pmove->pm_flags
enum pmflags_t : uint8_t {
    PMF_NONE = 0,
    PMF_DUCKED = bit_v<0>,
    PMF_JUMP_HELD = bit_v<1>,
    PMF_ON_GROUND = bit_v<2>,
    PMF_TIME_WATERJUMP = bit_v<3>, // pm_time is waterjump
    PMF_TIME_LAND = bit_v<4>,       // pm_time is time before rejump
    PMF_TIME_TELEPORT = bit_v<5>, // pm_time is non-moving time
    PMF_NO_POSITIONAL_PREDICTION = bit_v<6>,    // temporarily disables positional prediction (used for grappling hook)
    PMF_ON_LADDER = 0,    // signal to game that we are on a ladder
    PMF_NO_ANGULAR_PREDICTION = 0, // temporary disables angular prediction
    PMF_IGNORE_PLAYER_COLLISION = bit_v<7>, // don't collide with other players
    PMF_TIME_TRICK = 0, // pm_time is trick jump time
};

MAKE_ENUM_BITFLAGS(pmflags_t);

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
struct pmove_state_t {
    pmtype_t pm_type;

    int16_t                origin[3];
    int16_t                velocity[3];
    pmflags_t              pm_flags; // ducked, jump_held, etc
    uint8_t                pm_time;
    int16_t                gravity;
    int16_t                delta_angles[3]; // add to command angles to get view direction
                                            // changed by spawns, rotating objects, and teleporters
};

//
// button bits
//
enum button_t : uint8_t {
    BUTTON_NONE = 0,
    BUTTON_ATTACK = bit_v<0>,
    BUTTON_USE = bit_v<1>,
    BUTTON_HOLSTER = bit_v<2>, // [Paril-KEX]
    BUTTON_JUMP = bit_v<3>,
    BUTTON_CROUCH = bit_v<4>,
    BUTTON_ANY = bit_v<7> // any key whatsoever
};

MAKE_ENUM_BITFLAGS(button_t);

// usercmd_t is sent to the server each client frame
struct usercmd_t {
    byte            msec;
    button_t        buttons;
    int16_t         angles[3];
    int16_t         forwardmove, sidemove, upmove;
    uint8_t         impulse;
    uint8_t         lightlevel;
};

enum water_level_t : uint32_t {
    WATER_NONE,
    WATER_FEET,
    WATER_WAIST,
    WATER_UNDER
};

// player_state_t->refdef flags
enum refdef_flags_t : uint32_t {
    RDF_NONE = 0,
    RDF_UNDERWATER = bit_v<0>,    // warp the screen as appropriate
    RDF_NOWORLDMODEL = bit_v<1>, // used for player configuration screen

    // ROGUE
    RDF_IRGOGGLES = bit_v<2>,
    RDF_UVGOGGLES = bit_v<3>,
    // ROGUE

    RDF_NO_WEAPON_LERP = bit_v<4>
};

constexpr size_t MAXTOUCH = 32;

struct touch_list_t {
    uint32_t num = 0;
    std::array<trace_t, MAXTOUCH> traces;
};

struct pmove_t {
    // state (in / out)
    pmove_state_t s;

    // command (in)
    usercmd_t cmd;
    qboolean      snapinitial; // if s has been changed outside pmove

    // results (out)
    uint32_t numtouch;
    std::array<edict_t *, MAXTOUCH> touchents;

    gvec3_t viewangles; // clamped
    float viewheight;

    gvec3_t mins, maxs; // bounding box size

    edict_t      *groundentity;
    contents_t    watertype;
    water_level_t waterlevel;

    // clip against world & entities
    trace_t (*trace)(gvec3_cref_t start, gvec3_cref_t mins, gvec3_cref_t maxs, gvec3_cref_t end);
    contents_t (*pointcontents)(gvec3_cref_t point);
};


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
enum effects_t : uint32_t {
    EF_NONE             = 0,           // no effects
    EF_ROTATE           = bit_v<0>,  // rotate (bonus items)
    EF_GIB              = bit_v<1>,  // leave a trail
    EF_BOB              = bit_v<2>,  // bob (bonus items)
    EF_BLASTER          = bit_v<3>,  // redlight + trail
    EF_ROCKET           = bit_v<4>,  // redlight + trail
    EF_GRENADE          = bit_v<5>,
    EF_HYPERBLASTER     = bit_v<6>,
    EF_BFG              = bit_v<7>,
    EF_COLOR_SHELL      = bit_v<8>,
    EF_POWERSCREEN      = bit_v<9>,
    EF_ANIM01           = bit_v<10>,  // automatically cycle between frames 0 and 1 at 2 hz
    EF_ANIM23           = bit_v<11>,  // automatically cycle between frames 2 and 3 at 2 hz
    EF_ANIM_ALL         = bit_v<12>,  // automatically cycle through all frames at 2hz
    EF_ANIM_ALLFAST     = bit_v<13>,  // automatically cycle through all frames at 10hz
    EF_FLIES            = bit_v<14>,
    EF_QUAD             = bit_v<15>,
    EF_PENT             = bit_v<16>,
    EF_TELEPORTER       = bit_v<17>,  // particle fountain
    EF_FLAG1            = bit_v<18>,
    EF_FLAG2            = bit_v<19>,
    // RAFAEL
    EF_IONRIPPER        = bit_v<20>,
    EF_GREENGIB         = bit_v<21>,
    EF_BLUEHYPERBLASTER = bit_v<22>,
    EF_SPINNINGLIGHTS   = bit_v<23>,
    EF_PLASMA           = bit_v<24>,
    EF_TRAP             = bit_v<25>,

    // ROGUE
    EF_TRACKER          = bit_v<26>,
    EF_DOUBLE           = bit_v<27>,
    EF_SPHERETRANS      = bit_v<28>,
    EF_TAGTRAIL         = bit_v<29>,
    EF_HALF_DAMAGE      = bit_v<30>,
    EF_TRACKERTRAIL     = bit_v<31>,
    // ROGUE
};

MAKE_ENUM_BITFLAGS(effects_t);

enum morefx_t : uint32_t {
    EFX_NONE            = 0,
    EFX_DUALFIRE        = bit_v<0>, // [KEX] dualfire damage color shell
    EFX_HOLOGRAM        = bit_v<1>, // [Paril-KEX] N64 hologram
    EFX_FLASHLIGHT      = bit_v<2>, // [Paril-KEX] project flashlight, only for players
    EFX_BARREL_EXPLODING = bit_v<3>,
    EFX_TELEPORTER2     = bit_v<4>, // [Paril-KEX] n64 teleporter
    EFX_GRENADE_LIGHT   = bit_v<5>
};

MAKE_ENUM_BITFLAGS(morefx_t);

constexpr effects_t EF_FIREBALL = EF_ROCKET | EF_GIB;

// entity_state_t->renderfx flags
enum renderfx_t : uint32_t {
    RF_NONE = 0,
    RF_MINLIGHT = bit_v<0>, // always have some light (viewmodel)
    RF_VIEWERMODEL = bit_v<1>, // don't draw through eyes, only mirrors
    RF_WEAPONMODEL = bit_v<2>, // only draw through eyes
    RF_FULLBRIGHT = bit_v<3>,   // always draw full intensity
    RF_DEPTHHACK = bit_v<4>,    // for view weapon Z crunching
    RF_TRANSLUCENT = bit_v<5>,
    RF_NO_ORIGIN_LERP = bit_v<6>, // no interpolation for origins
    RF_BEAM = bit_v<7>,
    RF_CUSTOMSKIN = bit_v<8>, // [Paril-KEX] implemented; set skinnum (or frame for RF_FLARE) to specify
    // an image in CS_IMAGES to use as skin.
    RF_GLOW = bit_v<9>,      // pulse lighting for bonus items
    RF_SHELL_RED = bit_v<10>,
    RF_SHELL_GREEN = bit_v<11>,
    RF_SHELL_BLUE = bit_v<12>,
    RF_NOSHADOW = bit_v<13>,
    RF_CASTSHADOW = bit_v<14>, // [Sam-KEX]

    // ROGUE
    RF_IR_VISIBLE = bit_v<15>,
    RF_SHELL_DOUBLE = bit_v<16>,
    RF_SHELL_HALF_DAM = bit_v<17>,
    RF_USE_DISGUISE = bit_v<18>,
    // ROGUE

    RF_SHELL_LITE_GREEN = bit_v<19>,
    RF_CUSTOM_LIGHT = bit_v<20>, // [Paril-KEX] custom point dlight that is designed to strobe/be turned off; s.frame is radius, s.skinnum is color
    RF_FLARE = bit_v<21>, // [Sam-KEX]
    RF_OLD_FRAME_LERP = bit_v<22>, // [Paril-KEX] force model to lerp from oldframe in entity state; otherwise it uses last frame client received
    RF_DOT_SHADOW = bit_v<23>, // [Paril-KEX] draw blobby shadow
    RF_LOW_PRIORITY = bit_v<24>, // [Paril-KEX] low priority object; if we can't be added to the scene, don't bother replacing entities,
    // and we can be replaced if anything non-low-priority needs room
    RF_NO_LOD = bit_v<25>, // [Paril-KEX] never LOD
    RF_NO_STEREO = RF_WEAPONMODEL, // [Paril-KEX] this is a bit dumb, but, for looping noises if this is set there's no stereo
    RF_STAIR_STEP = bit_v<26>, // [Paril-KEX] re-tuned, now used to handle stair steps for monsters

    RF_FLARE_LOCK_ANGLE = RF_MINLIGHT
};

MAKE_ENUM_BITFLAGS(renderfx_t);

constexpr renderfx_t RF_BEAM_LIGHTNING = RF_BEAM | RF_GLOW; // [Paril-KEX] make a lightning bolt instead of a laser

MAKE_ENUM_BITFLAGS(refdef_flags_t);

//
// muzzle flashes / player effects
//
enum player_muzzle_t : uint8_t {
    MZ_BLASTER = 0,
    MZ_MACHINEGUN = 1,
    MZ_SHOTGUN = 2,
    MZ_CHAINGUN1 = 3,
    MZ_CHAINGUN2 = 4,
    MZ_CHAINGUN3 = 5,
    MZ_RAILGUN = 6,
    MZ_ROCKET = 7,
    MZ_GRENADE = 8,
    MZ_LOGIN = 9,
    MZ_LOGOUT = 10,
    MZ_RESPAWN = 11,
    MZ_BFG = 12,
    MZ_SSHOTGUN = 13,
    MZ_HYPERBLASTER = 14,
    MZ_ITEMRESPAWN = 15,
    // RAFAEL
    MZ_IONRIPPER = 16,
    MZ_BLUEHYPERBLASTER = 17,
    MZ_PHALANX = 18,
    MZ_BFG2 = 19,
    MZ_PHALANX2 = 20,

    // ROGUE
    MZ_ETF_RIFLE = 30,
    MZ_PROX = 31, // [Paril-KEX]
    MZ_ETF_RIFLE_2 = 32, // [Paril-KEX] unused, so using it for the other barrel
    MZ_HEATBEAM = 33,
    MZ_BLASTER2 = 34,
    MZ_TRACKER = 35,
    MZ_NUKE1 = 36,
    MZ_NUKE2 = 37,
    MZ_NUKE4 = 38,
    MZ_NUKE8 = 39,
    // ROGUE

    MZ_SILENCED = bit_v<7>, // bit flag ORed with one of the above numbers
    MZ_NONE = 0        // "no" bitflags
};

MAKE_ENUM_BITFLAGS(player_muzzle_t);

//
// monster muzzle flashes
// NOTE: this needs to match the m_flash table!
//
enum monster_muzzleflash_id_t : uint16_t {
    MZ2_UNUSED_0,

    MZ2_TANK_BLASTER_1,
    MZ2_TANK_BLASTER_2,
    MZ2_TANK_BLASTER_3,
    MZ2_TANK_MACHINEGUN_1,
    MZ2_TANK_MACHINEGUN_2,
    MZ2_TANK_MACHINEGUN_3,
    MZ2_TANK_MACHINEGUN_4,
    MZ2_TANK_MACHINEGUN_5,
    MZ2_TANK_MACHINEGUN_6,
    MZ2_TANK_MACHINEGUN_7,
    MZ2_TANK_MACHINEGUN_8,
    MZ2_TANK_MACHINEGUN_9,
    MZ2_TANK_MACHINEGUN_10,
    MZ2_TANK_MACHINEGUN_11,
    MZ2_TANK_MACHINEGUN_12,
    MZ2_TANK_MACHINEGUN_13,
    MZ2_TANK_MACHINEGUN_14,
    MZ2_TANK_MACHINEGUN_15,
    MZ2_TANK_MACHINEGUN_16,
    MZ2_TANK_MACHINEGUN_17,
    MZ2_TANK_MACHINEGUN_18,
    MZ2_TANK_MACHINEGUN_19,
    MZ2_TANK_ROCKET_1,
    MZ2_TANK_ROCKET_2,
    MZ2_TANK_ROCKET_3,

    MZ2_INFANTRY_MACHINEGUN_1,
    MZ2_INFANTRY_MACHINEGUN_2,
    MZ2_INFANTRY_MACHINEGUN_3,
    MZ2_INFANTRY_MACHINEGUN_4,
    MZ2_INFANTRY_MACHINEGUN_5,
    MZ2_INFANTRY_MACHINEGUN_6,
    MZ2_INFANTRY_MACHINEGUN_7,
    MZ2_INFANTRY_MACHINEGUN_8,
    MZ2_INFANTRY_MACHINEGUN_9,
    MZ2_INFANTRY_MACHINEGUN_10,
    MZ2_INFANTRY_MACHINEGUN_11,
    MZ2_INFANTRY_MACHINEGUN_12,
    MZ2_INFANTRY_MACHINEGUN_13,

    MZ2_SOLDIER_BLASTER_1,
    MZ2_SOLDIER_BLASTER_2,
    MZ2_SOLDIER_SHOTGUN_1,
    MZ2_SOLDIER_SHOTGUN_2,
    MZ2_SOLDIER_MACHINEGUN_1,
    MZ2_SOLDIER_MACHINEGUN_2,

    MZ2_GUNNER_MACHINEGUN_1,
    MZ2_GUNNER_MACHINEGUN_2,
    MZ2_GUNNER_MACHINEGUN_3,
    MZ2_GUNNER_MACHINEGUN_4,
    MZ2_GUNNER_MACHINEGUN_5,
    MZ2_GUNNER_MACHINEGUN_6,
    MZ2_GUNNER_MACHINEGUN_7,
    MZ2_GUNNER_MACHINEGUN_8,
    MZ2_GUNNER_GRENADE_1,
    MZ2_GUNNER_GRENADE_2,
    MZ2_GUNNER_GRENADE_3,
    MZ2_GUNNER_GRENADE_4,

    MZ2_CHICK_ROCKET_1,

    MZ2_FLYER_BLASTER_1,
    MZ2_FLYER_BLASTER_2,

    MZ2_MEDIC_BLASTER_1,

    MZ2_GLADIATOR_RAILGUN_1,

    MZ2_HOVER_BLASTER_1,

    MZ2_ACTOR_MACHINEGUN_1,

    MZ2_SUPERTANK_MACHINEGUN_1,
    MZ2_SUPERTANK_MACHINEGUN_2,
    MZ2_SUPERTANK_MACHINEGUN_3,
    MZ2_SUPERTANK_MACHINEGUN_4,
    MZ2_SUPERTANK_MACHINEGUN_5,
    MZ2_SUPERTANK_MACHINEGUN_6,
    MZ2_SUPERTANK_ROCKET_1,
    MZ2_SUPERTANK_ROCKET_2,
    MZ2_SUPERTANK_ROCKET_3,

    MZ2_BOSS2_MACHINEGUN_L1,
    MZ2_BOSS2_MACHINEGUN_L2,
    MZ2_BOSS2_MACHINEGUN_L3,
    MZ2_BOSS2_MACHINEGUN_L4,
    MZ2_BOSS2_MACHINEGUN_L5,
    MZ2_BOSS2_ROCKET_1,
    MZ2_BOSS2_ROCKET_2,
    MZ2_BOSS2_ROCKET_3,
    MZ2_BOSS2_ROCKET_4,

    MZ2_FLOAT_BLASTER_1,

    MZ2_SOLDIER_BLASTER_3,
    MZ2_SOLDIER_SHOTGUN_3,
    MZ2_SOLDIER_MACHINEGUN_3,
    MZ2_SOLDIER_BLASTER_4,
    MZ2_SOLDIER_SHOTGUN_4,
    MZ2_SOLDIER_MACHINEGUN_4,
    MZ2_SOLDIER_BLASTER_5,
    MZ2_SOLDIER_SHOTGUN_5,
    MZ2_SOLDIER_MACHINEGUN_5,
    MZ2_SOLDIER_BLASTER_6,
    MZ2_SOLDIER_SHOTGUN_6,
    MZ2_SOLDIER_MACHINEGUN_6,
    MZ2_SOLDIER_BLASTER_7,
    MZ2_SOLDIER_SHOTGUN_7,
    MZ2_SOLDIER_MACHINEGUN_7,
    MZ2_SOLDIER_BLASTER_8,
    MZ2_SOLDIER_SHOTGUN_8,
    MZ2_SOLDIER_MACHINEGUN_8,

    // --- Xian shit below ---
    MZ2_MAKRON_BFG,
    MZ2_MAKRON_BLASTER_1,
    MZ2_MAKRON_BLASTER_2,
    MZ2_MAKRON_BLASTER_3,
    MZ2_MAKRON_BLASTER_4,
    MZ2_MAKRON_BLASTER_5,
    MZ2_MAKRON_BLASTER_6,
    MZ2_MAKRON_BLASTER_7,
    MZ2_MAKRON_BLASTER_8,
    MZ2_MAKRON_BLASTER_9,
    MZ2_MAKRON_BLASTER_10,
    MZ2_MAKRON_BLASTER_11,
    MZ2_MAKRON_BLASTER_12,
    MZ2_MAKRON_BLASTER_13,
    MZ2_MAKRON_BLASTER_14,
    MZ2_MAKRON_BLASTER_15,
    MZ2_MAKRON_BLASTER_16,
    MZ2_MAKRON_BLASTER_17,
    MZ2_MAKRON_RAILGUN_1,
    MZ2_JORG_MACHINEGUN_L1,
    MZ2_JORG_MACHINEGUN_L2,
    MZ2_JORG_MACHINEGUN_L3,
    MZ2_JORG_MACHINEGUN_L4,
    MZ2_JORG_MACHINEGUN_L5,
    MZ2_JORG_MACHINEGUN_L6,
    MZ2_JORG_MACHINEGUN_R1,
    MZ2_JORG_MACHINEGUN_R2,
    MZ2_JORG_MACHINEGUN_R3,
    MZ2_JORG_MACHINEGUN_R4,
    MZ2_JORG_MACHINEGUN_R5,
    MZ2_JORG_MACHINEGUN_R6,
    MZ2_JORG_BFG_1,
    MZ2_BOSS2_MACHINEGUN_R1,
    MZ2_BOSS2_MACHINEGUN_R2,
    MZ2_BOSS2_MACHINEGUN_R3,
    MZ2_BOSS2_MACHINEGUN_R4,
    MZ2_BOSS2_MACHINEGUN_R5,

    // ROGUE
    MZ2_CARRIER_MACHINEGUN_L1,
    MZ2_CARRIER_MACHINEGUN_R1,
    MZ2_CARRIER_GRENADE,
    MZ2_TURRET_MACHINEGUN,
    MZ2_TURRET_ROCKET,
    MZ2_TURRET_BLASTER,
    MZ2_STALKER_BLASTER,
    MZ2_DAEDALUS_BLASTER,
    MZ2_MEDIC_BLASTER_2,
    MZ2_CARRIER_RAILGUN,
    MZ2_WIDOW_DISRUPTOR,
    MZ2_WIDOW_BLASTER,
    MZ2_WIDOW_RAIL,
    MZ2_WIDOW_PLASMABEAM, // PMM - not used
    MZ2_CARRIER_MACHINEGUN_L2,
    MZ2_CARRIER_MACHINEGUN_R2,
    MZ2_WIDOW_RAIL_LEFT,
    MZ2_WIDOW_RAIL_RIGHT,
    MZ2_WIDOW_BLASTER_SWEEP1,
    MZ2_WIDOW_BLASTER_SWEEP2,
    MZ2_WIDOW_BLASTER_SWEEP3,
    MZ2_WIDOW_BLASTER_SWEEP4,
    MZ2_WIDOW_BLASTER_SWEEP5,
    MZ2_WIDOW_BLASTER_SWEEP6,
    MZ2_WIDOW_BLASTER_SWEEP7,
    MZ2_WIDOW_BLASTER_SWEEP8,
    MZ2_WIDOW_BLASTER_SWEEP9,
    MZ2_WIDOW_BLASTER_100,
    MZ2_WIDOW_BLASTER_90,
    MZ2_WIDOW_BLASTER_80,
    MZ2_WIDOW_BLASTER_70,
    MZ2_WIDOW_BLASTER_60,
    MZ2_WIDOW_BLASTER_50,
    MZ2_WIDOW_BLASTER_40,
    MZ2_WIDOW_BLASTER_30,
    MZ2_WIDOW_BLASTER_20,
    MZ2_WIDOW_BLASTER_10,
    MZ2_WIDOW_BLASTER_0,
    MZ2_WIDOW_BLASTER_10L,
    MZ2_WIDOW_BLASTER_20L,
    MZ2_WIDOW_BLASTER_30L,
    MZ2_WIDOW_BLASTER_40L,
    MZ2_WIDOW_BLASTER_50L,
    MZ2_WIDOW_BLASTER_60L,
    MZ2_WIDOW_BLASTER_70L,
    MZ2_WIDOW_RUN_1,
    MZ2_WIDOW_RUN_2,
    MZ2_WIDOW_RUN_3,
    MZ2_WIDOW_RUN_4,
    MZ2_WIDOW_RUN_5,
    MZ2_WIDOW_RUN_6,
    MZ2_WIDOW_RUN_7,
    MZ2_WIDOW_RUN_8,
    MZ2_CARRIER_ROCKET_1,
    MZ2_CARRIER_ROCKET_2,
    MZ2_CARRIER_ROCKET_3,
    MZ2_CARRIER_ROCKET_4,
    MZ2_WIDOW2_BEAMER_1,
    MZ2_WIDOW2_BEAMER_2,
    MZ2_WIDOW2_BEAMER_3,
    MZ2_WIDOW2_BEAMER_4,
    MZ2_WIDOW2_BEAMER_5,
    MZ2_WIDOW2_BEAM_SWEEP_1,
    MZ2_WIDOW2_BEAM_SWEEP_2,
    MZ2_WIDOW2_BEAM_SWEEP_3,
    MZ2_WIDOW2_BEAM_SWEEP_4,
    MZ2_WIDOW2_BEAM_SWEEP_5,
    MZ2_WIDOW2_BEAM_SWEEP_6,
    MZ2_WIDOW2_BEAM_SWEEP_7,
    MZ2_WIDOW2_BEAM_SWEEP_8,
    MZ2_WIDOW2_BEAM_SWEEP_9,
    MZ2_WIDOW2_BEAM_SWEEP_10,
    MZ2_WIDOW2_BEAM_SWEEP_11,
    // ROGUE

    // [Paril-KEX]
    MZ2_SOLDIER_RIPPER_1,
    MZ2_SOLDIER_RIPPER_2,
    MZ2_SOLDIER_RIPPER_3,
    MZ2_SOLDIER_RIPPER_4,
    MZ2_SOLDIER_RIPPER_5,
    MZ2_SOLDIER_RIPPER_6,
    MZ2_SOLDIER_RIPPER_7,
    MZ2_SOLDIER_RIPPER_8,

    MZ2_SOLDIER_HYPERGUN_1,
    MZ2_SOLDIER_HYPERGUN_2,
    MZ2_SOLDIER_HYPERGUN_3,
    MZ2_SOLDIER_HYPERGUN_4,
    MZ2_SOLDIER_HYPERGUN_5,
    MZ2_SOLDIER_HYPERGUN_6,
    MZ2_SOLDIER_HYPERGUN_7,
    MZ2_SOLDIER_HYPERGUN_8,
    MZ2_GUARDIAN_BLASTER,
    MZ2_ARACHNID_RAIL1,
    MZ2_ARACHNID_RAIL2,
    MZ2_ARACHNID_RAIL_UP1,
    MZ2_ARACHNID_RAIL_UP2,

    MZ2_INFANTRY_MACHINEGUN_14, // run-attack
    MZ2_INFANTRY_MACHINEGUN_15, // run-attack
    MZ2_INFANTRY_MACHINEGUN_16, // run-attack
    MZ2_INFANTRY_MACHINEGUN_17, // run-attack
    MZ2_INFANTRY_MACHINEGUN_18, // run-attack
    MZ2_INFANTRY_MACHINEGUN_19, // run-attack
    MZ2_INFANTRY_MACHINEGUN_20, // run-attack
    MZ2_INFANTRY_MACHINEGUN_21, // run-attack

    MZ2_GUNCMDR_CHAINGUN_1, // straight
    MZ2_GUNCMDR_CHAINGUN_2, // dodging

    MZ2_GUNCMDR_GRENADE_MORTAR_1,
    MZ2_GUNCMDR_GRENADE_MORTAR_2,
    MZ2_GUNCMDR_GRENADE_MORTAR_3,
    MZ2_GUNCMDR_GRENADE_FRONT_1,
    MZ2_GUNCMDR_GRENADE_FRONT_2,
    MZ2_GUNCMDR_GRENADE_FRONT_3,
    MZ2_GUNCMDR_GRENADE_CROUCH_1,
    MZ2_GUNCMDR_GRENADE_CROUCH_2,
    MZ2_GUNCMDR_GRENADE_CROUCH_3,

    // prone
    MZ2_SOLDIER_BLASTER_9,
    MZ2_SOLDIER_SHOTGUN_9,
    MZ2_SOLDIER_MACHINEGUN_9,
    MZ2_SOLDIER_RIPPER_9,
    MZ2_SOLDIER_HYPERGUN_9,

    // alternate frontwards grenades
    MZ2_GUNNER_GRENADE2_1,
    MZ2_GUNNER_GRENADE2_2,
    MZ2_GUNNER_GRENADE2_3,
    MZ2_GUNNER_GRENADE2_4,

    MZ2_INFANTRY_MACHINEGUN_22,

    // supertonk
    MZ2_SUPERTANK_GRENADE_1,
    MZ2_SUPERTANK_GRENADE_2,

    // hover & daedalus other side
    MZ2_HOVER_BLASTER_2,
    MZ2_DAEDALUS_BLASTER_2,

    // medic (commander) sweeps
    MZ2_MEDIC_HYPERBLASTER1_1,
    MZ2_MEDIC_HYPERBLASTER1_2,
    MZ2_MEDIC_HYPERBLASTER1_3,
    MZ2_MEDIC_HYPERBLASTER1_4,
    MZ2_MEDIC_HYPERBLASTER1_5,
    MZ2_MEDIC_HYPERBLASTER1_6,
    MZ2_MEDIC_HYPERBLASTER1_7,
    MZ2_MEDIC_HYPERBLASTER1_8,
    MZ2_MEDIC_HYPERBLASTER1_9,
    MZ2_MEDIC_HYPERBLASTER1_10,
    MZ2_MEDIC_HYPERBLASTER1_11,
    MZ2_MEDIC_HYPERBLASTER1_12,

    MZ2_MEDIC_HYPERBLASTER2_1,
    MZ2_MEDIC_HYPERBLASTER2_2,
    MZ2_MEDIC_HYPERBLASTER2_3,
    MZ2_MEDIC_HYPERBLASTER2_4,
    MZ2_MEDIC_HYPERBLASTER2_5,
    MZ2_MEDIC_HYPERBLASTER2_6,
    MZ2_MEDIC_HYPERBLASTER2_7,
    MZ2_MEDIC_HYPERBLASTER2_8,
    MZ2_MEDIC_HYPERBLASTER2_9,
    MZ2_MEDIC_HYPERBLASTER2_10,
    MZ2_MEDIC_HYPERBLASTER2_11,
    MZ2_MEDIC_HYPERBLASTER2_12,

    // only used for compile time checks
    MZ2_LAST
};

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
enum temp_event_t : uint8_t {
    TE_GUNSHOT,
    TE_BLOOD,
    TE_BLASTER,
    TE_RAILTRAIL,
    TE_SHOTGUN,
    TE_EXPLOSION1,
    TE_EXPLOSION2,
    TE_ROCKET_EXPLOSION,
    TE_GRENADE_EXPLOSION,
    TE_SPARKS,
    TE_SPLASH,
    TE_BUBBLETRAIL,
    TE_SCREEN_SPARKS,
    TE_SHIELD_SPARKS,
    TE_BULLET_SPARKS,
    TE_LASER_SPARKS,
    TE_PARASITE_ATTACK,
    TE_ROCKET_EXPLOSION_WATER,
    TE_GRENADE_EXPLOSION_WATER,
    TE_MEDIC_CABLE_ATTACK,
    TE_BFG_EXPLOSION,
    TE_BFG_BIGEXPLOSION,
    TE_BOSSTPORT, // used as '22' in a map, so DON'T RENUMBER!!!
    TE_BFG_LASER,
    TE_GRAPPLE_CABLE,
    TE_WELDING_SPARKS,
    TE_GREENBLOOD,
    TE_BLUEHYPERBLASTER_DUMMY, // [Paril-KEX] leaving for compatibility, do not use; use TE_BLUEHYPERBLASTER
    TE_PLASMA_EXPLOSION,
    TE_TUNNEL_SPARKS,
    // ROGUE
    TE_BLASTER2,
    TE_RAILTRAIL2,
    TE_FLAME,
    TE_LIGHTNING,
    TE_DEBUGTRAIL,
    TE_PLAIN_EXPLOSION,
    TE_FLASHLIGHT,
    TE_FORCEWALL,
    TE_HEATBEAM,
    TE_MONSTER_HEATBEAM,
    TE_STEAM,
    TE_BUBBLETRAIL2,
    TE_MOREBLOOD,
    TE_HEATBEAM_SPARKS,
    TE_HEATBEAM_STEAM,
    TE_CHAINFIST_SMOKE,
    TE_ELECTRIC_SPARKS,
    TE_TRACKER_EXPLOSION,
    TE_TELEPORT_EFFECT,
    TE_DBALL_GOAL,
    TE_WIDOWBEAMOUT,
    TE_NUKEBLAST,
    TE_WIDOWSPLASH,
    TE_EXPLOSION1_BIG,
    TE_EXPLOSION1_NP,
    TE_FLECHETTE,
    // ROGUE

    // [Paril-KEX]
    TE_BLUEHYPERBLASTER,
    TE_BFG_ZAP,
    TE_BERSERK_SLAM,
    TE_GRAPPLE_CABLE_2,
    TE_POWER_SPLASH,
    TE_LIGHTNING_BEAM,
    TE_EXPLOSION1_NL,
    TE_EXPLOSION2_NL,
};

enum splash_color_t : uint8_t {
    SPLASH_UNKNOWN = 0,
    SPLASH_SPARKS = 1,
    SPLASH_BLUE_WATER = 2,
    SPLASH_BROWN_WATER = 3,
    SPLASH_SLIME = 4,
    SPLASH_LAVA = 5,
    SPLASH_BLOOD = 6,

    // [Paril-KEX] N64 electric sparks that go zap
    SPLASH_ELECTRIC = 7
};

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
enum soundchan_t : uint32_t {
    CHAN_AUTO = 0,
    CHAN_WEAPON = 1,
    CHAN_VOICE = 2,
    CHAN_ITEM = 3,
    CHAN_BODY = 4,
    CHAN_AUX = 5,
    CHAN_FOOTSTEP = 6,
    CHAN_AUX3 = 7,

    // modifier flags
    CHAN_NO_PHS_ADD = bit_v<3>, // send to all clients, not just ones in PHS (ATTN 0 will also do this)
    CHAN_RELIABLE = bit_v<4>,   // send by reliable message, not datagram
    CHAN_FORCE_POS = bit_v<5>,  // always use position sent in packet
};

MAKE_ENUM_BITFLAGS(soundchan_t);

// sound attenuation values
constexpr float ATTN_LOOP_NONE = -1; // full volume the entire level, for loop only
constexpr float ATTN_NONE = 0; // full volume the entire level, for sounds only
constexpr float ATTN_NORM = 1;
constexpr float ATTN_IDLE = 2;
constexpr float ATTN_STATIC = 3; // diminish very rapidly with distance

// total stat count
constexpr size_t MAX_STATS = 32;

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define ANGLE2SHORT(x)  ((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)  ((x)*(360.0f/65536))

#define COORD2SHORT(x)  ((int)((x)*8.0f))
#define SHORT2COORD(x)  ((x)*(1.0f/8))

//=============================================
// INFO STRINGS
//=============================================

//
// key / value info strings
//
constexpr size_t MAX_INFO_KEY       = 64;
constexpr size_t MAX_INFO_VALUE     = 64;
constexpr size_t MAX_INFO_STRING    = 512;

// CONFIG STRINGS

enum game_style_t : uint8_t {
    GAME_STYLE_PVE,
    GAME_STYLE_FFA,
    GAME_STYLE_TDM
};

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most CS_MAX_STRING_LENGTH characters.
//
enum {
    CS_NAME,
    CS_CDTRACK,
    CS_SKY,
    CS_SKYAXIS, // %f %f %f format
    CS_SKYROTATE,
    CS_STATUSBAR, // display program string

    CS_AIRACCEL = 59, // air acceleration control
    CS_MAXCLIENTS,
    CS_MAPCHECKSUM, // for catching cheater maps

    CS_MODELS,
    CS_SOUNDS           = (CS_MODELS + MAX_MODELS),
    CS_IMAGES           = (CS_SOUNDS + MAX_SOUNDS),
    CS_LIGHTS           = (CS_IMAGES + MAX_IMAGES),
    CS_ITEMS            = (CS_LIGHTS + MAX_LIGHTSTYLES),
    CS_PLAYERSKINS      = (CS_ITEMS + MAX_ITEMS),
    CS_GENERAL          = (CS_PLAYERSKINS + MAX_CLIENTS),
    MAX_CONFIGSTRINGS   = (CS_GENERAL + MAX_GENERAL)
};

static_assert(MAX_CONFIGSTRINGS <= 0x7FFF, "configstrings too big");

constexpr size_t MAX_MODELS_OLD = 256, MAX_SOUNDS_OLD = 256, MAX_IMAGES_OLD = 256;

//==============================================

// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
enum entity_event_t : uint32_t {
    EV_NONE,
    EV_ITEM_RESPAWN,
    EV_FOOTSTEP,
    EV_FALLSHORT,
    EV_FALL,
    EV_FALLFAR,
    EV_PLAYER_TELEPORT,
    EV_OTHER_TELEPORT,

    // [Paril-KEX]
    EV_OTHER_FOOTSTEP,
    EV_LADDER_STEP,
};

// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
struct entity_state_t {
    uint32_t       number; // edict index

    gvec3_t        origin;
    gvec3_t        angles;
    gvec3_t        old_origin; // for lerping
    int32_t        modelindex;
    int32_t        modelindex2, modelindex3, modelindex4; // weapons, CTF flags, etc
    int32_t        frame;
    int32_t        skinnum;
    effects_t      effects; // PGM - we're filling it, so it needs to be unsigned
    renderfx_t     renderfx;
    uint32_t       solid;   // for client side prediction
    int32_t        sound;   // for looping sounds, to guarantee shutoff
    entity_event_t event;   // impulse events -- muzzle flashes, footsteps, etc
                            // events only go out for a single frame, they
                            // are automatically cleared each frame
};

struct entity_state_extension_t {
    morefx_t        morefx;
    float           alpha;  // [Paril-KEX] alpha scalar; 0 is a "default" value, which will respect other
                            // settings (default 1.0 for most things, EF_TRANSLUCENT will default this
                            // to 0.3, etc)
    float           scale;  // [Paril-KEX] model scale scalar; 0 is a "default" value, like with alpha.
    // [Paril-KEX] allow specifying volume/attn for looping noises; note that
    // zero will be defaults (1.0 and 3.0 respectively); -1 attenuation is used
    // for "none" (similar to target_speaker) for no phs/pvs looping noises
    float          loop_volume;
    float          loop_attenuation;
};

//==============================================

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be relative to client
// frame rates
struct player_state_t {
    pmove_state_t pmove; // for prediction

    // these fields do not need to be communicated bit-precise

    gvec3_t viewangles;  // for fixed views
    gvec3_t viewoffset;  // add to pmovestate->origin
    gvec3_t kick_angles; // add to view direction to get render angles
                        // set by weapon kicks, pain effects, etc

    gvec3_t gunangles;
    gvec3_t gunoffset;
    int32_t gunindex;
    int32_t gunframe;

    std::array<float, 4> screen_blend; // rgba full screen effect

    float fov; // horizontal field of view

    refdef_flags_t rdflags; // refdef flags

    std::array<int16_t, MAX_STATS> stats; // fast status bar updates
};

// protocol bytes that can be directly added to messages
enum server_command_t : uint8_t {
    svc_bad,

    svc_muzzleflash,
    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_inventory,

    svc_nop,
    svc_disconnect,
    svc_reconnect,
    svc_sound,                  // <see code>
    svc_print,                  // [byte] id [string] null terminated string
    svc_stufftext,              // [string] stuffed into client's console buffer, should be \n terminated
    svc_serverdata,             // [long] protocol ...
    svc_configstring,           // [short] [string]
    svc_spawnbaseline,
    svc_centerprint,            // [string] to put in center of the screen
    svc_download,               // [short] size [size bytes]
    svc_playerinfo,             // variable
    svc_packetentities,         // [...]
    svc_deltapacketentities,    // [...]
    svc_frame,

    svc_last // only for checks
};

enum svc_poi_flags {
    POI_FLAG_NONE = 0,
    POI_FLAG_HIDE_ON_AIM = 1, // hide the POI if we get close to it with our aim
};

// edict->svflags
enum svflags_t : uint32_t {
    SVF_NONE        = 0,          // no serverflags
    SVF_NOCLIENT    = bit_v<0>,   // don't send entity to clients, even if it has effects
    SVF_DEADMONSTER = bit_v<1>,   // treat as CONTENTS_DEADMONSTER for collision
    SVF_MONSTER     = bit_v<2>,   // treat as CONTENTS_MONSTER for collision
    SVF_PLAYER      = bit_v<3>,   // [Paril-KEX] treat as CONTENTS_PLAYER for collision
    SVF_BOT         = bit_v<4>,   // entity is controlled by a bot AI.
    SVF_NOBOTS      = bit_v<5>,   // don't allow bots to use/interact with entity
    SVF_RESPAWNING  = bit_v<6>,   // entity will respawn on it's next think.
    SVF_PROJECTILE  = bit_v<7>,   // treat as CONTENTS_PROJECTILE for collision
    SVF_INSTANCED   = bit_v<8>,   // entity has different visibility per player
    SVF_DOOR        = bit_v<9>,   // entity is a door of some kind
    SVF_NOCULL      = bit_v<10>,  // always send, even if we normally wouldn't
    SVF_HULL        = bit_v<11>   // always use hull when appropriate (triggers, etc; for gi.clip)
};
MAKE_ENUM_BITFLAGS(svflags_t);

// edict->solid values
enum solid_t {
    SOLID_NOT,     // no interaction with other objects
    SOLID_TRIGGER, // only touch when inside, after moving
    SOLID_BBOX,    // touch on edge
    SOLID_BSP      // bsp clip, touch on edge
};

// flags for inVIS()
enum vis_t {
    VIS_PVS     = 0,
    VIS_PHS     = 1,
    VIS_NOAREAS = 2     // can be OR'ed with one of above
};
MAKE_ENUM_BITFLAGS(vis_t);

// bitflags for STAT_LAYOUTS
enum layout_flags_t : int16_t {
    LAYOUTS_LAYOUT            = bit_v<0>, // svc_layout is active; escape remapped to putaway
    LAYOUTS_INVENTORY         = bit_v<1>, // inventory is active; escape remapped to putaway
    LAYOUTS_HIDE_HUD          = bit_v<2>, // hide entire hud, for cameras, etc
    LAYOUTS_INTERMISSION      = bit_v<3>, // intermission is being drawn; collapse splitscreen into 1 view
    LAYOUTS_HELP              = bit_v<4>, // help is active; escape remapped to putaway
    LAYOUTS_HIDE_CROSSHAIR    = bit_v<5> // hide crosshair only
};
MAKE_ENUM_BITFLAGS(layout_flags_t);

enum gesture_type {
    GESTURE_NONE = -1,
    GESTURE_FLIP_OFF,
    GESTURE_SALUTE,
    GESTURE_TAUNT,
    GESTURE_WAVE,
    GESTURE_POINT,
    GESTURE_POINT_NO_PING,
    GESTURE_MAX
};

enum server_state_t {
    SS_DEAD,
    SS_LOADING,
    SS_GAME,
    SS_PIC,
    SS_BROADCAST,
    SS_CINEMATIC
};

//===============================================================

constexpr int32_t MODELINDEX_WORLD = 1;    // special index for world
constexpr int32_t MODELINDEX_PLAYER = MAX_MODELS_OLD - 1; // special index for player models

// short stubs only used by the engine; the game DLL's version
// must be compatible with this.
#ifndef GAME_INCLUDE
struct gclient_t
#else
struct gclient_shared_t
#endif
{
    player_state_t ps; // communicated by server to clients
    int32_t        ping;
    // the game dll can add anything it wants after
    // this point in the structure
};

struct list_t {
    list_t *prev;
    list_t *next;
};

constexpr size_t MAX_ENT_CLUSTERS = 16;

#ifndef GAME_INCLUDE
struct edict_t
#else
struct edict_shared_t
#endif
{
    entity_state_t s;
    gclient_t     *client; // nullptr if not a player
    // the server expects the first part
    // of gclient_t to be a player_state_t
    // but the rest of it is opaque

    qboolean    inuse;

    // world linkage data
    int32_t     linkcount;
    list_t      area; // linked to a division node or leaf
    int32_t     num_clusters; // if -1, use headnode instead
    int32_t     clusternums[MAX_ENT_CLUSTERS];
    int32_t     headnode; // unused if num_clusters != -1
    int32_t     areanum, areanum2;

    svflags_t  svflags;
    vec3_t     mins, maxs;
    vec3_t     absmin, absmax, size;
    solid_t    solid;
    contents_t clipmask;
    edict_t    *owner;

    entity_state_extension_t    x;
};

#define CHECK_INTEGRITY(from_type, to_type, member)                           \
    static_assert(offsetof(from_type, member) == offsetof(to_type, member) && \
                      sizeof(from_type::member) == sizeof(to_type::member),   \
                  "structure malformed; not compatible with server: check member \"" #member "\"")

#define CHECK_GCLIENT_INTEGRITY                       \
    CHECK_INTEGRITY(gclient_t, gclient_shared_t, ps); \
    CHECK_INTEGRITY(gclient_t, gclient_shared_t, ping)

#define CHECK_EDICT_INTEGRITY                               \
    CHECK_INTEGRITY(edict_t, edict_shared_t, s);            \
    CHECK_INTEGRITY(edict_t, edict_shared_t, client);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, inuse);        \
    CHECK_INTEGRITY(edict_t, edict_shared_t, linkcount);    \
    CHECK_INTEGRITY(edict_t, edict_shared_t, areanum);      \
    CHECK_INTEGRITY(edict_t, edict_shared_t, areanum2);     \
    CHECK_INTEGRITY(edict_t, edict_shared_t, svflags);      \
    CHECK_INTEGRITY(edict_t, edict_shared_t, mins);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, maxs);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, absmin);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, absmax);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, size);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, solid);        \
    CHECK_INTEGRITY(edict_t, edict_shared_t, clipmask);     \
    CHECK_INTEGRITY(edict_t, edict_shared_t, owner)

//===============================================================

enum class BoxEdictsResult_t {
    Keep, // keep the given entity in the result and keep looping
    Skip, // skip the given entity

    End = 64, // stop searching any further

    Flags = End
};

MAKE_ENUM_BITFLAGS(BoxEdictsResult_t);

using BoxEdictsFilter_t = BoxEdictsResult_t (*)(edict_t *, void *);

#if (defined __clang__)
#define q_printf(f, a) __attribute__((format(printf, f, a)))
#elif (defined __GNUC__)
#define q_printf(f, a) __attribute__((format(gnu_printf, f, a)))
#else
#define q_printf(f, a)
#endif

//
// functions provided by the main engine
//
struct game_import_t {
    // broadcast to all clients
    void (*q_printf(2, 3) bprintf)(print_type_t printlevel, const char *fmt, ...);

    // print to appropriate places (console, log file, etc)
    void (*q_printf(1, 2) dprintf)(const char *fmt, ...);

    // print directly to a single client (or nullptr for server console)
    void (*q_printf(3, 4) cprintf)(edict_t *ent, print_type_t printlevel, const char *fmt, ...);

    // center-print to player (legacy function)
    void (*q_printf(2, 3) centerprintf)(edict_t *ent, const char *fmt, ...);

    void (*sound)(edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    void (*positioned_sound)(gvec3_cref_t origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);

    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    void (*configstring)(int num, const char *string);

    void (*q_printf(1, 2) error)(const char *fmt, ...);

    // the *index functions create configstrings and some internal server state
    int (*modelindex)(const char *name);
    int (*soundindex)(const char *name);
    // [Paril-KEX] imageindex can precache both pics for the HUD and
    // textures used for RF_CUSTOMSKIN; to register an image as a texture,
    // the path must be relative to the mod dir and end in an extension
    // ie models/my_model/skin.tga
    int (*imageindex)(const char *name);

    void (*setmodel)(edict_t *ent, const char *name);

    // collision detection
    trace_t (*trace)(gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, const edict_t *passent, contents_t contentmask);
    contents_t (*pointcontents)(gvec3_cref_t point);
    qboolean (*inPVS)(gvec3_cref_t p1, gvec3_cref_t p2);
    qboolean (*inPHS)(gvec3_cref_t p1, gvec3_cref_t p2);
    void (*SetAreaPortalState)(int portalnum, qboolean open);
    qboolean (*AreasConnected)(int area1, int area2);

    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent); // call before removing an interactive edict

    // return a list of entities that touch the input absmin/absmax.
    // if maxcount is 0, it will return a count but not attempt to fill "list".
    // if maxcount > 0, once it reaches maxcount, it will keep going but not fill
    // any more of list (the return count will cap at maxcount).
    // the filter function can remove unnecessary entities from the final list; it is illegal
    // to modify world links in this callback.
    int (*BoxEdicts)(gvec3_cref_t mins, gvec3_cref_t maxs, edict_t **list, int maxcount, solidity_area_t areatype);

    void (*Pmove)(pmove_t *pmove); // player movement code common with client prediction

    // network messaging
    void (*multicast)(gvec3_cref_t origin, multicast_t to);
    void (*unicast)(edict_t *ent, qboolean reliable);

    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(const char *s);
    void (*WritePosition)(gvec3_cref_t pos);
    void (*WriteDir)(gvec3_cref_t pos);   // single byte encoded, very coarse
    void (*WriteAngle)(float f); // legacy 8-bit angle

    // managed memory allocation
    void *(*TagMalloc)(int size, int tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(int tag);

    // console variable interaction
    cvar_t *(*cvar)(const char *var_name, const char *value, cvar_flags_t flags);
    cvar_t *(*cvar_set)(const char *var_name, const char *value);
    cvar_t *(*cvar_forceset)(const char *var_name, const char *value);

    // ClientCommand and ServerCommand parameter access
    int (*argc)();
    const char *(*argv)(int n);
    const char *(*args)(); // concatenation of all argv >= 1

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    void (*DebugGraph)(float value, int color);
};

//
// functions exported by the game subsystem
//
struct game_export_t {
    int apiversion;

    // the init function will only be called when a game starts,
    // not each time a level is loaded.  Persistant data for clients
    // and the server can be allocated in init
    void (*Init)();
    void (*Shutdown)();

    // each new level entered will cause a call to SpawnEntities
    void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

    // Read/Write Game is for storing persistant cross level information
    // about the world state and the clients.
    // WriteGame is called every time a level is exited.
    // ReadGame is called on a loadgame.

    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    void (*WriteGame)(const char *filename, qboolean autosave);
    void (*ReadGame)(const char *filename);

    // ReadLevel is called after the default map information has been
    // loaded with SpawnEntities
    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    void (*WriteLevel)(const char *filename);
    void (*ReadLevel)(const char *filename);

    qboolean (*ClientConnect)(edict_t *ent, char *userinfo);
    void (*ClientBegin)(edict_t *ent);
    void (*ClientUserinfoChanged)(edict_t *ent, const char *userinfo);
    void (*ClientDisconnect)(edict_t *ent);
    void (*ClientCommand)(edict_t *ent);
    void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)();

    // ServerCommand will be called when an "sv <command>" command is issued on the
    // server console.
    // The game can issue gi.argc() / gi.argv() commands to get the rest
    // of the parameters
    void (*ServerCommand)();

    //
    // global variables shared between game and server
    //

    // The edict array is allocated in the game dll so it
    // can vary in size from one game to another.
    //
    // The size will be fixed when ge->Init() is called
    edict_t     *edicts;
    uint32_t    edict_size;
    uint32_t    num_edicts; // current number, <= max_edicts
    uint32_t    max_edicts;
};

struct game_import_ex_t {
    uint32_t    apiversion;
    uint32_t    structsize;

    void        (*local_sound)(edict_t *target, gvec3_cptr_t origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    const char  *(*get_configstring)(int index);
    trace_t     (*clip)(gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, edict_t *clip, contents_t contentmask);
    qboolean    (*inVIS)(gvec3_cref_t p1, gvec3_cref_t p2, vis_t vis);

    void        *(*GetExtension)(const char *name);
    void        *(*TagRealloc)(void *ptr, size_t size);
};

struct game_export_ex_t {
    uint32_t    apiversion;
    uint32_t    structsize;

    void        *(*GetExtension)(const char *name);
    qboolean    (*CanSave)();
    void        (*PrepFrame)();
    void        (*RestartFilesystem)(); // called when fs_restart is issued
};

// EOF
