project('boardproxy', [ 'cpp' ],
	default_options : [
	  'cpp_std=gnu++17',
	  'buildtype=release',
	],
       )

version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always', '--dirty', '--match=boardproxy-v[0-9]*'],
)

cxx = meson.get_compiler('cpp')

spdlog_proj = subproject('spdlog')
fmt_proj = subproject('fmt')

args = []

if get_option('local_test')
  args += '-DLOCAL_TEST'
endif

executable(
  'boardproxy',
  [
    'boardproxy.cpp',

    'board.cpp',
    'boards.cpp',
    'client.cpp',
    'daemon.cpp',
    'log.cpp',
    'session.cpp',
    'unix_socket.cpp',
    'version.cpp',
    'wrproxy.cpp',
    version_h
  ],
  dependencies : [
    fmt_proj.get_variable('fmt_dep'),
    spdlog_proj.get_variable('spdlog_dep'),
    cxx.find_library('ev'),
    dependency('boost', modules : [ 'program_options' ]),
  ],
  cpp_args : args,
  install : true,
)
