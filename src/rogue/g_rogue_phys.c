// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

//============
// ROGUE
/*
=============
SV_Physics_NewToss

Toss, bounce, and fly movement. When on ground and no velocity, do nothing. With velocity,
slide.
=============
*/
void SV_Physics_NewToss(edict_t *ent)
{
    trace_t  trace;
    vec3_t   move;
    edict_t *slave;
    bool     wasinwater;
    bool     isinwater;
    float    speed, newspeed;
    vec3_t   old_origin;

    // regular thinking
    SV_RunThink(ent);

    // if not a team captain, so movement will be handled elsewhere
    if (ent->flags & FL_TEAMSLAVE)
        return;

    wasinwater = ent->waterlevel;

    // find out what we're sitting on.
    VectorCopy(ent->s.origin, move);
    move[2] -= 0.25f;
    trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, move, ent, ent->clipmask);
    if (ent->groundentity && ent->groundentity->inuse)
        ent->groundentity = trace.ent;
    else
        ent->groundentity = NULL;

    // if we're sitting on something flat and have no velocity of our own, return.
    if (ent->groundentity && (trace.plane.normal[2] == 1.0f) && VectorEmpty(ent->velocity))
        return;

    // store the old origin
    VectorCopy(ent->s.origin, old_origin);

    SV_CheckVelocity(ent);

    // add gravity
    SV_AddGravity(ent);

    if (!VectorEmpty(ent->avelocity))
        SV_AddRotationalFriction(ent);

    // add friction
    speed = VectorLength(ent->velocity);
    if (ent->waterlevel) // friction for water movement
        newspeed = speed - (sv_waterfriction * 6 * ent->waterlevel);
    else if (!ent->groundentity) // friction for air movement
        newspeed = speed - sv_friction;
    else // use ground friction
        newspeed = speed - (sv_friction * 6);

    if (newspeed < 0)
        newspeed = 0;
    newspeed /= speed;
    VectorScale(ent->velocity, newspeed, ent->velocity);

    SV_FlyMove(ent, FRAME_TIME_SEC, ent->clipmask);
    gi.linkentity(ent);

    G_TouchTriggers(ent);

    // check for water transition
    wasinwater = (ent->watertype & MASK_WATER);
    ent->watertype = gi.pointcontents(ent->s.origin);
    isinwater = ent->watertype & MASK_WATER;

    if (isinwater)
        ent->waterlevel = WATER_FEET;
    else
        ent->waterlevel = WATER_NONE;

    if (!wasinwater && isinwater)
        gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
    else if (wasinwater && !isinwater)
        gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);

    // move teamslaves
    for (slave = ent->teamchain; slave; slave = slave->teamchain) {
        VectorCopy(ent->s.origin, slave->s.origin);
        gi.linkentity(slave);
    }
}

// ROGUE
//============
