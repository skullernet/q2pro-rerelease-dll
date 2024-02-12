// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

// PGM - some of these are mine, some id's. I added the define's.
#define SPAWNFLAG_TRIGGER_MONSTER       1
#define SPAWNFLAG_TRIGGER_NOT_PLAYER    2
#define SPAWNFLAG_TRIGGER_TRIGGERED     4
#define SPAWNFLAG_TRIGGER_TOGGLE        8
#define SPAWNFLAG_TRIGGER_LATCHED       16
#define SPAWNFLAG_TRIGGER_CLIP          32
// PGM

static void InitTrigger(edict_t *self)
{
    if (ED_WasKeySpecified("angle") || ED_WasKeySpecified("angles") || !VectorEmpty(self->s.angles))
        G_SetMovedir(self->s.angles, self->movedir);

    self->solid = SOLID_TRIGGER;
    self->movetype = MOVETYPE_NONE;
    // [Paril-KEX] adjusted to allow mins/maxs to be defined
    // by hand instead
    if (self->model)
        gi.setmodel(self, self->model);
    self->svflags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
void THINK(multi_wait)(edict_t *ent)
{
    ent->nextthink = 0;
}

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
static void multi_trigger(edict_t *ent)
{
    if (ent->nextthink)
        return; // already been triggered

    G_UseTargets(ent, ent->activator);

    if (ent->wait > 0) {
        ent->think = multi_wait;
        ent->nextthink = level.time + SEC(ent->wait);
    } else {
        // we can't just remove (self) here, because this is a touch function
        // called while looping through area links...
        ent->touch = NULL;
        ent->nextthink = level.time + FRAME_TIME;
        ent->think = G_FreeEdict;
    }
}

void USE(Use_Multi)(edict_t *ent, edict_t *other, edict_t *activator)
{
    // PGM
    if (ent->spawnflags & SPAWNFLAG_TRIGGER_TOGGLE) {
        if (ent->solid == SOLID_TRIGGER)
            ent->solid = SOLID_NOT;
        else
            ent->solid = SOLID_TRIGGER;
        gi.linkentity(ent);
    } else {
        ent->activator = activator;
        multi_trigger(ent);
    }
    // PGM
}

void TOUCH(Touch_Multi)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other->client) {
        if (self->spawnflags & SPAWNFLAG_TRIGGER_NOT_PLAYER)
            return;
    } else if (other->svflags & SVF_MONSTER) {
        if (!(self->spawnflags & SPAWNFLAG_TRIGGER_MONSTER))
            return;
    } else
        return;

    if (self->spawnflags & SPAWNFLAG_TRIGGER_CLIP) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    if (!VectorEmpty(self->movedir)) {
        vec3_t forward;

        AngleVectors(other->s.angles, forward, NULL, NULL);
        if (DotProduct(forward, self->movedir) < 0)
            return;
    }

    self->activator = other;
    multi_trigger(self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED TOGGLE LATCHED
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)

TOGGLE - using this trigger will activate/deactivate it. trigger will begin inactive.

sounds
1)  secret
2)  beep beep
3)  large switch
4)
set "message" to text string
*/
void USE(trigger_enable)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->solid = SOLID_TRIGGER;
    self->use = Use_Multi;
    gi.linkentity(self);
}

void THINK(latched_trigger_think)(edict_t *self)
{
    self->nextthink = level.time + FRAME_TIME;

    edict_t *list[MAX_EDICTS_OLD];
    int count = gi.BoxEdicts(self->absmin, self->absmax, list, q_countof(list), AREA_SOLID);
    bool any_inside = false;

    for (int i = 0; i < count; i++) {
        edict_t *other = list[i];

        if (other->client) {
            if (self->spawnflags & SPAWNFLAG_TRIGGER_NOT_PLAYER)
                continue;
        } else if (other->svflags & SVF_MONSTER) {
            if (!(self->spawnflags & SPAWNFLAG_TRIGGER_MONSTER))
                continue;
        } else
            continue;

        if (!VectorEmpty(self->movedir)) {
            vec3_t forward;

            AngleVectors(other->s.angles, forward, NULL, NULL);
            if (DotProduct(forward, self->movedir) < 0)
                continue;
        }

        self->activator = other;
        any_inside = true;
        break;
    }

    if (!!self->count != any_inside) {
        G_UseTargets(self, self->activator);
        self->count = any_inside;
    }
}

void SP_trigger_multiple(edict_t *ent)
{
    if (ent->sounds == 1)
        ent->noise_index = gi.soundindex("misc/secret.wav");
    else if (ent->sounds == 2)
        ent->noise_index = gi.soundindex("misc/talk.wav");
    else if (ent->sounds == 3)
        ent->noise_index = gi.soundindex("misc/trigger1.wav");

    if (!ent->wait)
        ent->wait = 0.2f;

    InitTrigger(ent);

    if (ent->spawnflags & SPAWNFLAG_TRIGGER_LATCHED) {
        if (ent->spawnflags & (SPAWNFLAG_TRIGGER_TRIGGERED | SPAWNFLAG_TRIGGER_TOGGLE))
            gi.dprintf("%s: latched and triggered/toggle are not supported\n", etos(ent));

        ent->think = latched_trigger_think;
        ent->nextthink = level.time + FRAME_TIME;
        ent->use = Use_Multi;
        return;
    }

    ent->touch = Touch_Multi;

    // PGM
    if (ent->spawnflags & (SPAWNFLAG_TRIGGER_TRIGGERED | SPAWNFLAG_TRIGGER_TOGGLE)) {
    // PGM
        ent->solid = SOLID_NOT;
        ent->use = trigger_enable;
    } else {
        ent->solid = SOLID_TRIGGER;
        ent->use = Use_Multi;
    }

    gi.linkentity(ent);

    if (ent->spawnflags & SPAWNFLAG_TRIGGER_CLIP)
        ent->svflags |= SVF_HULL;
}

/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.

sounds
 1) secret
 2) beep beep
 3) large switch
 4)

"message"   string to be displayed when triggered
*/

void SP_trigger_once(edict_t *ent)
{
    // make old maps work because I messed up on flag assignments here
    // triggered was on bit 1 when it should have been on bit 4
    if (ent->spawnflags & SPAWNFLAG_TRIGGER_MONSTER) {
        ent->spawnflags &= ~SPAWNFLAG_TRIGGER_MONSTER;
        ent->spawnflags |= SPAWNFLAG_TRIGGER_TRIGGERED;
        gi.dprintf("%s: fixed TRIGGERED flag\n", etos(ent));
    }

    ent->wait = -1;
    SP_trigger_multiple(ent);
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
#define SPAWNFLAGS_TRIGGER_RELAY_NO_SOUND   1

void USE(trigger_relay_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->crosslevel_flags && self->crosslevel_flags != (game.cross_level_flags & SFL_CROSS_TRIGGER_MASK & self->crosslevel_flags))
        return;

    G_UseTargets(self, activator);
}

void SP_trigger_relay(edict_t *self)
{
    self->use = trigger_relay_use;

    if (self->spawnflags & SPAWNFLAGS_TRIGGER_RELAY_NO_SOUND)
        self->noise_index = -1;
}

/*
==============================================================================

trigger_key

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires it's targets if player has the proper key.
Use "item" to specify the required key, for example "key_data_cd"
*/
void USE(trigger_key_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    item_id_t index;

    if (!self->item)
        return;
    if (!activator->client)
        return;

    index = self->item->id;
    if (!activator->client->pers.inventory[index]) {
        if (level.time < self->touch_debounce_time)
            return;
        self->touch_debounce_time = level.time + SEC(5);
        gi.centerprintf(activator, "You need %s", self->item->pickup_name_definite);
        gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"), 1, ATTN_NORM, 0);
        return;
    }

    gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NORM, 0);
    if (coop->integer) {
        edict_t *ent;

        if (index == IT_KEY_POWER_CUBE || index == IT_KEY_EXPLOSIVE_CHARGES) {
            int cube;

            for (cube = 0; cube < 8; cube++)
                if (activator->client->pers.power_cubes & (1 << cube))
                    break;

            for (int player = 1; player <= game.maxclients; player++) {
                ent = &g_edicts[player];
                if (!ent->inuse)
                    continue;
                if (!ent->client)
                    continue;
                if (ent->client->pers.power_cubes & (1 << cube)) {
                    ent->client->pers.inventory[index]--;
                    ent->client->pers.power_cubes &= ~(1 << cube);

                    // [Paril-KEX] don't allow respawning players to keep
                    // used keys
                    if (!P_UseCoopInstancedItems()) {
                        ent->client->resp.coop_respawn.inventory[index] = 0;
                        ent->client->resp.coop_respawn.power_cubes &= ~(1 << cube);
                    }
                }
            }
        } else {
            for (int player = 1; player <= game.maxclients; player++) {
                ent = &g_edicts[player];
                if (!ent->inuse)
                    continue;
                if (!ent->client)
                    continue;
                ent->client->pers.inventory[index] = 0;

                // [Paril-KEX] don't allow respawning players to keep
                // used keys
                if (!P_UseCoopInstancedItems())
                    ent->client->resp.coop_respawn.inventory[index] = 0;
            }
        }
    } else {
        activator->client->pers.inventory[index]--;
    }

    G_UseTargets(self, activator);

    self->use = NULL;
}

void SP_trigger_key(edict_t *self)
{
    if (!st.item) {
        gi.dprintf("%s: no key item\n", etos(self));
        return;
    }
    self->item = FindItemByClassname(st.item);

    if (!self->item) {
        gi.dprintf("%s: item %s not found\n", etos(self), st.item);
        return;
    }

    if (!self->target) {
        gi.dprintf("%s: no target\n", etos(self));
        return;
    }

    gi.soundindex("misc/keytry.wav");
    gi.soundindex("misc/keyuse.wav");

    self->use = trigger_key_use;
}

/*
==============================================================================

trigger_counter

==============================================================================
*/

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/

#define SPAWNFLAG_COUNTER_NOMESSAGE 1

void USE(trigger_counter_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->count == 0)
        return;

    self->count--;

    if (self->count) {
        if (!(self->spawnflags & SPAWNFLAG_COUNTER_NOMESSAGE)) {
            gi.centerprintf(activator, "%d more to go...", self->count);
            gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
        }
        return;
    }

    if (!(self->spawnflags & SPAWNFLAG_COUNTER_NOMESSAGE)) {
        gi.centerprintf(activator, "Sequence completed!");
        gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
    }
    self->activator = activator;
    multi_trigger(self);
}

void SP_trigger_counter(edict_t *self)
{
    self->wait = -1;
    if (!self->count)
        self->count = 2;

    self->use = trigger_counter_use;
}

/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always(edict_t *ent)
{
    // we must have some delay to make sure our use targets are present
    if (!ent->delay)
        ent->delay = 0.2f;
    G_UseTargets(ent, ent);
}

/*
==============================================================================

trigger_push

==============================================================================
*/

// PGM
#define SPAWNFLAG_PUSH_ONCE         1
#define SPAWNFLAG_PUSH_PLUS         2
#define SPAWNFLAG_PUSH_SILENT       4
#define SPAWNFLAG_PUSH_START_OFF    8
#define SPAWNFLAG_PUSH_CLIP         16
// PGM

void TOUCH(trigger_push_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (self->spawnflags & SPAWNFLAG_PUSH_CLIP) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    if (strcmp(other->classname, "grenade") == 0) {
        VectorScale(self->movedir, self->speed * 10, other->velocity);
    } else if (other->health > 0) {
        VectorScale(self->movedir, self->speed * 10, other->velocity);

        if (other->client) {
            // don't take falling damage immediately from this
            VectorCopy(other->velocity, other->client->oldvelocity);
            other->client->oldgroundentity = other->groundentity;
            if (!(self->spawnflags & SPAWNFLAG_PUSH_SILENT) && (other->fly_sound_debounce_time < level.time)) {
                other->fly_sound_debounce_time = level.time + SEC(1.5f);
                gi.sound(other, CHAN_AUTO, gi.soundindex("misc/windfly.wav"), 1, ATTN_NORM, 0);
            }
        }
    }

    SV_CheckVelocity(other);

    if (self->spawnflags & SPAWNFLAG_PUSH_ONCE)
        G_FreeEdict(self);
}

//======
// PGM
void USE(trigger_push_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->solid == SOLID_NOT)
        self->solid = SOLID_TRIGGER;
    else
        self->solid = SOLID_NOT;
    gi.linkentity(self);
}
// PGM
//======

// RAFAEL
void trigger_push_active(edict_t *self);

static void trigger_effect(edict_t *self)
{
    vec3_t origin;
    int    i;

    VectorAvg(self->absmin, self->absmax, origin);

    for (i = 0; i < 10; i++) {
        origin[2] += (self->speed * 0.01f) * (i + frandom());
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_TUNNEL_SPARKS);
        gi.WriteByte(1);
        gi.WritePosition(origin);
        gi.WriteDir(vec3_origin);
        gi.WriteByte(irandom2(0x74, 0x7C));
        gi.multicast(self->s.origin, MULTICAST_PVS);
    }
}

void THINK(trigger_push_inactive)(edict_t *self)
{
    if (self->timestamp > level.time) {
        self->nextthink = level.time + HZ(10);
    } else {
        self->touch = trigger_push_touch;
        self->think = trigger_push_active;
        self->nextthink = level.time + HZ(10);
        self->timestamp = self->nextthink + SEC(self->wait);
    }
}

void THINK(trigger_push_active)(edict_t *self)
{
    if (self->timestamp > level.time) {
        self->nextthink = level.time + HZ(10);
        trigger_effect(self);
    } else {
        self->touch = NULL;
        self->think = trigger_push_inactive;
        self->nextthink = level.time + HZ(10);
        self->timestamp = self->nextthink + SEC(self->wait);
    }
}
// RAFAEL

/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE PUSH_PLUS PUSH_SILENT START_OFF CLIP
Pushes the player
"speed" defaults to 1000
"wait"  defaults to 10, must use PUSH_PLUS

If targeted, it will toggle on and off when used.

START_OFF - toggled trigger_push begins in off setting
SILENT - doesn't make wind noise
*/
void SP_trigger_push(edict_t *self)
{
    InitTrigger(self);
    if (!(self->spawnflags & SPAWNFLAG_PUSH_SILENT))
        gi.soundindex("misc/windfly.wav");
    self->touch = trigger_push_touch;

    // RAFAEL
    if (self->spawnflags & SPAWNFLAG_PUSH_PLUS) {
        if (!self->wait)
            self->wait = 10;

        self->think = trigger_push_active;
        self->nextthink = level.time + HZ(10);
        self->timestamp = self->nextthink + SEC(self->wait);
    }
    // RAFAEL

    if (!self->speed)
        self->speed = 1000;

    // PGM
    if (self->targetname) { // toggleable
        self->use = trigger_push_use;
        if (self->spawnflags & SPAWNFLAG_PUSH_START_OFF)
            self->solid = SOLID_NOT;
    } else if (self->spawnflags & SPAWNFLAG_PUSH_START_OFF) {
        gi.dprintf("trigger_push is START_OFF but not targeted.\n");
        self->svflags = SVF_NONE;
        self->touch = NULL;
        self->solid = SOLID_BSP;
        self->movetype = MOVETYPE_PUSH;
    }
    // PGM

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_PUSH_CLIP)
        self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW NO_PLAYERS NO_MONSTERS
Any entity that touches this will be hurt.

It does dmg points of damage each server frame

SILENT          supresses playing the sound
SLOW            changes the damage rate to once per second
NO_PROTECTION   *nothing* stops the damage

"dmg"           default 5 (whole numbers only)

*/

#define SPAWNFLAG_HURT_START_OFF        1
#define SPAWNFLAG_HURT_TOGGLE           2
#define SPAWNFLAG_HURT_SILENT           4
#define SPAWNFLAG_HURT_NO_PROTECTION    8
#define SPAWNFLAG_HURT_SLOW             16
#define SPAWNFLAG_HURT_NO_PLAYERS       32
#define SPAWNFLAG_HURT_NO_MONSTERS      64
#define SPAWNFLAG_HURT_CLIPPED          128

void USE(hurt_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->solid == SOLID_NOT)
        self->solid = SOLID_TRIGGER;
    else
        self->solid = SOLID_NOT;
    gi.linkentity(self);

    if (!(self->spawnflags & SPAWNFLAG_HURT_TOGGLE))
        self->use = NULL;
}

void TOUCH(hurt_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    damageflags_t dflags;

    if (!other->takedamage)
        return;
    if (!(other->svflags & SVF_MONSTER) && !(other->flags & FL_DAMAGEABLE) && (!other->client) && (strcmp(other->classname, "misc_explobox") != 0))
        return;
    if ((self->spawnflags & SPAWNFLAG_HURT_NO_MONSTERS) && (other->svflags & SVF_MONSTER))
        return;
    if ((self->spawnflags & SPAWNFLAG_HURT_NO_PLAYERS) && (other->client))
        return;

    if (self->timestamp > level.time)
        return;

    if (self->spawnflags & SPAWNFLAG_HURT_CLIPPED) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    if (self->spawnflags & SPAWNFLAG_HURT_SLOW)
        self->timestamp = level.time + SEC(1);
    else
        self->timestamp = level.time + HZ(10);

    if (!(self->spawnflags & SPAWNFLAG_HURT_SILENT)) {
        if (self->fly_sound_debounce_time < level.time) {
            gi.sound(other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
            self->fly_sound_debounce_time = level.time + SEC(1);
        }
    }

    if (self->spawnflags & SPAWNFLAG_HURT_NO_PROTECTION)
        dflags = DAMAGE_NO_PROTECTION;
    else
        dflags = DAMAGE_NONE;

    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, self->dmg, dflags, (mod_t) { MOD_TRIGGER_HURT });
}

void SP_trigger_hurt(edict_t *self)
{
    InitTrigger(self);

    self->noise_index = gi.soundindex("world/electro.wav");
    self->touch = hurt_touch;

    if (!self->dmg)
        self->dmg = 5;

    if (self->spawnflags & SPAWNFLAG_HURT_START_OFF)
        self->solid = SOLID_NOT;
    else
        self->solid = SOLID_TRIGGER;

    if (self->spawnflags & SPAWNFLAG_HURT_TOGGLE)
        self->use = hurt_use;

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_HURT_CLIPPED)
        self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ? TOGGLE START_OFF
Changes the touching entites gravity to the value of "gravity".
1.0 is standard gravity for the level.

TOGGLE - trigger_gravity can be turned on and off
START_OFF - trigger_gravity starts turned off (implies TOGGLE)
*/

#define SPAWNFLAG_GRAVITY_TOGGLE    1
#define SPAWNFLAG_GRAVITY_START_OFF 2
#define SPAWNFLAG_GRAVITY_CLIPPED   4

// PGM
void USE(trigger_gravity_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->solid == SOLID_NOT)
        self->solid = SOLID_TRIGGER;
    else
        self->solid = SOLID_NOT;
    gi.linkentity(self);
}
// PGM

void TOUCH(trigger_gravity_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (self->spawnflags & SPAWNFLAG_GRAVITY_CLIPPED) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    other->gravity = self->gravity;
}

void SP_trigger_gravity(edict_t *self)
{
    if (!st.gravity || !*st.gravity) {
        gi.dprintf("%s: no gravity set\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    InitTrigger(self);

    // PGM
    self->gravity = atof(st.gravity);

    if (self->spawnflags & SPAWNFLAG_GRAVITY_TOGGLE)
        self->use = trigger_gravity_use;

    if (self->spawnflags & SPAWNFLAG_GRAVITY_START_OFF) {
        self->use = trigger_gravity_use;
        self->solid = SOLID_NOT;
    }

    self->touch = trigger_gravity_touch;
    // PGM

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_GRAVITY_CLIPPED)
        self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards

TOGGLE - trigger_monsterjump can be turned on and off
START_OFF - trigger_monsterjump starts turned off (implies TOGGLE)
*/

#define SPAWNFLAG_MONSTERJUMP_TOGGLE    1
#define SPAWNFLAG_MONSTERJUMP_START_OFF 2
#define SPAWNFLAG_MONSTERJUMP_CLIPPED   4

void USE(trigger_monsterjump_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->solid == SOLID_NOT)
        self->solid = SOLID_TRIGGER;
    else
        self->solid = SOLID_NOT;
    gi.linkentity(self);
}

void TOUCH(trigger_monsterjump_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other->flags & (FL_FLY | FL_SWIM))
        return;
    if (other->svflags & SVF_DEADMONSTER)
        return;
    if (!(other->svflags & SVF_MONSTER))
        return;

    if (self->spawnflags & SPAWNFLAG_MONSTERJUMP_CLIPPED) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    // set XY even if not on ground, so the jump will clear lips
    other->velocity[0] = self->movedir[0] * self->speed;
    other->velocity[1] = self->movedir[1] * self->speed;

    if (!other->groundentity)
        return;

    other->groundentity = NULL;
    other->velocity[2] = self->movedir[2];
}

void SP_trigger_monsterjump(edict_t *self)
{
    if (!self->speed)
        self->speed = 200;
    if (!st.height)
        st.height = 200;
    if (self->s.angles[YAW] == 0)
        self->s.angles[YAW] = 360;
    InitTrigger(self);
    self->touch = trigger_monsterjump_touch;
    self->movedir[2] = st.height;

    if (self->spawnflags & SPAWNFLAG_MONSTERJUMP_TOGGLE)
        self->use = trigger_monsterjump_use;

    if (self->spawnflags & SPAWNFLAG_MONSTERJUMP_START_OFF) {
        self->use = trigger_monsterjump_use;
        self->solid = SOLID_NOT;
    }

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_MONSTERJUMP_CLIPPED)
        self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_flashlight

==============================================================================
*/

/*QUAKED trigger_flashlight (.5 .5 .5) ?
Players moving against this trigger will have their flashlight turned on or off.
"style" default to 0, set to 1 to always turn flashlight on, 2 to always turn off,
        otherwise "angles" are used to control on/off state
*/

#define SPAWNFLAG_FLASHLIGHT_CLIPPED    1

void TOUCH(trigger_flashlight_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (!other->client)
        return;

    if (self->spawnflags & SPAWNFLAG_FLASHLIGHT_CLIPPED) {
        trace_t clip = gix.clip(other->s.origin, other->mins, other->maxs, other->s.origin, self, G_GetClipMask(other));

        if (clip.fraction == 1.0f)
            return;
    }

    if (self->style == 1)
        P_ToggleFlashlight(other, true);
    else if (self->style == 2)
        P_ToggleFlashlight(other, false);
    else if (VectorLengthSquared(other->velocity) > 32)
        P_ToggleFlashlight(other, DotProduct(other->velocity, self->movedir) > 0);
}

void SP_trigger_flashlight(edict_t *self)
{
    if (self->s.angles[YAW] == 0)
        self->s.angles[YAW] = 360;
    InitTrigger(self);
    self->touch = trigger_flashlight_touch;
    self->movedir[2] = st.height;

    if (self->spawnflags & SPAWNFLAG_FLASHLIGHT_CLIPPED)
        self->svflags |= SVF_HULL;
    gi.linkentity(self);
}


/*
==============================================================================

trigger_fog

==============================================================================
*/

/*QUAKED trigger_fog (.5 .5 .5) ? AFFECT_FOG AFFECT_HEIGHTFOG INSTANTANEOUS FORCE BLEND
Players moving against this trigger will have their fog settings changed.
Fog/heightfog will be adjusted if the spawnflags are set. Instantaneous
ignores any delays. Force causes it to ignore movement dir and always use
the "on" values. Blend causes it to change towards how far you are into the trigger
with respect to angles.
"target" can target an info_notnull to pull the keys below from.
"delay" default to 0.5; time in seconds a change in fog will occur over
"wait" default to 0.0; time in seconds before a re-trigger can be executed

"fog_density"; density value of fog, 0-1
"fog_color"; color value of fog, 3d vector with values between 0-1 (r g b)
"fog_density_off"; transition density value of fog, 0-1
"fog_color_off"; transition color value of fog, 3d vector with values between 0-1 (r g b)
"fog_sky_factor"; sky factor value of fog, 0-1
"fog_sky_factor_off"; transition sky factor value of fog, 0-1

"heightfog_falloff"; falloff value of heightfog, 0-1
"heightfog_density"; density value of heightfog, 0-1
"heightfog_start_color"; the start color for the fog (r g b, 0-1)
"heightfog_start_dist"; the start distance for the fog (units)
"heightfog_end_color"; the start color for the fog (r g b, 0-1)
"heightfog_end_dist"; the end distance for the fog (units)

"heightfog_falloff_off"; transition falloff value of heightfog, 0-1
"heightfog_density_off"; transition density value of heightfog, 0-1
"heightfog_start_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_start_dist_off"; transition the start distance for the fog (units)
"heightfog_end_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_end_dist_off"; transition the end distance for the fog (units)
*/

#define SPAWNFLAG_FOG_AFFECT_FOG        1
#define SPAWNFLAG_FOG_AFFECT_HEIGHTFOG  2
#define SPAWNFLAG_FOG_INSTANTANEOUS     4
#define SPAWNFLAG_FOG_FORCE             8
#define SPAWNFLAG_FOG_BLEND             16

void TOUCH(trigger_fog_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
}

void SP_trigger_fog(edict_t *self)
{
    if (self->s.angles[YAW] == 0)
        self->s.angles[YAW] = 360;

    InitTrigger(self);

    if (!(self->spawnflags & (SPAWNFLAG_FOG_AFFECT_FOG | SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)))
        gi.dprintf("WARNING: %s with no fog spawnflags set\n", etos(self));

    if (self->target) {
        self->movetarget = G_PickTarget(self->target);

        if (self->movetarget && !self->movetarget->delay)
            self->movetarget->delay = 0.5f;
    }

    if (!self->delay)
        self->delay = 0.5f;

    self->touch = trigger_fog_touch;
}

/*QUAKED trigger_coop_relay (.5 .5 .5) ? AUTO_FIRE
Like a trigger_relay, but all players must be touching its
mins/maxs in order to fire, otherwise a message will be printed.

AUTO_FIRE: check every `wait` seconds for containment instead of
requiring to be fired by something else. Frees itself after firing.

"message"; message to print to the one activating the relay if
           not all players are inside the bounds
"message2"; message to print to players not inside the trigger
            if they aren't in the bounds
*/

#define SPAWNFLAG_COOP_RELAY_AUTO_FIRE  1

static bool trigger_coop_relay_ok(edict_t *player)
{
    return player->inuse && player->client && player->health > 0 &&
        player->movetype != MOVETYPE_NOCLIP && player->s.modelindex == MODELINDEX_PLAYER;
}

static bool trigger_coop_relay_can_use(edict_t *self, edict_t *activator)
{
    // not coop, so act like a standard trigger_relay minus the message
    if (!coop->integer)
        return true;

    // coop; scan for all alive players, print appropriate message
    // to those in/out of range
    bool can_use = true;

    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];

        // dead or spectator, don't count them
        if (!trigger_coop_relay_ok(player))
            continue;

        if (boxes_intersect(player->absmin, player->absmax, self->absmin, self->absmax))
            continue;

        if (self->timestamp < level.time)
            gi.centerprintf(player, "%s", self->map);
        can_use = false;
    }

    return can_use;
}

void USE(trigger_coop_relay_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (!trigger_coop_relay_can_use(self, activator)) {
        if (self->timestamp < level.time)
            gi.centerprintf(activator, "%s", self->message);

        self->timestamp = level.time + SEC(5);
        return;
    }

    const char *msg = self->message;
    self->message = NULL;
    G_UseTargets(self, activator);
    self->message = msg;
}

void THINK(trigger_coop_relay_think)(edict_t *self)
{
    edict_t *players[MAX_CLIENTS];
    int i, j, num_active = 0, num_present = 0;

    for (i = 1; i <= game.maxclients; i++)
        if (trigger_coop_relay_ok(&g_edicts[i]))
            num_active++;

    edict_t *list[MAX_EDICTS_OLD];
    int count = gi.BoxEdicts(self->absmin, self->absmax, list, q_countof(list), AREA_SOLID);
    for (i = 0; i < count; i++) {
        edict_t *ent = list[i];
        if (trigger_coop_relay_ok(ent) && num_present < q_countof(players))
            players[num_present++] = ent;
    }

    if (num_present == num_active) {
        const char *msg = self->message;
        self->message = NULL;
        G_UseTargets(self, &g_edicts[1]);
        self->message = msg;

        G_FreeEdict(self);
        return;
    }

    if (num_present && self->timestamp < level.time) {
        for (i = 0; i < num_present; i++)
            gi.centerprintf(players[i], "%s", self->message);

        for (i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (!trigger_coop_relay_ok(player))
                continue;
            for (j = 0; j < num_present; j++)
                if (players[j] == player)
                    break;
            if (j == num_present)
                gi.centerprintf(player, "%s", self->map);
        }

        self->timestamp = level.time + SEC(5);
    }

    self->nextthink = level.time + SEC(self->wait);
}

void SP_trigger_coop_relay(edict_t *self)
{
    if (self->targetname && self->spawnflags & SPAWNFLAG_COOP_RELAY_AUTO_FIRE)
        gi.dprintf("%s: targetname and auto-fire are mutually exclusive\n", etos(self));

    InitTrigger(self);

    if (!self->message)
        self->message = "All players must be present to continue...";

    if (!self->map)
        self->map = "Players are waiting for you...";

    if (!self->wait)
        self->wait = 1;

    if (self->spawnflags & SPAWNFLAG_COOP_RELAY_AUTO_FIRE) {
        self->think = trigger_coop_relay_think;
        self->nextthink = level.time + SEC(self->wait);
    } else
        self->use = trigger_coop_relay_use;
    self->svflags |= SVF_NOCLIENT;
    gi.linkentity(self);
}
