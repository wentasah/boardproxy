pytest = find_program('pytest', required : false, disabler : true)
if not pytest.found()
  pytest = find_program('pytest-3', required : false, disabler : true)
endif

test(
  'pytest',
  pytest,
  args : [ meson.current_source_dir() ],
  workdir : meson.current_build_dir())
