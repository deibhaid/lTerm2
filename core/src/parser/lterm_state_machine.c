#include "lterm_state_machine.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t character;
    lterm_state *to_state;
    lterm_state_action action;
    void *user_data;
} transition;

struct lterm_state {
    char *name;
    uintptr_t identifier;
    lterm_state_action entry_action;
    void *entry_user_data;
    lterm_state_action exit_action;
    void *exit_user_data;
    transition *transitions;
    size_t transition_count;
    size_t transition_capacity;
};

struct lterm_state_machine {
    lterm_state **states;
    size_t state_count;
    size_t state_capacity;
    lterm_state *ground_state;
    lterm_state *current_state;
};

static char *
duplicate_string(const char *source)
{
    if (!source) {
        return NULL;
    }
    size_t len = strlen(source);
    char *copy = malloc(len + 1);
    if (copy) {
        memcpy(copy, source, len + 1);
    }
    return copy;
}

lterm_state *
lterm_state_create(const char *name, uintptr_t identifier)
{
    lterm_state *state = calloc(1, sizeof(*state));
    if (!state) {
        return NULL;
    }
    state->name = duplicate_string(name);
    state->identifier = identifier;
    return state;
}

void
lterm_state_destroy(lterm_state *state)
{
    if (!state) {
        return;
    }
    free(state->name);
    free(state->transitions);
    free(state);
}

void
lterm_state_set_entry_action(lterm_state *state, lterm_state_action action, void *user_data)
{
    if (!state) {
        return;
    }
    state->entry_action = action;
    state->entry_user_data = user_data;
}

void
lterm_state_set_exit_action(lterm_state *state, lterm_state_action action, void *user_data)
{
    if (!state) {
        return;
    }
    state->exit_action = action;
    state->exit_user_data = user_data;
}

static transition *
add_transition_slot(lterm_state *state)
{
    if (!state) {
        return NULL;
    }
    if (state->transition_count == state->transition_capacity) {
        size_t new_capacity = state->transition_capacity ? state->transition_capacity * 2 : 8;
        transition *new_transitions = realloc(state->transitions, new_capacity * sizeof(transition));
        if (!new_transitions) {
            return NULL;
        }
        state->transitions = new_transitions;
        state->transition_capacity = new_capacity;
    }
    return &state->transitions[state->transition_count++];
}

void
lterm_state_add_transition(lterm_state *state,
                           uint32_t character,
                           lterm_state *to_state,
                           lterm_state_action action,
                           void *user_data)
{
    transition *slot = add_transition_slot(state);
    if (!slot) {
        return;
    }
    slot->character = character;
    slot->to_state = to_state;
    slot->action = action;
    slot->user_data = user_data;
}

void
lterm_state_add_transition_range(lterm_state *state,
                                 uint32_t start,
                                 uint32_t length,
                                 lterm_state *to_state,
                                 lterm_state_action action,
                                 void *user_data)
{
    for (uint32_t i = 0; i < length; ++i) {
        lterm_state_add_transition(state, start + i, to_state, action, user_data);
    }
}

uintptr_t
lterm_state_identifier(const lterm_state *state)
{
    return state ? state->identifier : 0;
}

const char *
lterm_state_name(const lterm_state *state)
{
    return state ? state->name : NULL;
}

lterm_state_machine *
lterm_state_machine_create(void)
{
    return calloc(1, sizeof(lterm_state_machine));
}

void
lterm_state_machine_destroy(lterm_state_machine *machine)
{
    if (!machine) {
        return;
    }
    free(machine->states);
    free(machine);
}

void
lterm_state_machine_add_state(lterm_state_machine *machine, lterm_state *state)
{
    if (!machine || !state) {
        return;
    }
    if (machine->state_count == machine->state_capacity) {
        size_t new_capacity = machine->state_capacity ? machine->state_capacity * 2 : 8;
        lterm_state **new_states = realloc(machine->states, new_capacity * sizeof(lterm_state *));
        if (!new_states) {
            return;
        }
        machine->states = new_states;
        machine->state_capacity = new_capacity;
    }
    machine->states[machine->state_count++] = state;
}

void
lterm_state_machine_set_ground_state(lterm_state_machine *machine, lterm_state *state)
{
    if (!machine) {
        return;
    }
    machine->ground_state = state;
    if (!machine->current_state) {
        machine->current_state = state;
    }
}

static transition *
find_transition(lterm_state *state, uint32_t character)
{
    if (!state) {
        return NULL;
    }
    for (size_t i = 0; i < state->transition_count; ++i) {
        if (state->transitions[i].character == character) {
            return &state->transitions[i];
        }
    }
    return NULL;
}

void
lterm_state_machine_handle_char(lterm_state_machine *machine, uint8_t ch)
{
    if (!machine || !machine->current_state) {
        return;
    }
    transition *trans = find_transition(machine->current_state, ch);
    if (!trans) {
        return;
    }

    lterm_state *from_state = machine->current_state;
    lterm_state *to_state = trans->to_state;
    bool changing = (to_state && to_state != from_state);

    if (changing && from_state->exit_action) {
        from_state->exit_action(ch, from_state->exit_user_data);
    }

    if (trans->action) {
        trans->action(ch, trans->user_data);
    }

    if (changing) {
        machine->current_state = to_state;
        if (to_state && to_state->entry_action) {
            to_state->entry_action(ch, to_state->entry_user_data);
        }
    }
}

lterm_state *
lterm_state_machine_find_state(const lterm_state_machine *machine, uintptr_t identifier)
{
    if (!machine) {
        return NULL;
    }
    for (size_t i = 0; i < machine->state_count; ++i) {
        if (lterm_state_identifier(machine->states[i]) == identifier) {
            return machine->states[i];
        }
    }
    return NULL;
}

