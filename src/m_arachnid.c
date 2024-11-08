// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TANK

==============================================================================
*/

#include "g_local.h"
#include "m_arachnid.h"

static int sound_step;
static int sound_charge;
static int sound_melee;
static int sound_melee_hit;
static int sound_pain;
static int sound_death;
static int sound_sight;
static int sound_spawn;
static int sound_pissed;

void MONSTERINFO_SIGHT(arachnid_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

//
// stand
//

static const mframe_t arachnid_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(arachnid_move_stand) = { FRAME_idle1, FRAME_idle13, arachnid_frames_stand, NULL };

void MONSTERINFO_STAND(arachnid_stand)(edict_t *self)
{
    M_SetAnimation(self, &arachnid_move_stand);
}

//
// walk
//

static void arachnid_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 0.5f, ATTN_IDLE, 0.0f);
}

static const mframe_t arachnid_frames_walk[] = {
    { ai_walk, 2, arachnid_footstep },
    { ai_walk, 5 },
    { ai_walk, 12 },
    { ai_walk, 16 },
    { ai_walk, 5 },
    { ai_walk, 8, arachnid_footstep },
    { ai_walk, 8 },
    { ai_walk, 12 },
    { ai_walk, 9 },
    { ai_walk, 5 }
};
const mmove_t MMOVE_T(arachnid_move_walk) = { FRAME_walk1, FRAME_walk10, arachnid_frames_walk, NULL };

void MONSTERINFO_WALK(arachnid_walk)(edict_t *self)
{
    M_SetAnimation(self, &arachnid_move_walk);
}

//
// run
//

static const mframe_t arachnid_frames_run[] = {
    { ai_run, 2, arachnid_footstep },
    { ai_run, 5 },
    { ai_run, 12 },
    { ai_run, 16 },
    { ai_run, 5 },
    { ai_run, 8, arachnid_footstep },
    { ai_run, 8 },
    { ai_run, 12 },
    { ai_run, 9 },
    { ai_run, 5 }
};
const mmove_t MMOVE_T(arachnid_move_run) = { FRAME_walk1, FRAME_walk10, arachnid_frames_run, NULL };

void MONSTERINFO_RUN(arachnid_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        M_SetAnimation(self, &arachnid_move_stand);
        return;
    }

    M_SetAnimation(self, &arachnid_move_run);
}

//
// pain
//

static const mframe_t arachnid_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_pain1) = { FRAME_pain11, FRAME_pain15, arachnid_frames_pain1, arachnid_run };

static const mframe_t arachnid_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_pain2) = { FRAME_pain21, FRAME_pain26, arachnid_frames_pain2, arachnid_run };

void PAIN(arachnid_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);
    gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

    if (brandom())
        M_SetAnimation(self, &arachnid_move_pain1);
    else
        M_SetAnimation(self, &arachnid_move_pain2);
}

static void arachnid_charge_rail(edict_t *self, monster_muzzleflash_id_t mz)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);

    vec3_t forward, right, start;
    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[mz], forward, right, start);

    PredictAim(self, self->enemy, start, 0, false, 0.0f, NULL, self->pos1);
}

static void arachnid_charge_rail_left(edict_t *self)
{
    arachnid_charge_rail(self, MZ2_ARACHNID_RAIL1);
}

static void arachnid_charge_rail_right(edict_t *self)
{
    arachnid_charge_rail(self, MZ2_ARACHNID_RAIL2);
}

static void arachnid_charge_rail_up_left(edict_t *self)
{
    arachnid_charge_rail(self, MZ2_ARACHNID_RAIL_UP1);
}

static void arachnid_charge_rail_up_right(edict_t *self)
{
    arachnid_charge_rail(self, MZ2_ARACHNID_RAIL_UP2);
}

static void arachnid_rail_real(edict_t *self, monster_muzzleflash_id_t id)
{
    vec3_t start;
    vec3_t dir;
    vec3_t forward, right, up;

    AngleVectors(self->s.angles, forward, right, up);
    M_ProjectFlashSource(self, monster_flash_offset[id], forward, right, start);
    int dmg = 50;

    if (self->s.frame >= FRAME_melee_in1 && self->s.frame <= FRAME_melee_in16) {
        // scan our current direction for players
        edict_t *players_scanned[MAX_CLIENTS];
        int num_players = 0;

        for (int i = 0; i < game.maxclients; i++) {
            edict_t *player = &g_edicts[i + 1];

            if (!player->inuse)
                continue;
            if (!visible_ex(self, player, false))
                continue;

            if (infront_cone(self, player, 0.5f))
                players_scanned[num_players++] = player;
        }

        if (num_players) {
            edict_t *chosen = players_scanned[irandom1(num_players)];

            PredictAim(self, chosen, start, 0, false, 0.0f, NULL, self->pos1);

            VectorSubtract(chosen->s.origin, self->s.origin, dir);

            self->ideal_yaw = vectoyaw(dir);
            self->s.angles[YAW] = self->ideal_yaw;

            VectorSubtract(self->pos1, start, dir);
            VectorNormalize(dir);

            for (int i = 0; i < 3; i++)
                dir[i] += crandom_open() * 0.018f;
            VectorNormalize(dir);
        } else {
            VectorCopy(forward, dir);
        }
    } else {
        // calc direction to where we targeted
        VectorSubtract(self->pos1, start, dir);
        VectorNormalize(dir);
        dmg = 50;
    }

    bool hit = monster_fire_railgun(self, start, dir, dmg, dmg * 2.0f, id);

    if (dmg == 50) {
        if (hit)
            self->count = 0;
        else
            self->count++;
    }
}

static void arachnid_rail(edict_t *self)
{
    monster_muzzleflash_id_t id;

    switch (self->s.frame) {
    case FRAME_rails4:
    default:
        id = MZ2_ARACHNID_RAIL1;
        break;
    case FRAME_rails8:
        id = MZ2_ARACHNID_RAIL2;
        break;
    case FRAME_rails_up7:
        id = MZ2_ARACHNID_RAIL_UP1;
        break;
    case FRAME_rails_up11:
        id = MZ2_ARACHNID_RAIL_UP2;
        break;
    }

    arachnid_rail_real(self, id);
}

static const mframe_t arachnid_frames_attack1[] = {
    { ai_charge },
    { ai_charge, 0, arachnid_charge_rail_left },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge, 0, arachnid_charge_rail_right },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_attack1) = { FRAME_rails1, FRAME_rails11, arachnid_frames_attack1, arachnid_run };

static const mframe_t arachnid_frames_attack_up1[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_charge_rail_up_left },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge, 0, arachnid_charge_rail_up_right },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
};
const mmove_t MMOVE_T(arachnid_attack_up1) = { FRAME_rails_up1, FRAME_rails_up16, arachnid_frames_attack_up1, arachnid_run };

static void arachnid_melee_charge(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_melee, 1.f, ATTN_NORM, 0.f);
}

static void arachnid_melee_hit(edict_t *self)
{
    if (!fire_hit(self, (vec3_t) { MELEE_DISTANCE, 0, 0 }, 15, 50)) {
        self->monsterinfo.melee_debounce_time = level.time + SEC(1);
        self->count++;
    } else if (self->s.frame == FRAME_melee_atk11 &&
               self->monsterinfo.melee_debounce_time < level.time)
        self->monsterinfo.nextframe = FRAME_melee_atk2;
}

static const mframe_t arachnid_frames_melee_out[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_melee_out) = { FRAME_melee_out1, FRAME_melee_out3, arachnid_frames_melee_out, arachnid_run };

static void arachnid_to_out_melee(edict_t *self)
{
    M_SetAnimation(self, &arachnid_melee_out);
}

static const mframe_t arachnid_frames_melee[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_hit },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_hit },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_melee) = { FRAME_melee_atk1, FRAME_melee_atk12, arachnid_frames_melee, arachnid_to_out_melee };

static void arachnid_to_melee(edict_t *self)
{
    M_SetAnimation(self, &arachnid_melee);
}

static const mframe_t arachnid_frames_melee_in[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge }
};

const mmove_t MMOVE_T(arachnid_melee_in) = { FRAME_melee_in1, FRAME_melee_in3, arachnid_frames_melee_in, arachnid_to_melee };

#if 0
static void arachnid_stop_rails(edict_t *self)
{
    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
    arachnid_run(self);
}

static void arachnid_rail_left(edict_t *self)
{
    arachnid_rail_real(self, MZ2_ARACHNID_RAIL1);
}

static void arachnid_rail_right(edict_t *self)
{
    arachnid_rail_real(self, MZ2_ARACHNID_RAIL2);
}
#endif

static void arachnid_rail_rapid(edict_t *self)
{
    bool left_shot = self->s.frame == FRAME_melee_in9; //((self->s.frame - FRAME_melee_in5) / 2) % 2;

    arachnid_rail_real(self, left_shot ? MZ2_ARACHNID_RAIL1 : MZ2_ARACHNID_RAIL2);
}

static const mframe_t arachnid_frames_attack3[] = {
    { ai_charge },
    { ai_move, 0, arachnid_rail_rapid },
    { ai_move },
    { ai_move/*, 0, arachnid_rail_rapid*/ },
    { ai_move },
    { ai_move, 0, arachnid_rail_rapid },
    { ai_move },
    { ai_move/*, 0, arachnid_rail_rapid*/ },
    { ai_move },
    { ai_move, 0, arachnid_rail_rapid },
    { ai_move },
    { ai_move/*, 0, arachnid_rail_rapid*/ },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_attack3) = { FRAME_melee_in4, FRAME_melee_in16, arachnid_frames_attack3, arachnid_to_out_melee };

static void arachnid_rapid_fire(edict_t *self)
{
    self->count = 0;
    M_SetAnimation(self, &arachnid_attack3);
}

static void arachnid_spawn(edict_t *self)
{
    if (skill->integer < 3)
        return;

    static const vec3_t reinforcement_position[] = { { -24, 124, 0 }, { -24, -124, 0 } };
    vec3_t f, r, offset, startpoint, spawnpoint;
    int    count;

    AngleVectors(self->s.angles, f, r, NULL);

    int num_summoned = M_PickReinforcements(self, 2);

    float scale = self->x.scale;
    if (!scale)
        scale = 1;

    for (count = 0; count < num_summoned; count++) {
        VectorScale(reinforcement_position[count], scale, offset);

        M_ProjectFlashSource(self, offset, f, r, startpoint);

        // a little off the ground
        startpoint[2] += 10 * scale;

        const reinforcement_t *reinforcement = &self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[count]];

        if (!FindSpawnPoint(startpoint, reinforcement->mins, reinforcement->maxs, spawnpoint, 32, true))
            continue;
        if (!CheckGroundSpawnPoint(spawnpoint, reinforcement->mins, reinforcement->maxs, 256, -1))
            continue;

        edict_t *ent = CreateGroundMonster(spawnpoint, self->s.angles, reinforcement->mins, reinforcement->maxs, reinforcement->classname, 256);
        if (!ent)
            return;

        if (ent->think) {
            ent->nextthink = level.time;
            ent->think(ent);
        }

        ent->monsterinfo.aiflags |= AI_SPAWNED_COMMANDER | AI_DO_NOT_COUNT | AI_IGNORE_SHOTS;
        ent->monsterinfo.commander = self;
        ent->monsterinfo.slots_from_commander = reinforcement->strength;
        self->monsterinfo.monster_used += reinforcement->strength;

        gi.sound(ent, CHAN_BODY, sound_spawn, 1, ATTN_NONE, 0);

        if ((self->enemy->inuse) && (self->enemy->health > 0)) {
            ent->enemy = self->enemy;
            FoundTarget(ent);
        }

        vec3_t mid;
        VectorAvg(reinforcement->mins, reinforcement->maxs, mid);
        VectorAdd(spawnpoint, mid, spawnpoint);
        SpawnGrow_Spawn(spawnpoint, reinforcement->radius, reinforcement->radius * 2);
    }
}

static const mframe_t arachnid_frames_taunt[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_spawn },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};

const mmove_t MMOVE_T(arachnid_taunt) = { FRAME_melee_pain1, FRAME_melee_pain16, arachnid_frames_taunt, arachnid_rapid_fire };

void MONSTERINFO_ATTACK(arachnid_attack)(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->monsterinfo.melee_debounce_time < level.time && range_to(self, self->enemy) < MELEE_DISTANCE)
        M_SetAnimation(self, &arachnid_melee_in);
    // annoyed rapid fire attack
    else if (self->enemy->client &&
             self->last_move_time <= level.time &&
             self->count >= 4 &&
             frandom() < (max(self->count / 2.0f, 4.0f) + 1.0f) * 0.2f &&
             (M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL1]) || M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL2])))
    {
        M_SetAnimation(self, &arachnid_taunt);
        gi.sound(self, CHAN_VOICE, sound_pissed, 1, 0.25f, 0);
        self->count = 0;
        self->pain_debounce_time = level.time + SEC(4.5f);
        self->last_move_time = level.time + SEC(10);
    } else if ((self->enemy->s.origin[2] - self->s.origin[2]) > 150 &&
               (M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL_UP1]) || M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL_UP2])))
        M_SetAnimation(self, &arachnid_attack_up1);
    else if (M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL1]) || M_CheckClearShot(self, monster_flash_offset[MZ2_ARACHNID_RAIL2]))
        M_SetAnimation(self, &arachnid_attack1);
}

//
// death
//

static void arachnid_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}

static const mframe_t arachnid_frames_death1[] = {
    { ai_move, 0 },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.64f },
    { ai_move, -1.64f },
    { ai_move, -2.45f },
    { ai_move, -8.63f },
    { ai_move, -4.0f },
    { ai_move, -4.5f },
    { ai_move, -6.8f },
    { ai_move, -8.0f },
    { ai_move, -5.4f },
    { ai_move, -3.4f },
    { ai_move, -1.9f },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_death) = { FRAME_death1, FRAME_death20, arachnid_frames_death1, arachnid_dead };

static const gib_def_t arachnid_gibs_psx[] = {
    { "models/monsters/arachnid/gibs/chest.md2" },
    { "models/monsters/arachnid/gibs/stomach.md2" },
    { "models/monsters/arachnid/gibs/leg.md2", 3, GIB_UPRIGHT, 0 },
    { "models/monsters/arachnid/gibs/leg.md2", 3, GIB_UPRIGHT, 1 },
    { "models/monsters/arachnid/gibs/l_rail.md2", 1, GIB_UPRIGHT | GIB_RANDFRAME, 1 },
    { "models/monsters/arachnid/gibs/r_rail.md2", 1, GIB_UPRIGHT | GIB_RANDFRAME, 1 },
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 3 },
    { "models/objects/gibs/gear/tris.md2", 2, GIB_METALLIC },
    { "models/monsters/arachnid/gibs/head.md2", 1, GIB_HEAD },
    { 0 }
};

static const gib_def_t arachnid_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(arachnid_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        ThrowGibs(self, damage, use_psx_assets ? arachnid_gibs_psx : arachnid_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

    M_SetAnimation(self, &arachnid_move_death);
}

void MONSTERINFO_SETSKIN(arachnid_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

//
// monster_arachnid
//

#define DEFAULT_REINFORCEMENTS      "monster_stalker 1"
#define DEFAULT_MONSTER_SLOTS_BASE  2

static void arachnid_precache(void)
{
    sound_step      = gi.soundindex("insane/insane11.wav");
    sound_charge    = gi.soundindex("gladiator/railgun.wav");
    sound_melee     = gi.soundindex("gladiator/melee3.wav");
    sound_melee_hit = gi.soundindex("gladiator/melee2.wav");
    sound_pain      = gi.soundindex("arachnid/pain.wav");
    sound_death     = gi.soundindex("arachnid/death.wav");
    sound_sight     = gi.soundindex("arachnid/sight.wav");
    sound_pissed    = gi.soundindex("arachnid/angry.wav");

    if (skill->integer >= 3)
        sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");
}

/*QUAKED monster_arachnid (1 .5 0) (-40 -40 -20) (40 40 48) Ambush Trigger_Spawn Sight
 */
void SP_monster_arachnid(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(arachnid_precache);

    PrecacheGibs(use_psx_assets ? arachnid_gibs_psx : arachnid_gibs);

    if (skill->integer >= 3) {
        const char *reinforcements = DEFAULT_REINFORCEMENTS;

        if (!ED_WasKeySpecified("monster_slots"))
            self->monsterinfo.monster_slots = DEFAULT_MONSTER_SLOTS_BASE;
        if (ED_WasKeySpecified("reinforcements"))
            reinforcements = st.reinforcements;

        if (self->monsterinfo.monster_slots && reinforcements && *reinforcements)
            M_SetupReinforcements(reinforcements, &self->monsterinfo.reinforcements);
    }

    self->s.modelindex = gi.modelindex("models/monsters/arachnid/tris.md2");
    VectorSet(self->mins, -40, -40, -20);
    VectorSet(self->maxs, 40, 40, 48);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 1000 * st.health_multiplier;
    self->gib_health = -200;

    self->monsterinfo.scale = MODEL_SCALE;

    self->mass = 450;

    self->pain = arachnid_pain;
    self->die = arachnid_die;
    self->monsterinfo.stand = arachnid_stand;
    self->monsterinfo.walk = arachnid_walk;
    self->monsterinfo.run = arachnid_run;
    self->monsterinfo.attack = arachnid_attack;
    self->monsterinfo.sight = arachnid_sight;
    self->monsterinfo.setskin = arachnid_setskin;

    self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

    gi.linkentity(self);

    M_SetAnimation(self, &arachnid_move_stand);

    walkmonster_start(self);
}
