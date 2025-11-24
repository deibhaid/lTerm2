project(
  'iterm2-gtk',
  'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c17'
  ]
)

cc = meson.get_compiler('c')
gtkdep = dependency('gtk4', required : true)
core_inc = include_directories('../core/include')
corelib = cc.find_library('iterm_core', dirs : ['../core/builddir/src', '../core/builddir'], required : true)

src = [
  'src/main.c',
  'src/iterm_app.c',
  'src/app_window.c',
  'src/core_bridge.c',
  'src/terminal_view.c',
]

executable(
  'iterm2-gtk',
  src,
  dependencies : [gtkdep, corelib],
  include_directories : [include_directories('src'), core_inc],
  install : false
)

