// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

void fire_blueblaster(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, effects_t effect)
{
    edict_t *bolt;
    trace_t  tr;

    bolt = G_Spawn();
    VectorCopy(start, bolt->s.origin);
    VectorCopy(start, bolt->s.old_origin);
    vectoangles(dir, bolt->s.angles);
    VectorScale(dir, speed, bolt->velocity);
    bolt->svflags |= SVF_PROJECTILE;
    bolt->movetype = MOVETYPE_FLYMISSILE;
    bolt->flags |= FL_DODGE;
    bolt->clipmask = MASK_PROJECTILE;
    bolt->solid = SOLID_BBOX;
    bolt->s.effects |= effect;
    bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
    bolt->s.skinnum = 1;
    bolt->s.sound = gi.soundindex("misc/lasfly.wav");
    bolt->owner = self;
    bolt->touch = blaster_touch;
    bolt->nextthink = level.time + SEC(2);
    bolt->think = G_FreeEdict;
    bolt->dmg = damage;
    bolt->classname = "bolt";
    bolt->style = MOD_BLUEBLASTER;
    gi.linkentity(bolt);

    tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, bolt->clipmask);
    if (tr.fraction < 1.0f) {
        VectorAdd(tr.endpos, tr.plane.normal, bolt->s.origin);
        bolt->touch(bolt, tr.ent, &tr, false);
    }
}

/*
fire_ionripper
*/

void THINK(ionripper_sparks)(edict_t *self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_WELDING_SPARKS);
    gi.WriteByte(0);
    gi.WritePosition(self->s.origin);
    gi.WriteDir(vec3_origin);
    gi.WriteByte(irandom2(0xe4, 0xe8));
    gi.multicast(self->s.origin, MULTICAST_PVS);

    G_FreeEdict(self);
}

void TOUCH(ionripper_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == self->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(self);
        return;
    }

    if (self->owner->client)
        PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

    if (!other->takedamage)
        return;

    T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr->plane.normal, self->dmg, 1, DAMAGE_ENERGY, (mod_t) { MOD_RIPPER });
    G_FreeEdict(self);
}

void fire_ionripper(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, effects_t effect)
{
    edict_t *ion;
    trace_t  tr;

    ion = G_Spawn();
    VectorCopy(start, ion->s.origin);
    VectorCopy(start, ion->s.old_origin);
    vectoangles(dir, ion->s.angles);
    VectorScale(dir, speed, ion->velocity);
    ion->movetype = MOVETYPE_WALLBOUNCE;
    ion->clipmask = MASK_PROJECTILE;

    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        ion->clipmask &= ~CONTENTS_PLAYER;

    ion->solid = SOLID_BBOX;
    ion->s.effects |= effect;
    ion->svflags |= SVF_PROJECTILE;
    ion->flags |= FL_DODGE;
    ion->s.renderfx |= RF_FULLBRIGHT;
    ion->s.modelindex = gi.modelindex("models/objects/boomrang/tris.md2");
    ion->s.sound = gi.soundindex("misc/lasfly.wav");
    ion->owner = self;
    ion->touch = ionripper_touch;
    ion->nextthink = level.time + SEC(3);
    ion->think = ionripper_sparks;
    ion->dmg = damage;
    ion->dmg_radius = 100;
    gi.linkentity(ion);

    tr = gi.trace(self->s.origin, NULL, NULL, ion->s.origin, ion, ion->clipmask);
    if (tr.fraction < 1.0f) {
        VectorAdd(tr.endpos, tr.plane.normal, ion->s.origin);
        ion->touch(ion, tr.ent, &tr, false);
    }
}

/*
fire_heat
*/

void THINK(heat_think)(edict_t *self)
{
    edict_t *target = NULL;
    edict_t *acquire = NULL;
    vec3_t   vec;
    float    len;
    float    oldlen = 0;
    float    dot, olddot = 1;

    vec3_t fwd;
    AngleVectors(self->s.angles, fwd, NULL, NULL);

    // try to stay on current target if possible
    if (self->enemy) {
        acquire = self->enemy;

        if (acquire->health <= 0 || !visible(self, acquire))
            self->enemy = acquire = NULL;
    }

    if (!acquire) {
        // acquire new target
        while ((target = findradius(target, self->s.origin, 1024)) != NULL) {
            if (self->owner == target)
                continue;
            if (!target->client)
                continue;
            if (target->health <= 0)
                continue;
            if (!visible(self, target))
                continue;

            VectorSubtract(self->s.origin, target->s.origin, vec);
            len = VectorNormalize(vec);
            dot = DotProduct(vec, fwd);

            // targets that require us to turn less are preferred
            if (dot >= olddot)
                continue;

            if (!acquire || dot < olddot || len < oldlen) {
                acquire = target;
                oldlen = len;
                olddot = dot;
            }
        }
    }

    if (acquire) {
        VectorSubtract(acquire->s.origin, self->s.origin, vec);
        VectorNormalize(vec);

        float t = self->accel;
        float d = DotProduct(vec, self->movedir);

        if (d < 0.45f && d > -0.45f)
            VectorInverse(vec);

        slerp(self->movedir, vec, t, self->movedir);
        VectorNormalize(self->movedir);
        vectoangles(self->movedir, self->s.angles);

        if (self->enemy != acquire) {
            gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/railgr1a.wav"), 1, 0.25f, 0);
            self->enemy = acquire;
        }
    } else
        self->enemy = NULL;

    VectorScale(self->movedir, self->speed, self->velocity);
    self->nextthink = level.time + FRAME_TIME;
}

void fire_heat(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius, int radius_damage, float turn_fraction)
{
    edict_t *heat;

    heat = G_Spawn();
    VectorCopy(start, heat->s.origin);
    VectorCopy(dir, heat->movedir);
    vectoangles(dir, heat->s.angles);
    VectorScale(dir, speed, heat->velocity);
    heat->flags |= FL_DODGE;
    heat->movetype = MOVETYPE_FLYMISSILE;
    heat->svflags |= SVF_PROJECTILE;
    heat->clipmask = MASK_PROJECTILE;
    heat->solid = SOLID_BBOX;
    heat->s.effects |= EF_ROCKET;
    heat->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
    heat->owner = self;
    heat->touch = rocket_touch;
    heat->speed = speed;
    heat->accel = turn_fraction;

    heat->nextthink = level.time + FRAME_TIME;
    heat->think = heat_think;

    heat->dmg = damage;
    heat->radius_dmg = radius_damage;
    heat->dmg_radius = damage_radius;
    heat->s.sound = gi.soundindex("weapons/rockfly.wav");

    if (visible(heat, self->enemy)) {
        heat->enemy = self->enemy;
        gi.sound(heat, CHAN_WEAPON, gi.soundindex("weapons/railgr1a.wav"), 1.f, 0.25f, 0);
    }

    gi.linkentity(heat);
}

/*
fire_plasma
*/

void TOUCH(plasma_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t origin;

    if (other == ent->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(ent);
        return;
    }

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // calculate position for the explosion entity
    VectorAdd(ent->s.origin, tr->plane.normal, origin);

    if (other->takedamage)
        T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr->plane.normal, ent->dmg, ent->dmg, DAMAGE_ENERGY, (mod_t) { MOD_PHALANX });

    T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, DAMAGE_ENERGY, (mod_t) { MOD_PHALANX });

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_PLASMA_EXPLOSION);
    gi.WritePosition(origin);
    gi.multicast(ent->s.origin, MULTICAST_PHS);

    G_FreeEdict(ent);
}

void fire_plasma(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
    edict_t *plasma;

    plasma = G_Spawn();
    VectorCopy(start, plasma->s.origin);
    VectorCopy(dir, plasma->movedir);
    vectoangles(dir, plasma->s.angles);
    VectorScale(dir, speed, plasma->velocity);
    plasma->movetype = MOVETYPE_FLYMISSILE;
    plasma->clipmask = MASK_PROJECTILE;

    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        plasma->clipmask &= ~CONTENTS_PLAYER;

    plasma->solid = SOLID_BBOX;
    plasma->svflags |= SVF_PROJECTILE;
    plasma->flags |= FL_DODGE;
    plasma->owner = self;
    plasma->touch = plasma_touch;
    plasma->nextthink = level.time + SEC(8000.0f / speed);
    plasma->think = G_FreeEdict;
    plasma->dmg = damage;
    plasma->radius_dmg = radius_damage;
    plasma->dmg_radius = damage_radius;
    plasma->s.sound = gi.soundindex("weapons/rockfly.wav");

    plasma->s.modelindex = gi.modelindex("sprites/s_photon.sp2");
    plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

    gi.linkentity(plasma);
}

void THINK(Trap_Gib_Think)(edict_t *ent)
{
    if (ent->owner->s.frame != 5) {
        G_FreeEdict(ent);
        return;
    }

    vec3_t forward, right, up;
    vec3_t vec;

    AngleVectors(ent->owner->s.angles, forward, right, up);

    // rotate us around the center
    float degrees = (150 * FRAME_TIME_SEC) + ent->owner->delay;
    vec3_t diff;

    VectorSubtract(ent->owner->s.origin, ent->s.origin, diff);
    RotatePointAroundVector(vec, up, diff, degrees);

    ent->s.angles[1] += degrees;

    VectorSubtract(ent->owner->s.origin, vec, vec);

    trace_t tr = gi.trace(ent->s.origin, NULL, NULL, vec, ent, MASK_SOLID);
    VectorCopy(tr.endpos, ent->s.origin);

    // pull us towards the trap's center
    VectorNormalize(diff);
    VectorMA(ent->s.origin, 15.0f * FRAME_TIME_SEC, diff, ent->s.origin);

    ent->watertype = gi.pointcontents(ent->s.origin);
    if (ent->watertype & MASK_WATER)
        ent->waterlevel = WATER_FEET;

    ent->nextthink = level.time + FRAME_TIME;
    gi.linkentity(ent);
}

void DIE(trap_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    BecomeExplosion1(self);
}

void SP_item_foodcube(edict_t *best);
void SpawnDamage(int type, const vec3_t origin, const vec3_t normal, int damage);
bool player_start_point(edict_t *ent);

void THINK(Trap_Think)(edict_t *ent)
{
    edict_t *target = NULL;
    edict_t *best = NULL;
    vec3_t   vec;
    float    len;
    float    oldlen = 8000;

    if (ent->timestamp < level.time) {
        BecomeExplosion1(ent);
        // note to self
        // cause explosion damage???
        return;
    }

    ent->nextthink = level.time + HZ(10);

    if (!ent->groundentity)
        return;

    // ok lets do the blood effect
    if (ent->s.frame > 4) {
        if (ent->s.frame == 5) {
            bool spawn = ent->wait == 64;

            ent->wait -= 2;

            if (spawn)
                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trapdown.wav"), 1, ATTN_IDLE, 0);

            ent->delay += 2;

            if (ent->wait < 19)
                ent->s.frame++;

            return;
        }
        ent->s.frame++;
        if (ent->s.frame == 8) {
            ent->nextthink = level.time + SEC(1);
            ent->think = G_FreeEdict;
            ent->s.effects &= ~EF_TRAP;

            edict_t *cube = G_Spawn();
            cube->count = ent->mass;
            cube->x.scale = 1.0f + (ent->accel - 100.0f) / 300.0f;
            SP_item_foodcube(cube);
            VectorCopy(ent->s.origin, cube->s.origin);
            cube->s.origin[2] += 24 * cube->x.scale;
            cube->s.angles[YAW] = frandom() * 360;
            cube->velocity[2] = 400;
            cube->think(cube);
            cube->nextthink = 0;
            gi.linkentity(cube);

            gi.sound(best, CHAN_AUTO, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);

            return;
        }
        return;
    }

    ent->s.effects &= ~EF_TRAP;
    if (ent->s.frame >= 4) {
        ent->s.effects |= EF_TRAP;
        // clear the owner if in deathmatch
        if (deathmatch->integer)
            ent->owner = NULL;
    }

    if (ent->s.frame < 4) {
        ent->s.frame++;
        return;
    }

    while ((target = findradius(target, ent->s.origin, 256)) != NULL) {
        if (target == ent)
            continue;
        if (!target->classname)
            continue;

        // [Paril-KEX] don't allow traps to be placed near flags or teleporters
        // if it's a monster or player with health > 0
        // or it's a player start point
        // and we can see it
        // blow up
        if (player_start_point(target) && visible(target, ent)) {
            BecomeExplosion1(ent);
            return;
        }

        if (!(target->svflags & SVF_MONSTER) && !target->client)
            continue;
        if (target != ent->teammaster && CheckTeamDamage(target, ent->teammaster))
            continue;
        // [Paril-KEX]
        if (!deathmatch->integer && target->client)
            continue;
        if (target->health <= 0)
            continue;
        if (!visible(ent, target))
            continue;
        len = Distance(ent->s.origin, target->s.origin);
        if (!best || len < oldlen) {
            oldlen = len;
            best = target;
        }
    }

    // pull the enemy in
    if (!best)
        return;

    if (best->groundentity) {
        best->s.origin[2] += 1;
        best->groundentity = NULL;
    }

    VectorSubtract(ent->s.origin, best->s.origin, vec);
    len = VectorNormalize(vec);

    float max_speed = best->client ? 290 : 150;
    float speed = max(max_speed - len, 64);

    VectorMA(best->velocity, speed, vec, best->velocity);

    ent->s.sound = gi.soundindex("weapons/trapsuck.wav");

    if (len >= 48)
        return;

    if (best->mass >= 400) {
        BecomeExplosion1(ent);
        // note to self
        // cause explosion damage???
        return;
    }

    ent->takedamage = false;
    ent->solid = SOLID_NOT;
    ent->die = NULL;

    T_Damage(best, ent, ent->teammaster, vec3_origin, best->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_TRAP });

    if (best->svflags & SVF_MONSTER)
        M_ProcessPain(best);

    ent->enemy = best;
    ent->wait = 64;
    VectorCopy(ent->s.origin, ent->s.old_origin);
    ent->timestamp = level.time + SEC(30);
    ent->accel = best->mass;
    if (deathmatch->integer)
        ent->mass = best->mass / 4;
    else
        ent->mass = best->mass / 10;
    // ok spawn the food cube
    ent->s.frame = 5;

    // link up any gibs that this monster may have spawned
    for (int i = game.maxclients + 1; i < globals.num_edicts; i++) {
        edict_t *e = &g_edicts[i];

        if (!e->inuse)
            continue;
        if (strcmp(e->classname, "gib"))
            continue;
        if (Distance(e->s.origin, ent->s.origin) > 128)
            continue;

        e->movetype = MOVETYPE_NONE;
        e->nextthink = level.time + FRAME_TIME;
        e->think = Trap_Gib_Think;
        e->owner = ent;
        Trap_Gib_Think(e);
    }
}

// RAFAEL
void fire_trap(edict_t *self, const vec3_t start, const vec3_t aimdir, int speed)
{
    edict_t *trap;
    vec3_t   dir;
    vec3_t   forward, right, up;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    trap = G_Spawn();
    VectorCopy(start, trap->s.origin);
    VectorScale(aimdir, speed, trap->velocity);

    float scale = (200 + crandom() * 10.0f) * (level.gravity / 800.0f);
    VectorMA(trap->velocity, scale, up, trap->velocity);

    scale = crandom() * 10.0f;
    VectorMA(trap->velocity, scale, right, trap->velocity);

    VectorSet(trap->avelocity, 0, 300, 0);
    trap->movetype = MOVETYPE_BOUNCE;
    trap->solid = SOLID_BBOX;
    trap->takedamage = true;
    VectorSet(trap->mins, -4, -4, 0);
    VectorSet(trap->maxs, 4, 4, 8);
    trap->die = trap_die;
    trap->health = 20;
    trap->s.modelindex = gi.modelindex("models/weapons/z_trap/tris.md2");
    trap->owner = trap->teammaster = self;
    trap->nextthink = level.time + SEC(1);
    trap->think = Trap_Think;
    trap->classname = "food_cube_trap";
    // RAFAEL 16-APR-98
    trap->s.sound = gi.soundindex("weapons/traploop.wav");
    // END 16-APR-98

    trap->flags |= (FL_DAMAGEABLE | FL_MECHANICAL | FL_TRAP);
    trap->clipmask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;

    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        trap->clipmask &= ~CONTENTS_PLAYER;

    gi.linkentity(trap);

    trap->timestamp = level.time + SEC(30);
}
