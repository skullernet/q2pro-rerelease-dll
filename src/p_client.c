// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"

const vec3_t player_mins = { -16, -16, -24 };
const vec3_t player_maxs = { 16, 16, 32 };

void SP_misc_teleporter_dest(edict_t *ent);

void THINK(info_player_start_drop)(edict_t *self)
{
    // allow them to drop
    self->solid = SOLID_TRIGGER;
    self->movetype = MOVETYPE_TOSS;
    VectorCopy(player_mins, self->mins);
    VectorCopy(player_maxs, self->maxs);
    gi.linkentity(self);
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
    // fix stuck spawn points
    if (gi.trace(self->s.origin, player_mins, player_maxs, self->s.origin, self, MASK_SOLID).startsolid)
        G_FixStuckObject(self, self->s.origin);

    // [Paril-KEX] on n64, since these can spawn riding elevators,
    // allow them to "ride" the elevators so respawning works
    if (level.is_n64) {
        self->think = info_player_start_drop;
        self->nextthink = level.time + FRAME_TIME;
    }
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    SP_misc_teleporter_dest(self);
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/
void SP_info_player_coop(edict_t *self)
{
    if (!coop->integer) {
        G_FreeEdict(self);
        return;
    }

    SP_info_player_start(self);
}

/*QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games on rmine2 where lava level
needs to be checked
*/
void SP_info_player_coop_lava(edict_t *self)
{
    if (!coop->integer) {
        G_FreeEdict(self);
        return;
    }

    // fix stuck spawn points
    if (gi.trace(self->s.origin, player_mins, player_maxs, self->s.origin, self, MASK_SOLID).startsolid)
        G_FixStuckObject(self, self->s.origin);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(edict_t *ent)
{
}

// [Paril-KEX] whether instanced items should be used or not
bool P_UseCoopInstancedItems(void)
{
    // instanced items require EntityVisibleToClient support
    if (gix.apiversion < GAME_API_VERSION_EX_ENTITY_VISIBLE)
        return false;
    // squad respawn forces instanced items on, since we don't
    // want players to need to backtrack just to get their stuff.
    return g_coop_instanced_items->integer || g_coop_squad_respawn->integer;
}

//=======================================================================

static void ClientObituary(edict_t *self, edict_t *inflictor, edict_t *attacker, mod_t mod)
{
    const char *base = NULL;

    if (coop->integer && attacker->client)
        mod.friendly_fire = true;

    switch (mod.id) {
    case MOD_SUICIDE:
        base = "%s suicides.\n";
        break;
    case MOD_FALLING:
        base = "%s cratered.\n";
        break;
    case MOD_CRUSH:
        base = "%s was squished.\n";
        break;
    case MOD_WATER:
        base = "%s sank like a rock.\n";
        break;
    case MOD_SLIME:
        base = "%s melted.\n";
        break;
    case MOD_LAVA:
        base = "%s does a back flip into the lava.\n";
        break;
    case MOD_EXPLOSIVE:
    case MOD_BARREL:
        base = "%s blew up.\n";
        break;
    case MOD_EXIT:
        base = "%s found a way out.\n";
        break;
    case MOD_TARGET_LASER:
        base = "%s saw the light.\n";
        break;
    case MOD_TARGET_BLASTER:
        base = "%s got blasted.\n";
        break;
    case MOD_BOMB:
    case MOD_SPLASH:
    case MOD_TRIGGER_HURT:
        base = "%s was in the wrong place.\n";
        break;
    // RAFAEL
    case MOD_GEKK:
    case MOD_BRAINTENTACLE:
        base = "%s... that's gotta hurt!\n";
        break;
    // RAFAEL
    default:
        base = NULL;
        break;
    }

    if (attacker == self) {
        switch (mod.id) {
        case MOD_HELD_GRENADE:
            base = "%s tried to put the pin back in.\n";
            break;
        case MOD_HG_SPLASH:
        case MOD_G_SPLASH:
            base = "%s tripped on their own grenade.\n";
            break;
        case MOD_R_SPLASH:
            base = "%s blew themselves up.\n";
            break;
        case MOD_BFG_BLAST:
            base = "%s should have used a smaller gun.\n";
            break;
        // RAFAEL 03-MAY-98
        case MOD_TRAP:
            base = "%s was sucked into their own trap.\n";
            break;
        // RAFAEL
        // ROGUE
        case MOD_DOPPLE_EXPLODE:
            base = "%s was fooled by their own doppelganger.\n";
            break;
        // ROGUE
        default:
            base = "%s killed themselves.\n";
            break;
        }
    }

    // send generic/self
    if (base) {
        gi.bprintf(PRINT_MEDIUM, base, self->client->pers.netname);
        if (deathmatch->integer && !mod.no_point_loss) {
            self->client->resp.score--;

            if (teamplay->integer)
                G_AdjustTeamScore(self->client->resp.ctf_team, -1);
        }
        self->enemy = NULL;
        return;
    }

    // has a killer
    self->enemy = attacker;
    if (attacker && attacker->client) {
        switch (mod.id) {
        case MOD_BLASTER:
            base = "%s was blasted by %s.\n";
            break;
        case MOD_SHOTGUN:
            base = "%s was gunned down by %s.\n";
            break;
        case MOD_SSHOTGUN:
            base = "%s was blown away by %s's Super Shotgun.\n";
            break;
        case MOD_MACHINEGUN:
            base = "%s was machinegunned by %s.\n";
            break;
        case MOD_CHAINGUN:
            base = "%s was cut in half by %s's Chaingun.\n";
            break;
        case MOD_GRENADE:
            base = "%s was popped by %s's grenade.\n";
            break;
        case MOD_G_SPLASH:
            base = "%s was shredded by %s's shrapnel.\n";
            break;
        case MOD_ROCKET:
            base = "%s ate %s's rocket.\n";
            break;
        case MOD_R_SPLASH:
            base = "%s almost dodged %s's rocket.\n";
            break;
        case MOD_HYPERBLASTER:
            base = "%s was melted by %s's HyperBlaster.\n";
            break;
        case MOD_RAILGUN:
            base = "%s was railed by %s.\n";
            break;
        case MOD_BFG_LASER:
            base = "%s saw the pretty lights from %s's BFG.\n";
            break;
        case MOD_BFG_BLAST:
            base = "%s was disintegrated by %s's BFG blast.\n";
            break;
        case MOD_BFG_EFFECT:
            base = "%s couldn't hide from %s's BFG.\n";
            break;
        case MOD_HANDGRENADE:
            base = "%s caught %s's handgrenade.\n";
            break;
        case MOD_HG_SPLASH:
            base = "%s didn't see %s's handgrenade.\n";
            break;
        case MOD_HELD_GRENADE:
            base = "%s feels %s's pain.\n";
            break;
        case MOD_TELEFRAG:
        case MOD_TELEFRAG_SPAWN:
            base = "%s tried to invade %s's personal space.\n";
            break;
        // RAFAEL 14-APR-98
        case MOD_RIPPER:
            base = "%s ripped to shreds by %s's ripper gun.\n";
            break;
        case MOD_PHALANX:
            base = "%s was evaporated by %s.\n";
            break;
        case MOD_TRAP:
            base = "%s was caught in %s's trap.\n";
            break;
        // RAFAEL
        //===============
        // ROGUE
        case MOD_CHAINFIST:
            base = "%s was shredded by %s's ripsaw.\n";
            break;
        case MOD_DISINTEGRATOR:
            base = "%s lost his grip courtesy of %s's Disintegrator.\n";
            break;
        case MOD_ETF_RIFLE:
            base = "%s was perforated by %s.\n";
            break;
        case MOD_HEATBEAM:
            base = "%s was scorched by %s's Plasma Beam.\n";
            break;
        case MOD_TESLA:
            base = "%s was enlightened by %s's tesla mine.\n";
            break;
        case MOD_PROX:
            base = "%s got too close to %s's proximity mine.\n";
            break;
        case MOD_NUKE:
            base = "%s was nuked by %s's antimatter bomb.\n";
            break;
        case MOD_VENGEANCE_SPHERE:
            base = "%s was purged by %s's Vengeance Sphere.\n";
            break;
        case MOD_DEFENDER_SPHERE:
            base = "%s had a blast with %s's Defender Sphere.\n";
            break;
        case MOD_HUNTER_SPHERE:
            base = "%s was hunted down by %s's Hunter Sphere.\n";
            break;
        case MOD_TRACKER:
            base = "%s was annihilated by %s's Disruptor.\n";
            break;
        case MOD_DOPPLE_EXPLODE:
            base = "%s was tricked by %s's Doppelganger.\n";
            break;
        case MOD_DOPPLE_VENGEANCE:
            base = "%s was purged by %s's Doppelganger.\n";
            break;
        case MOD_DOPPLE_HUNTER:
            base = "%s was hunted down by %s's Doppelganger.\n";
            break;
        // ROGUE
        //===============
        // ZOID
        case MOD_GRAPPLE:
            base = "%s was caught by %s's grapple.\n";
            break;
        // ZOID
        default:
            base = "%s was killed by %s.\n";
            break;
        }

        gi.bprintf(PRINT_MEDIUM, base, self->client->pers.netname, attacker->client->pers.netname);

        if (G_TeamplayEnabled()) {
            // ZOID
            //  if at start and same team, clear.
            // [Paril-KEX] moved here so it's not an outlier in player_die.
            if (mod.id == MOD_TELEFRAG_SPAWN &&
                self->client->resp.ctf_state < 2 &&
                self->client->resp.ctf_team == attacker->client->resp.ctf_team) {
                self->client->resp.ctf_state = 0;
                return;
            }
        }

        // ROGUE
        if (gamerules->integer) {
            if (DMGame.Score) {
                if (mod.friendly_fire) {
                    if (!mod.no_point_loss)
                        DMGame.Score(attacker, self, -1, mod);
                } else
                    DMGame.Score(attacker, self, 1, mod);
            }
            return;
        }
        // ROGUE

        if (deathmatch->integer) {
            if (mod.friendly_fire) {
                if (!mod.no_point_loss) {
                    attacker->client->resp.score--;

                    if (teamplay->integer)
                        G_AdjustTeamScore(attacker->client->resp.ctf_team, -1);
                }
            } else {
                attacker->client->resp.score++;

                if (teamplay->integer)
                    G_AdjustTeamScore(attacker->client->resp.ctf_team, 1);
            }
        } else if (!coop->integer)
            self->client->resp.score--;

        return;
    }

    gi.bprintf(PRINT_MEDIUM, "%s died.\n", self->client->pers.netname);
    // ROGUE
    if (deathmatch->integer && !mod.no_point_loss) {
        if (gamerules->integer) {
            if (DMGame.Score) {
                DMGame.Score(self, self, -1, mod);
            }
            return;
        } else {
            self->client->resp.score--;

            if (teamplay->integer)
                G_AdjustTeamScore(attacker->client->resp.ctf_team, -1);
        }
    }
    // ROGUE
}

static void TossClientWeapon(edict_t *self)
{
    const gitem_t *item;
    edict_t       *drop;
    bool           quad;
    // RAFAEL
    bool quadfire;
    // RAFAEL
    float spread;

    if (!deathmatch->integer)
        return;

    item = self->client->pers.weapon;
    if (item && g_instagib->integer)
        item = NULL;
    if (item && !self->client->pers.inventory[self->client->pers.weapon->ammo])
        item = NULL;
    if (item && !G_CanDropItem(item))
        item = NULL;

    if (g_dm_no_quad_drop->integer)
        quad = false;
    else
        quad = (self->client->quad_time > (level.time + SEC(1)));

    // RAFAEL
    if (g_dm_no_quadfire_drop->integer)
        quadfire = false;
    else
        quadfire = (self->client->quadfire_time > (level.time + SEC(1)));
    // RAFAEL

    if (item && quad)
        spread = 22.5f;
    // RAFAEL
    else if (item && quadfire)
        spread = 12.5f;
    // RAFAEL
    else
        spread = 0.0f;

    if (item) {
        self->client->v_angle[YAW] -= spread;
        drop = Drop_Item(self, item);
        self->client->v_angle[YAW] += spread;
        drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
        drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
        drop->svflags &= ~SVF_INSTANCED;
    }

    if (quad) {
        self->client->v_angle[YAW] += spread;
        drop = Drop_Item(self, GetItemByIndex(IT_ITEM_QUAD));
        self->client->v_angle[YAW] -= spread;
        drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
        drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
        drop->svflags &= ~SVF_INSTANCED;

        drop->touch = Touch_Item;
        drop->nextthink = self->client->quad_time;
        drop->think = G_FreeEdict;
    }

    // RAFAEL
    if (quadfire) {
        self->client->v_angle[YAW] += spread;
        drop = Drop_Item(self, GetItemByIndex(IT_ITEM_QUADFIRE));
        self->client->v_angle[YAW] -= spread;
        drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
        drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
        drop->svflags &= ~SVF_INSTANCED;

        drop->touch = Touch_Item;
        drop->nextthink = self->client->quadfire_time;
        drop->think = G_FreeEdict;
    }
    // RAFAEL
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker)
{
    vec3_t dir;

    if (attacker && attacker != world && attacker != self) {
        VectorSubtract(attacker->s.origin, self->s.origin, dir);
    } else if (inflictor && inflictor != world && inflictor != self) {
        VectorSubtract(inflictor->s.origin, self->s.origin, dir);
    } else {
        self->client->killer_yaw = self->s.angles[YAW];
        return;
    }

    // PMM - fixed to correct for pitch of 0
    if (dir[0])
        self->client->killer_yaw = RAD2DEG(atan2(dir[1], dir[0]));
    else if (dir[1] > 0)
        self->client->killer_yaw = 90;
    else if (dir[1] < 0)
        self->client->killer_yaw = 270;
    else
        self->client->killer_yaw = 0;
}

/*
==================
player_die
==================
*/
void DIE(player_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    PlayerTrail_Destroy(self);

    VectorClear(self->avelocity);

    self->takedamage = true;
    self->movetype = MOVETYPE_TOSS;

    self->s.modelindex2 = 0; // remove linked weapon model
    // ZOID
    self->s.modelindex3 = 0; // remove linked ctf flag
    // ZOID

    self->s.angles[0] = 0;
    self->s.angles[2] = 0;

    self->s.sound = 0;
    self->client->weapon_sound = 0;

    self->maxs[2] = -8;

    //  self->solid = SOLID_NOT;
    self->svflags |= SVF_DEADMONSTER;

    if (!self->deadflag) {
        if (deathmatch->integer && g_dm_force_respawn_time->integer)
            self->client->respawn_time = (level.time + SEC(g_dm_force_respawn_time->value));
        else
            self->client->respawn_time = (level.time + SEC(1));

        LookAtKiller(self, inflictor, attacker);
        self->client->ps.pmove.pm_type = PM_DEAD;
        ClientObituary(self, inflictor, attacker, mod);

        CTFFragBonuses(self, inflictor, attacker);
        // ZOID
        TossClientWeapon(self);
        // ZOID
        CTFPlayerResetGrapple(self);
        CTFDeadDropFlag(self);
        CTFDeadDropTech(self);
        // ZOID
        if (deathmatch->integer && !self->client->showscores)
            Cmd_Help_f(self); // show scores

        if (coop->integer && !P_UseCoopInstancedItems()) {
            // clear inventory
            // this is kind of ugly, but it's how we want to handle keys in coop
            for (int n = 0; n < IT_TOTAL; n++) {
                if (coop->integer && (itemlist[n].flags & IF_KEY))
                    self->client->resp.coop_respawn.inventory[n] = self->client->pers.inventory[n];
                self->client->pers.inventory[n] = 0;
            }
        }
    }

    if (gamerules->integer) { // if we're in a dm game, alert the game
        if (DMGame.PlayerDeath)
            DMGame.PlayerDeath(self, inflictor, attacker);
    }

    // remove powerups
    self->client->quad_time = 0;
    self->client->invincible_time = 0;
    self->client->breather_time = 0;
    self->client->enviro_time = 0;
    self->client->invisible_time = 0;
    self->flags &= ~FL_POWER_ARMOR;

    // clear inventory
    if (G_TeamplayEnabled())
        memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

    // RAFAEL
    self->client->quadfire_time = 0;
    // RAFAEL

    //==============
    // ROGUE stuff
    self->client->double_time = 0;

    // if there's a sphere around, let it know the player died.
    // vengeance and hunter will die if they're not attacking,
    // defender should always die
    if (self->client->owned_sphere) {
        edict_t *sphere = self->client->owned_sphere;
        sphere->die(sphere, self, self, 0, vec3_origin, mod);
    }

    // if we've been killed by the tracker, GIB!
    if (mod.id == MOD_TRACKER) {
        self->health = -100;
        damage = 400;
    }

    // make sure no trackers are still hurting us.
    if (self->client->tracker_pain_time)
        RemoveAttackingPainDaemons(self);

    // if we got obliterated by the nuke, don't gib
    if ((self->health < -80) && (mod.id == MOD_NUKE))
        self->flags |= FL_NOGIB;

    // ROGUE
    //==============

    if (self->health < -40) {
        // PMM
        // don't toss gibs if we got vaped by the nuke
        if (!(self->flags & FL_NOGIB)) {
        // pmm
            int count;

            // gib
            gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

            // more meaty gibs for your dollar!
            if (deathmatch->integer && (self->health < -80))
                count = 8;
            else
                count = 4;

            while (count--)
                ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->x.scale);
        // PMM
        }
        self->flags &= ~FL_NOGIB;
        // pmm

        ThrowClientHead(self, damage);
        // ZOID
        self->client->anim_priority = ANIM_DEATH;
        self->client->anim_end = 0;
        // ZOID
        self->takedamage = false;
    } else {
        // normal death
        if (!self->deadflag) {
            // start a death animation
            self->client->anim_priority = ANIM_DEATH;
            if (self->client->ps.pmove.pm_flags & PMF_DUCKED) {
                self->s.frame = FRAME_crdeath1 - 1;
                self->client->anim_end = FRAME_crdeath5;
            } else {
                switch (irandom1(3)) {
                case 0:
                    self->s.frame = FRAME_death101 - 1;
                    self->client->anim_end = FRAME_death106;
                    break;
                case 1:
                    self->s.frame = FRAME_death201 - 1;
                    self->client->anim_end = FRAME_death206;
                    break;
                case 2:
                    self->s.frame = FRAME_death301 - 1;
                    self->client->anim_end = FRAME_death308;
                    break;
                }
            }
            static const char *const death_sounds[] = {
                "*death1.wav",
                "*death2.wav",
                "*death3.wav",
                "*death4.wav"
            };
            gi.sound(self, CHAN_VOICE, gi.soundindex(random_element(death_sounds)), 1, ATTN_NORM, 0);
            self->client->anim_time = 0;
        }
    }

    if (!self->deadflag) {
        if (coop->integer && (g_coop_squad_respawn->integer || g_coop_enable_lives->integer)) {
            if (g_coop_enable_lives->integer && self->client->pers.lives) {
                self->client->pers.lives--;
                self->client->resp.coop_respawn.lives--;
            }

            bool allPlayersDead = true;

            for (int i = 1; i <= game.maxclients; i++) {
                edict_t *player = &g_edicts[i];
                if (!player->inuse)
                    continue;
                if (player->health > 0 || (!level.deadly_kill_box && g_coop_enable_lives->integer && player->client->pers.lives > 0)) {
                    allPlayersDead = false;
                    break;
                }
            }

            if (allPlayersDead) { // allow respawns for telefrags and weird shit
                level.coop_level_restart_time = level.time + SEC(5);

                for (int i = 1; i <= game.maxclients; i++) {
                    edict_t *player = &g_edicts[i];
                    if (player->inuse)
                        gi.centerprintf(player, "Everyone is dead. You lose.\nRestarting level...");
                }
            }

            // in 3 seconds, attempt a respawn or put us into
            // spectator mode
            if (!level.coop_level_restart_time)
                self->client->respawn_time = level.time + SEC(3);
        }
    }

    self->deadflag = true;

    gi.linkentity(self);
}

//=======================================================================

// [Paril-KEX]
static void Player_GiveStartItems(edict_t *ent, const char *ptr)
{
    char copy[MAX_STRING_CHARS];
    Q_strlcpy(copy, ptr, sizeof(copy));

    const char *s = copy;
    while (1) {
        char *p = strchr(s, ';');
        if (p)
            *p = 0;

        const char *token = COM_Parse(&s);
        if (*token) {
            const gitem_t *item = FindItemByClassname(token);

            if (!item || !item->pickup)
                gi.error("Invalid g_start_item entry: %s\n", token);

            int count = 1;

            token = COM_Parse(&s);
            if (*token)
                count = Q_atoi(token);

            if (count == 0) {
                ent->client->pers.inventory[item->id] = 0;
            } else {
                edict_t *dummy = G_Spawn();
                dummy->item = item;
                dummy->count = count;
                dummy->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
                item->pickup(dummy, ent);
                G_FreeEdict(dummy);
            }
        }

        if (!p)
            break;
        s = p + 1;
    }
}

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant(edict_t *ent, gclient_t *client)
{
    // backup & restore userinfo
    char userinfo[MAX_INFO_STRING];
    Q_strlcpy(userinfo, client->pers.userinfo, sizeof(userinfo));

    memset(&client->pers, 0, sizeof(client->pers));
    ClientUserinfoChanged(ent, userinfo);

    client->pers.health = 100;
    client->pers.max_health = 100;

    // don't give us weapons if we shouldn't have any
    if ((G_TeamplayEnabled() && client->resp.ctf_team != CTF_NOTEAM) ||
        (!G_TeamplayEnabled() && !client->resp.spectator)) {
        // in coop, if there's already a player in the game and we're new,
        // steal their loadout. this would fix a potential softlock where a new
        // player may not have weapons at all.
        bool taken_loadout = false;

        if (coop->integer) {
            for (int i = 1; i <= game.maxclients; i++) {
                edict_t *player = &g_edicts[i];
                if (!player->inuse || player == ent || !player->client->pers.spawned ||
                    player->client->resp.spectator || player->movetype == MOVETYPE_NOCLIP)
                    continue;

                memcpy(client->pers.inventory, player->client->pers.inventory, sizeof(client->pers.inventory));
                memcpy(client->pers.max_ammo, player->client->pers.max_ammo, sizeof(client->pers.max_ammo));
                client->pers.power_cubes = player->client->pers.power_cubes;
                taken_loadout = true;
                break;
            }
        }

        if (!taken_loadout) {
            // fill with 50s, since it's our most common value
            for (int i = 0; i < AMMO_MAX; i++)
                client->pers.max_ammo[i] = 50;
            client->pers.max_ammo[AMMO_BULLETS] = 200;
            client->pers.max_ammo[AMMO_SHELLS] = 100;
            client->pers.max_ammo[AMMO_CELLS] = 200;

            // RAFAEL
            client->pers.max_ammo[AMMO_TRAP] = 5;
            // RAFAEL

            // ROGUE
            client->pers.max_ammo[AMMO_FLECHETTES] = 200;
            client->pers.max_ammo[AMMO_DISRUPTOR] = 12;
            client->pers.max_ammo[AMMO_TESLA] = 5;
            // ROGUE

            if (!deathmatch->integer || !g_instagib->integer)
                client->pers.inventory[IT_WEAPON_BLASTER] = 1;

            // [Kex]
            // start items!
            if (*g_start_items->string)
                Player_GiveStartItems(ent, g_start_items->string);
            else if (deathmatch->integer && g_instagib->integer) {
                client->pers.inventory[IT_WEAPON_RAILGUN] = 1;
                client->pers.inventory[IT_AMMO_SLUGS] = 99;
            }

            if (level.start_items && *level.start_items)
                Player_GiveStartItems(ent, level.start_items);

            // ZOID
            bool give_grapple = (!strcmp(g_allow_grapple->string, "auto")) ?
                                (ctf->integer ? !level.no_grapple : 0) :
                                g_allow_grapple->integer;

            if (give_grapple)
                client->pers.inventory[IT_WEAPON_GRAPPLE] = 1;
            // ZOID
        }

        NoAmmoWeaponChange(ent, false);

        client->pers.weapon = client->newweapon;
        if (client->newweapon)
            client->pers.selected_item = client->newweapon->id;
        client->newweapon = NULL;
        // ZOID
        client->pers.lastweapon = client->pers.weapon;
        // ZOID
    }

    if (coop->integer && g_coop_enable_lives->integer)
        client->pers.lives = g_coop_num_lives->integer + 1;

    if (ent->client->pers.autoshield >= AUTO_SHIELD_AUTO)
        ent->flags |= FL_WANTS_POWER_ARMOR;

    client->pers.connected = true;
    client->pers.spawned = true;
}

void InitClientResp(gclient_t *client)
{
    // ZOID
    ctfteam_t ctf_team = client->resp.ctf_team;
    bool id_state = client->resp.id_state;
    // ZOID

    memset(&client->resp, 0, sizeof(client->resp));

    // ZOID
    client->resp.ctf_team = ctf_team;
    client->resp.id_state = id_state;
    // ZOID

    client->resp.entertime = level.time;
    client->resp.coop_respawn = client->pers;
}

/*
==================
SaveClientData

Some information that should be persistent, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData(void)
{
    edict_t *ent;

    for (int i = 0; i < game.maxclients; i++) {
        ent = &g_edicts[1 + i];
        if (!ent->inuse)
            continue;
        game.clients[i].pers.health = ent->health;
        game.clients[i].pers.max_health = ent->max_health;
        game.clients[i].pers.savedFlags = (ent->flags & (FL_FLASHLIGHT | FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR | FL_WANTS_POWER_ARMOR));
        if (coop->integer)
            game.clients[i].pers.score = ent->client->resp.score;
    }
}

void FetchClientEntData(edict_t *ent)
{
    ent->health = ent->client->pers.health;
    ent->max_health = ent->client->pers.max_health;
    ent->flags |= ent->client->pers.savedFlags;
    if (coop->integer)
        ent->client->resp.score = ent->client->pers.score;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot(edict_t *spot)
{
    edict_t *player;
    float    bestplayerdistance;
    float    playerdistance;

    bestplayerdistance = 9999999;

    for (int n = 1; n <= game.maxclients; n++) {
        player = &g_edicts[n];

        if (!player->inuse)
            continue;

        if (player->health <= 0)
            continue;

        playerdistance = Distance(spot->s.origin, player->s.origin);

        if (playerdistance < bestplayerdistance)
            bestplayerdistance = playerdistance;
    }

    return bestplayerdistance;
}

bool SpawnPointClear(edict_t *spot)
{
    vec3_t p;
    VectorCopy(spot->s.origin, p);
    p[2] += 9;
    return !gi.trace(p, player_mins, player_maxs, p, spot, CONTENTS_PLAYER | CONTENTS_MONSTER).startsolid;
}

typedef struct {
    edict_t *spot;
    float dist;
} spawn_point_t;

static int add_spawn_points(spawn_point_t *spawn_points, int nb_spots, const char *classname)
{
    edict_t *spot = NULL;
    while (nb_spots < MAX_EDICTS_OLD && (spot = G_Find(spot, FOFS(classname), classname))) {
        spawn_point_t *p = &spawn_points[nb_spots++];
        p->spot = spot;
        p->dist = PlayersRangeFromSpot(spot);
    }
    return nb_spots;
}

static int spawnpointcmp(const void *p1, const void *p2)
{
    const spawn_point_t *a = (const spawn_point_t *)p1;
    const spawn_point_t *b = (const spawn_point_t *)p2;

    if (a->dist > b->dist)
        return 1;
    if (a->dist < b->dist)
        return -1;
    return 0;
}

edict_t *SelectDeathmatchSpawnPoint(bool farthest, bool force_spawn, bool fallback_to_ctf_or_start, bool *any_valid)
{
    spawn_point_t spawn_points[MAX_EDICTS_OLD];
    int nb_spots = 0;

    if (any_valid)
        *any_valid = false;

    // gather all spawn points
    nb_spots = add_spawn_points(spawn_points, nb_spots, "info_player_deathmatch");

    // no points
    if (nb_spots == 0) {
        if (!fallback_to_ctf_or_start)
            return NULL;

        // try CTF spawns...
        nb_spots = add_spawn_points(spawn_points, nb_spots, "info_player_team1");
        nb_spots = add_spawn_points(spawn_points, nb_spots, "info_player_team2");

        // we only have an info_player_start then
        if (nb_spots == 0) {
            nb_spots = add_spawn_points(spawn_points, nb_spots, "info_player_start");

            // map is malformed
            if (nb_spots == 0)
                return NULL;
        }
    }

    if (any_valid)
        *any_valid = true;

    // if there's only one spawn point, that's the one.
    if (nb_spots == 1) {
        if (force_spawn || SpawnPointClear(spawn_points[0].spot))
            return spawn_points[0].spot;

        return NULL;
    }

    // order by distances ascending (top of list has closest players to point)
    qsort(spawn_points, nb_spots, sizeof(spawn_points[0]), spawnpointcmp);

    // farthest spawn is simple
    if (!farthest) {
        // for random, select a random point other than the two
        // that are closest to the player if possible.
        // shuffle the non-distance-related spawn points
        for (int i = nb_spots - 3; i > 0; i--) {
            int j = irandom1(i + 1);
            SWAP(spawn_point_t, spawn_points[i + 2], spawn_points[j + 2]);
        }
        // if none clear, we have to pick one of the other two
    }

    // run down the list and pick the first one that we can use
    for (int i = nb_spots - 1; i >= 0; i--) {
        if (SpawnPointClear(spawn_points[i].spot))
            return spawn_points[i].spot;
    }

    // none clear
    if (force_spawn)
        return spawn_points[irandom1(nb_spots)].spot;

    return NULL;
}

//===============
// ROGUE
static edict_t *SelectLavaCoopSpawnPoint(edict_t *ent)
{
    int      index;
    edict_t *spot = NULL;
    float    lavatop;
    edict_t *lava;
    edict_t *pointWithLeastLava;
    float    lowest;
    edict_t *spawnPoints[64];
    vec3_t   center;
    int      numPoints;
    edict_t *highestlava;

    lavatop = -99999;
    highestlava = NULL;

    // first, find the highest lava
    // remember that some will stop moving when they've filled their
    // areas...
    lava = NULL;
    while (1) {
        lava = G_Find(lava, FOFS(classname), "func_water");
        if (!lava)
            break;

        VectorAvg(lava->absmin, lava->absmax, center);

        if ((lava->spawnflags & SPAWNFLAG_WATER_SMART) && (gi.pointcontents(center) & MASK_WATER)) {
            if (lava->absmax[2] > lavatop) {
                lavatop = lava->absmax[2];
                highestlava = lava;
            }
        }
    }

    // if we didn't find ANY lava, then return NULL
    if (!highestlava)
        return NULL;

    // find the top of the lava and include a small margin of error (plus bbox size)
    lavatop = highestlava->absmax[2] + 64;

    // find all the lava spawn points and store them in spawnPoints[]
    spot = NULL;
    numPoints = 0;
    while ((spot = G_Find(spot, FOFS(classname), "info_player_coop_lava"))) {
        if (numPoints == 64)
            break;

        spawnPoints[numPoints++] = spot;
    }

    // walk up the sorted list and return the lowest, open, non-lava spawn point
    spot = NULL;
    lowest = 999999;
    pointWithLeastLava = NULL;
    for (index = 0; index < numPoints; index++) {
        if (spawnPoints[index]->s.origin[2] < lavatop)
            continue;

        if (PlayersRangeFromSpot(spawnPoints[index]) <= 32)
            continue;

        if (spawnPoints[index]->s.origin[2] < lowest) {
            // save the last point
            pointWithLeastLava = spawnPoints[index];
            lowest = spawnPoints[index]->s.origin[2];
        }
    }

    return pointWithLeastLava;
}
// ROGUE
//===============

// [Paril-KEX]
static edict_t *SelectSingleSpawnPoint(edict_t *ent)
{
    edict_t *spot = NULL;

    while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL) {
        if (!game.spawnpoint[0] && !spot->targetname)
            break;

        if (!game.spawnpoint[0] || !spot->targetname)
            continue;

        if (Q_strcasecmp(game.spawnpoint, spot->targetname) == 0)
            break;
    }

    if (!spot) {
        // there wasn't a matching targeted spawnpoint, use one that has no targetname
        while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL)
            if (!spot->targetname)
                return spot;
    }

    // none at all, so just pick any
    if (!spot)
        return G_Find(spot, FOFS(classname), "info_player_start");

    return spot;
}

// [Paril-KEX]
static edict_t *G_UnsafeSpawnPosition(vec3_t spot, bool check_players)
{
    contents_t mask = MASK_PLAYERSOLID;

    if (!check_players)
        mask &= ~CONTENTS_PLAYER;

    trace_t tr = gi.trace(spot, player_mins, player_maxs, spot, NULL, mask);

    // sometimes the spot is too close to the ground, give it a bit of slack
    if (tr.startsolid && !tr.ent->client) {
        spot[2] += 1;
        tr = gi.trace(spot, player_mins, player_maxs, spot, NULL, mask);
    }

    // no idea why this happens in some maps..
    if (tr.startsolid && !tr.ent->client) {
        // try a nudge
        if (G_FixStuckObject_Generic(spot, player_mins, player_maxs, NULL, mask) == NO_GOOD_POSITION)
            return tr.ent; // what do we do here...?

        trace_t tr = gi.trace(spot, player_mins, player_maxs, spot, NULL, mask);

        if (tr.startsolid && !tr.ent->client)
            return tr.ent; // what do we do here...?
    }

    if (tr.fraction == 1.0f)
        return NULL;
    if (check_players && tr.ent && tr.ent->client)
        return tr.ent;

    return NULL;
}

static edict_t *SelectCoopSpawnPoint(edict_t *ent, bool force_spawn, bool check_players)
{
    edict_t *spot = NULL;
    const char *target;

    // ROGUE
    //  rogue hack, but not too gross...
    if (!Q_strcasecmp(level.mapname, "rmine2"))
        return SelectLavaCoopSpawnPoint(ent);
    // ROGUE

    // try the main spawn point first
    spot = SelectSingleSpawnPoint(ent);

    if (spot && !G_UnsafeSpawnPosition(spot->s.origin, check_players))
        return spot;

    spot = NULL;

    // assume there are four coop spots at each spawnpoint
    int num_valid_spots = 0;

    while (1) {
        spot = G_Find(spot, FOFS(classname), "info_player_coop");
        if (!spot)
            break; // we didn't have enough...

        target = spot->targetname;
        if (!target)
            target = "";
        if (Q_strcasecmp(game.spawnpoint, target) == 0) {
            // this is a coop spawn point for one of the clients here
            num_valid_spots++;

            if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
                return spot; // this is it
        }
    }

    bool use_targetname = true;

    // if we didn't find any spots, map is probably set up wrong.
    // use empty targetname ones.
    if (!num_valid_spots) {
        use_targetname = false;

        while (1) {
            spot = G_Find(spot, FOFS(classname), "info_player_coop");
            if (!spot)
                break; // we didn't have enough...

            target = spot->targetname;
            if (!target) {
                // this is a coop spawn point for one of the clients here
                num_valid_spots++;

                if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
                    return spot; // this is it
            }
        }
    }

    // if player collision is disabled, just pick a random spot
    if (!g_coop_player_collision->integer) {
        spot = NULL;

        num_valid_spots = irandom1(num_valid_spots);

        while (1) {
            spot = G_Find(spot, FOFS(classname), "info_player_coop");
            if (!spot)
                break; // we didn't have enough...

            target = spot->targetname;
            if (use_targetname && !target)
                target = "";
            if (use_targetname ? (Q_strcasecmp(game.spawnpoint, target) == 0) : !target) {
                // this is a coop spawn point for one of the clients here
                num_valid_spots++;

                if (!num_valid_spots)
                    return spot;

                --num_valid_spots;
            }
        }

        // if this fails, just fall through to some other spawn.
    }

    // no safe spots..?
    if (force_spawn || !g_coop_player_collision->integer)
        return SelectSingleSpawnPoint(spot);

    return NULL;
}

static bool TryLandmarkSpawn(edict_t *ent, vec3_t origin, vec3_t angles)
{
    // if transitioning from another level with a landmark seamless transition
    // just set the location here
    if (!ent->client->landmark_name || !ent->client->landmark_name[0])
        return false;

    edict_t *landmark = G_PickTarget(ent->client->landmark_name);
    if (!landmark)
        return false;

    vec3_t point;
    VectorCopy(ent->client->landmark_rel_pos, point);

    // rotate our relative landmark into our new landmark's frame of reference
    vec3_t axis[3];
    AnglesToAxis(landmark->s.angles, axis);
    RotatePoint(point, axis);

    VectorAdd(point, landmark->s.origin, point);

    if (landmark->spawnflags & SPAWNFLAG_LANDMARK_KEEP_Z)
        point[2] = origin[2];

    // sometimes, landmark spawns can cause slight inconsistencies in collision;
    // we'll do a bit of tracing to make sure the bbox is clear
    if (G_FixStuckObject_Generic(point, player_mins, player_maxs, ent, MASK_PLAYERSOLID & ~CONTENTS_PLAYER) == NO_GOOD_POSITION)
        return false;

    VectorCopy(point, origin);
    VectorAdd(ent->client->oldviewangles, landmark->s.angles, angles);

    // rotate the velocity that we grabbed from the map
    RotatePoint(ent->velocity, axis);

    return true;
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
bool SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles, bool force_spawn)
{
    edict_t *spot = NULL;

    // DM spots are simple
    if (deathmatch->integer) {
        if (G_TeamplayEnabled())
            spot = SelectCTFSpawnPoint(ent, force_spawn);
        else {
            bool any_valid;
            spot = SelectDeathmatchSpawnPoint(g_dm_spawn_farthest->integer, force_spawn, true, &any_valid);

            if (!any_valid)
                gi.error("no valid spawn points found");
        }

        if (spot) {
            VectorCopy(spot->s.origin, origin);
            origin[2] += 9;
            VectorCopy(spot->s.angles, angles);
            return true;
        }

        return false;
    }

    if (coop->integer) {
        spot = SelectCoopSpawnPoint(ent, force_spawn, true);

        if (!spot)
            spot = SelectCoopSpawnPoint(ent, force_spawn, false);

        // no open spot yet
        if (!spot) {
            // in worst case scenario in coop during intermission, just spawn us at intermission
            // spot. this only happens for a single frame, and won't break
            // anything if they come back.
            if (level.intermissiontime) {
                VectorCopy(level.intermission_origin, origin);
                VectorCopy(level.intermission_angle, angles);
                return true;
            }

            return false;
        }
    } else {
        spot = SelectSingleSpawnPoint(ent);

        // in SP, just put us at the origin if spawn fails
        if (!spot) {
            gi.dprintf("Couldn't find spawn point %s\n", game.spawnpoint);
            VectorClear(origin);
            VectorClear(angles);
            return true;
        }
    }

    // spot should always be non-null here

    VectorCopy(spot->s.origin, origin);
    VectorCopy(spot->s.angles, angles);

    // check landmark
    TryLandmarkSpawn(ent, origin, angles);
    return true;
}

//======================================================================

void InitBodyQue(void)
{
    int      i;
    edict_t *ent;

    level.body_que = 0;
    for (i = 0; i < BODY_QUEUE_SIZE; i++) {
        ent = G_Spawn();
        ent->classname = "bodyque";
    }
}

void DIE(body_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (self->s.modelindex == MODELINDEX_PLAYER && self->health < self->gib_health) {
        gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        for (int n = 0; n < 4; n++)
            ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_NONE, self->x.scale);
        self->s.origin[2] -= 48;
        ThrowClientHead(self, damage);
    }

    if (mod.id == MOD_CRUSH) {
        // prevent explosion singularities
        self->svflags = SVF_NOCLIENT;
        self->takedamage = false;
        self->solid = SOLID_NOT;
        self->movetype = MOVETYPE_NOCLIP;
        gi.linkentity(self);
    }
}

static void CopyToBodyQue(edict_t *ent)
{
    // if we were completely removed, don't bother with a body
    if (!ent->s.modelindex)
        return;

    edict_t *body;

    // grab a body que and cycle to the next one
    body = &g_edicts[game.maxclients + level.body_que + 1];
    level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

    // FIXME: send an effect on the removed body

    gi.unlinkentity(ent);
    gi.unlinkentity(body);

    body->s = ent->s;
    body->s.number = body - g_edicts;
    body->s.skinnum = ent->s.skinnum & 0xFF; // only copy the client #
    body->s.effects = EF_NONE;
    body->s.renderfx = RF_NONE;

    body->svflags = ent->svflags;
    body->solid = ent->solid;
    body->clipmask = ent->clipmask;
    body->owner = ent->owner;
    body->movetype = ent->movetype;
    body->health = ent->health;
    body->gib_health = ent->gib_health;
    body->s.event = EV_OTHER_TELEPORT;
    VectorCopy(ent->velocity, body->velocity);
    VectorCopy(ent->avelocity, body->avelocity);
    body->groundentity = ent->groundentity;
    body->groundentity_linkcount = ent->groundentity_linkcount;

    if (ent->takedamage) {
        VectorCopy(ent->mins, body->mins);
        VectorCopy(ent->maxs, body->maxs);
    } else {
        VectorClear(body->mins);
        VectorClear(body->maxs);
    }

    body->die = body_die;
    body->takedamage = true;

    gi.linkentity(body);
}

void G_PostRespawn(edict_t *self)
{
    if (self->svflags & SVF_NOCLIENT)
        return;

    // add a teleportation effect
    self->s.event = EV_PLAYER_TELEPORT;

    // hold in place briefly
    self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
    self->client->ps.pmove.pm_time = 112 >> PM_TIME_SHIFT;

    self->client->respawn_time = level.time;
}

void respawn(edict_t *self)
{
    if (deathmatch->integer || coop->integer) {
        // spectators don't leave bodies
        if (!self->client->resp.spectator)
            CopyToBodyQue(self);
        self->svflags &= ~SVF_NOCLIENT;
        PutClientInServer(self);

        G_PostRespawn(self);
        return;
    }

    // restart the entire server
    gi.AddCommandString("menu_loadgame\n");
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
static void spectator_respawn(edict_t *ent)
{
    int i, numspec;

    // if the user wants to become a spectator, make sure he doesn't
    // exceed max_spectators

    if (ent->client->pers.spectator) {
        char *value = Info_ValueForKey(ent->client->pers.userinfo, "spectator");

        if (*spectator_password->string &&
            strcmp(spectator_password->string, "none") &&
            strcmp(spectator_password->string, value)) {
            gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
            ent->client->pers.spectator = false;
            gi.WriteByte(svc_stufftext);
            gi.WriteString("spectator 0\n");
            gi.unicast(ent, true);
            return;
        }

        // count spectators
        for (i = 1, numspec = 0; i <= game.maxclients; i++)
            if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
                numspec++;

        if (numspec >= maxspectators->integer) {
            gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
            ent->client->pers.spectator = false;
            // reset his spectator var
            gi.WriteByte(svc_stufftext);
            gi.WriteString("spectator 0\n");
            gi.unicast(ent, true);
            return;
        }
    } else {
        // he was a spectator and wants to join the game
        // he must have the right password
        char *value = Info_ValueForKey(ent->client->pers.userinfo, "password");

        if (*password->string && strcmp(password->string, "none") &&
            strcmp(password->string, value)) {
            gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
            ent->client->pers.spectator = true;
            gi.WriteByte(svc_stufftext);
            gi.WriteString("spectator 1\n");
            gi.unicast(ent, true);
            return;
        }
    }

    // clear score on respawn
    ent->client->resp.score = ent->client->pers.score = 0;

    // move us to no team
    ent->client->resp.ctf_team = CTF_NOTEAM;

    // change spectator mode
    ent->client->resp.spectator = ent->client->pers.spectator;

    ent->svflags &= ~SVF_NOCLIENT;
    PutClientInServer(ent);

    // add a teleportation effect
    if (!ent->client->pers.spectator) {
        // send effect
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_LOGIN);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        // hold in place briefly
        ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
        ent->client->ps.pmove.pm_time = 112 >> PM_TIME_SHIFT;
    }

    ent->client->respawn_time = level.time;

    if (ent->client->pers.spectator)
        gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
    else
        gi.bprintf(PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}

//==============================================================

// [Paril-KEX]
// skinnum was historically used to pack data
// so we're going to build onto that.
void P_AssignClientSkinnum(edict_t *ent)
{
    if (ent->s.modelindex != MODELINDEX_PLAYER)
        return;

    int client_num = ent->client - game.clients;
    int vwep_index = 0;
    const gitem_t *item = ent->client->pers.weapon;
    if (item)
        vwep_index = gi.modelindex(item->vwep_model) - level.vwep_offset + 1;

    ent->s.skinnum = client_num | (vwep_index << 8);
}

// [Paril-KEX] ugly global to handle squad respawn origin
static bool use_squad_respawn;
static bool spawn_from_begin;
static vec3_t squad_respawn_position, squad_respawn_angles;

static void PutClientOnSpawnPoint(edict_t *ent, const vec3_t spawn_origin, const vec3_t spawn_angles)
{
    gclient_t *client = ent->client;

    for (int i = 0; i < 3; i++)
        client->ps.pmove.origin[i] = COORD2SHORT(spawn_origin[i]);

    VectorCopy(spawn_origin, ent->s.origin);
    if (!use_squad_respawn)
        ent->s.origin[2] += 1; // make sure off ground
    VectorCopy(ent->s.origin, ent->s.old_origin);

    // set the delta angle
    for (int i = 0; i < 3; i++)
        client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

    VectorCopy(spawn_angles, ent->s.angles);
    ent->s.angles[PITCH] /= 3;

    VectorCopy(spawn_angles, client->ps.viewangles);
    VectorCopy(spawn_angles, client->v_angle);

    AngleVectors(client->v_angle, client->v_forward, NULL, NULL);
}

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(edict_t *ent)
{
    int                 index;
    vec3_t              spawn_origin, spawn_angles;
    gclient_t         *client;
    client_persistent_t saved;
    client_respawn_t    resp;

    index = ent - g_edicts - 1;
    client = ent->client;

    // clear velocity now, since landmark may change it
    if (client->landmark_name)
        VectorCopy(client->oldvelocity, ent->velocity);
    else
        VectorClear(ent->velocity);

    // find a spawn point
    // do it before setting health back up, so farthest
    // ranging doesn't count this client
    bool valid_spawn = false;
    bool force_spawn = client->awaiting_respawn && level.time > client->respawn_timeout;

    if (use_squad_respawn) {
        VectorCopy(squad_respawn_position, spawn_origin);
        VectorCopy(squad_respawn_angles, spawn_angles);
        valid_spawn = true;
    // PGM
    } else if (gamerules->integer && DMGame.SelectSpawnPoint)
        valid_spawn = DMGame.SelectSpawnPoint(ent, spawn_origin, spawn_angles, force_spawn);
    // PGM
    else
        valid_spawn = SelectSpawnPoint(ent, spawn_origin, spawn_angles, force_spawn);

    // [Paril-KEX] if we didn't get a valid spawn, hold us in
    // limbo for a while until we do get one
    if (!valid_spawn) {
        // only do this once per spawn
        if (!client->awaiting_respawn) {
            char userinfo[MAX_INFO_STRING];
            memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
            ClientUserinfoChanged(ent, userinfo);

            client->respawn_timeout = level.time + SEC(3);
        }

        // find a spot to place us
        if (!level.respawn_intermission) {
            // find an intermission spot
            edict_t *pt = G_Find(NULL, FOFS(classname), "info_player_intermission");
            if (!pt) {
                // the map creator forgot to put in an intermission point...
                pt = G_Find(NULL, FOFS(classname), "info_player_start");
                if (!pt)
                    pt = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
            } else {
                // choose one of four spots
                int i = irandom1(4);
                while (i--) {
                    pt = G_Find(pt, FOFS(classname), "info_player_intermission");
                    if (!pt) // wrap around the list
                        pt = G_Find(pt, FOFS(classname), "info_player_intermission");
                }
            }

            VectorCopy(pt->s.origin, level.intermission_origin);
            VectorCopy(pt->s.angles, level.intermission_angle);
            level.respawn_intermission = true;
        }

        VectorCopy(level.intermission_origin, ent->s.origin);
        for (int i = 0; i < 3; i++)
            ent->client->ps.pmove.origin[i] = COORD2SHORT(level.intermission_origin[i]);
        VectorCopy(level.intermission_angle, ent->client->ps.viewangles);

        client->awaiting_respawn = true;
        client->ps.pmove.pm_type = PM_FREEZE;
        client->ps.rdflags = RDF_NONE;
        ent->deadflag = false;
        ent->solid = SOLID_NOT;
        ent->movetype = MOVETYPE_NOCLIP;
        ent->s.modelindex = 0;
        ent->svflags |= SVF_NOCLIENT;
        //ent->client->ps.team_id = ent->client->resp.ctf_team;
        gi.linkentity(ent);

        return;
    }

    client->resp.ctf_state++;

    bool was_waiting_for_respawn = client->awaiting_respawn;

    if (client->awaiting_respawn)
        ent->svflags &= ~SVF_NOCLIENT;

    client->awaiting_respawn = false;
    client->respawn_timeout = 0;

    char social_id[MAX_INFO_VALUE];
    Q_strlcpy(social_id, ent->client->pers.social_id, sizeof(social_id));

    // deathmatch wipes most client data every spawn
    if (deathmatch->integer) {
        client->pers.health = 0;
        resp = client->resp;
    } else {
        // [Kex] Maintain user info in singleplayer to keep the player skin.
        char userinfo[MAX_INFO_STRING];
        memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));

        if (coop->integer) {
            resp = client->resp;

            if (!P_UseCoopInstancedItems()) {
                resp.coop_respawn.game_help1changed = client->pers.game_help1changed;
                resp.coop_respawn.game_help2changed = client->pers.game_help2changed;
                resp.coop_respawn.helpchanged = client->pers.helpchanged;
                client->pers = resp.coop_respawn;
            } else {
                // fix weapon
                if (!client->pers.weapon)
                    client->pers.weapon = client->pers.lastweapon;
            }
        }

        ClientUserinfoChanged(ent, userinfo);

        if (coop->integer) {
            if (resp.score > client->pers.score)
                client->pers.score = resp.score;
        } else
            memset(&resp, 0, sizeof(resp));
    }

    // clear everything but the persistent data
    saved = client->pers;
    memset(client, 0, sizeof(*client));
    client->pers = saved;
    client->resp = resp;

    // on a new, fresh spawn (always in DM, clear inventory
    // or new spawns in SP/coop)
    if (client->pers.health <= 0)
        InitClientPersistant(ent, client);

    // restore social ID
    Q_strlcpy(ent->client->pers.social_id, social_id, sizeof(social_id));

    // fix level switch issue
    ent->client->pers.connected = true;

    // copy some data from the client to the entity
    FetchClientEntData(ent);

    // clear entity values
    ent->groundentity = NULL;
    ent->client = &game.clients[index];
    ent->takedamage = true;
    ent->movetype = MOVETYPE_WALK;
    ent->viewheight = 22;
    ent->inuse = qtrue;
    ent->classname = "player";
    ent->mass = 200;
    ent->solid = SOLID_BBOX;
    ent->deadflag = false;
    ent->air_finished = level.time + SEC(12);
    ent->clipmask = MASK_PLAYERSOLID;
    ent->model = "players/male/tris.md2";
    ent->die = player_die;
    ent->waterlevel = WATER_NONE;
    ent->watertype = CONTENTS_NONE;
    ent->flags &= ~(FL_NO_KNOCKBACK | FL_ALIVE_KNOCKBACK_ONLY | FL_NO_DAMAGE_EFFECTS);
    ent->svflags &= ~SVF_DEADMONSTER;
    ent->svflags |= SVF_PLAYER;

    ent->flags &= ~FL_SAM_RAIMI; // PGM - turn off sam raimi flag

    VectorCopy(player_mins, ent->mins);
    VectorCopy(player_maxs, ent->maxs);

    // clear playerstate values
    memset(&ent->client->ps, 0, sizeof(client->ps));

    char *val = Info_ValueForKey(ent->client->pers.userinfo, "fov");
    ent->client->ps.fov = Q_clip(Q_atoi(val), 1, 160);

    if (!G_ShouldPlayersCollide(false))
        ent->clipmask &= ~CONTENTS_PLAYER;

    // PGM
    if (client->pers.weapon)
        client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
    else
        client->ps.gunindex = 0;
    // PGM

    // clear entity state values
    ent->s.effects = EF_NONE;
    ent->s.modelindex = MODELINDEX_PLAYER;  // will use the skin specified model
    ent->s.modelindex2 = MODELINDEX_PLAYER; // custom gun model
    // sknum is player num and weapon number
    // weapon number will be added in changeweapon
    P_AssignClientSkinnum(ent);

    ent->s.frame = 0;

    PutClientOnSpawnPoint(ent, spawn_origin, spawn_angles);

    // ZOID
    if (CTFStartClient(ent))
        return;
    // ZOID

    // spawn a spectator
    if (client->pers.spectator) {
        client->chase_target = NULL;

        client->resp.spectator = true;

        ent->movetype = MOVETYPE_NOCLIP;
        ent->solid = SOLID_NOT;
        ent->svflags |= SVF_NOCLIENT;
        ent->client->ps.gunindex = 0;
        gi.linkentity(ent);
        return;
    }

    client->resp.spectator = false;

    // [Paril-KEX] a bit of a hack, but landmark spawns can sometimes cause
    // intersecting spawns, so we'll do a sanity check here...
    if (spawn_from_begin) {
        if (coop->integer) {
            edict_t *collision = G_UnsafeSpawnPosition(ent->s.origin, true);

            if (collision) {
                gi.linkentity(ent);

                if (collision->client) {
                    // we spawned in somebody else, so we're going to change their spawn position
                    SelectSpawnPoint(collision, spawn_origin, spawn_angles, true);
                    PutClientOnSpawnPoint(collision, spawn_origin, spawn_angles);
                }
                // else, no choice but to accept where ever we spawned :(
            }
        }

        // give us one (1) free fall ticket even if
        // we didn't spawn from landmark
        ent->client->landmark_free_fall = true;
    }

    gi.linkentity(ent);

    if (!KillBox(ent, true, MOD_TELEFRAG_SPAWN, false)) {
        // could't spawn in?
    }

    // my tribute to cash's level-specific hacks. I hope I live
    // up to his trailblazing cheese.
    if (Q_strcasecmp(level.mapname, "rboss") == 0) {
        // if you get on to rboss in single player or coop, ensure
        // the player has the nuke key. (not in DM)
        if (!deathmatch->integer)
            client->pers.inventory[IT_KEY_NUKE] = 1;
    }

    // force the current weapon up
    client->newweapon = client->pers.weapon;
    ChangeWeapon(ent);

    if (was_waiting_for_respawn)
        G_PostRespawn(ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
static void ClientBeginDeathmatch(edict_t *ent)
{
    G_InitEdict(ent);

    // make sure we have a known default
    ent->svflags |= SVF_PLAYER;

    InitClientResp(ent->client);

    // ZOID
    if (G_TeamplayEnabled() && ent->client->resp.ctf_team < CTF_TEAM1)
        CTFAssignTeam(ent->client);
    // ZOID

    // PGM
    if (gamerules->integer && DMGame.ClientBegin) {
        DMGame.ClientBegin(ent);
    }
    // PGM

    // locate ent at a spawn point
    PutClientInServer(ent);

    if (level.intermissiontime) {
        MoveClientToIntermission(ent);
    } else {
        if (!(ent->svflags & SVF_NOCLIENT)) {
            // send effect
            gi.WriteByte(svc_muzzleflash);
            gi.WriteShort(ent - g_edicts);
            gi.WriteByte(MZ_LOGIN);
            gi.multicast(ent->s.origin, MULTICAST_PVS);
        }
    }

    gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

    // make sure all view stuff is valid
    ClientEndServerFrame(ent);
}

static void G_SetLevelEntry(void)
{
    if (deathmatch->integer)
        return;
    // map is a hub map, so we shouldn't bother tracking any of this.
    // the next map will pick up as the start.
    if (level.hub_map)
        return;

    level_entry_t *found_entry = NULL;
    int highest_order = 0;

    for (int i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
        level_entry_t *entry = &game.level_entries[i];

        highest_order = max(highest_order, entry->visit_order);

        if (!strcmp(entry->map_name, level.mapname) || !*entry->map_name) {
            found_entry = entry;
            break;
        }
    }

    if (!found_entry) {
        gi.dprintf("WARNING: more than %d maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
        return;
    }

    level.entry = found_entry;
    Q_strlcpy(level.entry->map_name, level.mapname, sizeof(level.entry->map_name));

    // we're visiting this map for the first time, so
    // mark it in our order as being recent
    if (!*level.entry->pretty_name) {
        Q_strlcpy(level.entry->pretty_name, level.level_name, sizeof(level.entry->pretty_name));
        level.entry->visit_order = highest_order + 1;

        // give all of the clients an extra life back
        if (g_coop_enable_lives->integer)
            for (int i = 0; i < game.maxclients; i++)
                game.clients[i].pers.lives = min(g_coop_num_lives->integer + 1, game.clients[i].pers.lives + 1);
    }

    // scan for all new maps we can go to, for secret levels
    edict_t *changelevel = NULL;
    while ((changelevel = G_Find(changelevel, FOFS(classname), "target_changelevel"))) {
        if (!changelevel->map || !*changelevel->map)
            continue;

        // next unit map, don't count it
        if (strchr(changelevel->map, '*'))
            continue;

        const char *level = strchr(changelevel->map, '+');

        if (level)
            level++;
        else
            level = changelevel->map;

        // don't include end screen levels
        if (strstr(level, ".cin") || strstr(level, ".pcx"))
            continue;

        size_t level_length;

        const char *spawnpoint = strchr(level, '$');

        if (spawnpoint)
            level_length = spawnpoint - level;
        else
            level_length = strlen(level);

        // make an entry for this level that we may or may not visit
        level_entry_t *found_entry = NULL;

        for (int i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
            level_entry_t *entry = &game.level_entries[i];

            if (!*entry->map_name || !strncmp(entry->map_name, level, level_length)) {
                found_entry = entry;
                break;
            }
        }

        if (!found_entry) {
            gi.dprintf("WARNING: more than %d maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
            return;
        }

        Q_strlcpy(found_entry->map_name, level, min(level_length + 1, sizeof(found_entry->map_name)));
    }
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(edict_t *ent)
{
    ent->client = game.clients + (ent - g_edicts - 1);
    ent->client->awaiting_respawn = false;
    ent->client->respawn_timeout = 0;

    // [Paril-KEX] we're always connected by this point...
    ent->client->pers.connected = true;

    if (deathmatch->integer) {
        ClientBeginDeathmatch(ent);
        return;
    }

    // [Paril-KEX] set enter time now, so we can send messages slightly
    // after somebody first joins
    ent->client->resp.entertime = level.time;
    ent->client->pers.spawned = true;

    // if there is already a body waiting for us (a loadgame), just
    // take it, otherwise spawn one from scratch
    if (ent->inuse) {
        // the client has cleared the client side viewangles upon
        // connecting to the server, which is different than the
        // state when the game is saved, so we need to compensate
        // with deltaangles
        for (int i = 0; i < 3; i++)
            ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
    } else {
        // a spawn point will completely reinitialize the entity
        // except for the persistent data that was initialized at
        // ClientConnect() time
        G_InitEdict(ent);
        ent->classname = "player";
        InitClientResp(ent->client);
        spawn_from_begin = true;
        PutClientInServer(ent);
        spawn_from_begin = false;
    }

    // make sure we have a known default
    ent->svflags |= SVF_PLAYER;

    if (level.intermissiontime) {
        MoveClientToIntermission(ent);
    } else {
        // send effect if in a multiplayer game
        if (game.maxclients > 1 && !(ent->svflags & SVF_NOCLIENT))
            gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
    }

    level.coop_scale_players++;
    G_Monster_CheckCoopHealthScaling();

    // make sure all view stuff is valid
    ClientEndServerFrame(ent);

    // [Paril-KEX] send them goal, if needed
    G_PlayerNotifyGoal(ent);

    // [Paril-KEX] we're going to set this here just to be certain
    // that the level entry timer only starts when a player is actually
    // *in* the level
    G_SetLevelEntry();
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.
============
*/
void ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
    // set name
    char *val = Info_ValueForKey(userinfo, "name");
    if (!*val)
        val = "badinfo";
    Q_strlcpy(ent->client->pers.netname, val, sizeof(ent->client->pers.netname));

    // set spectator
    val = Info_ValueForKey(userinfo, "spectator");

    // spectators are only supported in deathmatch
    if (deathmatch->integer && !G_TeamplayEnabled() && *val && strcmp(val, "0"))
        ent->client->pers.spectator = true;
    else
        ent->client->pers.spectator = false;

    // set skin
    val = Info_ValueForKey(userinfo, "skin");
    if (!*val)
        val = "male/grunt";

    int playernum = ent - g_edicts - 1;

    // combine name and skin into a configstring
    // ZOID
    if (G_TeamplayEnabled())
        CTFAssignSkin(ent, val);
    else
    // ZOID
        gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s", ent->client->pers.netname, val));

    // ZOID
    //  set player name field (used in id_state view)
    if (G_TeamplayEnabled())
        gi.configstring(CONFIG_CTF_PLAYER_NAME + playernum, ent->client->pers.netname);
    // ZOID

    // fov
    val = Info_ValueForKey(userinfo, "fov");
    ent->client->ps.fov = Q_clip(Q_atoi(val), 1, 160);

    // handedness
    val = Info_ValueForKey(userinfo, "hand");
    if (*val) {
        ent->client->pers.hand = Q_clip(Q_atoi(val), RIGHT_HANDED, CENTER_HANDED);
    } else {
        ent->client->pers.hand = RIGHT_HANDED;
    }

    // [Paril-KEX] auto-switch
    val = Info_ValueForKey(userinfo, "autoswitch");
    if (*val) {
        ent->client->pers.autoswitch = Q_clip(Q_atoi(val), AUTOSW_SMART, AUTOSW_NEVER);
    } else {
        ent->client->pers.autoswitch = AUTOSW_SMART;
    }

    val = Info_ValueForKey(userinfo, "autoshield");
    if (*val) {
        ent->client->pers.autoshield = Q_atoi(val);
    } else {
        ent->client->pers.autoshield = -1;
    }

    // [Paril-KEX] wants bob
    val = Info_ValueForKey(userinfo, "bobskip");
    if (*val) {
        ent->client->pers.bob_skip = !!Q_atoi(val);
    } else {
        ent->client->pers.bob_skip = false;
    }

    // save off the userinfo in case we want to check something later
    Q_strlcpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo));
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect(edict_t *ent, char *userinfo)
{
    // check to see if they are on the banned IP list
#if 0
    value = Info_ValueForKey(userinfo, "ip");
    if (SV_FilterPacket(value)) {
        Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
        return qfalse;
    }
#endif

    // check for a spectator
    char *value = Info_ValueForKey(userinfo, "spectator");

    if (deathmatch->integer && *value && strcmp(value, "0")) {
        int i, numspec;

        if (*spectator_password->string &&
            strcmp(spectator_password->string, "none") &&
            strcmp(spectator_password->string, value)) {
            Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
            return qfalse;
        }

        // count spectators
        for (i = numspec = 0; i < game.maxclients; i++)
            if (g_edicts[i + 1].inuse && g_edicts[i + 1].client->pers.spectator)
                numspec++;

        if (numspec >= maxspectators->integer) {
            Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
            return qfalse;
        }
    } else {
        // check for a password ( if not a bot! )
        value = Info_ValueForKey(userinfo, "password");
        if (*password->string && strcmp(password->string, "none") &&
            strcmp(password->string, value)) {
            Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
            return qfalse;
        }
    }

    // they can connect
    ent->client = game.clients + (ent - g_edicts - 1);

    // set up userinfo early
    ClientUserinfoChanged(ent, userinfo);

    // if there is already a body waiting for us (a loadgame), just
    // take it, otherwise spawn one from scratch
    if (ent->inuse == false) {
        // clear the respawning variables
        // ZOID -- force team join
        ent->client->resp.ctf_team = CTF_NOTEAM;
        ent->client->resp.id_state = true;
        // ZOID
        InitClientResp(ent->client);
        if (!game.autosaved || !ent->client->pers.weapon)
            InitClientPersistant(ent, ent->client);
    }

    // make sure we start with known default(s)
    ent->svflags = SVF_PLAYER;

    if (game.maxclients > 1) {
        // [Paril-KEX] fetch name because now netname is kinda unsuitable
        value = Info_ValueForKey(userinfo, "name");
        gi.cprintf(NULL, PRINT_HIGH, "%s connected.\n", value);
    }

    ent->client->pers.connected = true;
    return qtrue;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(edict_t *ent)
{
    if (!ent->client)
        return;

    // ZOID
    CTFDeadDropFlag(ent);
    CTFDeadDropTech(ent);
    // ZOID

    PlayerTrail_Destroy(ent);

    //============
    // ROGUE
    // make sure no trackers are still hurting us.
    if (ent->client->tracker_pain_time)
        RemoveAttackingPainDaemons(ent);

    if (ent->client->owned_sphere) {
        if (ent->client->owned_sphere->inuse)
            G_FreeEdict(ent->client->owned_sphere);
        ent->client->owned_sphere = NULL;
    }

    if (gamerules->integer) {
        if (DMGame.PlayerDisconnect)
            DMGame.PlayerDisconnect(ent);
    }
    // ROGUE
    //============

    // send effect
    if (!(ent->svflags & SVF_NOCLIENT)) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_LOGOUT);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }

    gi.unlinkentity(ent);
    ent->s.modelindex = 0;
    ent->solid = SOLID_NOT;
    ent->inuse = qfalse;
    ent->classname = "disconnected";
    ent->client->pers.connected = false;
    ent->client->pers.spawned = false;
    ent->timestamp = level.time + SEC(1);

    // update active scoreboards
    if (deathmatch->integer) {
        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (player->inuse && player->client->showscores)
                player->client->menutime = level.time;
        }
    }
}

//==============================================================

bool G_ShouldPlayersCollide(bool weaponry)
{
    if (g_disable_player_collision->integer)
        return false; // only for debugging.

    // always collide on dm
    if (!coop->integer)
        return true;

    // weaponry collides if friendly fire is enabled
    if (weaponry && g_friendly_fire->integer)
        return true;

    // check collision cvar
    return g_coop_player_collision->integer;
}

static bool HandleMenuMovement(edict_t *ent, usercmd_t *ucmd)
{
    if (!ent->client->menu)
        return false;

    // [Paril-KEX] handle menu movement
    int menu_sign = ucmd->forwardmove > 0 ? 1 : ucmd->forwardmove < 0 ? -1 : 0;

    if (ent->client->menu_sign != menu_sign) {
        ent->client->menu_sign = menu_sign;

        if (menu_sign > 0) {
            PMenu_Prev(ent);
            return true;
        }
        if (menu_sign < 0) {
            PMenu_Next(ent);
            return true;
        }
    }

    if (ent->client->latched_buttons & (BUTTON_ATTACK | BUTTON_JUMP)) {
        PMenu_Select(ent);
        return true;
    }

    return false;
}

static edict_t *pm_passent;
static contents_t pm_clipmask;

// pmove doesn't need to know about passent and contentmask
static trace_t PM_trace(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end)
{
    return gi.trace(start, mins, maxs, end, pm_passent, pm_clipmask);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink(edict_t *ent, usercmd_t *ucmd)
{
    gclient_t *client;
    edict_t   *other;
    int        i, j;
    pmove_t    pm;

    level.current_entity = ent;
    client = ent->client;

    // [Paril-KEX] pass buttons through even if we are in intermission or
    // chasing.
    client->oldbuttons = client->buttons;
    client->buttons = ucmd->buttons;
    client->latched_buttons |= client->buttons & ~client->oldbuttons;
    client->cmd = *ucmd;

    if (level.intermissiontime || ent->client->awaiting_respawn) {
        client->ps.pmove.pm_type = PM_FREEZE;

        bool n64_sp = false;

        if (level.intermissiontime) {
            n64_sp = !deathmatch->integer && level.is_n64;

            // can exit intermission after five seconds
            // Paril: except in N64. the camera handles it.
            // Paril again: except on unit exits, we can leave immediately after camera finishes
            if (level.changemap && (!n64_sp || level.level_intermission_set) && level.time > level.intermissiontime + SEC(5) && (ucmd->buttons & BUTTON_ANY))
                level.exitintermission = true;
        }

        if (!n64_sp)
            ent->viewheight = 22;
        else
            ent->viewheight = 0;
        ent->movetype = MOVETYPE_NOCLIP;
        return;
    }

    if (ent->client->chase_target) {
        for (i = 0; i < 3; i++)
            client->resp.cmd_angles[i] = SHORT2ANGLE(ucmd->angles[i]);
        ent->movetype = MOVETYPE_NOCLIP;
    } else {
        // set up for pmove
        memset(&pm, 0, sizeof(pm));

        if (ent->movetype == MOVETYPE_NOCLIP) {
            if (ent->client->menu) {
                client->ps.pmove.pm_type = PM_FREEZE;

                // [Paril-KEX] handle menu movement
                HandleMenuMovement(ent, ucmd);
            } else
                client->ps.pmove.pm_type = PM_SPECTATOR;
        } else if (ent->s.modelindex != MODELINDEX_PLAYER)
            client->ps.pmove.pm_type = PM_GIB;
        else if (ent->deadflag)
            client->ps.pmove.pm_type = PM_DEAD;
        else
            client->ps.pmove.pm_type = PM_NORMAL;

        pm_passent = ent;
        if (client->ps.pmove.pm_type == PM_DEAD || client->ps.pmove.pm_type == PM_GIB)
            pm_clipmask = MASK_DEADSOLID;
        else
            pm_clipmask = MASK_PLAYERSOLID;

        // [Paril-KEX]
        if (!G_ShouldPlayersCollide(false) ||
            (coop->integer && !(ent->clipmask & CONTENTS_PLAYER)) // if player collision is on and we're temporarily ghostly...
           )
        {
            pm_clipmask &= ~CONTENTS_PLAYER;
            client->ps.pmove.pm_flags |= PMF_IGNORE_PLAYER_COLLISION;
        } else {
            client->ps.pmove.pm_flags &= ~PMF_IGNORE_PLAYER_COLLISION;
        }

        // PGM  trigger_gravity support
        client->ps.pmove.gravity = (short)(level.gravity * ent->gravity);
        pm.s = client->ps.pmove;

        for (i = 0; i < 3; i++) {
            pm.s.origin[i] = COORD2SHORT(ent->s.origin[i]);
            pm.s.velocity[i] = COORD2SHORT(ent->velocity[i]);
        }

        if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
            pm.snapinitial = qtrue;

        pm.cmd = *ucmd;
        pm.trace = PM_trace;
        pm.pointcontents = gi.pointcontents;

        // perform a pmove
        gi.Pmove(&pm);

        if (ent->client->landmark_free_fall && pm.groundentity) {
            ent->client->landmark_free_fall = false;
            ent->client->landmark_noise_time = level.time + HZ(10);
        }

        // [Paril-KEX] save old position for G_TouchProjectiles
        vec3_t old_origin;
        VectorCopy(ent->s.origin, old_origin);

        for (i = 0; i < 3; i++) {
            ent->s.origin[i] = SHORT2COORD(pm.s.origin[i]);
            ent->velocity[i] = SHORT2COORD(pm.s.velocity[i]);
        }

        // [Paril-KEX] if we stepped onto/off of a ladder, reset the
        // last ladder pos
        if ((pm.s.pm_flags & PMF_ON_LADDER) != (client->ps.pmove.pm_flags & PMF_ON_LADDER)) {
            VectorCopy(ent->s.origin, client->last_ladder_pos);

            if (pm.s.pm_flags & PMF_ON_LADDER) {
                if (!deathmatch->integer && client->last_ladder_sound < level.time) {
                    ent->s.event = EV_LADDER_STEP;
                    client->last_ladder_sound = level.time + LADDER_SOUND_TIME;
                }
            }
        }

        if (~client->ps.pmove.pm_flags & pm.s.pm_flags & PMF_JUMP_HELD && pm.waterlevel == 0) {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
            // Paril: removed to make ambushes more effective and to
            // not have monsters around corners come to jumps
            // PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
        }

        // save results of pmove
        client->ps.pmove = pm.s;
        client->old_pmove = pm.s;

        VectorCopy(pm.mins, ent->mins);
        VectorCopy(pm.maxs, ent->maxs);

        if (!ent->client->menu)
            for (i = 0; i < 3; i++)
                client->resp.cmd_angles[i] = SHORT2ANGLE(ucmd->angles[i]);

        // ROGUE sam raimi cam support
        if (ent->flags & FL_SAM_RAIMI)
            ent->viewheight = 8;
        else if (client->ps.pmove.pm_type == PM_FREEZE)
            ent->viewheight = 22; // FIXME: pmenu hack
        else
            ent->viewheight = pm.viewheight;
        // ROGUE

        ent->waterlevel = pm.waterlevel;
        ent->watertype = pm.watertype;
        ent->groundentity = pm.groundentity;
        if (pm.groundentity)
            ent->groundentity_linkcount = pm.groundentity->linkcount;

        if (ent->deadflag) {
            client->ps.viewangles[ROLL] = 40;
            client->ps.viewangles[PITCH] = -15;
            client->ps.viewangles[YAW] = client->killer_yaw;
        } else if (!ent->client->menu) {
            VectorCopy(pm.viewangles, client->v_angle);
            VectorCopy(pm.viewangles, client->ps.viewangles);
            AngleVectors(client->v_angle, client->v_forward, NULL, NULL);
        }

        // ZOID
        if (client->ctf_grapple)
            CTFGrapplePull(client->ctf_grapple);
        // ZOID

        gi.linkentity(ent);

        // PGM trigger_gravity support
        ent->gravity = 1.0f;
        // PGM

        if (ent->movetype != MOVETYPE_NOCLIP) {
            G_TouchTriggers(ent);
            G_TouchProjectiles(ent, old_origin);
        }

        // touch other objects
        for (i = 0; i < pm.numtouch; i++) {
            other = pm.touchents[i];
            for (j = 0; j < i; j++)
                if (pm.touchents[j] == other)
                    break;
            if (j != i)
                continue;   // duplicated

            if (other->touch)
                other->touch(other, ent, &null_trace, true);
        }
    }

    // fire weapon from final position if needed
    if (client->latched_buttons & BUTTON_ATTACK) {
        if (client->resp.spectator) {
            client->latched_buttons = BUTTON_NONE;

            if (client->chase_target) {
                client->chase_target = NULL;
                client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
            } else
                GetChaseTarget(ent);
        } else if (!ent->client->weapon_thunk) {
            // we can only do this during a ready state and
            // if enough time has passed from last fire
            if (ent->client->weaponstate == WEAPON_READY) {
                ent->client->weapon_fire_buffered = true;

                if (ent->client->weapon_fire_finished <= level.time) {
                    ent->client->weapon_thunk = true;
                    Think_Weapon(ent);
                }
            }
        }
    }

    if (client->resp.spectator) {
        if (!HandleMenuMovement(ent, ucmd)) {
            if (ucmd->buttons & BUTTON_JUMP) {
                if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
                    client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
                    if (client->chase_target)
                        ChaseNext(ent);
                    else
                        GetChaseTarget(ent);
                }
            } else
                client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
        }
    }

    // update chase cam if being followed
    for (i = 1; i <= game.maxclients; i++) {
        other = g_edicts + i;
        if (other->inuse && other->client->chase_target == ent)
            UpdateChaseCam(other);
    }
}

static bool G_MonstersSearchingFor(edict_t *player)
{
    for (int i = game.maxclients + BODY_QUEUE_SIZE + 1; i < globals.num_edicts; i++) {
        edict_t *ent = &g_edicts[i];
        if (!ent->inuse || !(ent->svflags & SVF_MONSTER) || ent->health <= 0)
            continue;

        // check for *any* player target
        if (!player && ent->enemy && !ent->enemy->client)
            continue;

        // they're not targeting us, so who cares
        if (player && ent->enemy != player)
            continue;

        // they lost sight of us
        if ((ent->monsterinfo.aiflags & AI_LOST_SIGHT) && level.time > ent->monsterinfo.trail_time + SEC(5))
            continue;

        // no sir
        return true;
    }

    // yes sir
    return false;
}

// [Paril-KEX] from the given player, find a good spot to
// spawn a player
static bool G_FindRespawnSpot(edict_t *player, vec3_t spot)
{
    // sanity check; make sure there's enough room for ourselves.
    // (crouching in a small area, etc)
    trace_t tr = gi.trace(player->s.origin, player_mins, player_maxs, player->s.origin, player, MASK_PLAYERSOLID);

    if (tr.startsolid || tr.allsolid)
        return false;

    // throw five boxes a short-ish distance from the player and see if they land in a good, visible spot
    static const float yaw_spread[] = { 0, 90, 45, -45, -90 };
    const float back_distance = 128;
    const float up_distance = 128;
    const float player_viewheight = 22;

    // we don't want to spawn inside of these
    contents_t mask = MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME;

    for (int i = 0; i < q_countof(yaw_spread); i++) {
        vec3_t angles = { 0, (player->s.angles[YAW] + 180) + yaw_spread[i], 0 };

        // throw the box three times:
        // one up & back
        // one back
        // one up, then back
        // pick the one that went the farthest
        vec3_t start, end;
        VectorCopy(player->s.origin, start);
        VectorCopy(player->s.origin, end);
        end[2] += up_distance;

        tr = gi.trace(start, player_mins, player_maxs, end, player, mask);

        // stuck
        if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
            continue;

        vec3_t fwd;
        AngleVectors(angles, fwd, NULL, NULL);

        VectorCopy(tr.endpos, start);
        VectorMA(start, back_distance, fwd, end);

        tr = gi.trace(start, player_mins, player_maxs, end, player, mask);

        // stuck
        if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
            continue;

        // plop us down now
        VectorCopy(tr.endpos, start);
        VectorCopy(tr.endpos, end);
        end[2] -= up_distance * 4;

        tr = gi.trace(start, player_mins, player_maxs, end, player, mask);

        // stuck, or floating, or touching some other entity
        if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)) || tr.fraction == 1.0f || tr.ent != world)
            continue;

        // don't spawn us *inside* liquids
        VectorCopy(tr.endpos, end);
        end[2] += player_viewheight;

        if (gi.pointcontents(end) & MASK_WATER)
            continue;

        // don't spawn us on steep slopes
        if (tr.plane.normal[2] < 0.7f)
            continue;

        VectorCopy(tr.endpos, spot);

        float z_diff = fabsf(player->s.origin[2] - tr.endpos[2]);

        // 5 steps is way too many steps
        if (z_diff > STEPSIZE * 4)
            continue;

        // if we went up or down 1 step, make sure we can still see their origin and their head
        if (z_diff > STEPSIZE) {
            tr = gi.trace(player->s.origin, NULL, NULL, tr.endpos, player, mask);

            if (tr.fraction != 1.0f)
                continue;

            VectorCopy(player->s.origin, start);
            start[2] += player_viewheight;

            VectorCopy(tr.endpos, end);
            end[2] += player_viewheight;

            tr = gi.trace(start, NULL, NULL, end, player, mask);

            if (tr.fraction != 1.0f)
                continue;
        }

        // good spot!
        return true;
    }

    return false;
}

// [Paril-KEX] check each player to find a good
// respawn target & position
static edict_t *G_FindSquadRespawnTarget(vec3_t spot)
{
    bool monsters_searching_for_anybody = G_MonstersSearchingFor(NULL);

    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        // no dead players
        if (!player->inuse || player->deadflag)
            continue;

        // check combat state; we can't have taken damage recently
        if (player->client->last_damage_time >= level.time) {
            player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
            continue;
        }

        // check if any monsters are currently targeting us
        // or searching for us
        if (G_MonstersSearchingFor(player)) {
            player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
            continue;
        }

        // check firing state; if any enemies are mad at any players,
        // don't respawn until everybody has cooled down
        if (monsters_searching_for_anybody && player->client->last_firing_time >= level.time) {
            player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
            continue;
        }

        // check positioning; we must be on world ground
        if (player->groundentity != world) {
            player->client->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
            continue;
        }

        // can't be in liquid
        if (player->waterlevel >= WATER_UNDER) {
            player->client->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
            continue;
        }

        // good player; pick a spot
        if (!G_FindRespawnSpot(player, spot)) {
            player->client->coop_respawn_state = COOP_RESPAWN_BLOCKED;
            continue;
        }

        // good player most likely
        return player;
    }

    // no good player
    return NULL;
}

typedef enum {
    RESPAWN_NONE,     // invalid state
    RESPAWN_SPECTATE, // move to spectator
    RESPAWN_SQUAD,    // move to good squad point
    RESPAWN_START     // move to start of map
} respawn_state_t;

// [Paril-KEX] return false to fall back to click-to-respawn behavior.
// note that this is only called if they are allowed to respawn (not
// restarting the level due to all being dead)
static bool G_CoopRespawn(edict_t *ent)
{
    // don't do this in non-coop
    if (!coop->integer)
        return false;
    // if we don't have squad or lives, it doesn't matter
    if (!g_coop_squad_respawn->integer && !g_coop_enable_lives->integer)
        return false;

    respawn_state_t state = RESPAWN_NONE;

    // first pass: if we have no lives left, just move to spectator
    if (g_coop_enable_lives->integer && !ent->client->pers.lives) {
        state = RESPAWN_SPECTATE;
        ent->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
    }

    // second pass: check for where to spawn
    if (state == RESPAWN_NONE) {
        // if squad respawn, don't respawn until we can find a good player to spawn on.
        if (g_coop_squad_respawn->integer) {
            bool allDead = true;

            for (int i = 1; i <= game.maxclients; i++) {
                edict_t *player = &g_edicts[i];
                if (player->inuse && player->health > 0) {
                    allDead = false;
                    break;
                }
            }

            // all dead, so if we ever get here we have lives enabled;
            // we should just respawn at the start of the level
            if (allDead)
                state = RESPAWN_START;
            else {
                edict_t *good_player;
                vec3_t good_spot;

                good_player = G_FindSquadRespawnTarget(good_spot);
                if (good_player) {
                    state = RESPAWN_SQUAD;

                    VectorCopy(good_spot, squad_respawn_position);
                    VectorCopy(good_player->s.angles, squad_respawn_angles);
                    squad_respawn_angles[2] = 0;

                    use_squad_respawn = true;
                } else {
                    state = RESPAWN_SPECTATE;
                }
            }
        } else
            state = RESPAWN_START;
    }

    if (state == RESPAWN_SQUAD || state == RESPAWN_START) {
        // give us our max health back since it will reset
        // to pers.health; in instanced items we'd lose the items
        // we touched so we always want to respawn with our max.
        if (P_UseCoopInstancedItems())
            ent->client->pers.health = ent->client->pers.max_health = ent->max_health;

        respawn(ent);

        ent->client->latched_buttons = BUTTON_NONE;
        use_squad_respawn = false;
    } else if (state == RESPAWN_SPECTATE) {
        if (!ent->client->coop_respawn_state)
            ent->client->coop_respawn_state = COOP_RESPAWN_WAITING;

        if (!ent->client->resp.spectator) {
            // move us to spectate just so we don't have to twiddle
            // our thumbs forever
            CopyToBodyQue(ent);
            ent->client->resp.spectator = true;
            ent->solid = SOLID_NOT;
            ent->takedamage = false;
            ent->s.modelindex = 0;
            ent->svflags |= SVF_NOCLIENT;
            ent->client->ps.blend[3] = 0;
            ent->client->ps.damage_blend[3] = 0;
            ent->client->ps.rdflags = RDF_NONE;
            ent->movetype = MOVETYPE_NOCLIP;
            // TODO: check if anything else needs to be reset
            gi.linkentity(ent);
            GetChaseTarget(ent);
        }
    }

    return true;
}

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t *ent)
{
    gclient_t *client;
    int        buttonMask;

    if (level.intermissiontime)
        return;

    client = ent->client;

    if (client->awaiting_respawn) {
        if ((TO_MSEC(level.time) % 500) == 0)
            PutClientInServer(ent);
        return;
    }

    if (deathmatch->integer && !G_TeamplayEnabled() &&
        client->pers.spectator != client->resp.spectator &&
        (level.time - client->respawn_time) >= SEC(5)) {
        spectator_respawn(ent);
        return;
    }

    // run weapon animations if it hasn't been done by a ucmd_t
    if (!client->weapon_thunk && !client->resp.spectator)
        Think_Weapon(ent);
    else
        client->weapon_thunk = false;

    if (ent->deadflag) {
        // don't respawn if level is waiting to restart
        if (level.time > client->respawn_time && !level.coop_level_restart_time) {
            // check for coop handling
            if (!G_CoopRespawn(ent)) {
                // in deathmatch, only wait for attack button
                if (deathmatch->integer)
                    buttonMask = BUTTON_ATTACK;
                else
                    buttonMask = -1;

                if ((client->latched_buttons & buttonMask) ||
                    (deathmatch->integer && g_dm_force_respawn->integer)) {
                    respawn(ent);
                    client->latched_buttons = BUTTON_NONE;
                }
            }
        }
        return;
    }

    // add player trail so monsters can follow
    if (!deathmatch->integer)
        PlayerTrail_Add(ent);

    client->latched_buttons = BUTTON_NONE;
}
/*
==============
RemoveAttackingPainDaemons

This is called to clean up the pain daemons that the disruptor attaches
to clients to damage them.
==============
*/
void RemoveAttackingPainDaemons(edict_t *self)
{
    edict_t *tracker;

    tracker = G_Find(NULL, FOFS(classname), "pain daemon");
    while (tracker) {
        if (tracker->enemy == self)
            G_FreeEdict(tracker);
        tracker = G_Find(tracker, FOFS(classname), "pain daemon");
    }

    if (self->client)
        self->client->tracker_pain_time = 0;
}
