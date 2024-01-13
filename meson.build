project('q2pro-rerelease-dll', 'c',
  license: 'GPL-2.0-or-later',
  version: run_command(find_program('python3'), 'version.py', check: true).stdout().strip(),
  default_options: [
    meson.version().version_compare('>=1.3.0') ? 'c_std=gnu11,c11' : 'c_std=gnu11',
    'buildtype=debugoptimized',
  ],
)

src = [
  'g_ai.c',
  'g_chase.c',
  'g_cmds.c',
  'g_combat.c',
  'g_func.c',
  'g_items.c',
  'g_l10n.c',
  'g_main.c',
  'g_misc.c',
  'g_monster.c',
  'g_phys.c',
  'g_save.c',
  'g_spawn.c',
  'g_svcmds.c',
  'g_target.c',
  'g_trigger.c',
  'g_turret.c',
  'g_utils.c',
  'g_weapon.c',
  'm_actor.c',
  'm_arachnid.c',
  'm_berserk.c',
  'm_boss2.c',
  'm_boss3.c',
  'm_boss31.c',
  'm_boss32.c',
  'm_brain.c',
  'm_chick.c',
  'm_flash.c',
  'm_flipper.c',
  'm_float.c',
  'm_flyer.c',
  'm_gladiator.c',
  'm_guardian.c',
  'm_guncmdr.c',
  'm_gunner.c',
  'm_hover.c',
  'm_infantry.c',
  'm_insane.c',
  'm_medic.c',
  'm_move.c',
  'm_mutant.c',
  'm_parasite.c',
  'm_shambler.c',
  'm_soldier.c',
  'm_supertank.c',
  'm_tank.c',
  'p_client.c',
  'p_hud.c',
  'p_move.c',
  'p_trail.c',
  'p_view.c',
  'p_weapon.c',
  'q_shared.c',
]

src += [
  'rogue/g_rogue_combat.c',
  'rogue/g_rogue_func.c',
  'rogue/g_rogue_items.c',
  'rogue/g_rogue_misc.c',
  'rogue/g_rogue_monster.c',
  'rogue/g_rogue_newai.c',
  'rogue/g_rogue_newdm.c',
  'rogue/g_rogue_newfnc.c',
  'rogue/g_rogue_newtarg.c',
  'rogue/g_rogue_newtrig.c',
  'rogue/g_rogue_newweap.c',
  'rogue/g_rogue_phys.c',
  'rogue/g_rogue_spawn.c',
  'rogue/g_rogue_sphere.c',
  'rogue/g_rogue_utils.c',
  'rogue/m_rogue_carrier.c',
  'rogue/m_rogue_stalker.c',
  'rogue/m_rogue_turret.c',
  'rogue/m_rogue_widow2.c',
  'rogue/m_rogue_widow.c',
  'rogue/p_rogue_weapon.c',
  'rogue/rogue_dm_ball.c',
  'rogue/rogue_dm_tag.c',
]

src += [
  'xatrix/g_xatrix_func.c',
  'xatrix/g_xatrix_items.c',
  'xatrix/g_xatrix_misc.c',
  'xatrix/g_xatrix_monster.c',
  'xatrix/g_xatrix_target.c',
  'xatrix/g_xatrix_weapon.c',
  'xatrix/m_xatrix_fixbot.c',
  'xatrix/m_xatrix_gekk.c',
  'xatrix/p_xatrix_weapon.c',
]

src += [
  'ctf/g_ctf.c',
  'ctf/p_ctf_menu.c',
]

python = find_program('python3')

src += custom_target('g_ptrs_c',
  input: src,
  output: 'g_ptrs.c',
  command: [python, '@SOURCE_ROOT@/genptr.py', '@INPUT@', '@OUTPUT@'],
)

cc = meson.get_compiler('c')

win32 = host_machine.system() == 'windows'
x86 = host_machine.cpu_family() == 'x86'

cpuremap = {
  'x86': win32 ? 'x86' : 'i386',
  'aarch64': 'arm64',
}

cpu = host_machine.cpu_family()
if cpu in cpuremap
  cpu = cpuremap[cpu]
endif

args = [
  '-DHAVE_CONFIG_H',
  '-DUSE_PROTOCOL_EXTENSIONS=1',
]

link_args = []

if cc.get_argument_syntax() == 'gcc'
  if x86
    add_global_arguments('-msse2', '-mfpmath=sse', language: 'cpp')
  endif

  test_args = [
    '-Werror=vla',
    '-Wformat-security',
    '-Wpointer-arith',
    '-Wstrict-prototypes',
    '-fno-math-errno',
    '-fsigned-char',
  ]

  args += cc.get_supported_arguments(test_args)

  if win32
    link_args += [
      '-Wl,--nxcompat,--dynamicbase',
      '-static-libgcc',
    ]
    if cpu == 'x86_64'
      link_args += '-Wl,--high-entropy-va,--image-base=0x180000000'
    endif
  endif
elif cc.get_id() == 'msvc'
  args += ['/D_USE_MATH_DEFINES', '/wd4244']
endif

add_project_arguments(args, language: 'c')
add_project_link_arguments(link_args, language: 'c')

fallback_opt = ['default_library=static']

zlib = dependency('zlib',
  required:        get_option('zlib'),
  default_options: fallback_opt
)

deps = [
  cc.find_library('m', required : false),
  zlib
]

config = configuration_data()
config.set('REVISION',         meson.project_version().substring(1).split('~')[0].to_int())
config.set_quoted('VERSION',   meson.project_version())
config.set_quoted('CPUSTRING', cpu)
config.set10('USE_' + host_machine.endian().to_upper() + '_ENDIAN', true)
config.set10('USE_ZLIB', zlib.found())

cfg_file = configure_file(output: 'config.h', configuration: config)

if win32
  src += import('windows').compile_resources('game.rc', args: '-DHAVE_CONFIG_H', include_directories: '.', depend_files: cfg_file)
endif

shared_library('game' + cpu, src,
  name_prefix:           '',
  gnu_symbol_visibility: 'hidden',
  dependencies:          deps,
)
