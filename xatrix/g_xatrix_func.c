// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

/*QUAKED rotating_light (0 .5 .8) (-8 -8 -8) (8 8 8) START_OFF ALARM
"health"    if set, the light may be killed.
*/

// RAFAEL
// note to self
// the lights will take damage from explosions
// this could leave a player in total darkness very bad

#define SPAWNFLAG_ROTATING_LIGHT_START_OFF  1
#define SPAWNFLAG_ROTATING_LIGHT_ALARM      2

void THINK(rotating_light_alarm)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_ROTATING_LIGHT_START_OFF) {
        self->think = NULL;
        self->nextthink = 0;
    } else {
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
        self->nextthink = level.time + SEC(1);
    }
}

void DIE(rotating_light_killed)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_WELDING_SPARKS);
    gi.WriteByte(30);
    gi.WritePosition(self->s.origin);
    gi.WriteDir(vec3_origin);
    gi.WriteByte(irandom2(0xe0, 0xe8));
    gi.multicast(self->s.origin, MULTICAST_PVS);

    self->s.effects &= ~EF_SPINNINGLIGHTS;
    self->use = NULL;

    self->think = G_FreeEdict;
    self->nextthink = level.time + FRAME_TIME;
}

void USE(rotating_light_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->spawnflags & SPAWNFLAG_ROTATING_LIGHT_START_OFF) {
        self->spawnflags &= ~SPAWNFLAG_ROTATING_LIGHT_START_OFF;
        self->s.effects |= EF_SPINNINGLIGHTS;

        if (self->spawnflags & SPAWNFLAG_ROTATING_LIGHT_ALARM) {
            self->think = rotating_light_alarm;
            self->nextthink = level.time + FRAME_TIME;
        }
    } else {
        self->spawnflags |= SPAWNFLAG_ROTATING_LIGHT_START_OFF;
        self->s.effects &= ~EF_SPINNINGLIGHTS;
    }
}

void SP_rotating_light(edict_t *self)
{
    self->movetype = MOVETYPE_STOP;
    self->solid = SOLID_BBOX;

    self->s.modelindex = gi.modelindex("models/objects/light/tris.md2");

    self->s.frame = 0;

    self->use = rotating_light_use;

    if (self->spawnflags & SPAWNFLAG_ROTATING_LIGHT_START_OFF)
        self->s.effects &= ~EF_SPINNINGLIGHTS;
    else
        self->s.effects |= EF_SPINNINGLIGHTS;

    if (!self->speed)
        self->speed = 32;
    // this is a real cheap way
    // to set the radius of the light
    // self->s.frame = self->speed;

    if (!self->health) {
        self->health = 10;
        self->max_health = self->health;
        self->die = rotating_light_killed;
        self->takedamage = true;
    } else {
        self->max_health = self->health;
        self->die = rotating_light_killed;
        self->takedamage = true;
    }

    if (self->spawnflags & SPAWNFLAG_ROTATING_LIGHT_ALARM)
        self->moveinfo.sound_start = gi.soundindex("misc/alarm.wav");

    gi.linkentity(self);
}

/*QUAKED func_object_repair (1 .5 0) (-8 -8 -8) (8 8 8)
object to be repaired.
The default delay is 1 second
"delay" the delay in seconds for spark to occur
*/

void THINK(object_repair_fx)(edict_t *ent)
{
    ent->nextthink = level.time + SEC(ent->delay);

    if (ent->health <= 100)
        ent->health++;
    else {
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_WELDING_SPARKS);
        gi.WriteByte(10);
        gi.WritePosition(ent->s.origin);
        gi.WriteDir(vec3_origin);
        gi.WriteByte(irandom2(0xe0, 0xe8));
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
}

void THINK(object_repair_dead)(edict_t *ent)
{
    G_UseTargets(ent, ent);
    ent->nextthink = level.time + HZ(10);
    ent->think = object_repair_fx;
}

void THINK(object_repair_sparks)(edict_t *ent)
{
    if (ent->health <= 0) {
        ent->nextthink = level.time + HZ(10);
        ent->think = object_repair_dead;
        return;
    }

    ent->nextthink = level.time + SEC(ent->delay);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_WELDING_SPARKS);
    gi.WriteByte(10);
    gi.WritePosition(ent->s.origin);
    gi.WriteDir(vec3_origin);
    gi.WriteByte(irandom2(0xe0, 0xe8));
    gi.multicast(ent->s.origin, MULTICAST_PVS);
}

void SP_object_repair(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    ent->classname = "object_repair";
    VectorSet(ent->mins, -8, -8, 8);
    VectorSet(ent->maxs, 8, 8, 8);
    ent->think = object_repair_sparks;
    ent->nextthink = level.time + SEC(1);
    ent->health = 100;
    if (!ent->delay)
        ent->delay = 1.0f;
}
