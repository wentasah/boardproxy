project('boardproxy', [ 'cpp' ],
	default_options : [ 'cpp_std=gnu++17' ],
       )

version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always'],
)


cxx = meson.get_compiler('cpp')

spdlog_proj = subproject('spdlog')
fmt_proj = subproject('fmt')

boost_dep = dependency('boost')

executable(
  'boardproxy',
  [
    'boardproxy.cpp',
    'client.cpp',
    'daemon.cpp',
    'log.cpp',
    'session.cpp',
    version_h
  ],
  dependencies : [
    fmt_proj.get_variable('fmt_dep'),
    spdlog_proj.get_variable('spdlog_dep'),
    boost_dep,
  ],
)
