pytest = find_program('pytest', required : false, disabler : true)

test(
  'pytest',
  pytest,
  args : [ meson.current_source_dir() ],
  workdir : meson.current_build_dir())
