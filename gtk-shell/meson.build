project(
  'lterm2-gtk',
  'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c17'
  ]
)

cc = meson.get_compiler('c')
gtkdep = dependency('gtk4', required : true)

core_source_root = meson.project_source_root()
core_build_dir = join_paths(core_source_root, '..', 'core', 'builddir')
core_build_src = join_paths(core_build_dir, 'src')

core_inc = include_directories('../core/include')
corelib = cc.find_library('lterm_core',
  dirs : [core_build_src, core_build_dir],
  required : true)

src = [
  'src/main.c',
  'src/lterm_app.c',
  'src/app_window.c',
  'src/core_bridge.c',
  'src/terminal_view.c',
]

executable(
  'lterm2-gtk',
  src,
  dependencies : [gtkdep, corelib],
  include_directories : [include_directories('src'), core_inc],
  install : false
)

