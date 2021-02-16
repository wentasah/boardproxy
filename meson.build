project('boardproxy', [ 'cpp' ],
	default_options : [
	  'cpp_std=gnu++17',
	  'buildtype=release',
	],
       )

subdir('src')

systemd = dependency('systemd')

foreach unit : [ 'boardproxy@.socket', 'boardproxy@.service' ]
  configure_file(copy : true,
		 input : unit,
		 output : unit,
		 install : true,
		 install_dir : systemd.get_pkgconfig_variable('systemdsystemunitdir'),
		)
endforeach

subdir('tests')
