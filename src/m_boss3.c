// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

boss3

==============================================================================
*/

#include "g_local.h"
#include "m_boss32.h"

void USE(Use_Boss3)(edict_t *self, edict_t *other, edict_t *activator)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_BOSSTPORT);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    // just hide, don't kill ent so we can trigger it again
    self->svflags |= SVF_NOCLIENT;
    self->solid = SOLID_NOT;
}

void THINK(Think_Boss3Stand)(edict_t *self)
{
    if (self->s.frame == FRAME_stand260)
        self->s.frame = FRAME_stand201;
    else
        self->s.frame++;
    self->nextthink = level.time + HZ(10);
}

/*QUAKED monster_boss3_stand (1 .5 0) (-32 -32 0) (32 32 90)

Just stands and cycles in one place until targeted, then teleports away.
*/
void SP_monster_boss3_stand(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->model = "models/monsters/boss3/rider/tris.md2";
    self->s.modelindex = gi.modelindex(self->model);
    self->s.frame = FRAME_stand201;

    gi.soundindex("misc/bigtele.wav");

    VectorSet(self->mins, -32, -32, 0);
    VectorSet(self->maxs, 32, 32, 90);

    self->use = Use_Boss3;
    self->think = Think_Boss3Stand;
    self->nextthink = level.time + FRAME_TIME;
    gi.linkentity(self);
}
