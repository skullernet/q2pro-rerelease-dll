// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// dm_ball.c
// pmack
// june 98

#include "g_local.h"

// defines

#define SPAWNFLAG_DBALL_GOAL_TEAM1      0x0001
// unused; assumed by not being team1
//#define SPAWNFLAG_DBALL_GOAL_TEAM2    0x0002

// globals

static edict_t *dball_ball_entity;
static int      dball_ball_startpt_count;
static int      dball_team1_goalscore;
static int      dball_team2_goalscore;

static cvar_t  *dball_team1_skin;
static cvar_t  *dball_team2_skin;
static cvar_t  *goallimit;

// prototypes

void DBall_BallDie(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);
void DBall_BallRespawn(edict_t *self);

// **************************
// Game rules
// **************************

static int DBall_CheckDMRules(void)
{
    if (goallimit->integer) {
        if (dball_team1_goalscore >= goallimit->integer)
            gi.bprintf(PRINT_HIGH, "Team 1 Wins.\n");
        else if (dball_team2_goalscore >= goallimit->integer)
            gi.bprintf(PRINT_HIGH, "Team 2 Wins.\n");
        else
            return 0;

        EndDMLevel();
        return 1;
    }

    return 0;
}

static void DBall_ClientBegin(edict_t *ent)
{
#if 0
    int          team1, team2, unassigned;
    edict_t     *other;
    char        *p;
    static char  value[MAX_INFO_STRING];

    team1 = 0;
    team2 = 0;
    unassigned = 0;

    for (int j = 1; j <= game.maxclients; j++) {
        other = &g_edicts[j];
        if (!other->inuse)
            continue;
        if (!other->client)
            continue;
        if (other == ent) // don't count the new player
            continue;

        Q_strlcpy(value, Info_ValueForKey(other->client->pers.userinfo, "skin"), sizeof(value));
        p = strchr(value, '/');
        if (p) {
            if (!strcmp(dball_team1_skin->string, value))
                team1++;
            else if (!strcmp(dball_team2_skin->string, value))
                team2++;
            else
                unassigned++;
        } else
            unassigned++;
    }

    if (team1 > team2) {
        gi.dprintf("assigned to team 2\n");
        Info_SetValueForKey(ent->client->pers.userinfo, "skin", dball_team2_skin->string);
    } else {
        gi.dprintf("assigned to team 1\n");
        Info_SetValueForKey(ent->client->pers.userinfo, "skin", dball_team1_skin->string);
    }

    ClientUserinfoChanged(ent, ent->client->pers.userinfo);

    if (unassigned)
        gi.dprintf("%d unassigned players present!\n", unassigned);
#endif
}

static bool DBall_SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles, bool force_spawn)
{
#if 0
    edict_t *bestspot;
    float    bestdistance, bestplayerdistance;
    edict_t *spot;
    const char *spottype;
    char skin[MAX_INFO_STRING];

    Q_strlcpy(skin, Info_ValueForKey(ent->client->pers.userinfo, "skin"), sizeof(skin));
    if (!strcmp(dball_team1_skin->string, skin))
        spottype = "dm_dball_team1_start";
    else if (!strcmp(dball_team2_skin->string, skin))
        spottype = "dm_dball_team2_start";
    else
        spottype = "info_player_deathmatch";

    spot = NULL;
    bestspot = NULL;
    bestdistance = 0;
    while ((spot = G_Find(spot, FOFS(classname), spottype)) != NULL) {
        bestplayerdistance = PlayersRangeFromSpot(spot);

        if (bestplayerdistance > bestdistance) {
            bestspot = spot;
            bestdistance = bestplayerdistance;
        }
    }

    if (bestspot) {
        VectorCopy(bestspot->s.origin, origin);
        origin[2] += 9;
        VectorCopy(bestspot->s.angles, angles);
        return true;
    }

    // if we didn't find an appropriate spawnpoint, just
    // call the standard one.
#endif
    return SelectSpawnPoint(ent, origin, angles, force_spawn);
}

static void DBall_GameInit(void)
{
    // we don't want a minimum speed for friction to take effect.
    // this will allow any knockback to move stuff.
    gi.cvar_forceset("sv_stopspeed", "0");
    dball_team1_goalscore = 0;
    dball_team2_goalscore = 0;

    gi.cvar_forceset(g_no_mines->name, "1");
    gi.cvar_forceset(g_no_nukes->name, "1");
    gi.cvar_forceset(g_dm_no_stack_double->name, "1");
    gi.cvar_forceset(g_friendly_fire->name, "0");
    //gi.cvar_forceset(g_no_mines->name, "1"); note: skin teams gone...

    dball_team1_skin = gi.cvar("dball_team1_skin", "male/ctf_r", 0);
    dball_team2_skin = gi.cvar("dball_team2_skin", "male/ctf_b", 0);
    goallimit = gi.cvar("goallimit", "0", 0);
}

static void DBall_PostInitSetup(void)
{
    edict_t *e;

    e = NULL;
    // turn teleporter destinations nonsolid.
    while ((e = G_Find(e, FOFS(classname), "misc_teleporter_dest"))) {
        e->solid = SOLID_NOT;
        gi.linkentity(e);
    }

    // count the ball start points
    dball_ball_startpt_count = 0;
    e = NULL;
    while ((e = G_Find(e, FOFS(classname), "dm_dball_ball_start")))
        dball_ball_startpt_count++;

    if (dball_ball_startpt_count == 0)
        gi.dprintf("No Deathball start points!\n");
}

//==================
// DBall_ChangeDamage - half damage between players. full if it involves
//      the ball entity
//==================
static int DBall_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, mod_t mod)
{
    // cut player -> ball damage to 1
    if (targ == dball_ball_entity)
        return 1;

    // damage player -> player is halved
    if (attacker != dball_ball_entity)
        return damage / 2;

    return damage;
}

static int DBall_ChangeKnockback(edict_t *targ, edict_t *attacker, int knockback, mod_t mod)
{
    if (targ != dball_ball_entity)
        return knockback;

    if (knockback < 1) {
        // FIXME - these don't account for quad/double
        if (mod.id == MOD_ROCKET) // rocket
            knockback = 70;
        else if (mod.id == MOD_BFG_EFFECT) // bfg
            knockback = 90;
        else
            gi.dprintf("zero knockback, mod %d\n", mod.id);
    } else {
        // FIXME - change this to an array?
        switch (mod.id) {
        case MOD_BLASTER:
            knockback *= 3;
            break;
        case MOD_SHOTGUN:
            knockback = (knockback * 3) / 8;
            break;
        case MOD_SSHOTGUN:
            knockback = knockback / 3;
            break;
        case MOD_MACHINEGUN:
            knockback = (knockback * 3) / 2;
            break;
        case MOD_HYPERBLASTER:
            knockback *= 4;
            break;
        case MOD_GRENADE:
        case MOD_HANDGRENADE:
        case MOD_PROX:
        case MOD_G_SPLASH:
        case MOD_HG_SPLASH:
        case MOD_HELD_GRENADE:
        case MOD_TRACKER:
        case MOD_DISINTEGRATOR:
            knockback /= 2;
            break;
        case MOD_R_SPLASH:
            knockback = (knockback * 3) / 2;
            break;
        case MOD_RAILGUN:
        case MOD_HEATBEAM:
            knockback /= 3;
            break;
        default:
            break;
        }
    }

    return knockback;
}

// **************************
// Goals
// **************************

void TOUCH(DBall_GoalTouch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
#if 0
    char     value[MAX_INFO_STRING];
    int      team_score;
    int      scorechange;
    char    *p;
    edict_t *ent;

    if (other != dball_ball_entity)
        return;

    self->health = self->max_health;

    // determine which team scored, and bump the team score
    if (self->spawnflags & SPAWNFLAG_DBALL_GOAL_TEAM1) {
        dball_team1_goalscore += self->wait;
        team_score = 1;
    } else {
        dball_team2_goalscore += self->wait;
        team_score = 2;
    }

    // bump the score for everyone on the correct team.
    for (int j = 1; j <= game.maxclients; j++) {
        ent = &g_edicts[j];
        if (!ent->inuse)
            continue;
        if (!ent->client)
            continue;

        if (ent == other->enemy)
            scorechange = self->wait + 5;
        else
            scorechange = self->wait;

        Q_strlcpy(value, Info_ValueForKey(ent->client->pers.userinfo, "skin"), sizeof(value));
        p = strchr(value, '/');
        if (p) {
            if (!strcmp(dball_team1_skin->string, value)) {
                if (team_score == 1)
                    ent->client->resp.score += scorechange;
                else if (other->enemy == ent)
                    ent->client->resp.score -= scorechange;
            } else if (!strcmp(dball_team2_skin->string, value)) {
                if (team_score == 2)
                    ent->client->resp.score += scorechange;
                else if (other->enemy == ent)
                    ent->client->resp.score -= scorechange;
            } else
                gi.dprintf("unassigned player!!!!\n");
        }
    }

    if (other->enemy)
        gi.dprintf("score for team %d by %s\n", team_score, other->enemy->client->pers.netname);
    else
        gi.dprintf("score for team %d by someone\n", team_score);

    DBall_BallDie(other, other->enemy, other->enemy, 0, vec3_origin, MOD_SUICIDE);

    G_UseTargets(self, other);
#endif
}

// **************************
// Ball
// **************************

static edict_t *PickBallStart(edict_t *ent)
{
    int      which, current;
    edict_t *e;

    which = irandom1(dball_ball_startpt_count);
    e = NULL;
    current = 0;

    while ((e = G_Find(e, FOFS(classname), "dm_dball_ball_start"))) {
        current++;
        if (current == which)
            return e;
    }

    if (current == 0)
        gi.dprintf("No ball start points found!\n");

    return G_Find(NULL, FOFS(classname), "dm_dball_ball_start");
}

//==================
// DBall_BallTouch - if the ball hit another player, hurt them
//==================
void TOUCH(DBall_BallTouch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t dir;
    float  dot;
    float  speed;

    if (other->takedamage == false)
        return;

    // hit a player
    if (other->client) {
        speed = VectorLength(ent->velocity);
        if (speed > 0) {
            VectorSubtract(ent->s.origin, other->s.origin, dir);
            dot = DotProduct(dir, ent->velocity);

            if (dot > 0.7f) {
                T_Damage(other, ent, ent, vec3_origin, ent->s.origin, vec3_origin,
                         speed / 10, speed / 10, DAMAGE_NONE, (mod_t) { MOD_DBALL_CRUSH });
            }
        }
    }
}

//==================
// DBall_BallPain
//==================
void PAIN(DBall_BallPain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    self->enemy = other;
    self->health = self->max_health;
}

void DIE(DBall_BallDie)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // do the splash effect
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_DBALL_GOAL);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    VectorClear(self->s.angles);
    VectorClear(self->velocity);
    VectorClear(self->avelocity);

    // make it invisible and desolid until respawn time
    self->solid = SOLID_NOT;
    //  self->s.modelindex = 0;
    self->think = DBall_BallRespawn;
    self->nextthink = level.time + SEC(2);
    gi.linkentity(self);
}

void THINK(DBall_BallRespawn)(edict_t *self)
{
    edict_t *start;

    // do the splash effect
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_DBALL_GOAL);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    // move the ball and stop it
    start = PickBallStart(self);
    if (start) {
        VectorCopy(start->s.origin, self->s.origin);
        VectorCopy(start->s.origin, self->s.old_origin);
    }

    VectorClear(self->s.angles);
    VectorClear(self->velocity);
    VectorClear(self->avelocity);

    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/objects/dball/tris.md2");
    self->s.event = EV_PLAYER_TELEPORT;
    self->groundentity = NULL;

    gi.linkentity(self);

    // kill anything at the destination
    KillBox(self, false);
}

// ************************
// SPEED CHANGES
// ************************

#define SPAWNFLAG_DBALL_SPEED_ONEWAY    1

void TOUCH(DBall_SpeedTouch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    float  dot;
    vec3_t vel;

    if (other != dball_ball_entity)
        return;

    if (self->timestamp >= level.time)
        return;

    if (VectorLength(other->velocity) < 1)
        return;

    if (self->spawnflags & SPAWNFLAG_DBALL_SPEED_ONEWAY) {
        VectorNormalize2(other->velocity, vel);
        dot = DotProduct(vel, self->movedir);
        if (dot < 0.8f)
            return;
    }

    self->timestamp = level.time + SEC(self->delay);
    VectorScale(other->velocity, self->speed, other->velocity);
}

// ************************
// SPAWN FUNCTIONS
// ************************

/*QUAKED dm_dball_ball (1 .5 .5) (-48 -48 -48) (48 48 48) ONEWAY
Deathball Ball
*/
void SP_dm_dball_ball(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }

    dball_ball_entity = self;
    //dball_ball_startpt = self->s.origin;

    self->s.modelindex = gi.modelindex("models/objects/dball/tris.md2");
    VectorSet(self->mins, -32, -32, -32);
    VectorSet(self->maxs, 32, 32, 32);
    self->solid = SOLID_BBOX;
    self->movetype = MOVETYPE_NEWTOSS;
    self->clipmask = MASK_MONSTERSOLID;

    self->takedamage = true;
    self->mass = 50;
    self->health = 50000;
    self->max_health = 50000;
    self->pain = DBall_BallPain;
    self->die = DBall_BallDie;
    self->touch = DBall_BallTouch;

    gi.linkentity(self);
}

/*QUAKED dm_dball_team1_start (1 .5 .5) (-16 -16 -24) (16 16 32)
Deathball team 1 start point
*/
void SP_dm_dball_team1_start(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }
    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }
}

/*QUAKED dm_dball_team2_start (1 .5 .5) (-16 -16 -24) (16 16 32)
Deathball team 2 start point
*/
void SP_dm_dball_team2_start(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }
    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }
}

/*QUAKED dm_dball_ball_start (1 .5 .5) (-48 -48 -48) (48 48 48)
Deathball ball start point
*/
void SP_dm_dball_ball_start(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }
    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }
}

/*QUAKED dm_dball_speed_change (1 .5 .5) ? ONEWAY
Deathball ball speed changing field.

speed: multiplier for speed (.5 = half, 2 = double, etc) (default = double)
angle: used with ONEWAY so speed change is only one way.
delay: time between speed changes (default: 0.2 sec)
*/
void SP_dm_dball_speed_change(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }
    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }

    if (!self->speed)
        self->speed = 2;

    if (!self->delay)
        self->delay = 0.2f;

    self->touch = DBall_SpeedTouch;
    self->solid = SOLID_TRIGGER;
    self->movetype = MOVETYPE_NONE;
    self->svflags |= SVF_NOCLIENT;

    if (!VectorEmpty(self->s.angles))
        G_SetMovedir(self->s.angles, self->movedir);
    else
        VectorSet(self->movedir, 1, 0, 0);

    gi.setmodel(self, self->model);
    gi.linkentity(self);
}

/*QUAKED dm_dball_goal (1 .5 .5) ? TEAM1 TEAM2
Deathball goal

Team1/Team2 - beneficiary of this goal. when the ball enters this goal, the beneficiary team will score.

"wait": score to be given for this goal (default 10) player gets score+5.
*/
void SP_dm_dball_goal(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (gamerules->integer != RDM_DEATHBALL) {
        G_FreeEdict(self);
        return;
    }

    if (!self->wait)
        self->wait = 10;

    self->touch = DBall_GoalTouch;
    self->solid = SOLID_TRIGGER;
    self->movetype = MOVETYPE_NONE;
    self->svflags |= SVF_NOCLIENT;

    if (!VectorEmpty(self->s.angles))
        G_SetMovedir(self->s.angles, self->movedir);

    gi.setmodel(self, self->model);
    gi.linkentity(self);
}

const dm_game_rt DMGame_DBall = {
    .GameInit = DBall_GameInit,
    .ChangeKnockback = DBall_ChangeKnockback,
    .ChangeDamage = DBall_ChangeDamage,
    .ClientBegin = DBall_ClientBegin,
    .SelectSpawnPoint = DBall_SelectSpawnPoint,
    .PostInitSetup = DBall_PostInitSetup,
    .CheckDMRules = DBall_CheckDMRules,
};
