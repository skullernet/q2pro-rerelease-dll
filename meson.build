project('q2pro-rerelease-dll', 'c',
  license: 'GPL-2.0-or-later',
  version: run_command(find_program('python3'), 'version.py', check: true).stdout().strip(),
  default_options: [
    'c_std=c11',
    'buildtype=debugoptimized',
  ],
)

subdir('src')
subdir('src/rogue')
subdir('src/xatrix')
subdir('src/ctf')

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

if win32
  args += ['-D_USE_MATH_DEFINES']
else
  args += ['-D_GNU_SOURCE']
endif

link_args = []

if cc.get_argument_syntax() == 'gcc'
  if x86
    args += ['-msse2', '-mfpmath=sse']
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
  args += ['/wd4244']
endif

add_project_arguments(args, language: 'c')
add_project_link_arguments(link_args, language: 'c')

fallback_opt = ['default_library=static']

zlib = dependency('zlib',
  required:        get_option('zlib'),
  default_options: fallback_opt + [ 'tests=disabled' ]
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
  src += import('windows').compile_resources('src/game.rc', args: '-DHAVE_CONFIG_H', include_directories: '.', depend_files: cfg_file)
endif

shared_library('game' + cpu, src,
  name_prefix:           '',
  gnu_symbol_visibility: 'hidden',
  dependencies:          deps,
  include_directories:   'src',
)
