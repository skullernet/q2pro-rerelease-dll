// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

void monster_fire_blueblaster(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, monster_muzzleflash_id_t flashtype, effects_t effect)
{
    fire_blueblaster(self, start, dir, damage, speed, effect);
    monster_muzzleflash(self, start, flashtype);
}

void monster_fire_ionripper(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, monster_muzzleflash_id_t flashtype, effects_t effect)
{
    fire_ionripper(self, start, dir, damage, speed, effect);
    monster_muzzleflash(self, start, flashtype);
}

void monster_fire_heat(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, monster_muzzleflash_id_t flashtype, float turn_fraction)
{
    fire_heat(self, start, dir, damage, speed, (float) damage, damage, turn_fraction);
    monster_muzzleflash(self, start, flashtype);
}

void dabeam_update(edict_t *self, bool damage)
{
    vec3_t start, end;

    VectorCopy(self->s.origin, start);
    VectorMA(start, 2048, self->movedir, end);

    pierce_t pierce;
    trace_t tr;

    pierce_begin(&pierce);

    do {
        tr = gi.trace(start, NULL, NULL, end, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER);

        // didn't hit anything, so we're done
        if (!tr.ent || tr.fraction == 1.0f)
            break;

        if (damage) {
            // hurt it if we can
            if (self->dmg > 0 && (tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
                T_Damage(tr.ent, self, self->owner, self->movedir, tr.endpos, vec3_origin, self->dmg, skill->integer, DAMAGE_ENERGY, (mod_t) { MOD_TARGET_LASER });

            if (self->dmg < 0) { // healer ray
                // when player is at 100 health
                // just undo health fix
                // keeping fx
                if (tr.ent->health < tr.ent->max_health)
                    tr.ent->health = min(tr.ent->max_health, tr.ent->health - self->dmg);
            }
        }

        // if we hit something that's not a monster or player or is immune to lasers, we're done
        if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client)) {
            if (damage) {
                gi.WriteByte(svc_temp_entity);
                gi.WriteByte(TE_LASER_SPARKS);
                gi.WriteByte(10);
                gi.WritePosition(tr.endpos);
                gi.WriteDir(tr.plane.normal);
                gi.WriteByte(self->s.skinnum);
                gi.multicast(tr.endpos, MULTICAST_PVS);
            }
            break;
        }
    } while (pierce_mark(&pierce, tr.ent));

    pierce_end(&pierce);

    VectorAdd(tr.endpos, tr.plane.normal, self->s.old_origin);
    gi.linkentity(self);
}

#define SPAWNFLAG_DABEAM_SECONDARY  1

void THINK(beam_think)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_DABEAM_SECONDARY)
        self->owner->beam2 = NULL;
    else
        self->owner->beam = NULL;
    G_FreeEdict(self);
}

void monster_fire_dabeam(edict_t *self, int damage, bool secondary, void (*update_func)(edict_t *self))
{
    edict_t *beam = secondary ? self->beam2 : self->beam;

    if (!beam) {
        beam = G_Spawn();

        beam->movetype = MOVETYPE_NONE;
        beam->solid = SOLID_NOT;
        beam->s.renderfx |= RF_BEAM;
        beam->s.modelindex = MODELINDEX_WORLD;
        beam->owner = self;
        beam->dmg = damage;
        beam->s.frame = 2;
        beam->spawnflags = secondary ? SPAWNFLAG_DABEAM_SECONDARY : SPAWNFLAG_NONE;

        if (self->monsterinfo.aiflags & AI_MEDIC)
            beam->s.skinnum = 0xf3f3f1f1;
        else
            beam->s.skinnum = 0xf2f2f0f0;

        beam->think = beam_think;
        beam->s.sound = gi.soundindex("misc/lasfly.wav");
        beam->postthink = update_func;
        if (secondary)
            self->beam2 = beam;
        else
            self->beam = beam;
    }

    beam->nextthink = level.time + SEC(0.2f);
    update_func(beam);
    dabeam_update(beam, true);
}
