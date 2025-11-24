#include <assert.h>
#include <stdio.h>

#include "lterm_state_machine.h"

typedef struct {
    int entry_count;
    int exit_count;
    int action_count;
} counters;

static void
entry_callback(uint8_t ch, void *user_data)
{
    (void)ch;
    counters *c = user_data;
    c->entry_count++;
}

static void
exit_callback(uint8_t ch, void *user_data)
{
    (void)ch;
    counters *c = user_data;
    c->exit_count++;
}

static void
action_callback(uint8_t ch, void *user_data)
{
    (void)ch;
    counters *c = user_data;
    c->action_count++;
}

int
main(void)
{
    lterm_state_machine *machine = lterm_state_machine_create();
    assert(machine);

    counters c = {0};
    lterm_state *ground = lterm_state_create("ground", 1);
    lterm_state_set_entry_action(ground, entry_callback, &c);
    lterm_state_set_exit_action(ground, exit_callback, &c);

    lterm_state *escape = lterm_state_create("escape", 2);
    lterm_state_set_entry_action(escape, entry_callback, &c);
    lterm_state_set_exit_action(escape, exit_callback, &c);

    lterm_state_machine_add_state(machine, ground);
    lterm_state_machine_add_state(machine, escape);
    lterm_state_machine_set_ground_state(machine, ground);

    lterm_state_add_transition(ground, 0x1B, escape, action_callback, &c);
    lterm_state_add_transition(escape, '[', ground, action_callback, &c);

    lterm_state_machine_handle_char(machine, 'a');
    assert(c.action_count == 0);

    lterm_state_machine_handle_char(machine, 0x1B);
    assert(c.action_count == 1);
    assert(c.exit_count == 1);
    assert(c.entry_count == 1);

    lterm_state_machine_handle_char(machine, '[');
    assert(c.action_count == 2);
    assert(c.exit_count == 2);
    assert(c.entry_count == 2);

    lterm_state *found = lterm_state_machine_find_state(machine, 2);
    assert(found == escape);

    lterm_state_machine_destroy(machine);
    lterm_state_destroy(ground);
    lterm_state_destroy(escape);
    printf("state machine tests passed\n");
    return 0;
}

