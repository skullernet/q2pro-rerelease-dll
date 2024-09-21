// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

typedef enum {
    PathReturnCode_ReachedGoal,             // we're at our destination
    PathReturnCode_ReachedPathEnd,          // we're as close to the goal as we can get with a path
    PathReturnCode_TraversalPending,        // the upcoming path segment is a traversal
    PathReturnCode_RawPathFound,            // user wanted ( and got ) just a raw path ( no processing )
    PathReturnCode_InProgress,              // pathing in progress
    PathReturnCode_StartPathErrors,         // any code after this one indicates an error of some kind.
    PathReturnCode_InvalidStart,            // start position is invalid.
    PathReturnCode_InvalidGoal,             // goal position is invalid.
    PathReturnCode_NoNavAvailable,          // no nav file available for this map.
    PathReturnCode_NoStartNode,             // can't find a nav node near the start position
    PathReturnCode_NoGoalNode,              // can't find a nav node near the goal position
    PathReturnCode_NoPathFound,             // can't find a path from the start to the goal
    PathReturnCode_MissingWalkOrSwimFlag    // MUST have at least Walk or Water path flags set!
} PathReturnCode;

typedef enum {
    PathLinkType_Walk,          // can walk between the path points
    PathLinkType_WalkOffLedge,  // will walk off a ledge going between path points
    PathLinkType_LongJump,      // will need to perform a long jump between path points
    PathLinkType_BarrierJump,   // will need to jump over a low barrier between path points
    PathLinkType_Elevator       // will need to use an elevator between path points
} PathLinkType;

typedef enum {
    PathFlags_All             = -1,
    PathFlags_Water           = BIT(0), // swim to your goal ( useful for fish/gekk/etc. )
    PathFlags_Walk            = BIT(1), // walk to your goal
    PathFlags_WalkOffLedge    = BIT(2), // allow walking over ledges
    PathFlags_LongJump        = BIT(3), // allow jumping over gaps
    PathFlags_BarrierJump     = BIT(4), // allow jumping over low barriers
    PathFlags_Elevator        = BIT(5)  // allow using elevators
} PathFlags;

typedef struct {
    vec3_t      start;
    vec3_t      goal;
    PathFlags   pathFlags;
    float       moveDist;

    struct DebugSettings {
        float   drawTime; // if > 0, how long ( in seconds ) to draw path in world
    } debugging;

    struct NodeSettings {
        bool    ignoreNodeFlags; // true = ignore node flags when considering nodes
        float   minHeight;       // 0 <= use default values
        float   maxHeight;       // 0 <= use default values
        float   radius;          // 0 <= use default values
    } nodeSearch;

    struct TraversalSettings {
        float dropHeight;   // 0 = don't drop down
        float jumpHeight;   // 0 = don't jump up
    } traversals;

    struct PathArray {
        vec3_t  *posArray;  // array to store raw path points
        int     count;      // number of elements in array
    } pathPoints;
} PathRequest;

typedef struct {
    int             numPathPoints;
    float           pathDistSqr;
    vec3_t          firstMovePoint;
    vec3_t          secondMovePoint;
    PathLinkType    pathLinkType;
    PathReturnCode  returnCode;
} PathInfo;

bool Nav_GetPathToGoal(const PathRequest *request, PathInfo *info);

// life cycle stuff
void Nav_Init(void);
void Nav_Shutdown(void);
void Nav_Load(const char *mapname);
void Nav_Unload(void);
void Nav_Frame(void);
