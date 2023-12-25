typedef enum {
    P_prethink,
    P_think,
    P_touch,
    P_use,
    P_pain,
    P_die,

    P_moveinfo_endfunc,
    P_moveinfo_blocked,

    P_mmove_t,

    P_monsterinfo_stand,
    P_monsterinfo_idle,
    P_monsterinfo_search,
    P_monsterinfo_walk,
    P_monsterinfo_run,
    P_monsterinfo_dodge,
    P_monsterinfo_attack,
    P_monsterinfo_melee,
    P_monsterinfo_sight,
    P_monsterinfo_checkattack,
    P_monsterinfo_setskin,
    P_monsterinfo_blocked,
    P_monsterinfo_physchanged,
    P_monsterinfo_duck,
    P_monsterinfo_unduck,
    P_monsterinfo_sidestep,

    P_num_types
} ptr_type_t;

typedef struct {
    const char *name;
    const void *ptr;
} save_ptr_t;

extern const save_ptr_t *const save_ptrs[P_num_types];
extern const int num_save_ptrs[P_num_types];
