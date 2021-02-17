project('boardproxy', [ 'cpp' ],
	default_options : [
	  'cpp_std=gnu++17',
	  'buildtype=release',
	],
       )

subdir('src')

systemd_dep = dependency('systemd')

if get_option('systemdsystemunitdir') != ''
  systemd_systemunitdir = get_option('systemdsystemunitdir')
else
  systemd_systemunitdir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')
endif

foreach unit : [ 'boardproxy@.socket', 'boardproxy@.service' ]
  configure_file(copy : true,
		 input : unit,
		 output : unit,
		 install : true,
		 install_dir : systemd_systemunitdir,
		)
endforeach

subdir('tests')
