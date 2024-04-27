// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

typedef enum {
    F_IGNORE,
    F_INT,
    F_BOOL,
    F_FLOAT,
    F_LSTRING,
    F_VECTOR,
    F_ANGLEHACK,
    F_COLOR,
    F_L10N_STRING,
    F_POWER_ARMOR,
} fieldtype_t;

typedef struct {
    const char *name;
    void (*spawn)(edict_t *ent);
} spawn_func_t;

typedef struct {
    const char *name;
    unsigned ofs;
    fieldtype_t type;
} spawn_field_t;

void SP_info_player_start(edict_t *ent);
void SP_info_player_deathmatch(edict_t *ent);
void SP_info_player_coop(edict_t *ent);
void SP_info_player_intermission(edict_t *ent);

void SP_func_plat(edict_t *ent);
void SP_func_rotating(edict_t *ent);
void SP_func_button(edict_t *ent);
void SP_func_door(edict_t *ent);
void SP_func_door_secret(edict_t *ent);
void SP_func_door_rotating(edict_t *ent);
void SP_func_water(edict_t *ent);
void SP_func_train(edict_t *ent);
void SP_func_conveyor(edict_t *self);
void SP_func_wall(edict_t *self);
void SP_func_object(edict_t *self);
void SP_func_explosive(edict_t *self);
void SP_func_timer(edict_t *self);
void SP_func_areaportal(edict_t *ent);
void SP_func_clock(edict_t *ent);
void SP_func_killbox(edict_t *ent);
void SP_func_eye(edict_t *ent); // [Paril-KEX]
void SP_func_animation(edict_t *ent); // [Paril-KEX]
void SP_func_spinning(edict_t *ent); // [Paril-KEX]

void SP_trigger_always(edict_t *ent);
void SP_trigger_once(edict_t *ent);
void SP_trigger_multiple(edict_t *ent);
void SP_trigger_relay(edict_t *ent);
void SP_trigger_push(edict_t *ent);
void SP_trigger_hurt(edict_t *ent);
void SP_trigger_key(edict_t *ent);
void SP_trigger_counter(edict_t *ent);
void SP_trigger_elevator(edict_t *ent);
void SP_trigger_gravity(edict_t *ent);
void SP_trigger_monsterjump(edict_t *ent);
void SP_trigger_flashlight(edict_t *self); // [Paril-KEX]
void SP_trigger_fog(edict_t *self); // [Paril-KEX]
void SP_trigger_coop_relay(edict_t *self); // [Paril-KEX]
void SP_trigger_health_relay(edict_t *self); // [Paril-KEX]

void SP_target_temp_entity(edict_t *ent);
void SP_target_speaker(edict_t *ent);
void SP_target_explosion(edict_t *ent);
void SP_target_changelevel(edict_t *ent);
void SP_target_secret(edict_t *ent);
void SP_target_goal(edict_t *ent);
void SP_target_splash(edict_t *ent);
void SP_target_spawner(edict_t *ent);
void SP_target_blaster(edict_t *ent);
void SP_target_crosslevel_trigger(edict_t *ent);
void SP_target_crosslevel_target(edict_t *ent);
void SP_target_crossunit_trigger(edict_t *ent); // [Paril-KEX]
void SP_target_crossunit_target(edict_t *ent); // [Paril-KEX]
void SP_target_laser(edict_t *self);
void SP_target_help(edict_t *ent);
void SP_target_actor(edict_t *ent);
void SP_target_lightramp(edict_t *self);
void SP_target_earthquake(edict_t *ent);
void SP_target_character(edict_t *ent);
void SP_target_string(edict_t *ent);
void SP_target_camera(edict_t *self); // [Sam-KEX]
void SP_target_gravity(edict_t *self); // [Sam-KEX]
void SP_target_soundfx(edict_t *self); // [Sam-KEX]
void SP_target_light(edict_t *self); // [Paril-KEX]
void SP_target_poi(edict_t *ent); // [Paril-KEX]
void SP_target_music(edict_t *ent);
void SP_target_healthbar(edict_t *self); // [Paril-KEX]
void SP_target_autosave(edict_t *self); // [Paril-KEX]
void SP_target_sky(edict_t *self); // [Paril-KEX]
void SP_target_achievement(edict_t *self); // [Paril-KEX]
void SP_target_story(edict_t *self); // [Paril-KEX]

void SP_worldspawn(edict_t *ent);

void SP_dynamic_light(edict_t *self);
void SP_light(edict_t *self);
void SP_light_mine1(edict_t *ent);
void SP_light_mine2(edict_t *ent);
void SP_info_null(edict_t *self);
void SP_info_notnull(edict_t *self);
void SP_info_landmark(edict_t *self);  // [Paril-KEX]
void SP_misc_player_mannequin(edict_t *self);
void SP_misc_model(edict_t *self); // [Paril-KEX]
void SP_path_corner(edict_t *self);
void SP_point_combat(edict_t *self);

void SP_misc_explobox(edict_t *self);
void SP_misc_banner(edict_t *self);
void SP_misc_satellite_dish(edict_t *self);
void SP_misc_actor(edict_t *self);
void SP_misc_gib_arm(edict_t *self);
void SP_misc_gib_leg(edict_t *self);
void SP_misc_gib_head(edict_t *self);
void SP_misc_insane(edict_t *self);
void SP_misc_deadsoldier(edict_t *self);
void SP_misc_viper(edict_t *self);
void SP_misc_viper_bomb(edict_t *self);
void SP_misc_bigviper(edict_t *self);
void SP_misc_strogg_ship(edict_t *self);
void SP_misc_teleporter(edict_t *self);
void SP_misc_teleporter_dest(edict_t *self);
void SP_misc_blackhole(edict_t *self);
void SP_misc_eastertank(edict_t *self);
void SP_misc_easterchick(edict_t *self);
void SP_misc_easterchick2(edict_t *self);

void SP_misc_flare(edict_t *ent); // [Sam-KEX]
void SP_misc_hologram(edict_t *ent);
void SP_misc_lavaball(edict_t *ent);

void SP_monster_berserk(edict_t *self);
void SP_monster_gladiator(edict_t *self);
void SP_monster_gunner(edict_t *self);
void SP_monster_infantry(edict_t *self);
void SP_monster_soldier_light(edict_t *self);
void SP_monster_soldier(edict_t *self);
void SP_monster_soldier_ss(edict_t *self);
void SP_monster_tank(edict_t *self);
void SP_monster_medic(edict_t *self);
void SP_monster_flipper(edict_t *self);
void SP_monster_chick(edict_t *self);
void SP_monster_parasite(edict_t *self);
void SP_monster_flyer(edict_t *self);
void SP_monster_brain(edict_t *self);
void SP_monster_floater(edict_t *self);
void SP_monster_hover(edict_t *self);
void SP_monster_mutant(edict_t *self);
void SP_monster_supertank(edict_t *self);
void SP_monster_boss2(edict_t *self);
void SP_monster_jorg(edict_t *self);
void SP_monster_boss3_stand(edict_t *self);
void SP_monster_makron(edict_t *self);
// Paril
void SP_monster_tank_stand(edict_t *self);
void SP_monster_guardian(edict_t *self);
void SP_monster_arachnid(edict_t *self);
void SP_monster_guncmdr(edict_t *self);
void SP_monster_shambler(edict_t *self);

void SP_monster_commander_body(edict_t *self);

void SP_turret_breach(edict_t *self);
void SP_turret_base(edict_t *self);
void SP_turret_driver(edict_t *self);

// RAFAEL 14-APR-98
void SP_monster_soldier_hypergun(edict_t *self);
void SP_monster_soldier_lasergun(edict_t *self);
void SP_monster_soldier_ripper(edict_t *self);
void SP_monster_fixbot(edict_t *self);
void SP_monster_gekk(edict_t *self);
void SP_monster_chick_heat(edict_t *self);
void SP_monster_gladb(edict_t *self);
void SP_monster_boss5(edict_t *self);
void SP_rotating_light(edict_t *self);
void SP_object_repair(edict_t *self);
void SP_misc_crashviper(edict_t *ent);
void SP_misc_viper_missile(edict_t *self);
void SP_misc_amb4(edict_t *ent);
void SP_target_mal_laser(edict_t *ent);
void SP_misc_transport(edict_t *ent);
// END 14-APR-98

void SP_misc_nuke(edict_t *ent);

//===========
// ROGUE
void SP_func_plat2(edict_t *ent);
void SP_func_door_secret2(edict_t *ent);
void SP_func_force_wall(edict_t *ent);
void SP_info_player_coop_lava(edict_t *self);
void SP_info_teleport_destination(edict_t *self);
void SP_trigger_teleport(edict_t *self);
void SP_trigger_disguise(edict_t *self);
void SP_monster_stalker(edict_t *self);
void SP_monster_turret(edict_t *self);
void SP_target_steam(edict_t *self);
void SP_target_anger(edict_t *self);
void SP_target_killplayers(edict_t *self);
// PMM - still experimental!
void SP_target_blacklight(edict_t *self);
void SP_target_orb(edict_t *self);
// pmm
void SP_hint_path(edict_t *self);
void SP_monster_carrier(edict_t *self);
void SP_monster_widow(edict_t *self);
void SP_monster_widow2(edict_t *self);
void SP_dm_tag_token(edict_t *self);
void SP_dm_dball_goal(edict_t *self);
void SP_dm_dball_ball(edict_t *self);
void SP_dm_dball_team1_start(edict_t *self);
void SP_dm_dball_team2_start(edict_t *self);
void SP_dm_dball_ball_start(edict_t *self);
void SP_dm_dball_speed_change(edict_t *self);
void SP_monster_kamikaze(edict_t *self);
void SP_turret_invisible_brain(edict_t *self);
void SP_misc_nuke_core(edict_t *self);
// ROGUE
//===========
//  ZOID
void SP_trigger_ctf_teleport(edict_t *self);
void SP_info_ctf_teleport_destination(edict_t *self);
// ZOID

// clang-format off
static const spawn_func_t spawn_funcs[] = {
    { "info_player_start", SP_info_player_start },
    { "info_player_deathmatch", SP_info_player_deathmatch },
    { "info_player_coop", SP_info_player_coop },
    { "info_player_intermission", SP_info_player_intermission },

    { "func_plat", SP_func_plat },
    { "func_button", SP_func_button },
    { "func_door", SP_func_door },
    { "func_door_secret", SP_func_door_secret },
    { "func_door_rotating", SP_func_door_rotating },
    { "func_rotating", SP_func_rotating },
    { "func_train", SP_func_train },
    { "func_water", SP_func_water },
    { "func_conveyor", SP_func_conveyor },
    { "func_areaportal", SP_func_areaportal },
    { "func_clock", SP_func_clock },
    { "func_wall", SP_func_wall },
    { "func_object", SP_func_object },
    { "func_timer", SP_func_timer },
    { "func_explosive", SP_func_explosive },
    { "func_killbox", SP_func_killbox },
    { "func_eye", SP_func_eye },
    { "func_animation", SP_func_animation },
    { "func_spinning", SP_func_spinning },

    { "trigger_always", SP_trigger_always },
    { "trigger_once", SP_trigger_once },
    { "trigger_multiple", SP_trigger_multiple },
    { "trigger_relay", SP_trigger_relay },
    { "trigger_push", SP_trigger_push },
    { "trigger_hurt", SP_trigger_hurt },
    { "trigger_key", SP_trigger_key },
    { "trigger_counter", SP_trigger_counter },
    { "trigger_elevator", SP_trigger_elevator },
    { "trigger_gravity", SP_trigger_gravity },
    { "trigger_monsterjump", SP_trigger_monsterjump },
    { "trigger_flashlight", SP_trigger_flashlight }, // [Paril-KEX]
    { "trigger_fog", SP_trigger_fog }, // [Paril-KEX]
    { "trigger_coop_relay", SP_trigger_coop_relay }, // [Paril-KEX]
    { "trigger_health_relay", SP_trigger_health_relay }, // [Paril-KEX]

    { "target_temp_entity", SP_target_temp_entity },
    { "target_speaker", SP_target_speaker },
    { "target_explosion", SP_target_explosion },
    { "target_changelevel", SP_target_changelevel },
    { "target_secret", SP_target_secret },
    { "target_goal", SP_target_goal },
    { "target_splash", SP_target_splash },
    { "target_spawner", SP_target_spawner },
    { "target_blaster", SP_target_blaster },
    { "target_crosslevel_trigger", SP_target_crosslevel_trigger },
    { "target_crosslevel_target", SP_target_crosslevel_target },
    { "target_crossunit_trigger", SP_target_crossunit_trigger }, // [Paril-KEX]
    { "target_crossunit_target", SP_target_crossunit_target }, // [Paril-KEX]
    { "target_laser", SP_target_laser },
    { "target_help", SP_target_help },
    { "target_actor", SP_target_actor },
    { "target_lightramp", SP_target_lightramp },
    { "target_earthquake", SP_target_earthquake },
    { "target_character", SP_target_character },
    { "target_string", SP_target_string },
    { "target_camera", SP_target_camera }, // [Sam-KEX]
    { "target_gravity", SP_target_gravity }, // [Sam-KEX]
    { "target_soundfx", SP_target_soundfx }, // [Sam-KEX]
    { "target_light", SP_target_light }, // [Paril-KEX]
    { "target_poi", SP_target_poi }, // [Paril-KEX]
    { "target_music", SP_target_music },
    { "target_healthbar", SP_target_healthbar }, // [Paril-KEX]
    { "target_autosave", SP_target_autosave }, // [Paril-KEX]
    { "target_sky", SP_target_sky }, // [Paril-KEX]
    { "target_achievement", SP_target_achievement }, // [Paril-KEX]
    { "target_story", SP_target_story }, // [Paril-KEX]

    { "worldspawn", SP_worldspawn },

    { "dynamic_light", SP_dynamic_light },
    { "light", SP_light },
    { "light_mine1", SP_light_mine1 },
    { "light_mine2", SP_light_mine2 },
    { "info_null", SP_info_null },
    { "func_group", SP_info_null },
    { "info_notnull", SP_info_notnull },
    { "info_landmark", SP_info_landmark },
    { "path_corner", SP_path_corner },
    { "point_combat", SP_point_combat },

    { "misc_explobox", SP_misc_explobox },
    { "misc_banner", SP_misc_banner },
    { "misc_satellite_dish", SP_misc_satellite_dish },
    { "misc_actor", SP_misc_actor },
    { "misc_player_mannequin", SP_misc_player_mannequin },
    { "misc_model", SP_misc_model }, // [Paril-KEX]
    { "misc_gib_arm", SP_misc_gib_arm },
    { "misc_gib_leg", SP_misc_gib_leg },
    { "misc_gib_head", SP_misc_gib_head },
    { "misc_insane", SP_misc_insane },
    { "misc_deadsoldier", SP_misc_deadsoldier },
    { "misc_viper", SP_misc_viper },
    { "misc_viper_bomb", SP_misc_viper_bomb },
    { "misc_bigviper", SP_misc_bigviper },
    { "misc_strogg_ship", SP_misc_strogg_ship },
    { "misc_teleporter", SP_misc_teleporter },
    { "misc_teleporter_dest", SP_misc_teleporter_dest },
    { "misc_blackhole", SP_misc_blackhole },
    { "misc_eastertank", SP_misc_eastertank },
    { "misc_easterchick", SP_misc_easterchick },
    { "misc_easterchick2", SP_misc_easterchick2 },
    { "misc_flare", SP_misc_flare }, // [Sam-KEX]
    { "misc_hologram", SP_misc_hologram }, // Paril
    { "misc_lavaball", SP_misc_lavaball }, // Paril

    { "monster_berserk", SP_monster_berserk },
    { "monster_gladiator", SP_monster_gladiator },
    { "monster_gunner", SP_monster_gunner },
    { "monster_infantry", SP_monster_infantry },
    { "monster_soldier_light", SP_monster_soldier_light },
    { "monster_soldier", SP_monster_soldier },
    { "monster_soldier_ss", SP_monster_soldier_ss },
    { "monster_tank", SP_monster_tank },
    { "monster_tank_commander", SP_monster_tank },
    { "monster_medic", SP_monster_medic },
    { "monster_flipper", SP_monster_flipper },
    { "monster_chick", SP_monster_chick },
    { "monster_parasite", SP_monster_parasite },
    { "monster_flyer", SP_monster_flyer },
    { "monster_brain", SP_monster_brain },
    { "monster_floater", SP_monster_floater },
    { "monster_hover", SP_monster_hover },
    { "monster_mutant", SP_monster_mutant },
    { "monster_supertank", SP_monster_supertank },
    { "monster_boss2", SP_monster_boss2 },
    { "monster_boss3_stand", SP_monster_boss3_stand },
    { "monster_jorg", SP_monster_jorg },
    // Paril: allow spawning makron
    { "monster_makron", SP_monster_makron },
    // Paril: N64
    { "monster_tank_stand", SP_monster_tank_stand },
    // Paril: PSX
    { "monster_guardian", SP_monster_guardian },
    { "monster_arachnid", SP_monster_arachnid },
    { "monster_guncmdr", SP_monster_guncmdr },
    { "monster_shambler", SP_monster_shambler },

    { "monster_commander_body", SP_monster_commander_body },

    { "turret_breach", SP_turret_breach },
    { "turret_base", SP_turret_base },
    { "turret_driver", SP_turret_driver },

    // RAFAEL
    { "func_object_repair", SP_object_repair },
    { "rotating_light", SP_rotating_light },
    { "target_mal_laser", SP_target_mal_laser },
    { "misc_crashviper", SP_misc_crashviper },
    { "misc_viper_missile", SP_misc_viper_missile },
    { "misc_amb4", SP_misc_amb4 },
    { "misc_transport", SP_misc_transport },
    { "misc_nuke", SP_misc_nuke },
    { "monster_soldier_hypergun", SP_monster_soldier_hypergun },
    { "monster_soldier_lasergun", SP_monster_soldier_lasergun },
    { "monster_soldier_ripper", SP_monster_soldier_ripper },
    { "monster_fixbot", SP_monster_fixbot },
    { "monster_gekk", SP_monster_gekk },
    { "monster_chick_heat", SP_monster_chick_heat },
    { "monster_gladb", SP_monster_gladb },
    { "monster_boss5", SP_monster_boss5 },
    // RAFAEL

    //==============
    // ROGUE
    { "func_plat2", SP_func_plat2 },
    { "func_door_secret2", SP_func_door_secret2 },
    { "func_force_wall", SP_func_force_wall },
    { "trigger_teleport", SP_trigger_teleport },
    { "trigger_disguise", SP_trigger_disguise },
    { "info_teleport_destination", SP_info_teleport_destination },
    { "info_player_coop_lava", SP_info_player_coop_lava },
    { "monster_stalker", SP_monster_stalker },
    { "monster_turret", SP_monster_turret },
    { "target_steam", SP_target_steam },
    { "target_anger", SP_target_anger },
    { "target_killplayers", SP_target_killplayers },
    // PMM - experiment
    { "target_blacklight", SP_target_blacklight },
    { "target_orb", SP_target_orb },
    // pmm
    { "monster_daedalus", SP_monster_hover },
    { "hint_path", SP_hint_path },
    { "monster_carrier", SP_monster_carrier },
    { "monster_widow", SP_monster_widow },
    { "monster_widow2", SP_monster_widow2 },
    { "monster_medic_commander", SP_monster_medic },
    { "dm_tag_token", SP_dm_tag_token },
    { "dm_dball_goal", SP_dm_dball_goal },
    { "dm_dball_ball", SP_dm_dball_ball },
    { "dm_dball_team1_start", SP_dm_dball_team1_start },
    { "dm_dball_team2_start", SP_dm_dball_team2_start },
    { "dm_dball_ball_start", SP_dm_dball_ball_start },
    { "dm_dball_speed_change", SP_dm_dball_speed_change },
    { "monster_kamikaze", SP_monster_kamikaze },
    { "turret_invisible_brain", SP_turret_invisible_brain },
    { "misc_nuke_core", SP_misc_nuke_core },
    // ROGUE
    //==============

    // ZOID
    { "trigger_ctf_teleport", SP_trigger_ctf_teleport },
    { "info_ctf_teleport_destination", SP_info_ctf_teleport_destination },
    { "misc_ctf_banner", SP_misc_ctf_banner },
    { "misc_ctf_small_banner", SP_misc_ctf_small_banner },
    { "info_player_team1", SP_info_player_team1 },
    { "info_player_team2", SP_info_player_team2 },
    // ZOID

    { NULL }
};
// clang-format on

// fields are used for spawning from the entity string
static const spawn_field_t entity_fields[] = {
    { "classname", FOFS(classname), F_LSTRING },
    { "model", FOFS(model), F_LSTRING },
    { "spawnflags", FOFS(spawnflags), F_INT },
    { "speed", FOFS(speed), F_FLOAT },
    { "accel", FOFS(accel), F_FLOAT },
    { "decel", FOFS(decel), F_FLOAT },
    { "target", FOFS(target), F_LSTRING },
    { "targetname", FOFS(targetname), F_LSTRING },
    { "pathtarget", FOFS(pathtarget), F_LSTRING },
    { "deathtarget", FOFS(deathtarget), F_LSTRING },
    { "healthtarget", FOFS(healthtarget), F_LSTRING },
    { "itemtarget", FOFS(itemtarget), F_LSTRING },
    { "killtarget", FOFS(killtarget), F_LSTRING },
    { "combattarget", FOFS(combattarget), F_LSTRING },
    { "message", FOFS(message), F_L10N_STRING },
    { "team", FOFS(team), F_LSTRING },
    { "wait", FOFS(wait), F_FLOAT },
    { "delay", FOFS(delay), F_FLOAT },
    { "random", FOFS(random), F_FLOAT },
    { "move_origin", FOFS(move_origin), F_VECTOR },
    { "move_angles", FOFS(move_angles), F_VECTOR },
    { "style", FOFS(style), F_INT },
    { "style_off", FOFS(style_off), F_LSTRING },
    { "style_on", FOFS(style_off), F_LSTRING },
    { "crosslevel_flags", FOFS(crosslevel_flags), F_INT },
    { "count", FOFS(count), F_INT },
    { "health", FOFS(health), F_INT },
    { "sounds", FOFS(sounds), F_INT },
    { "light", 0, F_IGNORE },
    { "dmg", FOFS(dmg), F_INT },
    { "mass", FOFS(mass), F_INT },
    { "volume", FOFS(volume), F_FLOAT },
    { "attenuation", FOFS(attenuation), F_FLOAT },
    { "map", FOFS(map), F_LSTRING },
    { "origin", FOFS(s.origin), F_VECTOR },
    { "angles", FOFS(s.angles), F_VECTOR },
    { "angle", FOFS(s.angles), F_ANGLEHACK },
    { "rgba", FOFS(s.skinnum), F_COLOR }, // [Sam-KEX]
    { "hackflags", FOFS(hackflags), F_INT }, // [Paril-KEX] n64
    { "alpha", FOFS(x.alpha), F_FLOAT }, // [Paril-KEX]
    { "scale", FOFS(x.scale), F_FLOAT }, // [Paril-KEX]
    { "morefx", FOFS(x.morefx), F_INT },
    { "mangle", 0, F_IGNORE }, // editor field
    { "dead_frame", FOFS(monsterinfo.start_frame), F_INT }, // [Paril-KEX]
    { "frame", FOFS(s.frame), F_INT },
    { "effects", FOFS(s.effects), F_INT },
    { "renderfx", FOFS(s.renderfx), F_INT },

    // [Paril-KEX] func_eye stuff
    { "eye_position", FOFS(move_origin), F_VECTOR },
    { "vision_cone", FOFS(yaw_speed), F_FLOAT },

    // [Paril-KEX] for trigger_coop_relay
    { "message2", FOFS(map), F_LSTRING },
    { "mins", FOFS(mins), F_VECTOR },
    { "maxs", FOFS(maxs), F_VECTOR },

    // [Paril-KEX] customizable bmodel animations
    { "bmodel_anim_start", FOFS(bmodel_anim.params[0].start), F_INT },
    { "bmodel_anim_end", FOFS(bmodel_anim.params[0].end), F_INT },
    { "bmodel_anim_style", FOFS(bmodel_anim.params[0].style), F_INT },
    { "bmodel_anim_speed", FOFS(bmodel_anim.params[0].speed), F_INT },
    { "bmodel_anim_nowrap", FOFS(bmodel_anim.params[0].nowrap), F_BOOL },

    { "bmodel_anim_alt_start", FOFS(bmodel_anim.params[1].start), F_INT },
    { "bmodel_anim_alt_end", FOFS(bmodel_anim.params[1].end), F_INT },
    { "bmodel_anim_alt_style", FOFS(bmodel_anim.params[1].style), F_INT },
    { "bmodel_anim_alt_speed", FOFS(bmodel_anim.params[1].speed), F_INT },
    { "bmodel_anim_alt_nowrap", FOFS(bmodel_anim.params[1].nowrap), F_BOOL },

    // [Paril-KEX] customizable power armor stuff
    { "power_armor_power", FOFS(monsterinfo.power_armor_power), F_INT },
    { "power_armor_type", FOFS(monsterinfo.power_armor_type), F_POWER_ARMOR },

    { "monster_slots", FOFS(monsterinfo.monster_slots), F_INT },
};

// temp spawn vars -- only valid when the spawn function is called
static const spawn_field_t temp_fields[] = {
    { "lip", STOFS(lip), F_INT },
    { "distance", STOFS(distance), F_INT },
    { "height", STOFS(height), F_INT },
    { "noise", STOFS(noise), F_LSTRING },
    { "pausetime", STOFS(pausetime), F_FLOAT },
    { "item", STOFS(item), F_LSTRING },

    { "gravity", STOFS(gravity), F_LSTRING },
    { "sky", STOFS(sky), F_LSTRING },
    { "skyrotate", STOFS(skyrotate), F_FLOAT },
    { "skyaxis", STOFS(skyaxis), F_VECTOR },
    { "skyautorotate", STOFS(skyautorotate), F_INT },
    { "minyaw", STOFS(minyaw), F_FLOAT },
    { "maxyaw", STOFS(maxyaw), F_FLOAT },
    { "minpitch", STOFS(minpitch), F_FLOAT },
    { "maxpitch", STOFS(maxpitch), F_FLOAT },
    { "nextmap", STOFS(nextmap), F_LSTRING },

    { "music", STOFS(music), F_LSTRING }, // [Edward-KEX]
    { "instantitems", STOFS(instantitems), F_INT },
    { "radius", STOFS(radius), F_FLOAT },
    { "hub_map", STOFS(hub_map), F_BOOL },
    { "achievement", STOFS(achievement), F_LSTRING },
    { "goals", STOFS(goals), F_LSTRING },
    { "image", STOFS(image), F_LSTRING },
    { "fade_start_dist", STOFS(fade_start_dist), F_INT },
    { "fade_end_dist", STOFS(fade_end_dist), F_INT },
    { "start_items", STOFS(start_items), F_LSTRING },
    { "no_grapple", STOFS(no_grapple), F_INT },
    { "health_multiplier", STOFS(health_multiplier), F_FLOAT },
    { "reinforcements", STOFS(reinforcements), F_LSTRING },
    { "noise_start", STOFS(noise_start), F_LSTRING },
    { "noise_middle", STOFS(noise_middle), F_LSTRING },
    { "noise_end", STOFS(noise_end), F_LSTRING },
    { "loop_count", STOFS(loop_count), F_INT },
};

static byte entity_bitmap[(q_countof(entity_fields) + 7) / 8];
static byte temp_bitmap[(q_countof(temp_fields) + 7) / 8];

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn(edict_t *ent)
{
    const spawn_func_t *s;
    const gitem_t      *item;
    int                 i;

    if (!ent->classname) {
        gi.dprintf("ED_CallSpawn: NULL classname\n");
        G_FreeEdict(ent);
        return;
    }

    // PGM - do this before calling the spawn function so it can be overridden.
    VectorSet(ent->gravityVector, 0, 0, -1);
    // PGM

    // FIXME - PMM classnames hack
    if (!strcmp(ent->classname, "weapon_nailgun"))
        ent->classname = GetItemByIndex(IT_WEAPON_ETF_RIFLE)->classname;
    if (!strcmp(ent->classname, "ammo_nails"))
        ent->classname = GetItemByIndex(IT_AMMO_FLECHETTES)->classname;
    if (!strcmp(ent->classname, "weapon_heatbeam"))
        ent->classname = GetItemByIndex(IT_WEAPON_PLASMABEAM)->classname;
    // pmm

    // check item spawn functions
    for (i = 0, item = itemlist; i < IT_TOTAL; i++, item++) {
        if (!item->classname)
            continue;
        if (!strcmp(item->classname, ent->classname)) {
            // found it
            // before spawning, pick random item replacement
            if (g_dm_random_items->integer) {
                ent->item = item;
                item_id_t new_item = DoRandomRespawn(ent);

                if (new_item) {
                    item = GetItemByIndex(new_item);
                    ent->classname = item->classname;
                }
            }

            SpawnItem(ent, item);
            return;
        }
    }

    // check normal spawn functions
    for (s = spawn_funcs; s->name; s++) {
        if (!strcmp(s->name, ent->classname)) {
            // found it
            s->spawn(ent);

            // Paril: swap classname with stored constant if we didn't change it
            if (strcmp(ent->classname, s->name) == 0)
                ent->classname = s->name;
            return;
        }
    }

    gi.dprintf("%s doesn't have a spawn function\n", etos(ent));
    G_FreeEdict(ent);
}

/*
=============
ED_NewString
=============
*/
char *ED_NewString(const char *string)
{
    char    *newb, *new_p;
    int     i;
    size_t  l;

    l = strlen(string) + 1;

    newb = gi.TagMalloc(l, TAG_LEVEL);

    new_p = newb;

    for (i = 0; i < l; i++) {
        if (string[i] == '\\' && i < l - 1) {
            i++;
            if (string[i] == 'n')
                *new_p++ = '\n';
            else
                *new_p++ = '\\';
        } else
            *new_p++ = string[i];
    }

    return newb;
}

static int ED_ParseColor(const char *value)
{
    vec4_t v = { 0, 0, 0, 1.0f };
    int i, n;

    // space means rgba as values
    n = sscanf(value, "%f %f %f %f", &v[0], &v[1], &v[2], &v[3]);
    if (n < 2)
        return strtoul(value, NULL, 0); // integral

    for (i = 0; i < n; i++)
        if (v[i] > 1.0f)
            break;

    if (i == n)
        for (i = 0; i < 4; i++)
            v[i] *= 255;
    else if (n < 4)
        v[3] = 255;

    // 0xRRGGBBAA encoding
    return MakeBigLong(v[0], v[1], v[2], v[3]);
}

static char *ED_ParseL10nString(const char *value)
{
    if (*value == '$') {
        const char *s = G_GetL10nString(value + 1);
        if (s)
            return G_CopyString(s, TAG_LEVEL);
    }
    return ED_NewString(value);
}

static int ED_ParsePowerArmor(const char *value)
{
    switch (Q_atoi(value)) {
    case 0:
        return IT_NULL;
    case 1:
        return IT_ITEM_POWER_SCREEN;
    default:
        return IT_ITEM_POWER_SHIELD;
    }
}

static void ED_LoadField(const spawn_field_t *f, const char *value, byte *b)
{
    float   v;
    vec3_t  vec;

    // found it
    switch (f->type) {
    case F_LSTRING:
        *(char **)(b + f->ofs) = ED_NewString(value);
        break;
    case F_VECTOR:
        if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) {
            gi.dprintf("%s: couldn't parse '%s'\n", __func__, f->name);
            VectorClear(vec);
        }
        ((float *)(b + f->ofs))[0] = vec[0];
        ((float *)(b + f->ofs))[1] = vec[1];
        ((float *)(b + f->ofs))[2] = vec[2];
        break;
    case F_INT:
        *(int *)(b + f->ofs) = Q_atoi(value);
        break;
    case F_FLOAT:
        *(float *)(b + f->ofs) = atof(value);
        break;
    case F_ANGLEHACK:
        v = atof(value);
        ((float *)(b + f->ofs))[0] = 0;
        ((float *)(b + f->ofs))[1] = v;
        ((float *)(b + f->ofs))[2] = 0;
        break;
    case F_BOOL:
        *(bool *)(b + f->ofs) = Q_atoi(value);
        break;
    case F_IGNORE:
        break;
    case F_COLOR:
        *(int *)(b + f->ofs) = ED_ParseColor(value);
        break;
    case F_L10N_STRING:
        *(char **)(b + f->ofs) = ED_ParseL10nString(value);
        break;
    case F_POWER_ARMOR:
        *(int *)(b + f->ofs) = ED_ParsePowerArmor(value);
        break;
    default:
        gi.error("%s: bad field type", __func__);
        break;
    }
}

/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField(const char *key, const char *value, edict_t *ent)
{
    const spawn_field_t *f;
    int i;

    if (!Q_strncasecmp(key, "shadowlight", 11))
        return;
    if (!Q_strncasecmp(key, "fog_", 4) || !Q_strncasecmp(key, "heightfog_", 10))
        return;

    // check st first
    for (i = 0, f = temp_fields; i < q_countof(temp_fields); i++, f++) {
        if (Q_strcasecmp(f->name, key))
            continue;

        Q_SetBit(temp_bitmap, i);

        // found it
        ED_LoadField(f, value, (byte *)&st);
        return;
    }

    // now entity
    for (i = 0, f = entity_fields; i < q_countof(entity_fields); i++, f++) {
        if (Q_strcasecmp(f->name, key))
            continue;

        Q_SetBit(entity_bitmap, i);

        // [Paril-KEX]
        if (!strcmp(f->name, "bmodel_anim_start") || !strcmp(f->name, "bmodel_anim_end"))
            ent->bmodel_anim.enabled = true;

        // found it
        ED_LoadField(f, value, (byte *)ent);
        return;
    }

    gi.dprintf("%s is not a valid field\n", key);
}

bool ED_WasKeySpecified(const char *key)
{
    const spawn_field_t *f;
    int i;

    // check st first
    for (i = 0, f = temp_fields; i < q_countof(temp_fields); i++, f++)
        if (!strcmp(f->name, key))
            return Q_IsBitSet(temp_bitmap, i);

    // now entity
    for (i = 0, f = entity_fields; i < q_countof(entity_fields); i++, f++)
        if (!strcmp(f->name, key))
            return Q_IsBitSet(entity_bitmap, i);

    return false;
}

void ED_SetKeySpecified(const char *key)
{
    const spawn_field_t *f;
    int i;

    // check st first
    for (i = 0, f = temp_fields; i < q_countof(temp_fields); i++, f++) {
        if (!strcmp(f->name, key)) {
            Q_SetBit(temp_bitmap, i);
            return;
        }
    }

    // now entity
    for (i = 0, f = entity_fields; i < q_countof(entity_fields); i++, f++) {
        if (!strcmp(f->name, key)) {
            Q_SetBit(entity_bitmap, i);
            return;
        }
    }
}

void ED_InitSpawnVars(void)
{
    memset(&st, 0, sizeof(st));
    st.skyautorotate = 1;
    st.fade_start_dist = 96;
    st.fade_end_dist = 384;
    st.health_multiplier = 1.0f;

    memset(temp_bitmap, 0, sizeof(temp_bitmap));
    memset(entity_bitmap, 0, sizeof(entity_bitmap));
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
static const char *ED_ParseEdict(const char *data, edict_t *ent)
{
    bool  init;
    char  keyname[256];
    const char *com_token;

    init = false;

    ED_InitSpawnVars();

    // go through all the dictionary pairs
    while (1) {
        // parse key
        com_token = COM_Parse(&data);
        if (com_token[0] == '}')
            break;
        if (!data)
            gi.error("ED_ParseEntity: EOF without closing brace");

        Q_strlcpy(keyname, com_token, sizeof(keyname));

        // parse value
        com_token = COM_Parse(&data);
        if (!data)
            gi.error("ED_ParseEntity: EOF without closing brace");

        if (com_token[0] == '}')
            gi.error("ED_ParseEntity: closing brace without data");

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_') {
            // [Sam-KEX] Hack for setting RGBA for shadow-casting lights
            if (!strcmp(keyname, "_color"))
                ent->s.skinnum = ED_ParseColor(com_token);
            continue;
        }

        ED_ParseField(keyname, com_token, ent);
    }

    if (!init)
        memset(ent, 0, sizeof(*ent));

    return data;
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/

// adjusts teams so that trains that move their children
// are in the front of the team
static void G_FixTeams(void)
{
    edict_t *e, *e2, *chain;
    int i, j;
    int c;

    c = 0;
    for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++) {
        if (!e->inuse)
            continue;
        if (!e->team)
            continue;
        if (!(e->flags & FL_TEAMSLAVE))
            continue;
        if (strcmp(e->classname, "func_train"))
            continue;
        if (!(e->spawnflags & SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN))
            continue;

        chain = e;
        e->teammaster = e;
        e->teamchain = NULL;
        e->flags &= ~FL_TEAMSLAVE;
        e->flags |= FL_TEAMMASTER;
        c++;
        for (j = 1, e2 = g_edicts + j; j < globals.num_edicts; j++, e2++) {
            if (e2 == e)
                continue;
            if (!e2->inuse)
                continue;
            if (!e2->team)
                continue;
            if (!strcmp(e->team, e2->team)) {
                chain->teamchain = e2;
                e2->teammaster = e;
                e2->teamchain = NULL;
                chain = e2;
                e2->flags |= FL_TEAMSLAVE;
                e2->flags &= ~FL_TEAMMASTER;
                e2->movetype = MOVETYPE_PUSH;
                e2->speed = e->speed;
            }
        }
    }

    gi.dprintf("%d teams repaired\n", c);
}

static void G_FindTeams(void)
{
    edict_t *e, *e2, *chain;
    int i, j;
    int c, c2;

    c = 0;
    c2 = 0;
    for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++) {
        if (!e->inuse)
            continue;
        if (!e->team)
            continue;
        if (e->flags & FL_TEAMSLAVE)
            continue;
        chain = e;
        e->teammaster = e;
        e->flags |= FL_TEAMMASTER;
        c++;
        c2++;
        for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++) {
            if (!e2->inuse)
                continue;
            if (!e2->team)
                continue;
            if (e2->flags & FL_TEAMSLAVE)
                continue;
            if (!strcmp(e->team, e2->team)) {
                c2++;
                chain->teamchain = e2;
                e2->teammaster = e;
                chain = e2;
                e2->flags |= FL_TEAMSLAVE;
            }
        }
    }

    // ROGUE
    G_FixTeams();
    // ROGUE

    gi.dprintf("%d teams with %d entities\n", c, c2);
}

// inhibit entities from game based on cvars & spawnflags
static bool G_InhibitEntity(edict_t *ent)
{
    // dm-only
    if (deathmatch->integer)
        return (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH);

    // coop flags
    if (coop->integer && (ent->spawnflags & SPAWNFLAG_NOT_COOP))
        return true;
    if (!coop->integer && (ent->spawnflags & SPAWNFLAG_COOP_ONLY))
        return true;

    // skill
    return ((skill->integer == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
           ((skill->integer == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
           ((skill->integer >= 2) && (ent->spawnflags & SPAWNFLAG_NOT_HARD));
}

// [Paril-KEX]
void G_PrecacheInventoryItems(void)
{
    if (deathmatch->integer)
        return;

    for (int i = 0; i < game.maxclients; i++) {
        gclient_t *cl = &game.clients[i];
        for (item_id_t id = IT_NULL; id < IT_TOTAL; id++)
            if (cl->pers.inventory[id])
                PrecacheItem(GetItemByIndex(id));
    }
}

// [Paril-KEX]
static void G_PrecacheStartItems(void)
{
    if (!*g_start_items->string)
        return;

    char copy[MAX_STRING_CHARS];
    Q_strlcpy(copy, g_start_items->string, sizeof(copy));

    const char *s = copy;
    while (1) {
        char *p = strchr(s, ';');
        if (p)
            *p = 0;

        char *token = COM_Parse(&s);
        if (*token) {
            const gitem_t *item = FindItemByClassname(token);

            if (!item || !item->pickup)
                gi.error("Invalid g_start_items entry: %s\n", token);

            PrecacheItem(item);
        }

        if (!p)
            break;
        s = p + 1;
    }
}

/*
==============
G_AddPrecache

Register new global precache function and call it (once).
==============
*/
void G_AddPrecache(void (*func)(void))
{
    precache_t *prec;

    for (prec = game.precaches; prec; prec = prec->next)
        if (prec->func == func)
            return;

    prec = gi.TagMalloc(sizeof(*prec), TAG_GAME);
    prec->func = func;
    prec->next = game.precaches;
    game.precaches = prec;

    prec->func();
}

/*
==============
G_RefreshPrecaches

Called from ReadLevel() to refresh all global precache indices registered by
spawn functions.
==============
*/
void G_RefreshPrecaches(void)
{
    precache_t *prec;

    for (prec = game.precaches; prec; prec = prec->next)
        prec->func();
}

/*
==============
G_FreePrecaches

Free precache functions from previous level.
==============
*/
static void G_FreePrecaches(void)
{
    precache_t *prec, *next;

    for (prec = game.precaches; prec; prec = next) {
        next = prec->next;
        gi.TagFree(prec);
    }

    game.precaches = NULL;
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint)
{
    edict_t *ent;
    int      inhibit;
    const char   *com_token;

    int skill_level = Q_clip(skill->integer, 0, 3);
    if (skill->integer != skill_level)
        gi.cvar_forceset("skill", va("%d", skill_level));

    SaveClientData();

    gi.FreeTags(TAG_LEVEL);

    G_FreePrecaches();

    memset(&level, 0, sizeof(level));
    memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

    // all other flags are not important atm
    //globals.server_flags &= SERVER_FLAG_LOADING;

    Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
    Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

    level.is_n64 = strncmp(level.mapname, "q64/", 4) == 0;

    level.coop_scale_players = 0;
    level.coop_health_scaling = Q_clipf(g_coop_health_scaling->value, 0, 1);

    // set client fields on player ents
    for (int i = 0; i < game.maxclients; i++) {
        g_edicts[i + 1].client = game.clients + i;

        // "disconnect" all players since the level is switching
        game.clients[i].pers.connected = false;
        game.clients[i].pers.spawned = false;
    }

    ent = NULL;
    inhibit = 0;

    // reserve some spots for dead player bodies for coop / deathmatch
    InitBodyQue();

    // parse ents
    while (1) {
        // parse the opening brace
        com_token = COM_Parse(&entities);
        if (!entities)
            break;
        if (com_token[0] != '{')
            gi.error("ED_LoadFromFile: found \"%s\" when expecting {", com_token);

        if (!ent)
            ent = g_edicts;
        else
            ent = G_Spawn();
        entities = ED_ParseEdict(entities, ent);

        // remove things (except the world) from different skill levels or deathmatch
        if (ent != g_edicts) {
            if (G_InhibitEntity(ent)) {
                G_FreeEdict(ent);
                inhibit++;
                continue;
            }

            ent->spawnflags &= ~SPAWNFLAG_EDITOR_MASK;
        }

        if (!ent)
            gi.error("invalid/empty entity string!");

        ED_CallSpawn(ent);

        ent->s.renderfx |= RF_IR_VISIBLE; // PGM
    }

    gi.dprintf("%d entities inhibited\n", inhibit);

    // precache start_items
    G_PrecacheStartItems();

    // precache player inventory items
    G_PrecacheInventoryItems();

    G_FindTeams();

    // ZOID
    CTFSpawn();
    // ZOID

    // ROGUE
    if (deathmatch->integer) {
        if (g_dm_random_items->integer)
            PrecacheForRandomRespawn();
    } else {
        InitHintPaths(); // if there aren't hintpaths on this map, enable quick aborts
    }
    // ROGUE

    // ROGUE    -- allow dm games to do init stuff right before game starts.
    if (deathmatch->integer && gamerules->integer) {
        if (DMGame.PostInitSetup)
            DMGame.PostInitSetup();
    }
    // ROGUE
}

//===================================================================

#include "g_statusbar.h"

// create & set the statusbar string for the current gamemode
static void G_InitStatusbar(void)
{
    statusbar_t sb, *sb_ptr = &sb;
    sb.size = 0;

    // ---- shared stuff that every gamemode uses ----
    sb_yb(-24)

    // health
    sb_xv(0) sb_hnum() sb_xv(50) sb_pic(STAT_HEALTH_ICON)

    // ammo
    sb_if(STAT_AMMO_ICON) sb_xv(100) sb_anum() sb_xv(150) sb_pic(STAT_AMMO_ICON) sb_endif()

    // armor
    sb_if(STAT_ARMOR_ICON) sb_xv(200) sb_rnum() sb_xv(250) sb_pic(STAT_ARMOR_ICON) sb_endif()

    // selected item
    sb_if(STAT_SELECTED_ICON) sb_xv(296) sb_pic(STAT_SELECTED_ICON) sb_endif()

    sb_yb(-50)

    // picked up item
    sb_if(STAT_PICKUP_ICON)
        sb_xv(0)
        sb_pic(STAT_PICKUP_ICON)
        sb_xv(26)
        sb_yb(-42)
        sb_stat_string(STAT_PICKUP_STRING)
        sb_yb(-50)
    sb_endif()

    // selected item name
    sb_if(STAT_SELECTED_ITEM_NAME)
        sb_yb(-34)
        sb_xv(319)
        sb_stat_rstring(STAT_SELECTED_ITEM_NAME)
        sb_yb(-58)
    sb_endif()

    // timer
    sb_if(STAT_TIMER_ICON)
        sb_xv(262)
        sb_num(2, STAT_TIMER)
        sb_xv(296)
        sb_pic(STAT_TIMER_ICON)
    sb_endif()

    sb_yb(-50)

    // help / weapon icon
    sb_if(STAT_HELPICON) sb_xv(150) sb_pic(STAT_HELPICON) sb_endif()

    // ---- gamemode-specific stuff ----
    if (!deathmatch->integer) {
        // SP/coop
        // key display
        // move up if the timer is active
        // FIXME: ugly af
        sb_if(STAT_TIMER_ICON) sb_yb(-76) sb_endif()

        sb_if(STAT_SELECTED_ITEM_NAME)
            sb_yb(-58)
            sb_if(STAT_TIMER_ICON) sb_yb(-84) sb_endif()
        sb_endif()

        sb_if(STAT_KEY_A) sb_xv(296) sb_pic(STAT_KEY_A) sb_endif()
        sb_if(STAT_KEY_B) sb_xv(272) sb_pic(STAT_KEY_B) sb_endif()
        sb_if(STAT_KEY_C) sb_xv(248) sb_pic(STAT_KEY_C) sb_endif()

        if (coop->integer) {
            // top of screen coop respawn display
            sb_if(STAT_COOP_RESPAWN)
                sb_xv(0)
                sb_yt(0)
                sb_stat_cstring2(STAT_COOP_RESPAWN)
            sb_endif()

            // coop lives
            sb_if(STAT_LIVES)
                sb_xr(-16)
                sb_yt(2)
                sb_num(3, STAT_LIVES)
                sb_xr(0)
                sb_yt(28)
                sb_rstring("Lives")
            sb_endif()
        }
    } else if (G_TeamplayEnabled()) {
        CTFPrecache();

        // ctf/tdm
        // red team
        sb_yb(-110)
        sb_if(STAT_CTF_TEAM1_PIC)
            sb_xr(-26)
            sb_pic(STAT_CTF_TEAM1_PIC)
        sb_endif()

        sb_xr(-78)
        sb_num(3, STAT_CTF_TEAM1_CAPS)

        // joined overlay
        sb_if(STAT_CTF_JOINED_TEAM1_PIC)
            sb_yb(-112)
            sb_xr(-28)
            sb_pic(STAT_CTF_JOINED_TEAM1_PIC)
        sb_endif()

        // blue team
        sb_yb(-83)
        sb_if(STAT_CTF_TEAM2_PIC)
            sb_xr(-26)
            sb_pic(STAT_CTF_TEAM2_PIC)
        sb_endif()

        sb_xr(-78)
        sb_num(3, STAT_CTF_TEAM2_CAPS)

        // joined overlay
        sb_if(STAT_CTF_JOINED_TEAM2_PIC)
            sb_yb(-85)
            sb_xr(-28)
            sb_pic(STAT_CTF_JOINED_TEAM2_PIC)
        sb_endif()

        if (ctf->integer) {
            // have flag graph
            sb_if(STAT_CTF_FLAG_PIC)
                sb_yt(26)
                sb_xr(-24)
                sb_pic(STAT_CTF_FLAG_PIC)
            sb_endif()
        }

        // id view state
        sb_if(STAT_CTF_ID_VIEW)
            sb_xv(112)
            sb_yb(-58)
            sb_stat_string(STAT_CTF_ID_VIEW)
        sb_endif()

        // id view color
        sb_if(STAT_CTF_ID_VIEW_COLOR)
            sb_xv(96)
            sb_yb(-58)
            sb_pic(STAT_CTF_ID_VIEW_COLOR)
        sb_endif()

        if (ctf->integer) {
            // match
            sb_if(STAT_CTF_MATCH)
                sb_xl(0)
                sb_yb(-78)
                sb_stat_string(STAT_CTF_MATCH)
            sb_endif()
        }

        // team info
        //sb_if(STAT_CTF_TEAMINFO) sb_xl(0) sb_yb(-88) sb_stat_string(STAT_CTF_TEAMINFO) sb_endif()
    } else {
        // dm
        // frags
        sb_xr(-50) sb_yt(2) sb_num(3, STAT_FRAGS)

        // spectator
        sb_if(STAT_SPECTATOR) sb_xv(0) sb_yb(-58) sb_string2("SPECTATOR MODE") sb_endif()

        // chase cam
        sb_if(STAT_CHASE)
            sb_xv(0)
            sb_yb(-68)
            sb_string("CHASING")
            sb_xv(64)
            sb_stat_string(STAT_CHASE)
        sb_endif()
    }

    // ---- more shared stuff ----
    if (deathmatch->integer) {
        // tech
        sb_if(STAT_CTF_TECH)
            sb_yb(-137)
            sb_xr(-26)
            sb_pic(STAT_CTF_TECH)
        sb_endif()
    }

    gi.configstring(CS_STATUSBAR, sb.buffer);
}

static const char *const lightstyles[] = {
    "m",

    // 1 FLICKER (first variety)
    "mmnmmommommnonmmonqnmmo",

    // 2 SLOW STRONG PULSE
    "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",

    // 3 CANDLE (first variety)
    "mmmmmaaaaammmmmaaaaaabcdefgabcdefg",

    // 4 FAST STROBE
    "mamamamamama",

    // 5 GENTLE PULSE 1
    "jklmnopqrstuvwxyzyxwvutsrqponmlkj",

    // 6 FLICKER (second variety)
    "nmonqnmomnmomomno",

    // 7 CANDLE (second variety)`map
    "mmmaaaabcdefgmmmmaaaammmaamm",

    // 8 CANDLE (third variety)
    "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",

    // 9 SLOW STROBE (fourth variety)
    "aaaaaaaazzzzzzzz",

    // 10 FLUORESCENT FLICKER
    "mmamammmmammamamaaamammma",

    // 11 SLOW PULSE NOT FADE TO BLACK
    "abcdefghijklmnopqrrqponmlkjihgfedcba",

    // [Paril-KEX] 12 N64's 2 (fast strobe)
    "zzazazzzzazzazazaaazazzza",

    // [Paril-KEX] 13 N64's 3 (half of strong pulse)
    "abcdefghijklmnopqrstuvwxyz",

    // [Paril-KEX] 14 N64's 4 (fast strobe)
    "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
};

const char *G_GetLightStyle(int style)
{
    if (style < 0 || style >= q_countof(lightstyles))
        return "a";
    return lightstyles[style];
}

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
"sounds"    music cd track number
"gravity"   800 is default gravity
"message"   text to print at user logon
*/
void SP_worldspawn(edict_t *ent)
{
    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->inuse = qtrue; // since the world doesn't use G_Spawn()
    ent->s.modelindex = MODELINDEX_WORLD;
    ent->gravity = 1.0f;

    if (st.hub_map) {
        level.hub_map = true;

        // clear helps
        game.help1changed = game.help2changed = 0;
        *game.helpmessage1 = *game.helpmessage2 = '\0';

        for (int i = 0; i < game.maxclients; i++) {
            game.clients[i].pers.game_help1changed = game.clients[i].pers.game_help2changed = 0;
            game.clients[i].resp.coop_respawn.game_help1changed = game.clients[i].resp.coop_respawn.game_help2changed = 0;
        }
    }

    if (st.achievement && st.achievement[0])
        level.achievement = st.achievement;

    //---------------

    // set configstrings for items
    SetItemNames();

    if (st.nextmap)
        Q_strlcpy(level.nextmap, st.nextmap, sizeof(level.nextmap));

    // make some data visible to the server

    if (ent->message && ent->message[0]) {
        gi.configstring(CS_NAME, ent->message);
        Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
    } else
        Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));

    if (st.sky && st.sky[0])
        gi.configstring(CS_SKY, st.sky);
    else
        gi.configstring(CS_SKY, "unit1_");

    gi.configstring(CS_SKYROTATE, va("%f %d", st.skyrotate, st.skyautorotate));
    level.skyrotate = st.skyrotate;
    level.skyautorotate = st.skyautorotate;

    gi.configstring(CS_SKYAXIS, va("%f %f %f", st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

    if (st.music && st.music[0])
        gi.configstring(CS_CDTRACK, st.music);
    else
        gi.configstring(CS_CDTRACK, va("%d", ent->sounds));

#if 0
    if (level.is_n64)
        gi.configstring(CS_CD_LOOP_COUNT, "0");
    else if (ED_WasKeySpecified("loop_count"))
        gi.configstring(CS_CD_LOOP_COUNT, va("%d", st.loop_count));
    else
        gi.configstring(CS_CD_LOOP_COUNT, "");
#endif

    level.instantitems = st.instantitems > 0 || level.is_n64;

    // [Paril-KEX]
    if (st.goals) {
        level.goals = st.goals;
        game.help1changed++;
    }

    if (st.start_items)
        level.start_items = st.start_items;

    if (st.no_grapple)
        level.no_grapple = st.no_grapple;

    gi.configstring(CS_MAXCLIENTS, va("%d", game.maxclients));

    // statusbar prog
    G_InitStatusbar();

    //---------------

    // help icon for statusbar
    gi.imageindex("i_help");
    level.pic_health = gi.imageindex("i_health");
    gi.imageindex("help");
    gi.imageindex("field_3");

    if (!st.gravity) {
        level.gravity = 800;
        gi.cvar_set("sv_gravity", "800");
    } else {
        level.gravity = atof(st.gravity);
        gi.cvar_set("sv_gravity", st.gravity);
    }

    level.snd_fry = gi.soundindex("player/fry.wav"); // standing in lava / slime

    PrecacheItem(GetItemByIndex(IT_WEAPON_BLASTER));

    if (g_dm_random_items->integer)
        for (item_id_t i = IT_NULL + 1; i < IT_TOTAL; i++)
            PrecacheItem(GetItemByIndex(i));

    gi.soundindex("player/lava1.wav");
    gi.soundindex("player/lava2.wav");

    gi.soundindex("misc/pc_up.wav");
    gi.soundindex("misc/talk1.wav");

    // gibs
    gi.soundindex("misc/udeath.wav");

    gi.soundindex("items/respawn1.wav");
    gi.soundindex("misc/mon_power2.wav");

    // sexed sounds
    gi.soundindex("*death1.wav");
    gi.soundindex("*death2.wav");
    gi.soundindex("*death3.wav");
    gi.soundindex("*death4.wav");
    gi.soundindex("*fall1.wav");
    gi.soundindex("*fall2.wav");
    gi.soundindex("*gurp1.wav"); // drowning damage
    gi.soundindex("*gurp2.wav");
    gi.soundindex("*jump1.wav"); // player jump
    gi.soundindex("*pain25_1.wav");
    gi.soundindex("*pain25_2.wav");
    gi.soundindex("*pain50_1.wav");
    gi.soundindex("*pain50_2.wav");
    gi.soundindex("*pain75_1.wav");
    gi.soundindex("*pain75_2.wav");
    gi.soundindex("*pain100_1.wav");
    gi.soundindex("*pain100_2.wav");
    gi.soundindex("*drown1.wav"); // [Paril-KEX]

    // sexed models
    for (item_id_t id = IT_NULL; id < IT_TOTAL; id++) {
        const gitem_t *item = &itemlist[id];

        if (!item->vwep_model)
            continue;

        int model = gi.modelindex(item->vwep_model);

        if (!level.vwep_offset)
            level.vwep_offset = model;
    }

    //-------------------

    gi.soundindex("player/gasp1.wav"); // gasping for air
    gi.soundindex("player/gasp2.wav"); // head breaking surface, not gasping

    gi.soundindex("player/watr_in.wav");  // feet hitting water
    gi.soundindex("player/watr_out.wav"); // feet leaving water

    gi.soundindex("player/watr_un.wav"); // head going underwater

    gi.soundindex("player/u_breath1.wav");
    gi.soundindex("player/u_breath2.wav");

    gi.soundindex("player/wade1.wav");
    gi.soundindex("player/wade2.wav");
    gi.soundindex("player/wade3.wav");

    gi.soundindex("items/pkup.wav");   // bonus item pickup
    gi.soundindex("world/land.wav");   // landing thud
    gi.soundindex("misc/h2ohit1.wav"); // landing splash

    gi.soundindex("items/damage.wav");
    gi.soundindex("items/protect.wav");
    gi.soundindex("items/protect4.wav");
    gi.soundindex("weapons/noammo.wav");
    gi.soundindex("weapons/lowammo.wav");
    gi.soundindex("weapons/change.wav");

    gi.soundindex("infantry/inflies1.wav");

    gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
    gi.modelindex("models/objects/gibs/arm/tris.md2");
    gi.modelindex("models/objects/gibs/bone/tris.md2");
    gi.modelindex("models/objects/gibs/bone2/tris.md2");
    gi.modelindex("models/objects/gibs/chest/tris.md2");
    gi.modelindex("models/objects/gibs/skull/tris.md2");
    gi.modelindex("models/objects/gibs/head2/tris.md2");
    gi.modelindex("models/objects/gibs/sm_metal/tris.md2");

    //gi.imageindex("loc_ping");

    //
    // Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
    //

    for (int i = 0; i < q_countof(lightstyles); i++)
        gi.configstring(CS_LIGHTS + i, lightstyles[i]);

    // styles 32-62 are assigned by the light program for switchable lights

    // 63 testing
    gi.configstring(CS_LIGHTS + 63, "a");

    // coop respawn strings
    if (coop->integer) {
        static const char *const str[] = {
            "Can't respawn - in combat",
            "Can't respawn - bad area",
            "Can't respawn - blocked by something",
            "Waiting to respawn...",
            "No lives left; leave the level to respawn the team"
        };

        for (int i = 0; i < q_countof(str); i++)
            gi.configstring(CONFIG_COOP_RESPAWN_STRING + i, str[i]);
    }
}
