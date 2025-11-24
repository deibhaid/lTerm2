core_includes = include_directories('..', '../include')

sources = [
  'stub_terminal.c',
  'lterm_pty.c',
  'parser/lterm_parser.c',
  'parser/lterm_state_machine.c',
  'parser/lterm_token.c',
  'parser/lterm_reader.c',
  'parser/vt100_control_parser.c',
  'parser/vt100_csi_parser.c',
  'parser/vt100_string_parser.c',
  'parser/vt100_ansi_parser.c',
  'parser/vt100_osc_parser.c',
  'parser/vt100_dcs_parser.c',
  'lterm_screen.c',
]

liblterm_core = library(
  'lterm_core',
  sources,
  include_directories : core_includes,
  install : false
)

liblterm_core_dep = declare_dependency(
  link_with : liblterm_core,
  include_directories : core_includes
)

