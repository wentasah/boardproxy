project('boardproxy', [ 'cpp' ],
	default_options : [
	  'cpp_std=gnu++17',
	  'buildtype=release',
	],
       )

version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['sh', '-c', 'git describe --always --dirty --match="v[0-9]*" | sed -e "s/^v//"'],
)

cxx = meson.get_compiler('cpp')

spdlog_proj = subproject('spdlog')
fmt_proj = subproject('fmt')

args = []

executable(
  'boardproxy',
  [
    'boardproxy.cpp',

    'board.cpp',
    'config.cpp',
    'client.cpp',
    'daemon.cpp',
    'log.cpp',
    'session.cpp',
    'tcpproxy.cpp',
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
    dependency('libsystemd'),
  ],
  cpp_args : args,
  install : true,
)

systemd = dependency('systemd')

foreach unit : [ 'boardproxy@.socket', 'boardproxy@.service' ]
  configure_file(copy : true,
		 input : unit,
		 output : unit,
		 install : true,
		 install_dir : systemd.get_pkgconfig_variable('systemdsystemunitdir'),
		)
endforeach
