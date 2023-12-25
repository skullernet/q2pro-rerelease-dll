// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include <float.h>

#define NUM_SIDE_CHECKS 6

typedef struct {
    int8_t normal[3];
    int8_t mins[3];
    int8_t maxs[3];
} side_check_t;

typedef struct {
    vec3_t origin;
    float dist;
} good_position_t;

static const side_check_t side_checks[NUM_SIDE_CHECKS] = {
    { { 0, 0, 1 }, {-1,-1, 0 }, { 1, 1, 0 } },
    { { 0, 0,-1 }, {-1,-1, 0 }, { 1, 1, 0 } },
    { { 1, 0, 0 }, { 0,-1,-1 }, { 0, 1, 1 } },
    { {-1, 0, 0 }, { 0,-1,-1 }, { 0, 1, 1 } },
    { { 0, 1, 0 }, {-1, 0,-1 }, { 1, 0, 1 } },
    { { 0,-1, 0 }, {-1, 0,-1 }, { 1, 0, 1 } },
};

// [Paril-KEX] generic code to detect & fix a stuck object
stuck_result_t G_FixStuckObject_Generic(vec3_t origin, const vec3_t own_mins, const vec3_t own_maxs, edict_t *ignore, contents_t mask)
{
    if (!gi.trace(origin, own_mins, own_maxs, origin, ignore, mask).startsolid)
        return GOOD_POSITION;

    good_position_t good_positions[NUM_SIDE_CHECKS];
    int num_good_positions = 0;

    for (int sn = 0; sn < NUM_SIDE_CHECKS; sn++) {
        const side_check_t *side = &side_checks[sn];

        vec3_t start, mins = { 0 }, maxs = { 0 };
        VectorCopy(origin, start);

        for (int n = 0; n < 3; n++) {
            if (side->normal[n] < 0)
                start[n] += own_mins[n];
            else if (side->normal[n] > 0)
                start[n] += own_maxs[n];

            if (side->mins[n] == -1)
                mins[n] = own_mins[n];
            else if (side->mins[n] == 1)
                mins[n] = own_maxs[n];

            if (side->maxs[n] == -1)
                maxs[n] = own_mins[n];
            else if (side->maxs[n] == 1)
                maxs[n] = own_maxs[n];
        }

        int needed_epsilon_fix = -1;
        int needed_epsilon_dir = 0;

        trace_t tr = gi.trace(start, mins, maxs, start, ignore, mask);

        if (tr.startsolid) {
            for (int e = 0; e < 3; e++) {
                if (side->normal[e] != 0)
                    continue;

                vec3_t ep_start;
                VectorCopy(start, ep_start);
                ep_start[e] += 1;

                tr = gi.trace(ep_start, mins, maxs, ep_start, ignore, mask);

                if (!tr.startsolid) {
                    VectorCopy(ep_start, start);
                    needed_epsilon_fix = e;
                    needed_epsilon_dir = 1;
                    break;
                }

                ep_start[e] -= 2;
                tr = gi.trace(ep_start, mins, maxs, ep_start, ignore, mask);

                if (!tr.startsolid) {
                    VectorCopy(ep_start, start);
                    needed_epsilon_fix = e;
                    needed_epsilon_dir = -1;
                    break;
                }
            }
        }

        // no good
        if (tr.startsolid)
            continue;

        vec3_t opposite_start;
        VectorCopy(origin, opposite_start);

        const side_check_t *other_side = &side_checks[sn ^ 1];

        for (int n = 0; n < 3; n++) {
            if (other_side->normal[n] < 0)
                opposite_start[n] += own_mins[n];
            else if (other_side->normal[n] > 0)
                opposite_start[n] += own_maxs[n];
        }

        if (needed_epsilon_fix >= 0)
            opposite_start[needed_epsilon_fix] += needed_epsilon_dir;

        // potentially a good side; start from our center, push back to the opposite side
        // to find how much clearance we have
        tr = gi.trace(start, mins, maxs, opposite_start, ignore, mask);

        // ???
        if (tr.startsolid)
            continue;

        // check the delta
        vec3_t end;
        VectorCopy(tr.endpos, end);

        // push us very slightly away from the wall
        VectorMA(end, 0.125f, side->normal, end);

        // calculate delta
        vec3_t delta, new_origin;
        VectorSubtract(end, opposite_start, delta);
        VectorAdd(origin, delta, new_origin);

        if (needed_epsilon_fix >= 0)
            new_origin[needed_epsilon_fix] += needed_epsilon_dir;

        tr = gi.trace(new_origin, own_mins, own_maxs, new_origin, ignore, mask);

        // bad
        if (tr.startsolid)
            continue;

        good_position_t *good = &good_positions[num_good_positions++];
        VectorCopy(new_origin, good->origin);
        good->dist = VectorLengthSquared(delta);
    }

    if (num_good_positions) {
        float best_dist = FLT_MAX;
        int best = 0;

        for (int i = 0; i < num_good_positions; i++) {
            good_position_t *good = &good_positions[i];
            if (good->dist < best_dist) {
                best_dist = good->dist;
                best = i;
            }
        }

        VectorCopy(good_positions[best].origin, origin);
        return STUCK_FIXED;
    }

    return NO_GOOD_POSITION;
}
