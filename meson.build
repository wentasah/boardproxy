project('boardproxy', [ 'cpp' ],
	default_options : [ 'cpp_std=gnu++17' ],
       )

version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always'],
)


cxx = meson.get_compiler('cpp')

executable(
  'boardproxy',
  [
    'boardproxy.cpp',
    'log.cpp',
    'daemon.cpp',
    'client.cpp',
    version_h
  ],
  dependencies : [
    dependency('fmt'),
    dependency('spdlog'),
  ],
)
