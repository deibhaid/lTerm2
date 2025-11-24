parser_test = executable(
  'parser_test',
  ['parser_test.c'],
  dependencies : [libiterm_core_dep],
  include_directories : core_includes
)

test('parser', parser_test)

state_machine_test = executable(
  'state_machine_test',
  ['state_machine_test.c'],
  dependencies : [libiterm_core_dep],
  include_directories : core_includes
)

test('state_machine', state_machine_test)

