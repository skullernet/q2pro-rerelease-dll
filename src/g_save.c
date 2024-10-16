/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "g_local.h"
#include "g_ptrs.h"

#if USE_ZLIB
#include <zlib.h>
#ifdef __GNUC__
int gzprintf(gzFile, const char *, ...) q_printf(2, 3);
#endif
#else
#define gzopen(name, mode)          fopen(name, mode)
#define gzclose(file)               fclose(file)
#define gzprintf(file, ...)         fprintf(file, __VA_ARGS__)
#define gzgets(file, buf, size)     fgets(buf, size, file)
#define gzbuffer(file, size)        (void)0
#define gzFile                      FILE *
#endif

typedef enum {
    F_BYTE,
    F_SHORT,
    F_INT,
    F_UINT,         // hexadecimal
    F_INT64,
    F_UINT64,       // hexadecimal
    F_BOOL,
    F_FLOAT,
    F_GRAVITY,
    F_LSTRING,      // string on disk, pointer in memory, TAG_LEVEL
    F_GSTRING,      // string on disk, pointer in memory, TAG_GAME
    F_ZSTRING,      // string on disk, string in memory
    F_VECTOR,
    F_EDICT,        // index on disk, pointer in memory
    F_CLIENT,       // index on disk, pointer in memory
    F_ITEM,
    F_POINTER,
    F_CLIENT_PERSISTENT,
    F_INVENTORY,
    F_MAX_AMMO,
    F_STATS,
    F_MOVEINFO,
    F_MONSTERINFO,
    F_REINFORCEMENTS,
    F_BMODEL_ANIM,
    F_PLAYER_FOG,
    F_PLAYER_HEIGHTFOG,
    F_LEVEL_ENTRY,
} fieldtype_t;

typedef struct {
    const char *name;
    uint32_t ofs;
    uint16_t size;
    uint16_t type;
} save_field_t;

#define FA_(type, name, size) { #name, _OFS(name), size, type }
#define F_(type, name) FA_(type, name, 1)
#define SZ(name, size) FA_(F_ZSTRING, name, size)
#define BA(name, size) FA_(F_BYTE, name, size)
#define B(name) BA(name, 1)
#define SA(name, size) FA_(F_SHORT, name, size)
#define S(name) SA(name, 1)
#define IA(name, size) FA_(F_INT, name, size)
#define I(name) IA(name, 1)
#define H(name) F_(F_UINT, name)
#define H64(name) F_(F_UINT64, name)
#define O(name) F_(F_BOOL, name)
#define FA(name, size) FA_(F_FLOAT, name, size)
#define F(name) FA(name, 1)
#define L(name) F_(F_LSTRING, name)
#define G(name) F_(F_GSTRING, name)
#define V(name) F_(F_VECTOR, name)
#define M(name) F_(F_ITEM, name)
#define E(name) F_(F_EDICT, name)
#define T(name) F_(F_INT64, name)
#define P(name, type) FA_(F_POINTER, name, type)

static const save_field_t moveinfo_fields[] = {
#define _OFS(x) q_offsetof(moveinfo_t, x)
    V(start_origin),
    V(start_angles),
    V(end_origin),
    V(end_angles),
    V(end_angles_reversed),

    I(sound_start),
    I(sound_middle),
    I(sound_end),

    F(accel),
    F(speed),
    F(decel),
    F(distance),

    F(wait),

    I(state),
    O(reversing),
    V(dir),
    V(dest),
    F(current_speed),
    F(move_speed),
    F(next_speed),
    F(remaining_distance),
    F(decel_distance),
    P(endfunc, P_moveinfo_endfunc),
    P(blocked, P_moveinfo_blocked),
#undef _OFS
};

static const save_field_t reinforcement_fields[] = {
#define _OFS(x) q_offsetof(reinforcement_t, x)
    L(classname),
    I(strength),
    F(radius),
    V(mins),
    V(maxs),
#undef _OFS
};

static const save_field_t monsterinfo_fields[] = {
#define _OFS(x) q_offsetof(monsterinfo_t, x)
    P(active_move, P_mmove_t),
    P(next_move, P_mmove_t),
    H64(aiflags),
    I(nextframe),
    F(scale),

    P(stand, P_monsterinfo_stand),
    P(idle, P_monsterinfo_idle),
    P(search, P_monsterinfo_search),
    P(walk, P_monsterinfo_walk),
    P(run, P_monsterinfo_run),
    P(dodge, P_monsterinfo_dodge),
    P(attack, P_monsterinfo_attack),
    P(melee, P_monsterinfo_melee),
    P(sight, P_monsterinfo_sight),
    P(checkattack, P_monsterinfo_checkattack),
    P(setskin, P_monsterinfo_setskin),
    P(physics_change, P_monsterinfo_physchanged),

    T(pausetime),
    T(attack_finished),
    T(fire_wait),

    V(saved_goal),
    T(search_time),
    T(trail_time),
    V(last_sighting),
    I(attack_state),
    O(lefty),
    T(idle_time),
    I(linkcount),

    I(power_armor_type),
    I(power_armor_power),

    I(initial_power_armor_type),
    I(max_power_armor_power),
    I(weapon_sound),
    I(engine_sound),

    P(blocked, P_monsterinfo_blocked),
    T(last_hint_time),
    E(goal_hint),
    I(medicTries),
    E(badMedic1),
    E(badMedic2),
    E(healer),
    P(duck, P_monsterinfo_duck),
    P(unduck, P_monsterinfo_unduck),
    P(sidestep, P_monsterinfo_sidestep),
    F(base_height),
    T(next_duck_time),
    T(duck_wait_time),
    E(last_player_enemy),
    O(blindfire),
    O(can_jump),
    O(had_visibility),
    F(drop_height),
    F(jump_height),
    T(blind_fire_delay),
    V(blind_fire_target),
    I(monster_slots),
    I(monster_used),
    E(commander),
    T(quad_time),
    T(invincible_time),
    T(double_time),

    T(surprise_time),
    I(armor_type),
    I(armor_power),
    O(close_sight_tripped),
    T(melee_debounce_time),
    T(strafe_check_time),
    I(base_health),
    I(health_scaling),
    T(next_move_time),
    T(bad_move_time),
    T(bump_time),
    T(random_change_time),
    T(path_blocked_counter),
    T(path_wait_time),
    I(combat_style),

    E(damage_attacker),
    E(damage_inflictor),
    I(damage_blood),
    I(damage_knockback),
    V(damage_from),
    I(damage_mod),

    F(fly_max_distance),
    F(fly_min_distance),
    F(fly_acceleration),
    F(fly_speed),
    V(fly_ideal_position),
    T(fly_position_time),
    O(fly_buzzard),
    O(fly_above),
    O(fly_pinned),
    O(fly_thrusters),
    T(fly_recovery_time),
    V(fly_recovery_dir),

    T(checkattack_time),
    I(start_frame),
    T(dodge_time),
    I(move_block_counter),
    T(move_block_change_time),
    T(react_to_damage_time),

    F_(F_REINFORCEMENTS, reinforcements),
    BA(chosen_reinforcements, MAX_REINFORCEMENTS),

    T(jump_time),
#undef _OFS
};

static const save_field_t bmodel_anim_fields[] = {
#define _OFS(x) q_offsetof(bmodel_anim_t, x)
    I(params[0].start),
    I(params[0].end),
    I(params[0].style),
    I(params[0].speed),
    O(params[0].nowrap),
    I(params[1].start),
    I(params[1].end),
    I(params[1].style),
    I(params[1].speed),
    O(params[1].nowrap),
    O(enabled),
    O(alternate),
    O(currently_alternate),
    T(next_tick),
#undef _OFS
};

static const save_field_t player_fog_fields[] = {
#define _OFS(x) q_offsetof(player_fog_t, x)
    V(color),
    F(density),
    F(sky_factor),
#undef _OFS
};

static const save_field_t player_heightfog_fields[] = {
#define _OFS(x) q_offsetof(player_heightfog_t, x)
    V(start.color),
    F(start.dist),
    V(end.color),
    F(end.dist),
    F(density),
    F(falloff),
#undef _OFS
};

static const save_field_t entityfields[] = {
#define _OFS FOFS
    V(s.origin),
    V(s.angles),
    V(s.old_origin),
    I(s.modelindex),
    I(s.modelindex2),
    I(s.modelindex3),
    I(s.modelindex4),
    I(s.frame),
    H(s.skinnum),
    H(s.effects),
    H(s.renderfx),
    I(s.sound),

    // [...]

    I(linkcount),
    H(svflags),
    V(mins),
    V(maxs),
    I(solid),
    H(clipmask),
    E(owner),

    H(x.morefx),
    F(x.alpha),
    F(x.scale),
    F(x.loop_volume),
    F(x.loop_attenuation),

    I(spawn_count),
    I(movetype),
    H64(flags),

    L(model),
    T(freetime),

    L(message),
    L(classname),
    H(spawnflags),

    T(timestamp),

    F(angle),
    L(target),
    L(targetname),
    L(killtarget),
    L(team),
    L(pathtarget),
    L(deathtarget),
    L(healthtarget),
    L(itemtarget),
    L(combattarget),
    E(target_ent),

    F(speed),
    F(accel),
    F(decel),
    V(movedir),
    V(pos1),
    V(pos2),
    V(pos3),

    V(velocity),
    V(avelocity),
    I(mass),
    T(air_finished),
    F_(F_GRAVITY, gravity),

    E(goalentity),
    E(movetarget),
    F(yaw_speed),
    F(ideal_yaw),

    T(nextthink),
    P(prethink, P_prethink),
    P(postthink, P_prethink),
    P(think, P_think),
    P(touch, P_touch),
    P(use, P_use),
    P(pain, P_pain),
    P(die, P_die),

    T(touch_debounce_time),
    T(pain_debounce_time),
    T(damage_debounce_time),
    T(fly_sound_debounce_time),
    T(last_move_time),

    I(health),
    I(max_health),
    I(gib_health),
    T(show_hostile),

    T(powerarmor_time),

    L(map),

    I(viewheight),
    O(deadflag),
    O(takedamage),
    I(dmg),
    I(radius_dmg),
    F(dmg_radius),
    I(sounds),
    I(count),

    E(chain),
    E(enemy),
    E(oldenemy),
    E(activator),
    E(groundentity),
    I(groundentity_linkcount),
    E(teamchain),
    E(teammaster),

    E(mynoise),
    E(mynoise2),

    I(noise_index),
    I(noise_index2),
    F(volume),
    F(attenuation),

    F(wait),
    F(delay),
    F(random),

    T(teleport_time),

    H(watertype),
    I(waterlevel),

    V(move_origin),
    V(move_angles),

    I(style),

    M(item),

    F_(F_MOVEINFO, moveinfo),
    F_(F_MONSTERINFO, monsterinfo),

    H(plat2flags),
    V(offset),
    FA_(F_GRAVITY, gravityVector, 3),
    E(bad_area),
    E(hint_chain),
    E(monster_hint_chain),
    E(target_hint_chain),
    I(hint_chain_id),

    SZ(clock_message, CLOCK_MESSAGE_SIZE),

    T(dead_time),
    E(beam),
    E(beam2),
    E(proboscus),
    E(disintegrator),
    T(disintegrator_time),
    H(hackflags),

    F_(F_PLAYER_FOG, fog_off),
    F_(F_PLAYER_FOG, fog),

    F_(F_PLAYER_HEIGHTFOG, heightfog_off),
    F_(F_PLAYER_HEIGHTFOG, heightfog),

    BA(item_picked_up_by, MAX_CLIENTS / 8),
    T(slime_debounce_time),

    F_(F_BMODEL_ANIM, bmodel_anim),

    L(style_on),
    L(style_off),

    H(crosslevel_flags),
#undef _OFS
};

static const save_field_t levelfields[] = {
#define _OFS(x) q_offsetof(level_locals_t, x)
    T(time),

    SZ(level_name, MAX_QPATH),
    SZ(mapname, MAX_QPATH),
    SZ(nextmap, MAX_QPATH),

    T(intermissiontime),
    L(changemap),
    L(achievement),
    O(exitintermission),
    O(intermission_clear),
    V(intermission_origin),
    V(intermission_angle),

    I(pic_health),
    I(snd_fry),

    I(total_secrets),
    I(found_secrets),

    I(total_goals),
    I(found_goals),

    I(total_monsters),
    I(killed_monsters),

    I(body_que),
    I(power_cubes),

    E(disguise_violator),
    T(disguise_violation_time),
    I(disguise_icon),

    T(coop_level_restart_time),

    L(goals),
    I(goal_num),

    I(vwep_offset),

    L(start_items),
    O(no_grapple),

    F(gravity),
    O(hub_map),
    E(health_bar_entities[0]),
    E(health_bar_entities[1]),
    O(story_active),
    T(next_auto_save),

    F(skyrotate),
    I(skyautorotate),
#undef _OFS
};

static const save_field_t client_persistent_fields[] = {
#define _OFS(x) q_offsetof(client_persistent_t, x)
    SZ(userinfo, MAX_INFO_STRING),
    SZ(netname, 16),
    I(hand),
    I(autoswitch),
    I(autoshield),

    I(health),
    I(max_health),
    H64(savedFlags),

    I(selected_item),
    T(selected_item_time),
    F_(F_INVENTORY, inventory),

    F_(F_MAX_AMMO, max_ammo),

    M(weapon),
    M(lastweapon),

    H(power_cubes),
    I(score),

    I(game_help1changed),
    I(game_help2changed),
    I(helpchanged),
    T(help_time),

    O(spectator),
    O(bob_skip),

    T(megahealth_time),
    I(lives),
#undef _OFS
};

static const save_field_t clientfields[] = {
#define _OFS CLOFS
    I(ps.pmove.pm_type),

    IA(ps.pmove.origin, 3),
    IA(ps.pmove.velocity, 3),
    S(ps.pmove.pm_flags),
    S(ps.pmove.pm_time),
    S(ps.pmove.gravity),
    SA(ps.pmove.delta_angles, 3),

    V(ps.viewangles),
    V(ps.viewoffset),
    V(ps.kick_angles),

    V(ps.gunangles),
    V(ps.gunoffset),
    I(ps.gunindex),
    I(ps.gunframe),

    FA(ps.blend, 4),
    FA(ps.damage_blend, 4),

    F(ps.fov),

    I(ps.rdflags),

    F_(F_STATS, ps.stats),

    F_(F_CLIENT_PERSISTENT, pers),

    F_(F_CLIENT_PERSISTENT, resp.coop_respawn),

    T(resp.entertime),
    I(resp.score),
    O(resp.spectator),

    M(newweapon),

    F(killer_yaw),

    I(weaponstate),

    V(kick.angles),
    V(kick.origin),
    T(kick.total),
    T(kick.time),
    T(quake_time),
    F(v_dmg_roll),
    F(v_dmg_pitch),
    T(v_dmg_time),
    T(fall_time),
    F(fall_value),
    F(damage_alpha),
    F(bonus_alpha),
    V(damage_blend),
    V(v_angle),
    V(v_forward),
    F(bobtime),
    V(oldviewangles),
    V(oldvelocity),
    E(oldgroundentity),
    T(flash_time),

    T(next_drown_time),
    I(old_waterlevel),
    I(breather_sound),

    I(anim_end),
    I(anim_priority),
    O(anim_duck),
    O(anim_run),
    T(anim_time),

    T(quad_time),
    T(invincible_time),
    T(breather_time),
    T(enviro_time),
    T(invisible_time),

    O(grenade_blew_up),
    T(grenade_time),
    T(grenade_finished_time),
    T(quadfire_time),
    I(silencer_shots),
    I(weapon_sound),

    T(pickup_msg_time),

    T(respawn_time),

    T(double_time),
    T(ir_time),
    T(nuke_time),
    T(tracker_pain_time),

    T(empty_click_sound),

    E(trail_head),
    E(trail_tail),

    O(landmark_free_fall),
    G(landmark_name),
    V(landmark_rel_pos),
    T(landmark_noise_time),

    T(invisibility_fade_time),
    V(last_ladder_pos),
    T(last_ladder_sound),

    E(sight_entity),
    T(sight_entity_time),
    E(sound_entity),
    T(sound_entity_time),
    E(sound2_entity),
    T(sound2_entity_time),

    F_(F_PLAYER_FOG, wanted_fog),
    F_(F_PLAYER_HEIGHTFOG, wanted_heightfog),

    T(last_firing_time),
#undef _OFS
};

static const save_field_t level_entry_fields[] = {
#define _OFS(x) q_offsetof(level_entry_t, x)
    SZ(map_name, MAX_QPATH),
    SZ(pretty_name, MAX_QPATH),
    I(total_secrets),
    I(found_secrets),
    I(total_monsters),
    I(killed_monsters),
    T(time),
    I(visit_order),
#undef _OFS
};

static const save_field_t gamefields[] = {
#define _OFS(x) q_offsetof(game_locals_t, x)
    SZ(helpmessage1, MAX_TOKEN_CHARS),
    SZ(helpmessage2, MAX_TOKEN_CHARS),
    I(help1changed),
    I(help2changed),

    I(maxclients),
    I(maxentities),

    H(cross_level_flags),
    H(cross_unit_flags),

    O(autosaved),

    FA_(F_LEVEL_ENTRY, level_entries, MAX_LEVELS_PER_UNIT),
#undef _OFS
};

//=========================================================

static gzFile fp;

//
// writing
//

static const union {
    client_persistent_t client_pers;
    moveinfo_t moveinfo;
    monsterinfo_t monsterinfo;
    bmodel_anim_t bmodel_anim;
    player_fog_t player_fog;
    player_heightfog_t player_heightfog;
    level_entry_t level_entries[MAX_LEVELS_PER_UNIT];
    int inventory[IT_TOTAL];
    int16_t max_ammo[AMMO_MAX];
} empty;

static struct {
    int indent;
} block;

#define indent(s)   (int)(block.indent * 2 + strlen(s)), s

static void begin_block(const char *name)
{
    gzprintf(fp, "%*s {\n", indent(name));
    block.indent++;
}

static void end_block(void)
{
    block.indent--;
    gzprintf(fp, "%*s}\n", indent(""));
}

static void write_tok(const char *name, const char *tok)
{
    gzprintf(fp, "%*s %s\n", indent(name), tok);
}

static void write_int(const char *name, int v)
{
    gzprintf(fp, "%*s %d\n", indent(name), v);
}

static void write_uint_hex(const char *name, unsigned v)
{
    if (v < 256)
        gzprintf(fp, "%*s %u\n", indent(name), v);
    else
        gzprintf(fp, "%*s %#x\n", indent(name), v);
}

static void write_int64(const char *name, int64_t v)
{
    gzprintf(fp, "%*s %"PRId64"\n", indent(name), v);
}

static void write_uint64_hex(const char *name, uint64_t v)
{
    gzprintf(fp, "%*s %#"PRIx64"\n", indent(name), v);
}

static void write_short_v(const char *name, int16_t *v, int n)
{
    gzprintf(fp, "%*s ", indent(name));
    for (int i = 0; i < n; i++)
        gzprintf(fp, "%d ", v[i]);
    gzprintf(fp, "\n");
}

static void write_int_v(const char *name, int *v, int n)
{
    gzprintf(fp, "%*s ", indent(name));
    for (int i = 0; i < n; i++)
        gzprintf(fp, "%d ", v[i]);
    gzprintf(fp, "\n");
}

static void write_float_v(const char *name, float *v, int n)
{
    gzprintf(fp, "%*s ", indent(name));
    for (int i = 0; i < n; i++)
        gzprintf(fp, "%.6g ", v[i]);
    gzprintf(fp, "\n");
}

static void write_string(const char *name, const char *s)
{
    char buffer[MAX_STRING_CHARS * 4];

    COM_EscapeString(buffer, s, sizeof(buffer));
    gzprintf(fp, "%*s \"%s\"\n", indent(name), buffer);
}

static void write_byte_v(const char *name, byte *p, int n)
{
    if (n == 1) {
        write_int(name, *p);
        return;
    }

    gzprintf(fp, "%*s ", indent(name));
    for (int i = 0; i < n; i++)
        gzprintf(fp, "%02x", p[i]);
    gzprintf(fp, "\n");
}

static void write_vector(const char *name, vec_t *v)
{
    gzprintf(fp, "%*s %.6g %.6g %.6g\n", indent(name), v[0], v[1], v[2]);
}

static void write_pointer(const char *name, void *p, ptr_type_t type)
{
    const save_ptr_t *ptr;
    int i;

    for (i = 0, ptr = save_ptrs[type]; i < num_save_ptrs[type]; i++, ptr++) {
        if (ptr->ptr == p) {
            write_tok(name, ptr->name);
            return;
        }
    }

    gi.error("unknown pointer of type %d: %p", type, p);
}

static void write_inventory(int *inven)
{
    begin_block("inventory");
    for (int i = IT_NULL + 1; i < IT_TOTAL; i++) {
        if (inven[i]) {
            Q_assert(itemlist[i].classname);
            write_int(itemlist[i].classname, inven[i]);
        }
    }
    end_block();
}

static void write_max_ammo(int16_t *max_ammo)
{
    begin_block("max_ammo");
    for (int i = AMMO_BULLETS; i < AMMO_MAX; i++)
        if (max_ammo[i])
            write_int(GetItemByAmmo(i)->classname, max_ammo[i]);
    end_block();
}

static const struct {
    const char *name;
    int         stat;
} statdefs[] = {
    { "pickup_icon", STAT_PICKUP_ICON },
    { "pickup_string", STAT_PICKUP_STRING },
    { "selected_item_name", STAT_SELECTED_ITEM_NAME },
};

static void write_stats(int16_t *stats)
{
    int i;

    for (i = 0; i < q_countof(statdefs); i++)
        if (stats[statdefs[i].stat])
            break;
    if (i == q_countof(statdefs))
        return;

    begin_block("ps.stats");
    for (i = 0; i < q_countof(statdefs); i++)
        if (stats[statdefs[i].stat])
            write_int(statdefs[i].name, stats[statdefs[i].stat]);
    end_block();
}

static void write_fields(const char *name, const save_field_t *fields, int count, void *base);

static void write_reinforcements(reinforcement_list_t *list)
{
    if (!list->num_reinforcements)
        return;

    begin_block(va("reinforcements %d", list->num_reinforcements));
    for (int i = 0; i < list->num_reinforcements; i++)
        write_fields(va("%d", i), reinforcement_fields, q_countof(reinforcement_fields), &list->reinforcements[i]);
    end_block();
}

static void write_level_entries(level_entry_t *entries)
{
    if (!memcmp(entries, &empty.level_entries, sizeof(empty.level_entries)))
        return;

    begin_block("level_entries");
    for (int i = 0; i < MAX_LEVELS_PER_UNIT; i++)
        if (memcmp(&entries[i], &empty.level_entries[0], sizeof(empty.level_entries[0])))
            write_fields(va("%d", i), level_entry_fields, q_countof(level_entry_fields), &entries[i]);
    end_block();
}

static const vec3_t default_gravity = { 0, 0, -1 };

static void write_field(const save_field_t *field, void *base)
{
    void *p = (byte *)base + field->ofs;

    switch (field->type) {
    case F_BYTE:
        if (memcmp(p, &empty, field->size))
            write_byte_v(field->name, p, field->size);
        break;
    case F_SHORT:
        if (memcmp(p, &empty, field->size * sizeof(int16_t)))
            write_short_v(field->name, p, field->size);
        break;
    case F_INT:
        if (memcmp(p, &empty, field->size * sizeof(int)))
            write_int_v(field->name, p, field->size);
        break;
    case F_UINT:
        if (*(unsigned *)p)
            write_uint_hex(field->name, *(unsigned *)p);
        break;
    case F_INT64:
        if (*(int64_t *)p)
            write_int64(field->name, *(int64_t *)p);
        break;
    case F_UINT64:
        if (*(uint64_t *)p)
            write_uint64_hex(field->name, *(uint64_t *)p);
        break;
    case F_BOOL:
        if (*(bool *)p)
            gzprintf(fp, "%*s\n", indent(field->name));
        break;
    case F_FLOAT:
        if (memcmp(p, &empty, field->size * sizeof(float)))
            write_float_v(field->name, p, field->size);
        break;
    case F_GRAVITY:
        if ((field->size == 1 && *(float *)p != 1.0f) ||
            (field->size == 3 && !VectorCompare((float *)p, default_gravity)))
            write_float_v(field->name, p, field->size);
        break;
    case F_VECTOR:
        if (memcmp(p, &empty, sizeof(vec3_t)))
            write_vector(field->name, p);
        break;

    case F_ZSTRING:
        if (*(char *)p)
            write_string(field->name, (char *)p);
        break;
    case F_LSTRING:
    case F_GSTRING:
        if (*(char **)p)
            write_string(field->name, *(char **)p);
        break;

    case F_EDICT:
        if (*(edict_t **)p)
            write_int(field->name, *(edict_t **)p - g_edicts);
        break;
    case F_CLIENT:
        if (*(gclient_t **)p)
            write_int(field->name, *(gclient_t **)p - game.clients);
        break;

    case F_ITEM:
        if (*(gitem_t **)p)
            write_tok(field->name, (*(gitem_t **)p)->classname);
        break;
    case F_POINTER:
        if (*(void **)p)
            write_pointer(field->name, *(void **)p, field->size);
        break;

    case F_CLIENT_PERSISTENT:
        if (memcmp(p, &empty.client_pers, sizeof(empty.client_pers)))
            write_fields(field->name, client_persistent_fields, q_countof(client_persistent_fields), p);
        break;

    case F_INVENTORY:
        if (memcmp(p, empty.inventory, sizeof(empty.inventory)))
            write_inventory(p);
        break;

    case F_MAX_AMMO:
        if (memcmp(p, empty.max_ammo, sizeof(empty.max_ammo)))
            write_max_ammo(p);
        break;

    case F_STATS:
        write_stats((int16_t *)p);
        break;

    case F_MOVEINFO:
        if (memcmp(p, &empty.moveinfo, sizeof(empty.moveinfo)))
            write_fields(field->name, moveinfo_fields, q_countof(moveinfo_fields), p);
        break;

    case F_MONSTERINFO:
        if (memcmp(p, &empty.monsterinfo, sizeof(empty.monsterinfo)))
            write_fields(field->name, monsterinfo_fields, q_countof(monsterinfo_fields), p);
        break;

    case F_REINFORCEMENTS:
        write_reinforcements(p);
        break;

    case F_BMODEL_ANIM:
        if (memcmp(p, &empty.bmodel_anim, sizeof(empty.bmodel_anim)))
            write_fields(field->name, bmodel_anim_fields, q_countof(bmodel_anim_fields), p);
        break;

    case F_PLAYER_FOG:
        if (memcmp(p, &empty.player_fog, sizeof(empty.player_fog)))
            write_fields(field->name, player_fog_fields, q_countof(player_fog_fields), p);
        break;

    case F_PLAYER_HEIGHTFOG:
        if (memcmp(p, &empty.player_heightfog, sizeof(empty.player_heightfog)))
            write_fields(field->name, player_heightfog_fields, q_countof(player_heightfog_fields), p);
        break;

    case F_LEVEL_ENTRY:
        write_level_entries(p);
        break;
    }
}

static void write_fields(const char *name, const save_field_t *fields, int count, void *base)
{
    begin_block(name);
    for (int i = 0; i < count; i++)
        write_field(&fields[i], base);
    end_block();
}

//
// reading
//

static struct {
    char data[MAX_STRING_CHARS * 4];
    char token[MAX_STRING_CHARS];
    const char *ptr;
    int len;
    int number;
    int version;
    const char *filename;
} line;

static const char *read_line(void)
{
    line.number++;
    line.ptr = gzgets(fp, line.data, sizeof(line.data));
    if (!line.ptr)
        gi.error("%s: error reading input file", line.filename);
    return line.ptr;
}

q_noreturn q_cold q_printf(1, 2)
static void parse_error(const char *fmt, ...)
{
    va_list     argptr;
    char        text[MAX_STRING_CHARS];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    gi.error("%s: line %d: %s", line.filename, line.number, text);
}

static int unescape_char(int c)
{
    switch (c) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case '\\': return '\\';
        case '"': return '"';
    }
    return 0;
}

static void parse_quoted(const char *s)
{
    int len = 0;

    while (1) {
        int c;

        if (!*s)
            parse_error("unterminated quoted string");

        if (*s == '\"') {
            s++;
            break;
        }

        if (*s == '\\') {
            c = unescape_char(s[1]);
            if (c) {
                s += 2;
            } else {
                int c1, c2;

                if (s[1] != 'x' || (c1 = Q_charhex(s[2])) == -1 || (c2 = Q_charhex(s[3])) == -1)
                    parse_error("bad escape sequence");

                c = (c1 << 4) | c2;
                s += 4;
            }
        } else {
            c = *s++;
        }

        if (len == sizeof(line.token) - 1)
            parse_error("oversize token");

        line.token[len++] = c;
    }

    line.len = len;
    line.ptr = s;
}

static void parse_word(const char *s)
{
    int len = 0;

    do {
        if (len == sizeof(line.token) - 1)
            parse_error("oversize token");
        line.token[len++] = *s++;
    } while (*s > 32);

    line.len = len;
    line.ptr = s;
}

static char *parse(void)
{
    const char *s = line.ptr;

    if (!s)
        s = read_line();
skip:
    while (*s <= 32) {
        if (!*s) {
            s = read_line();
            continue;
        }
        s++;
    }

    if (*s == '/' && s[1] == '/') {
        s = read_line();
        goto skip;
    }

    if (*s == '\"') {
        s++;
        parse_quoted(s);
    } else {
        parse_word(s);
    }

    line.token[line.len] = 0;
    return line.token;
}

static void expect(const char *what)
{
    char *token = parse();

    if (strcmp(token, what))
        parse_error("expected %s, got %s", what, COM_MakePrintable(token));
}

static void unknown(const char *what)
{
    char *token = COM_MakePrintable(line.token);

    if (g_strict_saves->integer)
        parse_error("unknown %s: %s", what, token);

    gi.dprintf("WARNING: %s: line %d: unknown %s: %s\n", line.filename, line.number, what, token);
    line.ptr = NULL;    // skip to next line
}

static int parse_int_tok(const char *tok, int v_min, int v_max)
{
    char *end;
    long v;

    v = strtol(tok, &end, 0);
    if (end == tok || *end)
        parse_error("expected int, got %s", COM_MakePrintable(tok));

    if (v < v_min || v > v_max)
        parse_error("value out of range: %ld", v);

    return v;
}

static int parse_int(int v_min, int v_max)
{
    return parse_int_tok(parse(), v_min, v_max);
}

static int parse_int16(void)
{
    return parse_int(INT16_MIN, INT16_MAX);
}

static int parse_int32(void)
{
    return parse_int(INT32_MIN, INT32_MAX);
}

static unsigned parse_uint_tok(const char *tok, unsigned v_max)
{
    char *end;
    unsigned long v;

    v = strtoul(tok, &end, 0);
    if (end == tok || *end)
        parse_error("expected int, got %s", COM_MakePrintable(tok));

    if (v > v_max)
        parse_error("value out of range: %lu", v);

    return v;
}

static unsigned parse_uint(unsigned v_max)
{
    return parse_uint_tok(parse(), v_max);
}

static int parse_array(int count)
{
    char *tok = parse();
    if (!strcmp(tok, "}"))
        return -1;
    return parse_uint_tok(tok, count - 1);
}

static float parse_float(void)
{
    char *tok, *end;
    float v;

    tok = parse();
    v = strtof(tok, &end);
    if (end == tok || *end)
        parse_error("expected float, got %s", COM_MakePrintable(tok));

    return v;
}

static uint64_t parse_uint64(void)
{
    char *tok, *end;
    uint64_t v;

    tok = parse();
    v = strtoull(tok, &end, 0);
    if (end == tok || *end)
        parse_error("expected int, got %s", COM_MakePrintable(tok));

    return v;
}

static void parse_short_v(int16_t *v, int n)
{
    for (int i = 0; i < n; i++)
        v[i] = parse_int16();
}

static void parse_int_v(int *v, int n)
{
    for (int i = 0; i < n; i++)
        v[i] = parse_int32();
}

static void parse_float_v(float *v, int n)
{
    for (int i = 0; i < n; i++)
        v[i] = parse_float();
}

static void parse_byte_v(byte *v, int n)
{
    if (n == 1) {
        *v = parse_uint(255);
        return;
    }

    parse();
    if (line.len != n * 2)
        parse_error("unexpected number of characters");

    for (int i = 0; i < n; i++) {
        int c1 = Q_charhex(line.token[i * 2 + 0]);
        int c2 = Q_charhex(line.token[i * 2 + 1]);
        if (c1 == -1 || c2 == -1)
            parse_error("not a hex character");
        v[i] = (c1 << 4) | c2;
    }
}

static char *read_string(int tag)
{
    char *s;

    parse();
    s = gi.TagMalloc(line.len + 1, tag);
    memcpy(s, line.token, line.len + 1);

    return s;
}

static void read_zstring(char *s, int size)
{
    if (Q_strlcpy(s, parse(), size) >= size)
        parse_error("oversize string");
}

static void read_vector(vec_t *v)
{
    v[0] = parse_float();
    v[1] = parse_float();
    v[2] = parse_float();
}

static const gitem_t *read_item(void)
{
    const gitem_t *item = FindItemByClassname(parse());

    if (!item)
        unknown("item");

    return item;
}

static int saveptrcmp(const void *p1, const void *p2)
{
    return strcmp(((const save_ptr_t *)p1)->name, ((const save_ptr_t *)p2)->name);
}

static void *read_pointer(ptr_type_t type)
{
    const save_ptr_t *ptr;

    save_ptr_t k = { .name = parse() };
    ptr = bsearch(&k, save_ptrs[type], num_save_ptrs[type], sizeof(save_ptr_t), saveptrcmp);

    if (!ptr) {
        unknown("pointer");
        return NULL;
    }

    return (void *)ptr->ptr;
}

static void read_inventory(int *inven)
{
    expect("{");
    while (1) {
        char *tok = parse();
        if (!strcmp(tok, "}"))
            break;
        const gitem_t *item = FindItemByClassname(tok);
        if (item)
            inven[item->id] = parse_int32();
        else
            unknown("item");
    }
}

static void read_max_ammo(int16_t *max_ammo)
{
    expect("{");
    while (1) {
        char *tok = parse();
        if (!strcmp(tok, "}"))
            break;
        const gitem_t *item = FindItemByClassname(tok);
        if (item && (item->flags & IF_AMMO))
            max_ammo[item->tag] = parse_int16();
        else
            unknown("ammo");
    }
}

static void read_stats(int16_t *stats)
{
    if (line.version == 1) {
        parse_short_v(stats, 32);
        return;
    }

    expect("{");
    while (1) {
        char *tok = parse();
        if (!strcmp(tok, "}"))
            break;
        int i;
        for (i = 0; i < q_countof(statdefs); i++)
            if (!strcmp(tok, statdefs[i].name))
                break;
        if (i < q_countof(statdefs))
            stats[statdefs[i].stat] = parse_int16();
        else
            unknown("stat");
    }
}

static void read_fields(const save_field_t *fields, int count, void *base);

static void read_reinforcements(reinforcement_list_t *list)
{
    int count = parse_int(1, MAX_REINFORCEMENTS_TOTAL);
    int num;

    expect("{");
    list->num_reinforcements = count;
    list->reinforcements = gi.TagMalloc(sizeof(list->reinforcements[0]) * count, TAG_LEVEL);
    while ((num = parse_array(count)) != -1)
        read_fields(reinforcement_fields, q_countof(reinforcement_fields), &list->reinforcements[num]);
}

static void read_level_entries(level_entry_t *entries)
{
    int num;

    expect("{");
    while ((num = parse_array(MAX_LEVELS_PER_UNIT)) != -1)
        read_fields(level_entry_fields, q_countof(level_entry_fields), &entries[num]);
}

static void read_field(const save_field_t *field, void *base)
{
    void *p = (byte *)base + field->ofs;

    switch (field->type) {
    case F_BYTE:
        parse_byte_v(p, field->size);
        break;
    case F_SHORT:
        parse_short_v(p, field->size);
        break;
    case F_INT:
        parse_int_v(p, field->size);
        break;
    case F_UINT:
        *(unsigned *)p = parse_uint(UINT32_MAX);
        break;
    case F_INT64:
    case F_UINT64:
        *(uint64_t *)p = parse_uint64();
        break;
    case F_BOOL:
        *(bool *)p = true;
        break;
    case F_FLOAT:
    case F_GRAVITY:
        parse_float_v(p, field->size);
        break;
    case F_VECTOR:
        read_vector((vec_t *)p);
        break;

    case F_LSTRING:
        *(char **)p = read_string(TAG_LEVEL);
        break;
    case F_GSTRING:
        *(char **)p = read_string(TAG_GAME);
        break;
    case F_ZSTRING:
        read_zstring(p, field->size);
        break;

    case F_EDICT:
        *(edict_t **)p = &g_edicts[parse_uint(game.maxentities - 1)];
        break;
    case F_CLIENT:
        *(gclient_t **)p = &game.clients[parse_uint(game.maxclients - 1)];
        break;

    case F_ITEM:
        *(const gitem_t **)p = read_item();
        break;
    case F_POINTER:
        *(void **)p = read_pointer(field->size);
        break;

    case F_CLIENT_PERSISTENT:
        read_fields(client_persistent_fields, q_countof(client_persistent_fields), p);
        break;

    case F_INVENTORY:
        read_inventory(p);
        break;
    case F_MAX_AMMO:
        read_max_ammo(p);
        break;
    case F_STATS:
        read_stats(p);
        break;

    case F_MOVEINFO:
        read_fields(moveinfo_fields, q_countof(moveinfo_fields), p);
        break;

    case F_MONSTERINFO:
        read_fields(monsterinfo_fields, q_countof(monsterinfo_fields), p);
        break;

    case F_REINFORCEMENTS:
        read_reinforcements(p);
        break;

    case F_BMODEL_ANIM:
        read_fields(bmodel_anim_fields, q_countof(bmodel_anim_fields), p);
        break;

    case F_PLAYER_FOG:
        read_fields(player_fog_fields, q_countof(player_fog_fields), p);
        break;

    case F_PLAYER_HEIGHTFOG:
        read_fields(player_heightfog_fields, q_countof(player_heightfog_fields), p);
        break;

    case F_LEVEL_ENTRY:
        read_level_entries(p);
        break;
    }
}

static void read_fields(const save_field_t *fields, int count, void *base)
{
    const save_field_t *f;
    char *tok;
    int i;

    expect("{");
    while (1) {
        tok = parse();
        if (!strcmp(tok, "}"))
            break;
        for (i = 0, f = fields; i < count; i++, f++)
            if (!strcmp(f->name, tok))
                break;
        if (i == count)
            unknown("field");
        else
            read_field(f, base);
    }
}


//=========================================================

#define SAVE_MAGIC1     "SSV2"
#define SAVE_MAGIC2     "SAV2"
#define SAVE_VERSION    "2"

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void WriteGame(const char *filename, qboolean autosave)
{
    int     i;

    if (!autosave)
        SaveClientData();

    fp = gzopen(filename, "wb");
    if (!fp)
        gi.error("Couldn't open %s", filename);

    memset(&block, 0, sizeof(block));
    gzprintf(fp, SAVE_MAGIC1 " version " SAVE_VERSION "\n");

    game.autosaved = autosave;
    write_fields("game", gamefields, q_countof(gamefields), &game);
    game.autosaved = false;

    begin_block("clients");
    for (i = 0; i < game.maxclients; i++)
        write_fields(va("%d", i), clientfields, q_countof(clientfields), &game.clients[i]);
    end_block();

    i = gzclose(fp);
    fp = NULL;
    if (i)
        gi.error("Couldn't write %s", filename);
}

void ReadGame(const char *filename)
{
    int num;

    gi.FreeTags(TAG_GAME);

    fp = gzopen(filename, "rb");
    if (!fp)
        gi.error("Couldn't open %s", filename);

    gzbuffer(fp, 65536);

    memset(&line, 0, sizeof(line));
    line.filename = COM_SkipPath(filename);
    expect(SAVE_MAGIC1);
    expect("version");
    line.version = parse_int32();
    if (line.version < 1 || line.version > 2)
        gi.error("Savegame has bad version");

    int maxclients = game.maxclients;
    int maxentities = game.maxentities;

    expect("game");
    read_fields(gamefields, q_countof(gamefields), &game);

    // should agree with server's version
    if (game.maxclients != maxclients)
        gi.error("Savegame has bad maxclients");

    if (game.maxentities != maxentities)
        gi.error("Savegame has bad maxentities");

    g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
    globals.edicts = g_edicts;
    globals.max_edicts = game.maxentities;

    game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);

    expect("clients");
    expect("{");
    while ((num = parse_array(game.maxclients)) != -1)
        read_fields(clientfields, q_countof(clientfields), &game.clients[num]);

    gzclose(fp);
    fp = NULL;
}

//==========================================================

/*
=================
WriteLevel

=================
*/
void WriteLevel(const char *filename)
{
    int     i;
    edict_t *ent;

    fp = gzopen(filename, "wb");
    if (!fp)
        gi.error("Couldn't open %s", filename);

    memset(&block, 0, sizeof(block));
    gzprintf(fp, SAVE_MAGIC2 " version " SAVE_VERSION "\n");

    // write out level_locals_t
    write_fields("level", levelfields, q_countof(levelfields), &level);

    // write out all the entities
    begin_block("entities");
    for (i = 0; i < globals.num_edicts; i++) {
        ent = &g_edicts[i];
        if (!ent->inuse)
            continue;
        write_fields(va("%d", i), entityfields, q_countof(entityfields), ent);
    }
    end_block();

    i = gzclose(fp);
    fp = NULL;
    if (i)
        gi.error("Couldn't write %s", filename);
}

/*
=================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel(const char *filename)
{
    int     entnum;
    int     i;
    edict_t *ent;

    // free any dynamic memory allocated by loading the level
    // base state
    gi.FreeTags(TAG_LEVEL);

    fp = gzopen(filename, "rb");
    if (!fp)
        gi.error("Couldn't open %s", filename);

    gzbuffer(fp, 65536);

    memset(&line, 0, sizeof(line));
    line.filename = COM_SkipPath(filename);
    expect(SAVE_MAGIC2);
    expect("version");
    line.version = parse_int32();
    if (line.version < 1 || line.version > 2)
        gi.error("Savegame has bad version");

    // wipe all the entities
    memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
    globals.num_edicts = game.maxclients + 1;

    // load the level locals
    expect("level");
    read_fields(levelfields, q_countof(levelfields), &level);

    // load all the entities
    expect("entities");
    expect("{");
    while ((entnum = parse_array(game.maxentities)) != -1) {
        if (entnum >= globals.num_edicts)
            globals.num_edicts = entnum + 1;

        ent = &g_edicts[entnum];
        if (ent->inuse)
            parse_error("duplicate entity: %d", entnum);
        G_InitEdict(ent);
        read_fields(entityfields, q_countof(entityfields), ent);

        // let the server rebuild world links for this ent
        gi.linkentity(ent);
    }

    gzclose(fp);
    fp = NULL;

    // mark all clients as unconnected
    for (i = 0; i < game.maxclients; i++) {
        ent = &g_edicts[i + 1];
        ent->client = game.clients + i;
        ent->client->pers.connected = false;
        ent->client->pers.spawned = false;
    }

    // do any load time things at this point
    for (i = 0; i < globals.num_edicts; i++) {
        ent = &g_edicts[i];

        if (!ent->inuse)
            continue;

        Q_assert(ent->classname);

        // fire any cross-level triggers
        if (strcmp(ent->classname, "target_crosslevel_target") == 0 ||
            strcmp(ent->classname, "target_crossunit_target") == 0)
            ent->nextthink = level.time + SEC(ent->delay);
    }

    // precache player inventory items
    G_PrecacheInventoryItems();

    // refresh global precache indices
    G_RefreshPrecaches();
}

// clean up if error was thrown mid-save
void G_CleanupSaves(void)
{
    if (fp) {
        gzclose(fp);
        fp = NULL;
    }
}

// [Paril-KEX]
qboolean G_CanSave(void)
{
    if (game.maxclients == 1 && g_edicts[1].health <= 0) {
        gi.cprintf(&g_edicts[1], PRINT_HIGH, "Can't savegame while dead!\n");
        return qfalse;
    }

    // don't allow saving during cameras/intermissions as this
    // causes the game to act weird when these are loaded
    if (level.intermissiontime) {
        gi.cprintf(&g_edicts[1], PRINT_HIGH, "Can't savegame during intermission!\n");
        return qfalse;
    }

    return qtrue;
}
