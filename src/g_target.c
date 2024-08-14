// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
Fire an origin based temp entity event to the clients.
"style"     type byte
*/
void USE(Use_Target_Tent)(edict_t *ent, edict_t *other, edict_t *activator)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(ent->style);
    gi.WritePosition(ent->s.origin);
    gi.multicast(ent->s.origin, MULTICAST_PVS);
}

void SP_target_temp_entity(edict_t *ent)
{
    if (level.is_n64 && ent->style == 27)
        ent->style = TE_TELEPORT_EFFECT;

    ent->use = Use_Target_Tent;
}

//==========================================================

//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable
"noise"     wav file to play
"attenuation"
-1 = none, send to whole level
1 = normal fighting sounds
2 = idle sound level
3 = ambient sound level
"volume"    0.0 to 1.0

Normal sounds play each time the target is used.  The reliable flag can be set for crucial voiceovers.

[Paril-KEX] looped sounds are by default atten 3 / vol 1, and the use function toggles it on/off.
*/

#define SPAWNFLAG_SPEAKER_LOOPED_ON     1
#define SPAWNFLAG_SPEAKER_LOOPED_OFF    2
#define SPAWNFLAG_SPEAKER_RELIABLE      4
#define SPAWNFLAG_SPEAKER_NO_STEREO     8

void USE(Use_Target_Speaker)(edict_t *ent, edict_t *other, edict_t *activator)
{
    soundchan_t chan;

    if (ent->spawnflags & (SPAWNFLAG_SPEAKER_LOOPED_ON | SPAWNFLAG_SPEAKER_LOOPED_OFF)) {
        // looping sound toggles
        if (ent->s.sound)
            ent->s.sound = 0; // turn it off
        else
            ent->s.sound = ent->noise_index; // start it
    } else {
        // normal sound
        if (ent->spawnflags & SPAWNFLAG_SPEAKER_RELIABLE)
            chan = CHAN_VOICE | CHAN_RELIABLE;
        else
            chan = CHAN_VOICE;
        // use a positioned_sound, because this entity won't normally be
        // sent to any clients because it is invisible
        gi.positioned_sound(ent->s.origin, ent, chan, ent->noise_index, ent->volume, ent->attenuation, 0);
    }
}

void SP_target_speaker(edict_t *ent)
{
    if (!st.noise) {
        gi.dprintf("%s: no noise set\n", etos(ent));
        return;
    }

    if (!strstr(st.noise, ".wav"))
        ent->noise_index = gi.soundindex(va("%s.wav", st.noise));
    else
        ent->noise_index = gi.soundindex(st.noise);

    if (!ent->volume)
        ent->volume = 1;
    if (ent->volume != 1)
        ent->x.loop_volume = ent->volume;

    if (!ent->attenuation) {
        if (ent->spawnflags & (SPAWNFLAG_SPEAKER_LOOPED_OFF | SPAWNFLAG_SPEAKER_LOOPED_ON))
            ent->attenuation = ATTN_STATIC;
        else
            ent->attenuation = ATTN_NORM;
    } else if (ent->attenuation == -1) { // use -1 so 0 defaults to 1
        if (ent->spawnflags & (SPAWNFLAG_SPEAKER_LOOPED_OFF | SPAWNFLAG_SPEAKER_LOOPED_ON)) {
            ent->attenuation = ATTN_LOOP_NONE;
            ent->svflags |= SVF_NOCULL;
        } else
            ent->attenuation = ATTN_NONE;
    }

    if (ent->attenuation != ATTN_STATIC)
        ent->x.loop_attenuation = ent->attenuation;

    // check for prestarted looping sound
    if (ent->spawnflags & SPAWNFLAG_SPEAKER_LOOPED_ON)
        ent->s.sound = ent->noise_index;

    if (ent->spawnflags & SPAWNFLAG_SPEAKER_NO_STEREO)
        ent->s.renderfx |= RF_NO_STEREO;

    ent->use = Use_Target_Speaker;

    // must link the entity so we get areas and clusters so
    // the server can determine who to send updates to
    gi.linkentity(ent);
}

//==========================================================

#define SPAWNFLAG_HELP_HELP1    1
#define SPAWNFLAG_SET_POI       2

void target_poi_use(edict_t *ent, edict_t *other, edict_t *activator);

void USE(Use_Target_Help)(edict_t *ent, edict_t *other, edict_t *activator)
{
    if (ent->spawnflags & SPAWNFLAG_HELP_HELP1) {
        if (strcmp(game.helpmessage1, ent->message)) {
            Q_strlcpy(game.helpmessage1, ent->message, sizeof(game.helpmessage1));
            game.help1changed++;
        }
    } else {
        if (strcmp(game.helpmessage2, ent->message)) {
            Q_strlcpy(game.helpmessage2, ent->message, sizeof(game.helpmessage2));
            game.help2changed++;
        }
    }

    if (ent->spawnflags & SPAWNFLAG_SET_POI)
        target_poi_use(ent, other, activator);
}

/*QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1 setpoi
When fired, the "message" key becomes the current personal computer string, and the message light will be set on all clients status bars.
*/
void SP_target_help(edict_t *ent)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(ent);
        return;
    }

    if (!ent->message) {
        gi.dprintf("%s: no message\n", etos(ent));
        G_FreeEdict(ent);
        return;
    }

    ent->use = Use_Target_Help;

    if (ent->spawnflags & SPAWNFLAG_SET_POI) {
        if (st.image)
            ent->noise_index = gi.imageindex(st.image);
        else
            ent->noise_index = gi.imageindex("friend");
    }
}

//==========================================================

/*QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)
Counts a secret found.
These are single use targets.
*/
void USE(use_target_secret)(edict_t *ent, edict_t *other, edict_t *activator)
{
    gi.sound(ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

    level.found_secrets++;

    G_UseTargets(ent, activator);
    G_FreeEdict(ent);
}

void THINK(G_VerifyTargetted)(edict_t *ent)
{
    if (!ent->targetname || !*ent->targetname)
        gi.dprintf("WARNING: missing targetname on %s\n", etos(ent));
    else if (!G_Find(NULL, FOFS(target), ent->targetname))
        gi.dprintf("WARNING: doesn't appear to be anything targeting %s\n", etos(ent));
}

void SP_target_secret(edict_t *ent)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(ent);
        return;
    }

    ent->think = G_VerifyTargetted;
    ent->nextthink = level.time + HZ(10);

    ent->use = use_target_secret;
    if (!st.noise)
        st.noise = "misc/secret.wav";
    ent->noise_index = gi.soundindex(st.noise);
    ent->svflags = SVF_NOCLIENT;
    level.total_secrets++;
}

//==========================================================
// [Paril-KEX] notify this player of a goal change
void G_PlayerNotifyGoal(edict_t *player)
{
    // no goals in DM
    if (deathmatch->integer)
        return;

    if (!player->client->pers.spawned)
        return;
    if ((level.time - player->client->resp.entertime) < SEC(0.3f))
        return;

    // N64 goals
    if (level.goals) {
        // if the goal has updated, commit it first
        if (game.help1changed != game.help2changed) {
            const char *current_goal = level.goals;

            // skip ahead by the number of goals we've finished
            for (int i = 0; i < level.goal_num; i++) {
                while (*current_goal && *current_goal != '\t')
                    current_goal++;

                if (!*current_goal)
                    gi.error("invalid n64 goals; tell Paril\n");

                current_goal++;
            }

            // find the end of this goal
            const char *goal_end = current_goal;

            while (*goal_end && *goal_end != '\t')
                goal_end++;

            Q_strlcpy(game.helpmessage1, current_goal, min(goal_end - current_goal + 1, sizeof(game.helpmessage1)));

            // translate it
            const char *s = game.helpmessage1;
            if (*s == '$' && (s = G_GetL10nString(s + 1)))
                Q_strlcpy(game.helpmessage1, s, sizeof(game.helpmessage1));

            game.help2changed = game.help1changed;
        }

        if (player->client->pers.game_help1changed != game.help1changed) {
            gi.cprintf(player, PRINT_TYPEWRITER, "%s", game.helpmessage1);
            gix.local_sound(player, NULL, player, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk.wav"), 1.0f, ATTN_NONE, 0.0f);

            player->client->pers.game_help1changed = game.help1changed;
        }

        // no regular goals
        return;
    }

    if (player->client->pers.game_help1changed != game.help1changed) {
        player->client->pers.game_help1changed = game.help1changed;
        player->client->pers.helpchanged = 1;
        player->client->pers.help_time = level.time + SEC(5);

        if (*game.helpmessage1)
            // [Sam-KEX] Print objective to screen
            gi.cprintf(player, PRINT_TYPEWRITER, "Primary Objective:\n%s", game.helpmessage1);
    }

    if (player->client->pers.game_help2changed != game.help2changed) {
        player->client->pers.game_help2changed = game.help2changed;
        player->client->pers.helpchanged = 1;
        player->client->pers.help_time = level.time + SEC(5);

        if (*game.helpmessage2)
            // [Sam-KEX] Print objective to screen
            gi.cprintf(player, PRINT_TYPEWRITER, "Secondary Objective:\n%s", game.helpmessage2);
    }
}

/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8) KEEP_MUSIC
Counts a goal completed.
These are single use targets.
*/
#define SPAWNFLAG_GOAL_KEEP_MUSIC   1

void USE(use_target_goal)(edict_t *ent, edict_t *other, edict_t *activator)
{
    gi.sound(ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

    level.found_goals++;

    if (level.found_goals == level.total_goals && !(ent->spawnflags & SPAWNFLAG_GOAL_KEEP_MUSIC))
        gi.configstring(CS_CDTRACK, va("%d", ent->sounds));

    // [Paril-KEX] n64 goals
    if (level.goals) {
        level.goal_num++;
        game.help1changed++;

        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (player->inuse)
                G_PlayerNotifyGoal(player);
        }
    }

    G_UseTargets(ent, activator);
    G_FreeEdict(ent);
}

void SP_target_goal(edict_t *ent)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(ent);
        return;
    }

    ent->use = use_target_goal;
    if (!st.noise)
        st.noise = "misc/secret.wav";
    ent->noise_index = gi.soundindex(st.noise);
    ent->svflags = SVF_NOCLIENT;
    level.total_goals++;
}

//==========================================================

/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
Spawns an explosion temporary entity when used.

"delay"     wait this long before going off
"dmg"       how much radius damage should be done, defaults to 0
*/
void THINK(target_explosion_explode)(edict_t *self)
{
    float save;

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    T_RadiusDamage(self, self->activator, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, (mod_t) { MOD_EXPLOSIVE });

    save = self->delay;
    self->delay = 0;
    G_UseTargets(self, self->activator);
    self->delay = save;
}

void USE(use_target_explosion)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->activator = activator;

    if (!self->delay) {
        target_explosion_explode(self);
        return;
    }

    self->think = target_explosion_explode;
    self->nextthink = level.time + SEC(self->delay);
}

void SP_target_explosion(edict_t *ent)
{
    ent->use = use_target_explosion;
    ent->svflags = SVF_NOCLIENT;
}

//==========================================================

/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8) END_OF_UNIT UNKNOWN UNKNOWN CLEAR_INVENTORY NO_END_OF_UNIT FADE_OUT IMMEDIATE_LEAVE
Changes level to "map" when fired
*/
void USE(use_target_changelevel)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (level.intermissiontime)
        return; // already activated

    if (!deathmatch->integer && !coop->integer && g_edicts[1].health <= 0)
        return;

    // if noexit, do a ton of damage to other
    if (deathmatch->integer && !g_dm_allow_exit->integer && other != world) {
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 10 * other->max_health, 1000, DAMAGE_NONE, (mod_t) { MOD_EXIT });
        return;
    }

    // if multiplayer, let everyone know who hit the exit
    if (deathmatch->integer) {
        if (level.time < SEC(10))
            return;

        if (activator && activator->client)
            gi.bprintf(PRINT_HIGH, "%s exited the level.\n", activator->client->pers.netname);
    }

    // if going to a new unit, clear cross triggers
    if (strstr(self->map, "*"))
        game.cross_level_flags &= ~(SFL_CROSS_TRIGGER_MASK);

    // if map has a landmark, store position instead of using spawn next map
    if (activator && activator->client && !deathmatch->integer) {
        gclient_t *client = activator->client;

        if (client->landmark_name) {
            gi.TagFree(client->landmark_name);
            client->landmark_name = NULL;
        }
        VectorClear(client->landmark_rel_pos);

        self->target_ent = G_PickTarget(self->target);
        if (self->target_ent) {
            client->landmark_name = G_CopyString(self->target_ent->targetname, TAG_GAME);

            // get relative vector to landmark pos, and unrotate by the landmark angles in preparation to be
            // rotated by the next map
            VectorSubtract(activator->s.origin, self->target_ent->s.origin, client->landmark_rel_pos);

            vec3_t axis[3];
            AnglesToAxis(self->target_ent->s.angles, axis);
            TransposeAxis(axis);
            RotatePoint(client->landmark_rel_pos, axis);
            RotatePoint(client->oldvelocity, axis);

            // unrotate our view angles for the next map too
            VectorSubtract(client->ps.viewangles, self->target_ent->s.angles, client->oldviewangles);
        }
    }

    BeginIntermission(self);
}

void SP_target_changelevel(edict_t *ent)
{
    if (!ent->map) {
        gi.dprintf("%s: no map\n", etos(ent));
        G_FreeEdict(ent);
        return;
    }

    ent->use = use_target_changelevel;
    ent->svflags = SVF_NOCLIENT;
}

//==========================================================

/*QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
Creates a particle splash effect when used.

Set "sounds" to one of the following:
  1) sparks
  2) blue water
  3) brown water
  4) slime
  5) lava
  6) blood

"count" how many pixels in the splash
"dmg"   if set, does a radius damage at this location when it splashes
        useful for lava/sparks
*/

void USE(use_target_splash)(edict_t *self, edict_t *other, edict_t *activator)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_SPLASH);
    gi.WriteByte(self->count);
    gi.WritePosition(self->s.origin);
    gi.WriteDir(self->movedir);
    gi.WriteByte(self->sounds);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    if (self->dmg)
        T_RadiusDamage(self, activator, self->dmg, NULL, self->dmg + 40, DAMAGE_NONE, (mod_t) { MOD_SPLASH });
}

void SP_target_splash(edict_t *self)
{
    self->use = use_target_splash;
    G_SetMovedir(self->s.angles, self->movedir);

    if (!self->count)
        self->count = 32;

    // N64 "sparks" are blue, not yellow.
    if (level.is_n64 && self->sounds == 1)
        self->sounds = 7;

    self->svflags = SVF_NOCLIENT;
}

//==========================================================

/*QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
Set target to the type of entity you want spawned.
Useful for spawning monsters and gibs in the factory levels.

For monsters:
    Set direction to the facing you want it to have.

For gibs:
    Set direction if you want it moving and
    speed how fast it should be moving otherwise it
    will just be dropped
*/
void ED_CallSpawn(edict_t *ent);

void USE(use_target_spawner)(edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t *ent;

    ent = G_Spawn();
    ent->classname = self->target;
    // RAFAEL
    ent->flags = self->flags;
    // RAFAEL
    VectorCopy(self->s.origin, ent->s.origin);
    VectorCopy(self->s.angles, ent->s.angles);

    // [Paril-KEX] although I fixed these in our maps, this is just
    // in case anybody else does this by accident. Don't count these monsters
    // so they don't inflate the monster count.
    ent->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

    ED_InitSpawnVars();
    ED_CallSpawn(ent);
    gi.linkentity(ent);

    KillBox(ent, false, MOD_TELEFRAG, false);
    if (self->speed)
        VectorCopy(self->movedir, ent->velocity);

    ent->s.renderfx |= RF_IR_VISIBLE; // PGM
}

void SP_target_spawner(edict_t *self)
{
    self->use = use_target_spawner;
    self->svflags = SVF_NOCLIENT;
    if (self->speed) {
        G_SetMovedir(self->s.angles, self->movedir);
        VectorScale(self->movedir, self->speed, self->movedir);
    }
}

//==========================================================

/*QUAKED target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
Fires a blaster bolt in the set direction when triggered.

dmg     default is 15
speed   default is 1000
*/

#define SPAWNFLAG_BLASTER_NOTRAIL   1
#define SPAWNFLAG_BLASTER_NOEFFECTS 2

void USE(use_target_blaster)(edict_t *self, edict_t *other, edict_t *activator)
{
    effects_t effect;

    if (self->spawnflags & SPAWNFLAG_BLASTER_NOEFFECTS)
        effect = EF_NONE;
    else if (self->spawnflags & SPAWNFLAG_BLASTER_NOTRAIL)
        effect = EF_HYPERBLASTER;
    else
        effect = EF_BLASTER;

    fire_blaster(self, self->s.origin, self->movedir, self->dmg, self->speed, effect, (mod_t) { MOD_TARGET_BLASTER });
    gi.sound(self, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);
}

void SP_target_blaster(edict_t *self)
{
    self->use = use_target_blaster;
    G_SetMovedir(self->s.angles, self->movedir);
    self->noise_index = gi.soundindex("weapons/laser2.wav");

    if (!self->dmg)
        self->dmg = 15;
    if (!self->speed)
        self->speed = 1000;

    self->svflags = SVF_NOCLIENT;
}

//==========================================================

/*QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crosslevel_target with the same trigger number is automatically used when a level is started within the same unit.  It is OK to check multiple triggers.  Message, delay, target, and killtarget also work.
*/
void USE(trigger_crosslevel_trigger_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    game.cross_level_flags |= self->spawnflags;
    G_FreeEdict(self);
}

void SP_target_crosslevel_trigger(edict_t *self)
{
    self->svflags = SVF_NOCLIENT;
    self->use = trigger_crosslevel_trigger_use;
}

/*QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8 - - - - - - - - trigger9 trigger10 trigger11 trigger12 trigger13 trigger14 trigger15 trigger16
Triggered by a trigger_crosslevel elsewhere within a unit.  If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

"delay"     delay before using targets if the trigger has been activated (default 1)
*/
void THINK(target_crosslevel_target_think)(edict_t *self)
{
    if (self->spawnflags == (game.cross_level_flags & SFL_CROSS_TRIGGER_MASK & self->spawnflags)) {
        G_UseTargets(self, self);
        G_FreeEdict(self);
    }
}

void SP_target_crosslevel_target(edict_t *self)
{
    if (!self->delay)
        self->delay = 1;
    self->svflags = SVF_NOCLIENT;

    self->think = target_crosslevel_target_think;
    self->nextthink = level.time + SEC(self->delay);
}

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT WINDOWSTOP
When triggered, fires a laser.  You can either set a target or a direction.

WINDOWSTOP - stops at CONTENTS_WINDOW
*/

//======
// PGM
#define SPAWNFLAG_LASER_STOPWINDOW  0x0080
// PGM
//======

void THINK(target_laser_think)(edict_t *self)
{
    int count;

    if (self->spawnflags & SPAWNFLAG_LASER_ZAP)
        count = 8;
    else
        count = 4;

    if (self->enemy) {
        vec3_t last_movedir, point;
        VectorCopy(self->movedir, last_movedir);
        VectorAvg(self->enemy->absmin, self->enemy->absmax, point);
        VectorSubtract(point, self->s.origin, self->movedir);
        VectorNormalize(self->movedir);
        if (!VectorCompare(self->movedir, last_movedir))
            self->spawnflags |= SPAWNFLAG_LASER_ZAP;
    }

    vec3_t start, end;
    VectorCopy(self->s.origin, start);
    VectorMA(start, 2048, self->movedir, end);

    pierce_t pierce;
    trace_t tr;
    bool damaged_thing = false;

    contents_t mask = (self->spawnflags & SPAWNFLAG_LASER_STOPWINDOW) ? MASK_SHOT : (CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER);

    pierce_begin(&pierce);

    do {
        tr = gi.trace(start, NULL, NULL, end, self, mask);

        // didn't hit anything, so we're done
        if (!tr.ent || tr.fraction == 1.0f)
            break;

        // hurt it if we can
        if (self->dmg > 0 && (tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && self->damage_debounce_time <= level.time) {
            damaged_thing = true;
            T_Damage(tr.ent, self, self->activator, self->movedir, tr.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, (mod_t) { MOD_TARGET_LASER });
        }

        // if we hit something that's not a monster or player or is immune to lasers, we're done
        // ROGUE
        if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client) && !(tr.ent->flags & FL_DAMAGEABLE)) {
        // ROGUE
            if (self->spawnflags & SPAWNFLAG_LASER_ZAP) {
                self->spawnflags &= ~SPAWNFLAG_LASER_ZAP;
                gi.WriteByte(svc_temp_entity);
                gi.WriteByte(TE_LASER_SPARKS);
                gi.WriteByte(count);
                gi.WritePosition(tr.endpos);
                gi.WriteDir(tr.plane.normal);
                gi.WriteByte(self->s.skinnum);
                gi.multicast(tr.endpos, MULTICAST_PVS);
            }
            break;
        }
    } while (pierce_mark(&pierce, tr.ent));

    pierce_end(&pierce);

    VectorCopy(tr.endpos, self->s.old_origin);

    if (damaged_thing)
        self->damage_debounce_time = level.time + HZ(10);

    self->nextthink = level.time + FRAME_TIME;
    gi.linkentity(self);
}

static void target_laser_on(edict_t *self)
{
    if (!self->activator)
        self->activator = self;
    self->spawnflags |= SPAWNFLAG_LASER_ZAP | SPAWNFLAG_LASER_ON;
    self->svflags &= ~SVF_NOCLIENT;
    self->flags |= FL_TRAP;
    target_laser_think(self);
}

void target_laser_off(edict_t *self)
{
    self->spawnflags &= ~SPAWNFLAG_LASER_ON;
    self->svflags |= SVF_NOCLIENT;
    self->flags &= ~FL_TRAP;
    self->nextthink = 0;
}

void USE(target_laser_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->activator = activator;
    if (self->spawnflags & SPAWNFLAG_LASER_ON)
        target_laser_off(self);
    else
        target_laser_on(self);
}

void THINK(target_laser_start)(edict_t *self)
{
    edict_t *ent;

    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_NOT;
    self->s.renderfx |= RF_BEAM;
    self->s.modelindex = MODELINDEX_WORLD; // must be non-zero

    // [Sam-KEX] On Q2N64, spawnflag of 128 turns it into a lightning bolt
    if (level.is_n64) {
        // Paril: fix for N64
        if (self->spawnflags & SPAWNFLAG_LASER_STOPWINDOW) {
            self->spawnflags &= ~SPAWNFLAG_LASER_STOPWINDOW;
            self->spawnflags |= SPAWNFLAG_LASER_LIGHTNING;
        }
    }

    if (self->spawnflags & SPAWNFLAG_LASER_LIGHTNING) {
        self->s.renderfx |= RF_BEAM_LIGHTNING; // tell renderer it is lightning
        if (!self->s.skinnum)
            self->s.skinnum = 0xf3f3f1f1; // default lightning color
    }

    // set the beam diameter
    // [Paril-KEX] lab has this set prob before lightning was implemented
    if (!level.is_n64 && (self->spawnflags & SPAWNFLAG_LASER_FAT))
        self->s.frame = 16;
    else
        self->s.frame = 4;

    // set the color
    if (!self->s.skinnum) {
        if (self->spawnflags & SPAWNFLAG_LASER_RED)
            self->s.skinnum = 0xf2f2f0f0;
        else if (self->spawnflags & SPAWNFLAG_LASER_GREEN)
            self->s.skinnum = 0xd0d1d2d3;
        else if (self->spawnflags & SPAWNFLAG_LASER_BLUE)
            self->s.skinnum = 0xf3f3f1f1;
        else if (self->spawnflags & SPAWNFLAG_LASER_YELLOW)
            self->s.skinnum = 0xdcdddedf;
        else if (self->spawnflags & SPAWNFLAG_LASER_ORANGE)
            self->s.skinnum = 0xe0e1e2e3;
    }

    if (!self->enemy)  {
        if (self->target) {
            ent = G_Find(NULL, FOFS(targetname), self->target);
            if (!ent)
                gi.dprintf("%s: %s is a bad target\n", etos(self), self->target);
            else {
                self->enemy = ent;

                // N64 fix
                // FIXME: which map was this for again? oops
                if (level.is_n64 && !strcmp(self->enemy->classname, "func_train") && !(self->enemy->spawnflags & SPAWNFLAG_TRAIN_START_ON))
                    self->enemy->use(self->enemy, self, self);
            }
        } else {
            G_SetMovedir(self->s.angles, self->movedir);
        }
    }
    self->use = target_laser_use;
    self->think = target_laser_think;

    if (!self->dmg)
        self->dmg = 1;

    VectorSet(self->mins, -8, -8, -8);
    VectorSet(self->maxs, 8, 8, 8);
    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_LASER_ON)
        target_laser_on(self);
    else
        target_laser_off(self);
}

void SP_target_laser(edict_t *self)
{
    // let everything else get spawned before we start firing
    self->think = target_laser_start;
    self->flags |= FL_TRAP_LASER_FIELD;
    self->nextthink = level.time + SEC(1);
}

//==========================================================

/*QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
speed       How many seconds the ramping will take
message     two letters; starting lightlevel and ending lightlevel
*/

#define SPAWNFLAG_LIGHTRAMP_TOGGLE  1

void THINK(target_lightramp_think)(edict_t *self)
{
    char    style[2];
    float   diff = TO_SEC(level.time - self->timestamp);

    style[0] = (char)('a' + self->movedir[0] + diff * self->movedir[2]);
    style[1] = 0;

    gi.configstring(CS_LIGHTS + self->enemy->style, style);

    if (diff < self->speed) {
        self->nextthink = level.time + FRAME_TIME;
    } else if (self->spawnflags & SPAWNFLAG_LIGHTRAMP_TOGGLE) {
        SWAP(float, self->movedir[0], self->movedir[1]);
        self->movedir[2] = -self->movedir[2];
    }
}

void USE(target_lightramp_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (!self->enemy) {
        edict_t *e;

        // check all the targets
        e = NULL;
        while (1) {
            e = G_Find(e, FOFS(targetname), self->target);
            if (!e)
                break;
            if (strcmp(e->classname, "light") != 0)
                gi.dprintf("%s: target %s (%s) is not a light\n", etos(self), self->target, etos(e));
            else
                self->enemy = e;
        }

        if (!self->enemy) {
            gi.dprintf("%s: target %s not found\n", etos(self), self->target);
            G_FreeEdict(self);
            return;
        }
    }

    self->timestamp = level.time;
    target_lightramp_think(self);
}

void SP_target_lightramp(edict_t *self)
{
    if (!self->message || strlen(self->message) != 2 || self->message[0] < 'a' || self->message[0] > 'z' || self->message[1] < 'a' || self->message[1] > 'z' || self->message[0] == self->message[1]) {
        gi.dprintf("%s: bad ramp (%s)\n", etos(self), self->message ? self->message : "null string");
        G_FreeEdict(self);
        return;
    }

    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (!self->target) {
        gi.dprintf("%s: no target\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->svflags |= SVF_NOCLIENT;
    self->use = target_lightramp_use;
    self->think = target_lightramp_think;

    self->movedir[0] = (float)(self->message[0] - 'a');
    self->movedir[1] = (float)(self->message[1] - 'a');
    self->movedir[2] = (self->movedir[1] - self->movedir[0]) / self->speed;
}

//==========================================================

/*QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8) SILENT TOGGLE UNKNOWN_ROGUE ONE_SHOT
When triggered, this initiates a level-wide earthquake.
All players are affected with a screen shake.
"speed"     severity of the quake (default:200)
"count"     duration of the quake (default:5)
*/

#define SPAWNFLAGS_EARTHQUAKE_SILENT        1
#define SPAWNFLAGS_EARTHQUAKE_TOGGLE        2
#define SPAWNFLAGS_EARTHQUAKE_UNKNOWN_ROGUE 4
#define SPAWNFLAGS_EARTHQUAKE_ONE_SHOT      8

void THINK(target_earthquake_think)(edict_t *self)
{
    int      i;
    edict_t *e;

    // PGM
    if (!(self->spawnflags & SPAWNFLAGS_EARTHQUAKE_SILENT)) {
    // PGM
        if (self->last_move_time < level.time) {
            gi.positioned_sound(self->s.origin, self, CHAN_VOICE, self->noise_index, 1.0f, ATTN_NONE, 0);
            self->last_move_time = level.time + SEC(6.5f);
        }
    }

    for (i = 1, e = g_edicts + i; i <= game.maxclients; i++, e++) {
        if (!e->inuse)
            continue;
        e->client->quake_time = level.time + SEC(1);
    }

    if (level.time < self->timestamp)
        self->nextthink = level.time + HZ(10);
}

void USE(target_earthquake_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->spawnflags & SPAWNFLAGS_EARTHQUAKE_ONE_SHOT) {
        int      i;
        edict_t *e;

        for (i = 1, e = g_edicts + i; i <= game.maxclients; i++, e++) {
            if (!e->inuse)
                continue;
            e->client->v_dmg_pitch = -self->speed * 0.1f;
            e->client->v_dmg_time = level.time + DAMAGE_TIME;
        }

        return;
    }

    self->timestamp = level.time + SEC(self->count);

    if (self->spawnflags & SPAWNFLAGS_EARTHQUAKE_TOGGLE) {
        if (self->style)
            self->nextthink = 0;
        else
            self->nextthink = level.time + FRAME_TIME;

        self->style = !self->style;
    } else {
        self->nextthink = level.time + FRAME_TIME;
        self->last_move_time = 0;
    }

    self->activator = activator;
}

void SP_target_earthquake(edict_t *self)
{
    if (!self->targetname)
        gi.dprintf("%s: untargeted\n", etos(self));

    if (level.is_n64) {
        self->spawnflags |= SPAWNFLAGS_EARTHQUAKE_TOGGLE;
        self->speed = 5;
    }

    if (!self->count)
        self->count = 5;

    if (!self->speed)
        self->speed = 200;

    self->svflags |= SVF_NOCLIENT;
    self->think = target_earthquake_think;
    self->use = target_earthquake_use;

    if (!(self->spawnflags & SPAWNFLAGS_EARTHQUAKE_SILENT)) // PGM
        self->noise_index = gi.soundindex("world/quake.wav");
}

/*QUAKED target_camera (1 0 0) (-8 -8 -8) (8 8 8)
[Sam-KEX] Creates a camera path as seen in the N64 version.
*/

#define HACKFLAG_TELEPORT_OUT   2
#define HACKFLAG_SKIPPABLE      64
#define HACKFLAG_END_OF_UNIT    128

void THINK(update_target_camera)(edict_t *self)
{
    bool do_skip = false;

    // only allow skipping after 2 seconds
    if ((self->hackflags & HACKFLAG_SKIPPABLE) && level.time > SEC(2)) {
        for (int i = 0; i < game.maxclients; i++) {
            edict_t *client = g_edicts + 1 + i;
            if (!client->inuse || !client->client->pers.connected)
                continue;

            if (client->client->buttons & BUTTON_ANY) {
                do_skip = true;
                break;
            }
        }
    }

    if (!do_skip && self->movetarget) {
        self->moveinfo.remaining_distance -= (self->moveinfo.move_speed * FRAME_TIME_SEC) * 0.8f;

        if (self->moveinfo.remaining_distance <= 0) {
            if (self->movetarget->hackflags & HACKFLAG_TELEPORT_OUT) {
                if (self->enemy) {
                    self->enemy->s.event = EV_PLAYER_TELEPORT;
                    self->enemy->hackflags = HACKFLAG_TELEPORT_OUT;
                    self->enemy->pain_debounce_time = self->enemy->timestamp = SEC(self->movetarget->wait);
                }
            }

            VectorCopy(self->movetarget->s.origin, self->s.origin);
            self->nextthink = level.time + SEC(self->movetarget->wait);
            if (self->movetarget->target) {
                self->movetarget = G_PickTarget(self->movetarget->target);

                if (self->movetarget) {
                    self->moveinfo.move_speed = self->movetarget->speed ? self->movetarget->speed : 55;
                    self->moveinfo.remaining_distance = Distance(self->movetarget->s.origin, self->s.origin);
                    self->moveinfo.distance = self->moveinfo.remaining_distance;
                }
            } else
                self->movetarget = NULL;

            return;
        }

        float frac = 1.0f - (self->moveinfo.remaining_distance / self->moveinfo.distance);

        if (self->enemy && (self->enemy->hackflags & HACKFLAG_TELEPORT_OUT))
            self->enemy->x.alpha = max(1.0f / 255, frac);

        vec3_t delta, newpos;
        VectorSubtract(self->movetarget->s.origin, self->s.origin, delta);
        VectorMA(self->s.origin, frac, delta, newpos);

        if (self->pathtarget) {
            edict_t *pt = G_Find(NULL, FOFS(targetname), self->pathtarget);
            if (pt) {
                VectorSubtract(pt->s.origin, newpos, delta);
                vectoangles(delta, level.intermission_angle);
            }
        }

        VectorCopy(newpos, level.intermission_origin);

        // move all clients to the intermission point
        for (int i = 0; i < game.maxclients; i++) {
            edict_t *client = g_edicts + 1 + i;
            if (client->inuse)
                MoveClientToIntermission(client);
        }
    } else {
        if (self->killtarget) {
            // destroy dummy player
            if (self->enemy)
                G_FreeEdict(self->enemy);

            edict_t *t = NULL;
            level.intermissiontime = 0;
            level.level_intermission_set = true;

            while ((t = G_Find(t, FOFS(targetname), self->killtarget)))
                t->use(t, self, self->activator);

            level.intermissiontime = level.time;
            //level.intermission_server_frame = gi.ServerFrame();

            // end of unit requires a wait
            if (level.changemap && !strchr(level.changemap, '*'))
                level.exitintermission = true;
        }

        self->think = NULL;
        return;
    }

    self->nextthink = level.time + FRAME_TIME;
}

void G_SetClientFrame(edict_t *ent);

extern float xyspeed;

void THINK(target_camera_dummy_think)(edict_t *self)
{
    // bit of a hack, but this will let the dummy
    // move like a player
    self->client = self->owner->client;
    xyspeed = sqrtf(self->velocity[0] * self->velocity[0] + self->velocity[1] * self->velocity[1]);
    G_SetClientFrame(self);
    self->client = NULL;

    // alpha fade out for voops
    if (self->hackflags & HACKFLAG_TELEPORT_OUT) {
        self->timestamp = max(0, self->timestamp - HZ(10));
        float frac = TO_SEC(self->timestamp) / TO_SEC(self->pain_debounce_time);
        self->x.alpha = max(1.0f / 255, frac);
    }

    self->nextthink = level.time + HZ(10);
}

void USE(use_target_camera)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->sounds)
        gi.configstring(CS_CDTRACK, va("%d", self->sounds));

    if (!self->target)
        return;

    self->movetarget = G_PickTarget(self->target);
    if (!self->movetarget)
        return;

    level.intermissiontime = level.time;
    //level.intermission_server_frame = gi.ServerFrame();
    level.exitintermission = 0;

    // spawn fake player dummy where we were
    if (activator->client) {
        edict_t *dummy = self->enemy = G_Spawn();
        dummy->owner = activator;
        dummy->clipmask = activator->clipmask;
        VectorCopy(activator->s.origin, dummy->s.origin);
        VectorCopy(activator->s.angles, dummy->s.angles);
        dummy->groundentity = activator->groundentity;
        dummy->groundentity_linkcount = dummy->groundentity ? dummy->groundentity->linkcount : 0;
        dummy->think = target_camera_dummy_think;
        dummy->nextthink = level.time + HZ(10);
        dummy->solid = SOLID_BBOX;
        dummy->movetype = MOVETYPE_STEP;
        VectorCopy(activator->mins, dummy->mins);
        VectorCopy(activator->maxs, dummy->maxs);
        dummy->s.modelindex = dummy->s.modelindex2 = MODELINDEX_PLAYER;
        dummy->s.skinnum = activator->s.skinnum;
        VectorCopy(activator->velocity, dummy->velocity);
        dummy->s.renderfx = RF_MINLIGHT;
        dummy->s.frame = activator->s.frame;
        gi.linkentity(dummy);
    }

    if (self->pathtarget) {
        edict_t *pt = G_Find(NULL, FOFS(targetname), self->pathtarget);
        if (pt) {
            vec3_t delta;
            VectorSubtract(pt->s.origin, self->s.origin, delta);
            vectoangles(delta, level.intermission_angle);
        }
    }

    VectorCopy(self->s.origin, level.intermission_origin);

    // move all clients to the intermission point
    for (int i = 0; i < game.maxclients; i++) {
        edict_t *client = g_edicts + 1 + i;
        if (!client->inuse)
            continue;

        // respawn any dead clients
        if (client->health <= 0) {
            // give us our max health back since it will reset
            // to pers.health; in instanced items we'd lose the items
            // we touched so we always want to respawn with our max.
            if (P_UseCoopInstancedItems())
                client->client->pers.health = client->client->pers.max_health = client->max_health;

            respawn(client);
        }

        MoveClientToIntermission(client);
    }

    self->activator = activator;
    self->think = update_target_camera;
    self->nextthink = level.time + SEC(self->wait);
    self->moveinfo.move_speed = self->speed;

    self->moveinfo.remaining_distance = Distance(self->movetarget->s.origin, self->s.origin);
    self->moveinfo.distance = self->moveinfo.remaining_distance;

    if (self->hackflags & HACKFLAG_END_OF_UNIT)
        G_EndOfUnitMessage();
}

void SP_target_camera(edict_t *self)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(self);
        return;
    }

    self->use = use_target_camera;
    self->svflags = SVF_NOCLIENT;
}

/*QUAKED target_gravity (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
[Sam-KEX] Changes gravity, as seen in the N64 version
*/

void USE(use_target_gravity)(edict_t *self, edict_t *other, edict_t *activator)
{
    gi.cvar_set("sv_gravity", va("%f", self->gravity));
    level.gravity = self->gravity;
}

void SP_target_gravity(edict_t *self)
{
    self->use = use_target_gravity;
    self->gravity = Q_atof(st.gravity);
}

/*QUAKED target_soundfx (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
[Sam-KEX] Plays a sound fx, as seen in the N64 version
*/

void THINK(update_target_soundfx)(edict_t *self)
{
    gi.positioned_sound(self->s.origin, self, CHAN_VOICE, self->noise_index, self->volume, self->attenuation, 0);
}

void USE(use_target_soundfx)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->think = update_target_soundfx;
    self->nextthink = level.time + SEC(self->delay);
}

void SP_target_soundfx(edict_t *self)
{
    if (!self->volume)
        self->volume = 1.0f;

    if (!self->attenuation)
        self->attenuation = 1.0f;
    else if (self->attenuation == -1) // use -1 so 0 defaults to 1
        self->attenuation = 0;

    self->noise_index = Q_atoi(st.noise);

    switch (self->noise_index) {
    case 1:
        self->noise_index = gi.soundindex("world/x_alarm.wav");
        break;
    case 2:
        self->noise_index = gi.soundindex("world/flyby1.wav");
        break;
    case 4:
        self->noise_index = gi.soundindex("world/amb12.wav");
        break;
    case 5:
        self->noise_index = gi.soundindex("world/amb17.wav");
        break;
    case 7:
        self->noise_index = gi.soundindex("world/bigpump2.wav");
        break;
    default:
        gi.dprintf("%s: unknown noise %d\n", etos(self), self->noise_index);
        return;
    }

    self->use = use_target_soundfx;
}

/*QUAKED target_light (1 0 0) (-8 -8 -8) (8 8 8) START_ON NO_LERP FLICKER
[Paril-KEX] dynamic light entity that follows a lightstyle.

*/

#define SPAWNFLAG_TARGET_LIGHT_START_ON 1
#define SPAWNFLAG_TARGET_LIGHT_NO_LERP  2 // not used in N64, but I'll use it for this
#define SPAWNFLAG_TARGET_LIGHT_FLICKER  4

void THINK(target_light_flicker_think)(edict_t *self)
{
    if (brandom())
        self->svflags ^= SVF_NOCLIENT;

    self->nextthink = level.time + HZ(10);
}

// think function handles interpolation from start to finish.
void THINK(target_light_think)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_FLICKER)
        target_light_flicker_think(self);

    const char *style = G_GetLightStyle(self->style);
    self->delay += self->speed;

    int index = (int)self->delay % strlen(style);
    float current_lerp = (float)(style[index] - 'a') / (float)('z' - 'a');
    float lerp = current_lerp;

    if (!(self->spawnflags & SPAWNFLAG_TARGET_LIGHT_NO_LERP)) {
        int next_index = (index + 1) % strlen(style);
        float next_lerp = (float)(style[next_index] - 'a') / (float)('z' - 'a');
        float mod_lerp = self->delay - (int)self->delay;
        lerp = (next_lerp * mod_lerp) + (current_lerp * (1.0f - mod_lerp));
    }

    uint32_t my_rgb = self->count;
    uint32_t target_rgb = self->chain->s.skinnum;

    int my_b = ((my_rgb >> 8) & 0xff);
    int my_g = ((my_rgb >> 16) & 0xff);
    int my_r = ((my_rgb >> 24) & 0xff);

    int target_b = ((target_rgb >> 8) & 0xff);
    int target_g = ((target_rgb >> 16) & 0xff);
    int target_r = ((target_rgb >> 24) & 0xff);

    float backlerp = 1.0f - lerp;

    int b = (target_b * lerp) + (my_b * backlerp);
    int g = (target_g * lerp) + (my_g * backlerp);
    int r = (target_r * lerp) + (my_r * backlerp);

    self->s.skinnum = MakeBigLong(r, g, b, 0);

    self->nextthink = level.time + HZ(10);
}

void USE(target_light_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->health = !self->health;

    if (self->health)
        self->svflags &= ~SVF_NOCLIENT;
    else
        self->svflags |= SVF_NOCLIENT;

    if (!self->health) {
        self->think = NULL;
        self->nextthink = 0;
        return;
    }

    // has dynamic light "target"
    if (self->chain) {
        self->think = target_light_think;
        self->nextthink = level.time + HZ(10);
    } else if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_FLICKER) {
        self->think = target_light_flicker_think;
        self->nextthink = level.time + HZ(10);
    }
}

void SP_target_light(edict_t *self)
{
    self->s.modelindex = 1;
    self->s.renderfx = RF_CUSTOM_LIGHT;
    self->s.frame = st.radius ? st.radius : 150;
    self->count = self->s.skinnum;
    self->svflags |= SVF_NOCLIENT;
    self->health = 0;

    if (self->target)
        self->chain = G_PickTarget(self->target);

    if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_START_ON)
        target_light_use(self, self, self);

    if (!self->speed)
        self->speed = 1.0f;
    else
        self->speed = 0.1f / self->speed;

    if (level.is_n64)
        self->style += 10;

    self->use = target_light_use;

    gi.linkentity(self);
}

/*QUAKED target_poi (1 0 0) (-4 -4 -4) (4 4 4) NEAREST DUMMY DYNAMIC
[Paril-KEX] point of interest for help in player navigation.
Without any additional setup, targeting this entity will switch
the current POI in the level to the one this is linked to.

"count": if set, this value is the 'stage' linked to this POI. A POI
with this set that is activated will only take effect if the current
level's stage value is <= this value, and if it is, will also set
the current level's stage value to this value.

"style": only used for teamed POIs; the POI with the lowest style will
be activated when checking for which POI to activate. This is mainly
useful during development, to easily insert or change the order of teamed
POIs without needing to manually move the entity definitions around.

"team": if set, this will create a team of POIs. Teamed POIs act like
a single unit; activating any of them will do the same thing. When activated,
it will filter through all of the POIs on the team selecting the one that
best fits the current situation. This includes checking "count" and "style"
values. You can also set the NEAREST spawnflag on any of the teamed POIs,
which will additionally cause activation to prefer the nearest one to the player.
Killing a POI via killtarget will remove it from the chain, allowing you to
adjust valid POIs at runtime.

The DUMMY spawnflag is to allow you to use a single POI as a team member
that can be activated, if you're using killtargets to remove POIs.

The DYNAMIC spawnflag is for very specific circumstances where you want
to direct the player to the nearest teamed POI, but want the path to pick
the nearest at any given time rather than only when activated.

The DISABLED flag is mainly intended to work with DYNAMIC & teams; the POI
will be disabled until it is targeted, and afterwards will be enabled until
it is killed.
*/

#define SPAWNFLAG_POI_NEAREST   1
#define SPAWNFLAG_POI_DUMMY     2
#define SPAWNFLAG_POI_DYNAMIC   4
#define SPAWNFLAG_POI_DISABLED  8

void USE(target_poi_use)(edict_t *ent, edict_t *other, edict_t *activator)
{
}

void THINK(target_poi_setup)(edict_t *self)
{
}

void SP_target_poi(edict_t *self)
{
    if (deathmatch->integer) {
        // auto-remove for deathmatch
        G_FreeEdict(self);
        return;
    }

    if (st.image)
        self->noise_index = gi.imageindex(st.image);
    else
        self->noise_index = gi.imageindex("friend");

    self->use = target_poi_use;
    self->svflags |= SVF_NOCLIENT;
    self->think = target_poi_setup;
    self->nextthink = level.time + FRAME_TIME;

    if (!self->team) {
        if (self->spawnflags & SPAWNFLAG_POI_NEAREST)
            gi.dprintf("%s has useless spawnflag 'NEAREST'\n", etos(self));
        if (self->spawnflags & SPAWNFLAG_POI_DYNAMIC)
            gi.dprintf("%s has useless spawnflag 'DYNAMIC'\n", etos(self));
    }
}

/*QUAKED target_music (1 0 0) (-8 -8 -8) (8 8 8)
Change music when used
*/

void USE(use_target_music)(edict_t *ent, edict_t *other, edict_t *activator)
{
    gi.configstring(CS_CDTRACK, va("%d", ent->sounds));
}

void SP_target_music(edict_t *self)
{
    self->use = use_target_music;
}

/*QUAKED target_healthbar (0 1 0) (-8 -8 -8) (8 8 8) PVS_ONLY
*
* Hook up health bars to monsters.
* "delay" is how long to show the health bar for after death.
* "message" is their name
*/

void USE(use_target_healthbar)(edict_t *ent, edict_t *other, edict_t *activator)
{
    edict_t *target = G_PickTarget(ent->target);

    if (!target || ent->health != target->spawn_count) {
        if (target)
            gi.dprintf("%s: target %s changed from what it used to be\n", etos(ent), etos(target));
        else
            gi.dprintf("%s: no target\n", etos(ent));
        G_FreeEdict(ent);
        return;
    }

    for (int i = 0; i < MAX_HEALTH_BARS; i++) {
        if (level.health_bar_entities[i])
            continue;

        ent->enemy = target;
        level.health_bar_entities[i] = ent;
        gi.configstring(CONFIG_HEALTH_BAR_NAME, ent->message);
        return;
    }

    gi.dprintf("%s: too many health bars\n", etos(ent));
    G_FreeEdict(ent);
}

void THINK(check_target_healthbar)(edict_t *ent)
{
    edict_t *target = G_PickTarget(ent->target);

    if (!target || !(target->svflags & SVF_MONSTER)) {
        if (target)
            gi.dprintf("%s: target %s does not appear to be a monster\n", etos(ent), etos(target));
        G_FreeEdict(ent);
        return;
    }

    // just for sanity check
    ent->health = target->spawn_count;
}

void SP_target_healthbar(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (!self->target || !*self->target) {
        gi.dprintf("%s: missing target\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    if (!self->message) {
        gi.dprintf("%s: missing message\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->use = use_target_healthbar;
    self->think = check_target_healthbar;
    self->nextthink = level.time + SEC(0.025f);
}

/*QUAKED target_autosave (0 1 0) (-8 -8 -8) (8 8 8)
*
* Auto save on command.
*/

void USE(use_target_autosave)(edict_t *ent, edict_t *other, edict_t *activator)
{
    gtime_t save_time = SEC(gi.cvar("g_athena_auto_save_min_time", "60", CVAR_NOSET)->value);

    if (level.time - level.next_auto_save > save_time) {
        gi.AddCommandString("autosave\n");
        level.next_auto_save = level.time;
    }
}

void SP_target_autosave(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    self->use = use_target_autosave;
}

/*QUAKED target_sky (0 1 0) (-8 -8 -8) (8 8 8)
*
* Change sky parameters.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
*/

void USE(use_target_sky)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->map)
        gi.configstring(CS_SKY, self->map);

    if (self->count & 3) {
        if (self->count & 1)
            level.skyrotate = self->accel;

        if (self->count & 2)
            level.skyautorotate = self->style;

        gi.configstring(CS_SKYROTATE, va("%f %d", level.skyrotate, level.skyautorotate));
    }

    if (self->count & 4)
        gi.configstring(CS_SKYAXIS, va("%f %f %f", self->movedir[0], self->movedir[1], self->movedir[2]));
}

void SP_target_sky(edict_t *self)
{
    self->use = use_target_sky;
    if (ED_WasKeySpecified("sky"))
        self->map = st.sky;
    if (ED_WasKeySpecified("skyaxis")) {
        self->count |= 4;
        VectorCopy(st.skyaxis, self->movedir);
    }
    if (ED_WasKeySpecified("skyrotate")) {
        self->count |= 1;
        self->accel = st.skyrotate;
    }
    if (ED_WasKeySpecified("skyautorotate")) {
        self->count |= 2;
        self->style = st.skyautorotate;
    }
}

//==========================================================

/*QUAKED target_crossunit_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crossunit_target with the same trigger number is automatically used when a level is started within the same unit.  It is OK to check multiple triggers.  Message, delay, target, and killtarget also work.
*/
void USE(trigger_crossunit_trigger_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    game.cross_unit_flags |= self->spawnflags;
    G_FreeEdict(self);
}

void SP_target_crossunit_trigger(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    self->svflags = SVF_NOCLIENT;
    self->use = trigger_crossunit_trigger_use;
}

/*QUAKED target_crossunit_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8 - - - - - - - - trigger9 trigger10 trigger11 trigger12 trigger13 trigger14 trigger15 trigger16
Triggered by a trigger_crossunit elsewhere within a unit.  If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

"delay"     delay before using targets if the trigger has been activated (default 1)
*/
void THINK(target_crossunit_target_think)(edict_t *self)
{
    if (self->spawnflags == (game.cross_unit_flags & SFL_CROSS_TRIGGER_MASK & self->spawnflags)) {
        G_UseTargets(self, self);
        G_FreeEdict(self);
    }
}

void SP_target_crossunit_target(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (!self->delay)
        self->delay = 1;
    self->svflags = SVF_NOCLIENT;

    self->think = target_crossunit_target_think;
    self->nextthink = level.time + SEC(self->delay);
}

/*QUAKED target_achievement (.5 .5 .5) (-8 -8 -8) (8 8 8)
Give an achievement.

"achievement"       cheevo to give
*/
void USE(use_target_achievement)(edict_t *self, edict_t *other, edict_t *activator)
{
}

void SP_target_achievement(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    self->map = st.achievement;
    self->use = use_target_achievement;
}

void USE(use_target_story)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->message && *self->message)
        level.story_active = true;
    else
        level.story_active = false;

    if (level.story_active) {
        gi.WriteByte(svc_layout);
        gi.WriteString(va("xv 0 yv 0 cstring \"%s\"", self->message));
        gi.multicast(vec3_origin, MULTICAST_ALL_R);
    }
}

void SP_target_story(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    self->use = use_target_story;
}
