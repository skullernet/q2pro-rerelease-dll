project('q2pro-rerelease-dll', 'cpp',
  license: 'GPL-2.0-or-later',
  default_options: [
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
)

src = [
  'g_ai.cpp',
  'g_chase.cpp',
  'g_cmds.cpp',
  'g_combat.cpp',
  'g_func.cpp',
  'g_items.cpp',
  'g_main.cpp',
  'g_misc.cpp',
  'g_monster.cpp',
  'g_phys.cpp',
  'g_save.cpp',
  'g_spawn.cpp',
  'g_svcmds.cpp',
  'g_target.cpp',
  'g_trigger.cpp',
  'g_turret.cpp',
  'g_utils.cpp',
  'g_weapon.cpp',
  'm_actor.cpp',
  'm_arachnid.cpp',
  'm_berserk.cpp',
  'm_boss2.cpp',
  'm_boss31.cpp',
  'm_boss32.cpp',
  'm_boss3.cpp',
  'm_brain.cpp',
  'm_chick.cpp',
  'm_flipper.cpp',
  'm_float.cpp',
  'm_flyer.cpp',
  'm_gladiator.cpp',
  'm_guardian.cpp',
  'm_guncmdr.cpp',
  'm_gunner.cpp',
  'm_hover.cpp',
  'm_infantry.cpp',
  'm_insane.cpp',
  'm_medic.cpp',
  'm_move.cpp',
  'm_mutant.cpp',
  'm_parasite.cpp',
  'm_shambler.cpp',
  'm_soldier.cpp',
  'm_supertank.cpp',
  'm_tank.cpp',
  'p_client.cpp',
  'p_hud.cpp',
  'p_move.cpp',
  'p_trail.cpp',
  'p_view.cpp',
  'p_weapon.cpp',
  'q_std.cpp',
]

rogue_src = [
  'rogue/g_rogue_combat.cpp',
  'rogue/g_rogue_func.cpp',
  'rogue/g_rogue_items.cpp',
  'rogue/g_rogue_misc.cpp',
  'rogue/g_rogue_monster.cpp',
  'rogue/g_rogue_newai.cpp',
  'rogue/g_rogue_newdm.cpp',
  'rogue/g_rogue_newfnc.cpp',
  'rogue/g_rogue_newtarg.cpp',
  'rogue/g_rogue_newtrig.cpp',
  'rogue/g_rogue_newweap.cpp',
  'rogue/g_rogue_phys.cpp',
  'rogue/g_rogue_spawn.cpp',
  'rogue/g_rogue_sphere.cpp',
  'rogue/g_rogue_utils.cpp',
  'rogue/m_rogue_carrier.cpp',
  'rogue/m_rogue_stalker.cpp',
  'rogue/m_rogue_turret.cpp',
  'rogue/m_rogue_widow2.cpp',
  'rogue/m_rogue_widow.cpp',
  'rogue/p_rogue_weapon.cpp',
  'rogue/rogue_dm_ball.cpp',
  'rogue/rogue_dm_tag.cpp',
]

xatrix_src = [
  'xatrix/g_xatrix_func.cpp',
  'xatrix/g_xatrix_items.cpp',
  'xatrix/g_xatrix_misc.cpp',
  'xatrix/g_xatrix_monster.cpp',
  'xatrix/g_xatrix_target.cpp',
  'xatrix/g_xatrix_weapon.cpp',
  'xatrix/m_xatrix_fixbot.cpp',
  'xatrix/m_xatrix_gekk.cpp',
  'xatrix/p_xatrix_weapon.cpp',
]

ctf_src = [
  'ctf/g_ctf.cpp',
  'ctf/p_ctf_menu.cpp',
]

cpp = meson.get_compiler('cpp')

args = [
  '-DKEX_Q2GAME_DYNAMIC',
  '-DKEX_Q2GAME_EXPORTS',
  '-DNO_FMT_SOURCE',
]

warns = [
  '-Wformat-security',
  '-Wno-class-memaccess',
  '-Wno-narrowing',
  '-Wno-sign-compare',
  '-Wno-unused',
  '-Wpointer-arith',
]

args += cpp.get_supported_arguments(warns)
add_project_arguments(args, language: 'cpp')

cpu = host_machine.cpu_family()

deps = [
  dependency('fmt'),
  dependency('jsoncpp'),
]

shared_library('game' + cpu, src, rogue_src, xatrix_src, ctf_src,
  name_prefix:           '',
  gnu_symbol_visibility: 'hidden',
  dependencies:          deps,
)
