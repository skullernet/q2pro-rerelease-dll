// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "q_std.h"

#define GAME_INCLUDE
#include "bg_local.h"

// [Paril-KEX] generic code to detect & fix a stuck object
stuck_result_t G_FixStuckObject_Generic(vec3_t &origin, const vec3_t &own_mins, const vec3_t &own_maxs, std::function<stuck_object_trace_fn_t> trace)
{
    if (!trace(origin, own_mins, own_maxs, origin).startsolid)
        return stuck_result_t::GOOD_POSITION;

    struct {
        float distance;
        vec3_t origin;
    } good_positions[6];
    size_t num_good_positions = 0;

    constexpr struct {
        std::array<int8_t, 3> normal;
        std::array<int8_t, 3> mins, maxs;
    } side_checks[] = {
        { { 0, 0, 1 }, { -1, -1, 0 }, { 1, 1, 0 } },
        { { 0, 0, -1 }, { -1, -1, 0 }, { 1, 1, 0 } },
        { { 1, 0, 0 }, { 0, -1, -1 }, { 0, 1, 1 } },
        { { -1, 0, 0 }, { 0, -1, -1 }, { 0, 1, 1 } },
        { { 0, 1, 0 }, { -1, 0, -1 }, { 1, 0, 1 } },
        { { 0, -1, 0 }, { -1, 0, -1 }, { 1, 0, 1 } },
    };

    for (size_t sn = 0; sn < q_countof(side_checks); sn++) {
        auto &side = side_checks[sn];
        vec3_t start = origin;
        vec3_t mins {}, maxs {};

        for (size_t n = 0; n < 3; n++) {
            if (side.normal[n] < 0)
                start[n] += own_mins[n];
            else if (side.normal[n] > 0)
                start[n] += own_maxs[n];

            if (side.mins[n] == -1)
                mins[n] = own_mins[n];
            else if (side.mins[n] == 1)
                mins[n] = own_maxs[n];

            if (side.maxs[n] == -1)
                maxs[n] = own_mins[n];
            else if (side.maxs[n] == 1)
                maxs[n] = own_maxs[n];
        }

        trace_t tr = trace(start, mins, maxs, start);

        int8_t needed_epsilon_fix = -1;
        int8_t needed_epsilon_dir;

        if (tr.startsolid) {
            for (size_t e = 0; e < 3; e++) {
                if (side.normal[e] != 0)
                    continue;

                vec3_t ep_start = start;
                ep_start[e] += 1;

                tr = trace(ep_start, mins, maxs, ep_start);

                if (!tr.startsolid) {
                    start = ep_start;
                    needed_epsilon_fix = e;
                    needed_epsilon_dir = 1;
                    break;
                }

                ep_start[e] -= 2;
                tr = trace(ep_start, mins, maxs, ep_start);

                if (!tr.startsolid) {
                    start = ep_start;
                    needed_epsilon_fix = e;
                    needed_epsilon_dir = -1;
                    break;
                }
            }
        }

        // no good
        if (tr.startsolid)
            continue;

        vec3_t opposite_start = origin;
        auto &other_side = side_checks[sn ^ 1];

        for (size_t n = 0; n < 3; n++) {
            if (other_side.normal[n] < 0)
                opposite_start[n] += own_mins[n];
            else if (other_side.normal[n] > 0)
                opposite_start[n] += own_maxs[n];
        }

        if (needed_epsilon_fix >= 0)
            opposite_start[needed_epsilon_fix] += needed_epsilon_dir;

        // potentially a good side; start from our center, push back to the opposite side
        // to find how much clearance we have
        tr = trace(start, mins, maxs, opposite_start);

        // ???
        if (tr.startsolid)
            continue;

        // check the delta
        vec3_t end = tr.endpos;
        // push us very slightly away from the wall
        end += vec3_t{(float) side.normal[0], (float) side.normal[1], (float) side.normal[2]} * 0.125f;

        // calculate delta
        const vec3_t delta = end - opposite_start;
        vec3_t new_origin = origin + delta;

        if (needed_epsilon_fix >= 0)
            new_origin[needed_epsilon_fix] += needed_epsilon_dir;

        tr = trace(new_origin, own_mins, own_maxs, new_origin);

        // bad
        if (tr.startsolid)
            continue;

        good_positions[num_good_positions].origin = new_origin;
        good_positions[num_good_positions].distance = delta.lengthSquared();
        num_good_positions++;
    }

    if (num_good_positions) {
        std::sort(&good_positions[0], &good_positions[num_good_positions - 1], [](const auto & a, const auto & b) {
            return a.distance < b.distance;
        });

        origin = good_positions[0].origin;

        return stuck_result_t::FIXED;
    }

    return stuck_result_t::NO_GOOD_POSITION;
}

/*

  walking up a step should kill some velocity

*/

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
void PM_ClipVelocity(const vec3_t &in, const vec3_t &normal, vec3_t &out, float overbounce)
{
    float backoff;
    float change;
    int   i;

    backoff = in.dot(normal) * overbounce;

    for (i = 0; i < 3; i++) {
        change = normal[i] * backoff;
        out[i] = in[i] - change;
        if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
            out[i] = 0;
    }
}

/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
constexpr float  MIN_STEP_NORMAL = 0.7f; // can't step up onto very steep slopes
constexpr size_t MAX_CLIP_PLANES = 5;

inline void PM_RecordTrace(touch_list_t &touch, trace_t &tr)
{
    if (touch.num == MAXTOUCH)
        return;

    for (size_t i = 0; i < touch.num; i++)
        if (touch.traces[i].ent == tr.ent)
            return;

    touch.traces[touch.num++] = tr;
}

// [Paril-KEX] made generic so you can run this without
// needing a pml/pm
void PM_StepSlideMove_Generic(vec3_t &origin, vec3_t &velocity, float frametime, const vec3_t &mins, const vec3_t &maxs, touch_list_t &touch, bool has_time, pm_trace_t trace_func)
{
    int     bumpcount, numbumps;
    vec3_t  dir;
    float   d;
    int     numplanes;
    vec3_t  planes[MAX_CLIP_PLANES];
    vec3_t  primal_velocity;
    int     i, j;
    trace_t trace;
    vec3_t  end;
    float   time_left;

    numbumps = 4;

    primal_velocity = velocity;
    numplanes = 0;

    time_left = frametime;

    for (bumpcount = 0; bumpcount < numbumps; bumpcount++) {
        for (i = 0; i < 3; i++)
            end[i] = origin[i] + time_left * velocity[i];

        trace = trace_func(origin, mins, maxs, end);

        if (trace.allsolid) {
            // entity is trapped in another solid
            velocity[2] = 0; // don't build up falling damage

            // save entity for contact
            PM_RecordTrace(touch, trace);
            return;
        }

        if (trace.fraction > 0) {
            // actually covered some distance
            origin = trace.endpos;
            numplanes = 0;
        }

        if (trace.fraction == 1)
            break; // moved the entire distance

        // save entity for contact
        PM_RecordTrace(touch, trace);

        time_left -= time_left * trace.fraction;

        // slide along this plane
        if (numplanes >= MAX_CLIP_PLANES) {
            // this shouldn't really happen
            velocity = vec3_origin;
            break;
        }

        //
        // if this is the same plane we hit before, nudge origin
        // out along it, which fixes some epsilon issues with
        // non-axial planes (xswamp, q2dm1 sometimes...)
        //
        for (i = 0; i < numplanes; i++) {
            if (trace.plane.normal.dot(planes[i]) > 0.99f) {
                origin.x += trace.plane.normal.x * 0.01f;
                origin.y += trace.plane.normal.y * 0.01f;
                G_FixStuckObject_Generic(origin, mins, maxs, trace_func);
                break;
            }
        }

        if (i < numplanes)
            continue;

        planes[numplanes] = trace.plane.normal;
        numplanes++;

        //
        // modify original_velocity so it parallels all of the clip planes
        //
        for (i = 0; i < numplanes; i++) {
            PM_ClipVelocity(velocity, planes[i], velocity, 1.01f);
            for (j = 0; j < numplanes; j++)
                if (j != i) {
                    if (velocity.dot(planes[j]) < 0)
                        break; // not ok
                }
            if (j == numplanes)
                break;
        }

        if (i != numplanes) {
            // go along this plane
        } else {
            // go along the crease
            if (numplanes != 2) {
                velocity = vec3_origin;
                break;
            }
            dir = planes[0].cross(planes[1]);
            d = dir.dot(velocity);
            velocity = dir * d;
        }

        //
        // if velocity is against the original velocity, stop dead
        // to avoid tiny oscillations in sloping corners
        //
        if (velocity.dot(primal_velocity) <= 0) {
            velocity = vec3_origin;
            break;
        }
    }

    if (has_time) {
        velocity = primal_velocity;
    }
}
