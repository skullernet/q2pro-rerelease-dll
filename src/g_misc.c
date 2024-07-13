// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_misc.c

#include "g_local.h"

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void USE(Use_Areaportal)(edict_t *ent, edict_t *other, edict_t *activator)
{
    ent->count ^= 1; // toggle state
    gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(edict_t *ent)
{
    ent->use = Use_Areaportal;
    ent->count = 0; // always start closed
}

//=====================================================

/*
=================
Misc functions
=================
*/
void VelocityForDamage(int damage, vec3_t v)
{
    v[0] = 100.0f * crandom();
    v[1] = 100.0f * crandom();
    v[2] = frandom2(200.0f, 300.0f);

    if (damage < 50)
        VectorScale(v, 0.7f, v);
    else
        VectorScale(v, 1.2f, v);
}

void ClipGibVelocity(edict_t *ent)
{
    ent->velocity[0] = Q_clipf(ent->velocity[0], -300, 300);
    ent->velocity[1] = Q_clipf(ent->velocity[1], -300, 300);
    ent->velocity[2] = Q_clipf(ent->velocity[2],  200, 500); // always some upwards
}

/*
=================
gibs
=================
*/
void DIE(gib_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (mod.id == MOD_CRUSH)
        G_FreeEdict(self);
}

void TOUCH(gib_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (tr->plane.normal[2] > 0.7f) {
        self->s.angles[0] = Q_clipf(self->s.angles[0], -5.0f, 5.0f);
        self->s.angles[2] = Q_clipf(self->s.angles[2], -5.0f, 5.0f);
    }
}

edict_t *ThrowGib(edict_t *self, const char *gibname, int damage, gib_type_t type, float scale)
{
    edict_t *gib;
    vec3_t   vd;
    vec3_t   origin;
    float    vscale;
    int      i;

    if (type & GIB_HEAD) {
        gib = self;
        gib->s.event = EV_OTHER_TELEPORT;
        // remove setskin so that it doesn't set the skin wrongly later
        self->monsterinfo.setskin = NULL;
    } else
        gib = G_Spawn();

    VectorAvg(self->absmin, self->absmax, origin);

    for (i = 0; i < 3; i++) {
        VectorMA(origin, crandom() * 0.5f, self->size, gib->s.origin);

        // try 3 times to get a good, non-solid position
        if (!(gi.pointcontents(gib->s.origin) & MASK_SOLID))
            break;
    }

    if (i == 3 && gib != self) {
        // only free us if we're not being turned into the gib, otherwise
        // just spawn inside a wall
        G_FreeEdict(gib);
        return NULL;
    }

    gib->s.modelindex = gi.modelindex(gibname);
    gib->s.modelindex2 = 0;
    gib->x.scale = scale;
    gib->solid = SOLID_NOT;
    gib->svflags |= SVF_DEADMONSTER;
    gib->svflags &= ~SVF_MONSTER;
    gib->clipmask = MASK_SOLID;
    gib->s.effects = EF_NONE;
    gib->s.renderfx = RF_LOW_PRIORITY;
    gib->s.renderfx |= RF_NOSHADOW;
    if (!(type & GIB_DEBRIS)) {
        if (type & GIB_ACID)
            gib->s.effects |= EF_GREENGIB;
        else
            gib->s.effects |= EF_GIB;
        gib->s.renderfx |= RF_IR_VISIBLE;
    }
    gib->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
    gib->takedamage = true;
    gib->die = gib_die;
    gib->classname = "gib";
    if (type & GIB_SKINNED)
        gib->s.skinnum = self->s.skinnum;
    else
        gib->s.skinnum = 0;
    gib->s.frame = 0;
    VectorClear(gib->mins);
    VectorClear(gib->maxs);
    gib->s.sound = 0;
    gib->monsterinfo.engine_sound = 0;

    if (!(type & GIB_METALLIC)) {
        gib->movetype = MOVETYPE_TOSS;
        vscale = (type & GIB_ACID) ? 3.0f : 0.5f;
    } else {
        gib->movetype = MOVETYPE_BOUNCE;
        vscale = 1.0f;
    }

    if (type & GIB_DEBRIS) {
        vec3_t v = {
            100 * crandom(),
            100 * crandom(),
            100 + 100 * crandom()
        };
        VectorMA(self->velocity, damage, v, gib->velocity);
    } else {
        VelocityForDamage(damage, vd);
        VectorMA(self->velocity, vscale, vd, gib->velocity);
        ClipGibVelocity(gib);
    }

    if (type & GIB_UPRIGHT) {
        gib->touch = gib_touch;
        gib->flags |= FL_ALWAYS_TOUCH;
    }

    frandom_vec(gib->avelocity, 600);
    frandom_vec(gib->s.angles, 360);

    gib->think = G_FreeEdict;

    if (g_instagib->integer)
        gib->nextthink = level.time + random_time_sec(1, 5);
    else
        gib->nextthink = level.time + random_time_sec(10, 20);

    gi.linkentity(gib);

    gib->watertype = gi.pointcontents(gib->s.origin);

    if (gib->watertype & MASK_WATER)
        gib->waterlevel = WATER_FEET;
    else
        gib->waterlevel = WATER_NONE;

    return gib;
}

void ThrowClientHead(edict_t *self, int damage)
{
    vec3_t      vd;
    const char *gibname;

    if (brandom()) {
        gibname = "models/objects/gibs/head2/tris.md2";
        self->s.skinnum = 1; // second skin is player
    } else {
        gibname = "models/objects/gibs/skull/tris.md2";
        self->s.skinnum = 0;
    }

    self->s.origin[2] += 32;
    self->s.frame = 0;
    gi.setmodel(self, gibname);
    VectorSet(self->mins, -16, -16, 0);
    VectorSet(self->maxs, 16, 16, 16);

    self->takedamage = true; // [Paril-KEX] allow takedamage so we get crushed
    self->solid = SOLID_TRIGGER; // [Paril-KEX] make 'trigger' so we still move but don't block shots/explode
    self->svflags |= SVF_DEADMONSTER;
    self->s.effects = EF_GIB;
    // PGM
    self->s.renderfx |= RF_IR_VISIBLE;
    // PGM
    self->s.sound = 0;
    self->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;

    self->movetype = MOVETYPE_BOUNCE;
    VelocityForDamage(damage, vd);
    VectorAdd(self->velocity, vd, self->velocity);

    if (self->client) { // bodies in the queue don't have a client anymore
        self->client->anim_priority = ANIM_DEATH;
        self->client->anim_end = self->s.frame;
    } else {
        self->think = NULL;
        self->nextthink = 0;
    }

    gi.linkentity(self);
}

void ThrowGibs(edict_t *self, int damage, const gib_def_t *gibs)
{
    for (const gib_def_t *gib = gibs; gib->gibname; gib++) {
        float scale = gib->scale;
        if (!scale)
            scale = 1;
        if (self->x.scale)
            scale *= self->x.scale;
        for (int i = 0; i < gib->count; i++)
            ThrowGib(self, gib->gibname, damage, gib->type, scale);
    }
}

void PrecacheGibs(const gib_def_t *gibs)
{
    for (const gib_def_t *gib = gibs; gib->gibname; gib++)
        gi.modelindex(gib->gibname);
}

void BecomeExplosion1(edict_t *self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    G_FreeEdict(self);
}

static void BecomeExplosion2(edict_t *self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION2);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    G_FreeEdict(self);
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
    this path_corner targeted touches it
*/

void TOUCH(path_corner_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t   v;
    edict_t *next;

    if (other->movetarget != self)
        return;

    if (other->enemy)
        return;

    if (self->pathtarget) {
        const char *savetarget;

        savetarget = self->target;
        self->target = self->pathtarget;
        G_UseTargets(self, other);
        self->target = savetarget;
    }

    // see m_move; this is just so we don't needlessly check it
    self->flags |= FL_PARTIALGROUND;

    if (self->target)
        next = G_PickTarget(self->target);
    else
        next = NULL;

    // [Paril-KEX] don't teleport to a point_combat, it means HOLD for them.
    if (next && !strcmp(next->classname, "path_corner") && next->spawnflags & SPAWNFLAG_PATH_CORNER_TELEPORT) {
        VectorCopy(next->s.origin, v);
        v[2] += next->mins[2];
        v[2] -= other->mins[2];
        VectorCopy(v, other->s.origin);
        next = G_PickTarget(next->target);
        other->s.event = EV_OTHER_TELEPORT;
    }

    other->goalentity = other->movetarget = next;

    if (self->wait) {
        other->monsterinfo.pausetime = level.time + SEC(self->wait);
        other->monsterinfo.stand(other);
        return;
    }

    if (!other->movetarget) {
        // N64 cutscene behavior
        if (other->hackflags & HACKFLAG_END_CUTSCENE) {
            G_FreeEdict(other);
            return;
        }

        other->monsterinfo.pausetime = HOLD_FOREVER;
        other->monsterinfo.stand(other);
    } else {
        VectorSubtract(other->goalentity->s.origin, other->s.origin, v);
        other->ideal_yaw = vectoyaw(v);
    }
}

void SP_path_corner(edict_t *self)
{
    if (!self->targetname) {
        gi.dprintf("%s with no targetname\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->solid = SOLID_TRIGGER;
    self->touch = path_corner_touch;
    VectorSet(self->mins, -8, -8, -8);
    VectorSet(self->maxs, 8, 8, 8);
    self->svflags |= SVF_NOCLIENT;
    gi.linkentity(self);
}

/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void TOUCH(point_combat_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    edict_t *activator;

    if (other->movetarget != self)
        return;

    if (self->target) {
        other->target = self->target;
        other->goalentity = other->movetarget = G_PickTarget(other->target);
        if (!other->goalentity) {
            gi.dprintf("%s target %s does not exist\n", etos(self), self->target);
            other->movetarget = self;
        }
        // [Paril-KEX] allow them to be re-used
        //self->target = NULL;
    } else if ((self->spawnflags & SPAWNFLAG_POINT_COMBAT_HOLD) && !(other->flags & (FL_SWIM | FL_FLY))) {
        // already standing
        if (other->monsterinfo.aiflags & AI_STAND_GROUND)
            return;

        other->monsterinfo.pausetime = HOLD_FOREVER;
        other->monsterinfo.aiflags |= AI_STAND_GROUND | AI_REACHED_HOLD_COMBAT | AI_THIRD_EYE;
        other->monsterinfo.stand(other);
    }

    if (other->movetarget == self) {
        // [Paril-KEX] if we're holding, keep movetarget set; we will
        // use this to make sure we haven't moved too far from where
        // we want to "guard".
        if (!(self->spawnflags & SPAWNFLAG_POINT_COMBAT_HOLD)) {
            other->target = NULL;
            other->movetarget = NULL;
        }

        other->goalentity = other->enemy;
        other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
    }

    if (self->pathtarget) {
        const char *savetarget;

        savetarget = self->target;
        self->target = self->pathtarget;
        if (other->enemy && other->enemy->client)
            activator = other->enemy;
        else if (other->oldenemy && other->oldenemy->client)
            activator = other->oldenemy;
        else if (other->activator && other->activator->client)
            activator = other->activator;
        else
            activator = other;
        G_UseTargets(self, activator);
        self->target = savetarget;
    }
}

void SP_point_combat(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }
    self->solid = SOLID_TRIGGER;
    self->touch = point_combat_touch;
    VectorSet(self->mins, -8, -8, -16);
    VectorSet(self->maxs, 8, 8, 16);
    self->svflags = SVF_NOCLIENT;
    gi.linkentity(self);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null(edict_t *self)
{
    G_FreeEdict(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull(edict_t *self)
{
    VectorCopy(self->s.origin, self->absmin);
    VectorCopy(self->s.origin, self->absmax);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF ALLOW_IN_DM
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

#define SPAWNFLAG_LIGHT_START_OFF   1
#define SPAWNFLAG_LIGHT_ALLOW_IN_DM 2

void USE(light_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->spawnflags & SPAWNFLAG_LIGHT_START_OFF) {
        gi.configstring(CS_LIGHTS + self->style, self->style_on);
        self->spawnflags &= ~SPAWNFLAG_LIGHT_START_OFF;
    } else {
        gi.configstring(CS_LIGHTS + self->style, self->style_off);
        self->spawnflags |= SPAWNFLAG_LIGHT_START_OFF;
    }
}

// ---------------------------------------------------------------------------------

void USE(dynamic_light_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->svflags ^= SVF_NOCLIENT;
}

void SP_dynamic_light(edict_t *self)
{
    if (self->targetname)
        self->use = dynamic_light_use;

    if (self->spawnflags & SPAWNFLAG_LIGHT_START_OFF)
        self->svflags ^= SVF_NOCLIENT;
}

void SP_light(edict_t *self)
{
    // no targeted lights in deathmatch, because they cause global messages
    if ((!self->targetname || (deathmatch->integer && !(self->spawnflags & SPAWNFLAG_LIGHT_ALLOW_IN_DM)))) { // [Sam-KEX]
        G_FreeEdict(self);
        return;
    }

    if (self->style >= 32) {
        self->use = light_use;

        if (!self->style_on || !*self->style_on)
            self->style_on = "m";
        else if (*self->style_on >= '0' && *self->style_on <= '9')
            self->style_on = G_GetLightStyle(Q_atoi(self->style_on));

        if (!self->style_off || !*self->style_off)
            self->style_off = "a";
        else if (*self->style_off >= '0' && *self->style_off <= '9')
            self->style_off = G_GetLightStyle(Q_atoi(self->style_off));

        if (self->spawnflags & SPAWNFLAG_LIGHT_START_OFF)
            gi.configstring(CS_LIGHTS + self->style, self->style_off);
        else
            gi.configstring(CS_LIGHTS + self->style, self->style_on);
    }
}

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN   the wall will not be present until triggered
                it will then blink in to existance; it will
                kill anything that was in it's way

TOGGLE          only valid for TRIGGER_SPAWN walls
                this allows the wall to be turned on and off

START_ON        only valid for TRIGGER_SPAWN walls
                the wall will initially be present
*/

#define SPAWNFLAG_WALL_TRIGGER_SPAWN    1
#define SPAWNFLAG_WALL_TOGGLE           2
#define SPAWNFLAG_WALL_START_ON         4
#define SPAWNFLAG_WALL_ANIMATED         8
#define SPAWNFLAG_WALL_ANIMATED_FAST    16

void USE(func_wall_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->solid == SOLID_NOT) {
        self->solid = SOLID_BSP;
        self->svflags &= ~SVF_NOCLIENT;
        gi.linkentity(self);
        KillBox(self, false, MOD_TELEFRAG, true);
    } else {
        self->solid = SOLID_NOT;
        self->svflags |= SVF_NOCLIENT;
        gi.linkentity(self);
    }

    if (!(self->spawnflags & SPAWNFLAG_WALL_TOGGLE))
        self->use = NULL;
}

void SP_func_wall(edict_t *self)
{
    self->movetype = MOVETYPE_PUSH;
    gi.setmodel(self, self->model);

    if (self->spawnflags & SPAWNFLAG_WALL_ANIMATED)
        self->s.effects |= EF_ANIM_ALL;
    if (self->spawnflags & SPAWNFLAG_WALL_ANIMATED_FAST)
        self->s.effects |= EF_ANIM_ALLFAST;

    // just a wall
    if (!(self->spawnflags & (SPAWNFLAG_WALL_TRIGGER_SPAWN | SPAWNFLAG_WALL_TOGGLE | SPAWNFLAG_WALL_START_ON))) {
        self->solid = SOLID_BSP;
        gi.linkentity(self);
        return;
    }

    // it must be TRIGGER_SPAWN
    if (!(self->spawnflags & SPAWNFLAG_WALL_TRIGGER_SPAWN))
        self->spawnflags |= SPAWNFLAG_WALL_TRIGGER_SPAWN;

    // yell if the spawnflags are odd
    if ((self->spawnflags & SPAWNFLAG_WALL_START_ON) && !(self->spawnflags & SPAWNFLAG_WALL_TOGGLE)) {
        gi.dprintf("func_wall START_ON without TOGGLE\n");
        self->spawnflags |= SPAWNFLAG_WALL_TOGGLE;
    }

    self->use = func_wall_use;
    if (self->spawnflags & SPAWNFLAG_WALL_START_ON) {
        self->solid = SOLID_BSP;
    } else {
        self->solid = SOLID_NOT;
        self->svflags |= SVF_NOCLIENT;
    }
    gi.linkentity(self);
}

// [Paril-KEX]
/*QUAKED func_animation (0 .5 .8) ? START_ON
Similar to func_wall, but triggering it will toggle animation
state rather than going on/off.

START_ON        will start in alterate animation
*/

#define SPAWNFLAG_ANIMATION_START_ON    1

void USE(func_animation_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->bmodel_anim.alternate = !self->bmodel_anim.alternate;
}

void SP_func_animation(edict_t *self)
{
    if (!self->bmodel_anim.enabled) {
        gi.dprintf("%s has no animation data\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_PUSH;
    gi.setmodel(self, self->model);
    self->solid = SOLID_BSP;

    self->use = func_animation_use;
    self->bmodel_anim.alternate = self->spawnflags & SPAWNFLAG_ANIMATION_START_ON;
    self->s.frame = self->bmodel_anim.params[self->bmodel_anim.alternate].start;

    gi.linkentity(self);
}

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

#define SPAWNFLAGS_OBJECT_TRIGGER_SPAWN 1
#define SPAWNFLAGS_OBJECT_ANIMATED      2
#define SPAWNFLAGS_OBJECT_ANIMATED_FAST 4

void TOUCH(func_object_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    // only squash thing we fall on top of
    if (other_touching_self)
        return;
    if (tr->plane.normal[2] < 1.0f)
        return;
    if (other->takedamage == false)
        return;
    if (other->damage_debounce_time > level.time)
        return;

    vec3_t pos;
    closest_point_to_box(other->s.origin, self->absmin, self->absmax, pos);
    T_Damage(other, self, self, vec3_origin, pos, tr->plane.normal, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });

    other->damage_debounce_time = level.time + HZ(10);
}

void THINK(func_object_release)(edict_t *self)
{
    self->movetype = MOVETYPE_TOSS;
    self->touch = func_object_touch;
}

void USE(func_object_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->solid = SOLID_BSP;
    self->svflags &= ~SVF_NOCLIENT;
    self->use = NULL;
    func_object_release(self);
    KillBox(self, false, MOD_TELEFRAG, true);
}

void SP_func_object(edict_t *self)
{
    gi.setmodel(self, self->model);

    self->mins[0] += 1;
    self->mins[1] += 1;
    self->mins[2] += 1;
    self->maxs[0] -= 1;
    self->maxs[1] -= 1;
    self->maxs[2] -= 1;

    if (!self->dmg)
        self->dmg = 100;

    if (!(self->spawnflags & SPAWNFLAGS_OBJECT_TRIGGER_SPAWN)) {
        self->solid = SOLID_BSP;
        self->movetype = MOVETYPE_PUSH;
        self->think = func_object_release;
        self->nextthink = level.time + HZ(5);
    } else {
        self->solid = SOLID_NOT;
        self->movetype = MOVETYPE_PUSH;
        self->use = func_object_use;
        self->svflags |= SVF_NOCLIENT;
    }

    if (self->spawnflags & SPAWNFLAGS_OBJECT_ANIMATED)
        self->s.effects |= EF_ANIM_ALL;
    if (self->spawnflags & SPAWNFLAGS_OBJECT_ANIMATED_FAST)
        self->s.effects |= EF_ANIM_ALLFAST;

    self->clipmask = MASK_MONSTERSOLID;
    self->flags |= FL_NO_STANDING;

    gi.linkentity(self);
}

/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST INACTIVE ALWAYS_SHOOTABLE
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

INACTIVE - specifies that the entity is not explodable until triggered. If you use this you must
target the entity you want to trigger it. This is the only entity approved to activate it.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/

#define SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN      1
#define SPAWNFLAGS_EXPLOSIVE_ANIMATED           2
#define SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST      4
#define SPAWNFLAGS_EXPLOSIVE_INACTIVE           8
#define SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE   16

void DIE(func_explosive_explode)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    int      count;
    int      mass;
    edict_t *master;
    bool     done = false;

    self->takedamage = false;

    if (self->dmg)
        T_RadiusDamage(self, attacker, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, (mod_t) { MOD_EXPLOSIVE });

    vec3_t dir;
    VectorSubtract(inflictor->s.origin, self->s.origin, dir);
    VectorNormalize(dir);
    VectorScale(dir, 150, self->velocity);

    mass = self->mass;
    if (!mass)
        mass = 75;

    // big chunks
    if (mass >= 100) {
        count = mass / 100;
        if (count > 8)
            count = 8;
        for (int i = 0; i < count; i++)
            ThrowGib(self, "models/objects/debris1/tris.md2", 1, GIB_METALLIC | GIB_DEBRIS, self->x.scale);
    }

    // small chunks
    count = mass / 25;
    if (count > 16)
        count = 16;
    for (int i = 0; i < count; i++)
        ThrowGib(self, "models/objects/debris2/tris.md2", 2, GIB_METALLIC | GIB_DEBRIS, self->x.scale);

    // PMM - if we're part of a train, clean ourselves out of it
    if (self->flags & FL_TEAMSLAVE) {
        master = self->teammaster;
        if (master && master->inuse) { // because mappers (other than jim (usually)) are stupid....
            while (!done) {
                if (master->teamchain == self) {
                    master->teamchain = self->teamchain;
                    done = true;
                }
                master = master->teamchain;
            }
        }
    }

    G_UseTargets(self, attacker);

    VectorAvg(self->absmin, self->absmax, self->s.origin);

    if (self->noise_index)
        gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);

    if (self->dmg)
        BecomeExplosion1(self);
    else
        G_FreeEdict(self);
}

void USE(func_explosive_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    // Paril: pass activator to explode as attacker. this fixes
    // "strike" trying to centerprint to the relay. Should be
    // a safe change.
    func_explosive_explode(self, self, activator, self->health, vec3_origin, (mod_t) { MOD_EXPLOSIVE });
}

// PGM
void USE(func_explosive_activate)(edict_t *self, edict_t *other, edict_t *activator)
{
    bool approved = false;

    // PMM - looked like target and targetname were flipped here
    if (other && other->target && !strcmp(other->target, self->targetname))
        approved = true;
    else if (activator && activator->target && !strcmp(activator->target, self->targetname))
        approved = true;

    if (!approved)
        return;

    self->use = func_explosive_use;
    if (!self->health)
        self->health = 100;
    self->die = func_explosive_explode;
    self->takedamage = true;
}
// PGM

void USE(func_explosive_spawn)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->solid = SOLID_BSP;
    self->svflags &= ~SVF_NOCLIENT;
    self->use = NULL;
    gi.linkentity(self);
    KillBox(self, false, MOD_TELEFRAG, true);
}

void SP_func_explosive(edict_t *self)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_PUSH;

    gi.modelindex("models/objects/debris1/tris.md2");
    gi.modelindex("models/objects/debris2/tris.md2");

    gi.setmodel(self, self->model);

    if (self->spawnflags & SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN) {
        self->svflags |= SVF_NOCLIENT;
        self->solid = SOLID_NOT;
        self->use = func_explosive_spawn;
    // PGM
    } else if (self->spawnflags & SPAWNFLAGS_EXPLOSIVE_INACTIVE) {
        self->solid = SOLID_BSP;
        if (self->targetname)
            self->use = func_explosive_activate;
    // PGM
    } else {
        self->solid = SOLID_BSP;
        if (self->targetname)
            self->use = func_explosive_use;
    }

    if (self->spawnflags & SPAWNFLAGS_EXPLOSIVE_ANIMATED)
        self->s.effects |= EF_ANIM_ALL;
    if (self->spawnflags & SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST)
        self->s.effects |= EF_ANIM_ALLFAST;

    // PGM
    if ((self->spawnflags & SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE) || ((self->use != func_explosive_use) && (self->use != func_explosive_activate))) {
    // PGM
        if (!self->health)
            self->health = 100;
        self->die = func_explosive_explode;
        self->takedamage = true;
    }

    if (self->sounds) {
        if (self->sounds == 1)
            self->noise_index = gi.soundindex("world/brkglas.wav");
        else
            gi.dprintf("%s: invalid \"sounds\" %d\n", etos(self), self->sounds);
    }

    gi.linkentity(self);
}

/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/

void TOUCH(barrel_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    float  ratio;
    vec3_t v;

    if ((!other->groundentity) || (other->groundentity == self))
        return;
    if (!other_touching_self)
        return;

    ratio = (float)other->mass / (float)self->mass;
    VectorSubtract(self->s.origin, other->s.origin, v);
    M_walkmove(self, vectoyaw(v), 20 * ratio * FRAME_TIME_SEC);
}

static const gib_def_t barrel_gibs[] = {
    { "models/objects/debris1/tris.md2", 2, GIB_METALLIC | GIB_DEBRIS },
    { "models/objects/debris3/tris.md2", 4, GIB_METALLIC | GIB_DEBRIS },
    { "models/objects/debris2/tris.md2", 8, GIB_METALLIC | GIB_DEBRIS },
    { 0 }
};

void THINK(barrel_explode)(edict_t *self)
{
    self->takedamage = false;

    T_RadiusDamage(self, self->activator, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, (mod_t) { MOD_BARREL });

    ThrowGibs(self, 1.5f * self->dmg / 200, barrel_gibs);

    if (self->groundentity)
        BecomeExplosion2(self);
    else
        BecomeExplosion1(self);
}

void THINK(barrel_burn)(edict_t *self)
{
    if (level.time >= self->timestamp)
        self->think = barrel_explode;

    self->x.morefx |= EFX_BARREL_EXPLODING;
    self->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
    self->nextthink = level.time + FRAME_TIME;
}

void DIE(barrel_delay)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // allow "dead" barrels waiting to explode to still receive knockback
    if (self->think == barrel_burn || self->think == barrel_explode)
        return;

    // allow big booms to immediately blow up barrels (rockets, rail, other explosions) because it feels good and powerful
    if (damage >= 90) {
        self->think = barrel_explode;
        self->activator = attacker;
    } else {
        self->timestamp = level.time + SEC(0.75f);
        self->think = barrel_burn;
        self->activator = attacker;
    }
}

//=========
// PGM  - change so barrels will think and hence, blow up
void THINK(barrel_think)(edict_t *self)
{
    // the think needs to be first since later stuff may override.
    self->think = barrel_think;
    self->nextthink = level.time + FRAME_TIME;

    M_CategorizePosition(self, self->s.origin, &self->waterlevel, &self->watertype);
    self->flags |= FL_IMMUNE_SLIME;
    self->air_finished = level.time + SEC(100);
    M_WorldEffects(self);
}

void THINK(barrel_start)(edict_t *self)
{
    M_droptofloor(self);
    self->think = barrel_think;
    self->nextthink = level.time + FRAME_TIME;
}
// PGM
//=========

void SP_misc_explobox(edict_t *self)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(self);
        return;
    }

    PrecacheGibs(barrel_gibs);
    gi.soundindex("weapons/bfg__l1a.wav");

    self->solid = SOLID_BBOX;
    self->movetype = MOVETYPE_STEP;

    self->model = "models/objects/barrels/tris.md2";
    self->s.modelindex = gi.modelindex(self->model);
    VectorSet(self->mins, -16, -16, 0);
    VectorSet(self->maxs, 16, 16, 40);

    if (!self->mass)
        self->mass = 50;
    if (!self->health)
        self->health = 10;
    if (!self->dmg)
        self->dmg = 150;

    self->die = barrel_delay;
    self->takedamage = true;
    self->flags |= FL_TRAP;

    self->touch = barrel_touch;

    // PGM - change so barrels will think and hence, blow up
    self->think = barrel_start;
    self->nextthink = level.time + HZ(5);
    // PGM

    gi.linkentity(self);
}

//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8) AUTO_NOISE
model="models/objects/black/tris.md2"
*/

#define SPAWNFLAG_BLACKHOLE_AUTO_NOISE  1

void USE(misc_blackhole_use)(edict_t *ent, edict_t *other, edict_t *activator)
{
    /*
    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_BOSSTPORT);
    gi.WritePosition (ent->s.origin);
    gi.multicast (ent->s.origin, MULTICAST_PVS);
    */
    G_FreeEdict(ent);
}

void THINK(misc_blackhole_think)(edict_t *self)
{
    if (self->timestamp <= level.time) {
        if (++self->s.frame >= 19)
            self->s.frame = 0;
        self->timestamp = level.time + HZ(10);
    }

    if (self->spawnflags & SPAWNFLAG_BLACKHOLE_AUTO_NOISE) {
        self->s.angles[0] += 50.0f * FRAME_TIME_SEC;
        self->s.angles[1] += 50.0f * FRAME_TIME_SEC;
    }

    self->nextthink = level.time + FRAME_TIME;
}

void SP_misc_blackhole(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_NOT;
    VectorSet(ent->mins, -64, -64, 0);
    VectorSet(ent->maxs, 64, 64, 8);
    ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
    ent->s.renderfx = RF_TRANSLUCENT;
    ent->use = misc_blackhole_use;
    ent->think = misc_blackhole_think;
    ent->nextthink = level.time + HZ(5);

    if (ent->spawnflags & SPAWNFLAG_BLACKHOLE_AUTO_NOISE) {
        ent->s.sound = gi.soundindex("world/blackhole.wav");
        ent->x.loop_attenuation = ATTN_NORM;
    }

    gi.linkentity(ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
 */

void THINK(misc_eastertank_think)(edict_t *self)
{
    if (++self->s.frame < 293)
        self->nextthink = level.time + HZ(10);
    else {
        self->s.frame = 254;
        self->nextthink = level.time + HZ(10);
    }
}

void SP_misc_eastertank(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -32, -32, -16);
    VectorSet(ent->maxs, 32, 32, 32);
    ent->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
    ent->s.frame = 254;
    ent->think = misc_eastertank_think;
    ent->nextthink = level.time + HZ(5);
    gi.linkentity(ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
 */

void THINK(misc_easterchick_think)(edict_t *self)
{
    if (++self->s.frame < 247)
        self->nextthink = level.time + HZ(10);
    else {
        self->s.frame = 208;
        self->nextthink = level.time + HZ(10);
    }
}

void SP_misc_easterchick(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -32, -32, 0);
    VectorSet(ent->maxs, 32, 32, 32);
    ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
    ent->s.frame = 208;
    ent->think = misc_easterchick_think;
    ent->nextthink = level.time + HZ(5);
    gi.linkentity(ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
 */

void THINK(misc_easterchick2_think)(edict_t *self)
{
    if (++self->s.frame < 287)
        self->nextthink = level.time + HZ(10);
    else {
        self->s.frame = 248;
        self->nextthink = level.time + HZ(10);
    }
}

void SP_misc_easterchick2(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -32, -32, 0);
    VectorSet(ent->maxs, 32, 32, 32);
    ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
    ent->s.frame = 248;
    ent->think = misc_easterchick2_think;
    ent->nextthink = level.time + HZ(5);
    gi.linkentity(ent);
}

/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

void THINK(commander_body_think)(edict_t *self)
{
    if (++self->s.frame < 24)
        self->nextthink = level.time + HZ(10);
    else
        self->nextthink = 0;

    if (self->s.frame == 22)
        gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1, ATTN_NORM, 0);
}

void USE(commander_body_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->think = commander_body_think;
    self->nextthink = level.time + HZ(10);
    gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

void THINK(commander_body_drop)(edict_t *self)
{
    self->movetype = MOVETYPE_TOSS;
    self->s.origin[2] += 2;
}

void SP_monster_commander_body(edict_t *self)
{
    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_BBOX;
    self->model = "models/monsters/commandr/tris.md2";
    self->s.modelindex = gi.modelindex(self->model);
    VectorSet(self->mins, -32, -32, 0);
    VectorSet(self->maxs, 32, 32, 48);
    self->use = commander_body_use;
    self->takedamage = true;
    self->flags = FL_GODMODE;
    gi.linkentity(self);

    gi.soundindex("tank/thud.wav");
    gi.soundindex("tank/pain.wav");

    self->think = commander_body_drop;
    self->nextthink = level.time + HZ(2);
}

/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
model="models/objects/banner/tris.md2"
*/
void THINK(misc_banner_think)(edict_t *ent)
{
    ent->s.frame = (ent->s.frame + 1) % 16;
    ent->nextthink = level.time + HZ(10);
}

void SP_misc_banner(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_NOT;
    ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
    ent->s.frame = irandom1(16);
    gi.linkentity(ent);

    ent->think = misc_banner_think;
    ent->nextthink = level.time + HZ(10);
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/

#define SPAWNFLAGS_DEADSOLDIER_ON_BACK      1
#define SPAWNFLAGS_DEADSOLDIER_ON_STOMACH   2
#define SPAWNFLAGS_DEADSOLDIER_BACK_DECAP   4
#define SPAWNFLAGS_DEADSOLDIER_FETAL_POS    8
#define SPAWNFLAGS_DEADSOLDIER_SIT_DECAP    16
#define SPAWNFLAGS_DEADSOLDIER_IMPALED      32

static const gib_def_t deadsoldier_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(misc_deadsoldier_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (self->health > -30)
        return;

    gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
    ThrowGibs(self, damage, deadsoldier_gibs);
}

void SP_misc_deadsoldier(edict_t *ent)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(ent);
        return;
    }

    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    ent->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

    // Defaults to frame 0
    if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_ON_STOMACH)
        ent->s.frame = 1;
    else if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_BACK_DECAP)
        ent->s.frame = 2;
    else if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_FETAL_POS)
        ent->s.frame = 3;
    else if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_SIT_DECAP)
        ent->s.frame = 4;
    else if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_IMPALED)
        ent->s.frame = 5;
    else if (ent->spawnflags & SPAWNFLAGS_DEADSOLDIER_ON_BACK)
        ent->s.frame = 0;
    else
        ent->s.frame = 0;

    VectorSet(ent->mins, -16, -16, 0);
    VectorSet(ent->maxs, 16, 16, 16);
    ent->deadflag = true;
    ent->takedamage = true;
    // nb: SVF_MONSTER is here so it bleeds
    ent->svflags |= SVF_MONSTER | SVF_DEADMONSTER;
    ent->die = misc_deadsoldier_die;
    ent->monsterinfo.aiflags |= AI_GOOD_GUY | AI_DO_NOT_COUNT;

    gi.linkentity(ent);
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast the Viper should fly
*/

void USE(misc_viper_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->svflags &= ~SVF_NOCLIENT;
    self->use = train_use;
    train_use(self, other, activator);
}

void SP_misc_viper(edict_t *ent)
{
    if (!ent->target) {
        gi.dprintf("%s without a target\n", etos(ent));
        G_FreeEdict(ent);
        return;
    }

    if (!ent->speed)
        ent->speed = 300;

    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_NOT;
    ent->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
    VectorSet(ent->mins, -16, -16, 0);
    VectorSet(ent->maxs, 16, 16, 32);

    ent->think = func_train_find;
    ent->nextthink = level.time + HZ(10);
    ent->use = misc_viper_use;
    ent->svflags |= SVF_NOCLIENT;
    ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

    gi.linkentity(ent);
}

/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -176, -120, -24);
    VectorSet(ent->maxs, 176, 120, 72);
    ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
    gi.linkentity(ent);
}

/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"   how much boom should the bomb make?
*/
void TOUCH(misc_viper_bomb_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    G_UseTargets(self, self->activator);

    self->s.origin[2] = self->absmin[2] + 1;
    T_RadiusDamage(self, self, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, (mod_t) { MOD_BOMB });
    BecomeExplosion2(self);
}

void PRETHINK(misc_viper_bomb_prethink)(edict_t *self)
{
    self->groundentity = NULL;

    float diff = TO_SEC(self->timestamp - level.time);
    if (diff < -1.0f)
        diff = -1.0f;

    vec3_t v;
    VectorScale(self->moveinfo.dir, 1.0f + diff, v);
    v[2] = diff;

    diff = self->s.angles[2];
    vectoangles(v, self->s.angles);
    self->s.angles[2] = diff + 10;
}

void USE(misc_viper_bomb_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t *viper;

    self->solid = SOLID_BBOX;
    self->svflags &= ~SVF_NOCLIENT;
    self->s.effects |= EF_ROCKET;
    self->use = NULL;
    self->movetype = MOVETYPE_TOSS;
    self->prethink = misc_viper_bomb_prethink;
    self->touch = misc_viper_bomb_touch;
    self->activator = activator;
    self->timestamp = level.time;

    viper = G_Find(NULL, FOFS(classname), "misc_viper");
    if (viper) {
        VectorScale(viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);
        VectorCopy(viper->moveinfo.dir, self->moveinfo.dir);
    }
}

void SP_misc_viper_bomb(edict_t *self)
{
    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_NOT;
    VectorSet(self->mins, -8, -8, -8);
    VectorSet(self->maxs, 8, 8, 8);

    self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

    if (!self->dmg)
        self->dmg = 1000;

    self->use = misc_viper_bomb_use;
    self->svflags |= SVF_NOCLIENT;

    gi.linkentity(self);
}

/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast it should fly
*/
void USE(misc_strogg_ship_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->svflags &= ~SVF_NOCLIENT;
    self->use = train_use;
    train_use(self, other, activator);
}

void SP_misc_strogg_ship(edict_t *ent)
{
    if (!ent->target) {
        gi.dprintf("%s without a target\n", etos(ent));
        G_FreeEdict(ent);
        return;
    }

    if (!ent->speed)
        ent->speed = 300;

    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_NOT;
    ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
    VectorSet(ent->mins, -16, -16, 0);
    VectorSet(ent->maxs, 16, 16, 32);

    ent->think = func_train_find;
    ent->nextthink = level.time + HZ(10);
    ent->use = misc_strogg_ship_use;
    ent->svflags |= SVF_NOCLIENT;
    ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

    gi.linkentity(ent);
}

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
model="models/objects/satellite/tris.md2"
*/
void THINK(misc_satellite_dish_think)(edict_t *self)
{
    self->s.frame++;
    if (self->s.frame < 38)
        self->nextthink = level.time + HZ(10);
}

void USE(misc_satellite_dish_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->s.frame = 0;
    self->think = misc_satellite_dish_think;
    self->nextthink = level.time + HZ(10);
}

void SP_misc_satellite_dish(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -64, -64, 0);
    VectorSet(ent->maxs, 64, 64, 128);
    ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
    ent->use = misc_satellite_dish_use;
    gi.linkentity(ent);
}

/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void SP_light_mine1(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_NOT;
    ent->svflags = SVF_DEADMONSTER;
    ent->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
    gi.linkentity(ent);
}

/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void SP_light_mine2(edict_t *ent)
{
    ent->movetype = MOVETYPE_NONE;
    ent->solid = SOLID_NOT;
    ent->svflags = SVF_DEADMONSTER;
    ent->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
    gi.linkentity(ent);
}

/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(edict_t *ent)
{
    gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
    ent->solid = SOLID_NOT;
    ent->s.effects |= EF_GIB;
    ent->takedamage = true;
    ent->die = gib_die;
    ent->movetype = MOVETYPE_TOSS;
    ent->deadflag = true;
    frandom_vec(ent->avelocity, 200);
    ent->think = G_FreeEdict;
    ent->nextthink = level.time + SEC(10);
    gi.linkentity(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(edict_t *ent)
{
    gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
    ent->solid = SOLID_NOT;
    ent->s.effects |= EF_GIB;
    ent->takedamage = true;
    ent->die = gib_die;
    ent->movetype = MOVETYPE_TOSS;
    ent->deadflag = true;
    frandom_vec(ent->avelocity, 200);
    ent->think = G_FreeEdict;
    ent->nextthink = level.time + SEC(10);
    gi.linkentity(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head(edict_t *ent)
{
    gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
    ent->solid = SOLID_NOT;
    ent->s.effects |= EF_GIB;
    ent->takedamage = true;
    ent->die = gib_die;
    ent->movetype = MOVETYPE_TOSS;
    ent->deadflag = true;
    frandom_vec(ent->avelocity, 200);
    ent->think = G_FreeEdict;
    ent->nextthink = level.time + SEC(10);
    gi.linkentity(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(edict_t *self)
{
    self->movetype = MOVETYPE_PUSH;
    gi.setmodel(self, self->model);
    self->solid = SOLID_BSP;
    self->s.frame = 12;
    gi.linkentity(self);
}

/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
 */

void USE(target_string_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t *e;
    int      n;
    size_t   l;
    char     c;

    l = strlen(self->message);
    for (e = self->teammaster; e; e = e->teamchain) {
        if (!e->count)
            continue;
        n = e->count - 1;
        if (n > l) {
            e->s.frame = 12;
            continue;
        }

        c = self->message[n];
        if (c >= '0' && c <= '9')
            e->s.frame = c - '0';
        else if (c == '-')
            e->s.frame = 10;
        else if (c == ':')
            e->s.frame = 11;
        else
            e->s.frame = 12;
    }
}

void SP_target_string(edict_t *self)
{
    if (!self->message)
        self->message = "";
    self->use = target_string_use;
}

/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and then fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"     0 "xx"
            1 "xx:xx"
            2 "xx:xx:xx"
*/

#define SPAWNFLAG_TIMER_UP          1
#define SPAWNFLAG_TIMER_DOWN        2
#define SPAWNFLAG_TIMER_START_OFF   4
#define SPAWNFLAG_TIMER_MULTI_USE   8

static void func_clock_reset(edict_t *self)
{
    self->activator = NULL;

    if (self->spawnflags & SPAWNFLAG_TIMER_UP) {
        self->health = 0;
        self->wait = self->count;
    } else if (self->spawnflags & SPAWNFLAG_TIMER_DOWN) {
        self->health = self->count;
        self->wait = 0;
    }
}

static void func_clock_format_countdown(edict_t *self)
{
    if (self->style == 0) {
        Q_snprintf(self->clock_message, sizeof(self->clock_message), "%2i", self->health);
        return;
    }

    if (self->style == 1) {
        Q_snprintf(self->clock_message, sizeof(self->clock_message), "%2i:%02i", self->health / 60, self->health % 60);
        return;
    }

    if (self->style == 2) {
        Q_snprintf(self->clock_message, sizeof(self->clock_message), "%2i:%02i:%02i", self->health / 3600,
                   (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
        return;
    }
}

void THINK(func_clock_think)(edict_t *self)
{
    if (!self->enemy) {
        self->enemy = G_Find(NULL, FOFS(targetname), self->target);
        if (!self->enemy)
            return;
    }

    if (self->spawnflags & SPAWNFLAG_TIMER_UP) {
        func_clock_format_countdown(self);
        self->health++;
    } else if (self->spawnflags & SPAWNFLAG_TIMER_DOWN) {
        func_clock_format_countdown(self);
        self->health--;
    } else {
        struct tm *ltime;
        time_t     gmtime;

        gmtime = time(NULL);
        ltime = localtime(&gmtime);
        if (ltime)
            Q_snprintf(self->clock_message, sizeof(self->clock_message), "%2i:%02i:%02i",
                       ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
        else
            strcpy(self->clock_message, "00:00:00");
    }

    self->enemy->message = self->clock_message;
    self->enemy->use(self->enemy, self, self);

    if (((self->spawnflags & SPAWNFLAG_TIMER_UP) && (self->health > self->wait)) ||
        ((self->spawnflags & SPAWNFLAG_TIMER_DOWN) && (self->health < self->wait))) {
        if (self->pathtarget) {
            const char *savetarget;

            savetarget = self->target;
            self->target = self->pathtarget;
            G_UseTargets(self, self->activator);
            self->target = savetarget;
        }

        if (!(self->spawnflags & SPAWNFLAG_TIMER_MULTI_USE))
            return;

        func_clock_reset(self);

        if (self->spawnflags & SPAWNFLAG_TIMER_START_OFF)
            return;
    }

    self->nextthink = level.time + SEC(1);
}

void USE(func_clock_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (!(self->spawnflags & SPAWNFLAG_TIMER_MULTI_USE))
        self->use = NULL;
    if (self->activator)
        return;
    self->activator = activator;
    self->think(self);
}

void SP_func_clock(edict_t *self)
{
    if (!self->target) {
        gi.dprintf("%s with no target\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    if ((self->spawnflags & SPAWNFLAG_TIMER_DOWN) && (!self->count)) {
        gi.dprintf("%s with no count\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    if ((self->spawnflags & SPAWNFLAG_TIMER_UP) && (!self->count))
        self->count = 60 * 60;

    func_clock_reset(self);

    self->think = func_clock_think;

    if (self->spawnflags & SPAWNFLAG_TIMER_START_OFF)
        self->use = func_clock_use;
    else
        self->nextthink = level.time + SEC(1);
}

//=================================================================================

#define SPAWNFLAG_TELEPORTER_NO_SOUND           1
#define SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT 2

void TOUCH(teleporter_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    edict_t *dest;

    if (!other->client)
        return;

    dest = G_Find(NULL, FOFS(targetname), self->target);
    if (!dest) {
        gi.dprintf("Couldn't find destination\n");
        return;
    }

    // ZOID
    CTFPlayerResetGrapple(other);
    // ZOID

    // unlink to make sure it can't possibly interfere with KillBox
    gi.unlinkentity(other);

    VectorCopy(dest->s.origin, other->s.origin);
    VectorCopy(dest->s.origin, other->s.old_origin);
    other->s.origin[2] += 10;

    // clear the velocity and hold them in place briefly
    VectorClear(other->velocity);
    other->client->ps.pmove.pm_time = 160 >> PM_TIME_SHIFT; // hold time
    other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

    // draw the teleport splash at source and on the player
    if (!(self->spawnflags & SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT)) {
        self->owner->s.event = EV_PLAYER_TELEPORT;
        other->s.event = EV_PLAYER_TELEPORT;
    } else {
        self->owner->s.event = EV_OTHER_TELEPORT;
        other->s.event = EV_OTHER_TELEPORT;
    }

    // set angles
    for (int i = 0; i < 3; i++)
        other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);

    VectorCopy(dest->s.angles, other->s.angles);
    VectorCopy(dest->s.angles, other->client->ps.viewangles);
    VectorCopy(dest->s.angles, other->client->v_angle);
    AngleVectors(other->client->v_angle, other->client->v_forward, NULL, NULL);

    gi.linkentity(other);

    // kill anything at the destination
    KillBox(other, !!other->client, MOD_TELEFRAG, false);

    // [Paril-KEX] move sphere, if we own it
    if (other->client->owned_sphere) {
        edict_t *sphere = other->client->owned_sphere;
        VectorCopy(other->s.origin, sphere->s.origin);
        sphere->s.origin[2] = other->absmax[2];
        sphere->s.angles[YAW] = other->s.angles[YAW];
        gi.linkentity(sphere);
    }
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16) NO_SOUND NO_TELEPORT_EFFECT N64_EFFECT
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
#define SPAWNFLAG_TEMEPORTER_N64_EFFECT 4

void SP_misc_teleporter(edict_t *ent)
{
    edict_t *trig;

    gi.setmodel(ent, "models/objects/dmspot/tris.md2");
    ent->s.skinnum = 1;
    if (level.is_n64 || (ent->spawnflags & SPAWNFLAG_TEMEPORTER_N64_EFFECT))
        ent->x.morefx = EFX_TELEPORTER2;
    else
        ent->s.effects = EF_TELEPORTER;
    if (!(ent->spawnflags & SPAWNFLAG_TELEPORTER_NO_SOUND))
        ent->s.sound = gi.soundindex("world/amb10.wav");
    ent->solid = SOLID_BBOX;

    VectorSet(ent->mins, -32, -32, -24);
    VectorSet(ent->maxs, 32, 32, -16);
    gi.linkentity(ent);

    // N64 has some of these for visual effects
    if (!ent->target)
        return;

    trig = G_Spawn();
    trig->touch = teleporter_touch;
    trig->solid = SOLID_TRIGGER;
    trig->target = ent->target;
    trig->owner = ent;
    VectorCopy(ent->s.origin, trig->s.origin);
    VectorSet(trig->mins, -8, -8, 8);
    VectorSet(trig->maxs, 8, 8, 24);
    gi.linkentity(trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(edict_t *ent)
{
    // Paril-KEX N64 doesn't display these
    if (level.is_n64)
        return;

    gi.setmodel(ent, "models/objects/dmspot/tris.md2");
    ent->s.skinnum = 0;
    ent->solid = SOLID_BBOX;
    VectorSet(ent->mins, -32, -32, -24);
    VectorSet(ent->maxs, 32, 32, -16);
    gi.linkentity(ent);
}

/*QUAKED misc_flare (1.0 1.0 0.0) (-32 -32 -32) (32 32 32) RED GREEN BLUE LOCK_ANGLE
Creates a flare seen in the N64 version.
*/

#define SPAWNFLAG_FLARE_RED         1
#define SPAWNFLAG_FLARE_GREEN       2
#define SPAWNFLAG_FLARE_BLUE        4
#define SPAWNFLAG_FLARE_LOCK_ANGLE  8

void USE(misc_flare_use)(edict_t *ent, edict_t *other, edict_t *activator)
{
    ent->svflags ^= SVF_NOCLIENT;
    gi.linkentity(ent);
}

void SP_misc_flare(edict_t *ent)
{
    ent->s.modelindex = 1;
    ent->s.renderfx = RF_FLARE;
    ent->solid = SOLID_NOT;
    ent->x.scale = st.radius;

    if (ent->spawnflags & SPAWNFLAG_FLARE_RED)
        ent->s.renderfx |= RF_SHELL_RED;

    if (ent->spawnflags & SPAWNFLAG_FLARE_GREEN)
        ent->s.renderfx |= RF_SHELL_GREEN;

    if (ent->spawnflags & SPAWNFLAG_FLARE_BLUE)
        ent->s.renderfx |= RF_SHELL_BLUE;

    if (ent->spawnflags & SPAWNFLAG_FLARE_LOCK_ANGLE)
        ent->s.renderfx |= RF_FLARE_LOCK_ANGLE;

    if (st.image && *st.image) {
        ent->s.renderfx |= RF_CUSTOMSKIN;
        ent->s.frame = gi.imageindex(st.image);
    }

    VectorSet(ent->mins, -32, -32, -32);
    VectorSet(ent->maxs, 32, 32, 32);

    ent->s.modelindex2 = st.fade_start_dist;
    ent->s.modelindex3 = st.fade_end_dist;

    if (ent->targetname)
        ent->use = misc_flare_use;

    gi.linkentity(ent);
}

void THINK(misc_hologram_think)(edict_t *ent)
{
    ent->s.angles[1] += 100 * FRAME_TIME_SEC;
    ent->nextthink = level.time + FRAME_TIME;
    ent->x.alpha = frandom2(0.2f, 0.6f);
}

/*QUAKED misc_hologram (1.0 1.0 0.0) (-16 -16 0) (16 16 32)
Ship hologram seen in the N64 version.
*/
void SP_misc_hologram(edict_t *ent)
{
    ent->solid = SOLID_NOT;
    ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
    VectorSet(ent->mins, -16, -16, 0);
    VectorSet(ent->maxs, 16, 16, 32);
    ent->x.morefx = EFX_HOLOGRAM;
    ent->think = misc_hologram_think;
    ent->nextthink = level.time + FRAME_TIME;
    ent->x.alpha = frandom2(0.2f, 0.6f);
    ent->x.scale = 0.75f;
    gi.linkentity(ent);
}


/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8) NO_EXPLODE
Lava Balls. Shamelessly copied from Quake 1, like N64 guys
probably did too.
*/

#define SPAWNFLAG_LAVABALL_NO_EXPLODE   1

void TOUCH(fire_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (self->spawnflags & SPAWNFLAG_LAVABALL_NO_EXPLODE) {
        G_FreeEdict(self);
        return;
    }

    if (other->takedamage)
        T_Damage(other, self, self, vec3_origin, self->s.origin, vec3_origin, 20, 0, DAMAGE_NONE, (mod_t) { MOD_EXPLOSIVE });

    if (gi.pointcontents(self->s.origin) & CONTENTS_LAVA)
        G_FreeEdict(self);
    else
        BecomeExplosion1(self);
}

void THINK(fire_fly)(edict_t *self)
{
    edict_t *fireball = G_Spawn();
    fireball->s.effects = EF_ROCKET | EF_GIB;
    fireball->s.renderfx = RF_MINLIGHT;
    fireball->solid = SOLID_BBOX;
    fireball->movetype = MOVETYPE_TOSS;
    fireball->clipmask = MASK_SHOT;
    fireball->velocity[0] = crandom() * 50;
    fireball->velocity[1] = crandom() * 50;
    fireball->velocity[2] = (self->speed * 1.75f) + (frandom() * 200);
    crandom_vec(fireball->avelocity, 360);
    fireball->classname = "fireball";
    gi.setmodel(fireball, "models/objects/gibs/sm_meat/tris.md2");
    VectorCopy(self->s.origin, fireball->s.origin);
    fireball->nextthink = level.time + SEC(5);
    fireball->think = G_FreeEdict;
    fireball->touch = fire_touch;
    fireball->spawnflags = self->spawnflags;
    gi.linkentity(fireball);
    self->nextthink = level.time + random_time_sec(0, 5);
}

void SP_misc_lavaball(edict_t *self)
{
    self->classname = "fireball";
    self->nextthink = level.time + random_time_sec(0, 5);
    self->think = fire_fly;
    if (!self->speed)
        self->speed = 185;
}

void SP_info_landmark(edict_t *self)
{
    VectorCopy(self->s.origin, self->absmin);
    VectorCopy(self->s.origin, self->absmax);
}

#include "m_player.h"

void USE(misc_player_mannequin_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->monsterinfo.aiflags |= AI_TARGET_ANGER;
    self->enemy = activator;

    switch (self->count) {
    case GESTURE_FLIP_OFF:
        self->s.frame = FRAME_flip01;
        self->monsterinfo.nextframe = FRAME_flip12;
        break;

    case GESTURE_SALUTE:
        self->s.frame = FRAME_salute01;
        self->monsterinfo.nextframe = FRAME_salute11;
        break;

    case GESTURE_TAUNT:
        self->s.frame = FRAME_taunt01;
        self->monsterinfo.nextframe = FRAME_taunt17;
        break;

    case GESTURE_WAVE:
        self->s.frame = FRAME_wave01;
        self->monsterinfo.nextframe = FRAME_wave11;
        break;

    case GESTURE_POINT:
        self->s.frame = FRAME_point01;
        self->monsterinfo.nextframe = FRAME_point12;
        break;
    }
}

void THINK(misc_player_mannequin_think)(edict_t *self)
{
    if (self->teleport_time <= level.time) {
        self->s.frame++;

        if (!(self->monsterinfo.aiflags & AI_TARGET_ANGER)) {
            if (self->s.frame > FRAME_stand40)
                self->s.frame = FRAME_stand01;
        } else {
            if (self->s.frame > self->monsterinfo.nextframe) {
                self->s.frame = FRAME_stand01;
                self->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
                self->enemy = NULL;
            }
        }

        self->teleport_time = level.time + HZ(10);
    }

    if (self->enemy) {
        vec3_t vec;
        VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
        self->ideal_yaw = vectoyaw(vec);
        M_ChangeYaw(self);
    }

    self->nextthink = level.time + FRAME_TIME;
}

static void SetupMannequinModel(edict_t *self, int model_type, const char *weapon, const char *skin)
{
    const char *model_name;
    const char *default_skin;

    switch (model_type) {
    case 1:
    default:
        self->s.skinnum = MAX_CLIENTS - 1;
        model_name = "female";
        default_skin = "venus";
        break;

    case 2:
        self->s.skinnum = MAX_CLIENTS - 2;
        model_name = "male";
        default_skin = "rampage";
        break;

    case 3:
        self->s.skinnum = MAX_CLIENTS - 3;
        model_name = "cyborg";
        default_skin = "oni911";
        break;
    }

    if (!weapon)
        weapon = "w_hyperblaster";
    if (!skin)
        skin = default_skin;

    self->model = G_CopyString(va("players/%s/tris.md2", model_name), TAG_LEVEL);
    self->s.modelindex2 = gi.modelindex(va("players/%s/%s.md2", model_name, weapon));
    gi.configstring(CS_PLAYERSKINS + self->s.skinnum, va("mannequin\\%s/%s", model_name, skin));
}

/*QUAKED misc_player_mannequin (1.0 1.0 0.0) (-32 -32 -32) (32 32 32)
    Creates a player mannequin that stands around.

    NOTE: this is currently very limited, and only allows one unique model
    from each of the three player model types.

 "distance"     - Sets the type of gesture mannequin when use when triggered
 "height"       - Sets the type of model to use ( valid numbers: 1 - 3 )
 "goals"        - Name of the weapon to use.
 "image"        - Name of the player skin to use.
 "radius"       - How much to scale the model in-game
*/
void SP_misc_player_mannequin(edict_t *self)
{
    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_BBOX;
    if (!ED_WasKeySpecified("effects"))
        self->s.effects = EF_NONE;
    if (!ED_WasKeySpecified("renderfx"))
        self->s.renderfx = RF_MINLIGHT;
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);
    self->yaw_speed = 30;
    self->ideal_yaw = 0;
    self->teleport_time = level.time + HZ(10);
    self->s.modelindex = MODELINDEX_PLAYER;
    self->count = st.distance;

    SetupMannequinModel(self, st.height, st.goals, st.image);

    self->x.scale = 1.0f;
    if (ai_model_scale->value > 0.0f)
        self->x.scale = ai_model_scale->value;
    else if (st.radius > 0.0f)
        self->x.scale = st.radius;

    VectorScale(self->mins, self->x.scale, self->mins);
    VectorScale(self->maxs, self->x.scale, self->maxs);

    self->think = misc_player_mannequin_think;
    self->nextthink = level.time + FRAME_TIME;

    if (self->targetname)
        self->use = misc_player_mannequin_use;

    gi.linkentity(self);
}

/*QUAKED misc_model (1 0 0) (-8 -8 -8) (8 8 8)
*/
void SP_misc_model(edict_t *ent)
{
    gi.setmodel(ent, ent->model);
    gi.linkentity(ent);
}
