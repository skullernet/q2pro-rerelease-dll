#!/usr/bin/python3

import re
import sys

pointers = {
    'prethink'  : 'void {p}(edict_t *)',
    'think'     : 'void {p}(edict_t *)',
    'touch'     : 'void {p}(edict_t *, edict_t *, const trace_t *, bool)',
    'use'       : 'void {p}(edict_t *, edict_t *, edict_t *)',
    'pain'      : 'void {p}(edict_t *, edict_t *, float, int, mod_t)',
    'die'       : 'void {p}(edict_t *, edict_t *, edict_t *, int, const vec3_t, mod_t)',
    'moveinfo_endfunc'          : 'void {p}(edict_t *)',
    'moveinfo_blocked'          : 'void {p}(edict_t *, edict_t *)',
    'mmove_t'                   : 'const mmove_t {p}',
    'monsterinfo_stand'         : 'void {p}(edict_t *)',
    'monsterinfo_idle'          : 'void {p}(edict_t *)',
    'monsterinfo_search'        : 'void {p}(edict_t *)',
    'monsterinfo_walk'          : 'void {p}(edict_t *)',
    'monsterinfo_run'           : 'void {p}(edict_t *)',
    'monsterinfo_dodge'         : 'void {p}(edict_t *, edict_t *, gtime_t, trace_t *, bool)',
    'monsterinfo_attack'        : 'void {p}(edict_t *)',
    'monsterinfo_melee'         : 'void {p}(edict_t *)',
    'monsterinfo_sight'         : 'void {p}(edict_t *, edict_t *)',
    'monsterinfo_checkattack'   : 'bool {p}(edict_t *)',
    'monsterinfo_setskin'       : 'void {p}(edict_t *)',
    'monsterinfo_blocked'       : 'bool {p}(edict_t *, float)',
    'monsterinfo_physchanged'   : 'void {p}(edict_t *)',
    'monsterinfo_duck'          : 'bool {p}(edict_t *, gtime_t)',
    'monsterinfo_unduck'        : 'void {p}(edict_t *)',
    'monsterinfo_sidestep'      : 'bool {p}(edict_t *)',

}

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print('Usage: genptr.py <input> [...] <output>')
        sys.exit(1)

    exprs = '|'.join(p.upper() for p in pointers.keys())
    regex = re.compile(r'\s(%s)\s*\(\s*(\w+)\s*\)' % exprs, re.ASCII)

    types = {}
    for p in pointers.keys():
        types[p] = []

    for a in sys.argv[1:-1]:
        with open(a) as f:
            skip = 0
            for line in f:
                line = line.lstrip()
                if line.startswith('//'):
                    continue
                if line.startswith('#if 0'):
                    skip += 1
                    continue
                if line.startswith('#endif'):
                    skip -= 1
                    continue
                if skip > 0:
                    continue
                match = regex.search(line)
                if match:
                    t = types[match[1].lower()]
                    p = match[2]
                    if not p in t:
                        t.append(p)

    with open(sys.argv[-1], 'w') as f:
        sys.stdout = f
        print('// generated by genptr.py, do not modify')
        print('#include "g_local.h"')
        print('#include "g_ptrs.h"')

        decls = []
        for k, v in types.items():
            for p in v:
                decls.append('extern ' + pointers[k].replace('{p}', p) + ';')
        for d in sorted(decls, key=str.lower):
            print(d)

        print('const save_ptr_t *const save_ptrs[P_num_types] = {')
        for k, v in types.items():
            print('[%s] = (const save_ptr_t []) {' % ('P_' + k,))
            for p in sorted(v):
                amp = '&' if k == 'mmove_t' else ''
                print('{ "%s", %s%s },' % (p, amp, p))
            print('},')
        print('};')

        print('const int num_save_ptrs[P_num_types] = {')
        for v in types.values():
            print(f'{len(v)}, ', end='')
        print('\n};')
