core_includes = include_directories('..', '../include')

sources = [
  'stub_terminal.c',
  'iterm_pty.c',
  'parser/iterm_parser.c',
  'parser/iterm_state_machine.c',
  'parser/iterm_token.c',
  'parser/iterm_reader.c',
  'parser/vt100_control_parser.c',
  'parser/vt100_csi_parser.c',
  'parser/vt100_string_parser.c',
  'parser/vt100_ansi_parser.c',
  'parser/vt100_osc_parser.c',
  'parser/vt100_dcs_parser.c',
  'iterm_screen.c',
]

libiterm_core = library(
  'iterm_core',
  sources,
  include_directories : core_includes,
  install : false
)

libiterm_core_dep = declare_dependency(
  link_with : libiterm_core,
  include_directories : core_includes
)

