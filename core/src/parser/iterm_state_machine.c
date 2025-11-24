#include "iterm_state_machine.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t character;
    iterm_state *to_state;
    iterm_state_action action;
    void *user_data;
} transition;

struct iterm_state {
    char *name;
    uintptr_t identifier;
    iterm_state_action entry_action;
    void *entry_user_data;
    iterm_state_action exit_action;
    void *exit_user_data;
    transition *transitions;
    size_t transition_count;
    size_t transition_capacity;
};

struct iterm_state_machine {
    iterm_state **states;
    size_t state_count;
    size_t state_capacity;
    iterm_state *ground_state;
    iterm_state *current_state;
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

iterm_state *
iterm_state_create(const char *name, uintptr_t identifier)
{
    iterm_state *state = calloc(1, sizeof(*state));
    if (!state) {
        return NULL;
    }
    state->name = duplicate_string(name);
    state->identifier = identifier;
    return state;
}

void
iterm_state_destroy(iterm_state *state)
{
    if (!state) {
        return;
    }
    free(state->name);
    free(state->transitions);
    free(state);
}

void
iterm_state_set_entry_action(iterm_state *state, iterm_state_action action, void *user_data)
{
    if (!state) {
        return;
    }
    state->entry_action = action;
    state->entry_user_data = user_data;
}

void
iterm_state_set_exit_action(iterm_state *state, iterm_state_action action, void *user_data)
{
    if (!state) {
        return;
    }
    state->exit_action = action;
    state->exit_user_data = user_data;
}

static transition *
add_transition_slot(iterm_state *state)
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
iterm_state_add_transition(iterm_state *state,
                           uint32_t character,
                           iterm_state *to_state,
                           iterm_state_action action,
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
iterm_state_add_transition_range(iterm_state *state,
                                 uint32_t start,
                                 uint32_t length,
                                 iterm_state *to_state,
                                 iterm_state_action action,
                                 void *user_data)
{
    for (uint32_t i = 0; i < length; ++i) {
        iterm_state_add_transition(state, start + i, to_state, action, user_data);
    }
}

uintptr_t
iterm_state_identifier(const iterm_state *state)
{
    return state ? state->identifier : 0;
}

const char *
iterm_state_name(const iterm_state *state)
{
    return state ? state->name : NULL;
}

iterm_state_machine *
iterm_state_machine_create(void)
{
    return calloc(1, sizeof(iterm_state_machine));
}

void
iterm_state_machine_destroy(iterm_state_machine *machine)
{
    if (!machine) {
        return;
    }
    free(machine->states);
    free(machine);
}

void
iterm_state_machine_add_state(iterm_state_machine *machine, iterm_state *state)
{
    if (!machine || !state) {
        return;
    }
    if (machine->state_count == machine->state_capacity) {
        size_t new_capacity = machine->state_capacity ? machine->state_capacity * 2 : 8;
        iterm_state **new_states = realloc(machine->states, new_capacity * sizeof(iterm_state *));
        if (!new_states) {
            return;
        }
        machine->states = new_states;
        machine->state_capacity = new_capacity;
    }
    machine->states[machine->state_count++] = state;
}

void
iterm_state_machine_set_ground_state(iterm_state_machine *machine, iterm_state *state)
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
find_transition(iterm_state *state, uint32_t character)
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
iterm_state_machine_handle_char(iterm_state_machine *machine, uint8_t ch)
{
    if (!machine || !machine->current_state) {
        return;
    }
    transition *trans = find_transition(machine->current_state, ch);
    if (!trans) {
        return;
    }

    iterm_state *from_state = machine->current_state;
    iterm_state *to_state = trans->to_state;
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

iterm_state *
iterm_state_machine_find_state(const iterm_state_machine *machine, uintptr_t identifier)
{
    if (!machine) {
        return NULL;
    }
    for (size_t i = 0; i < machine->state_count; ++i) {
        if (iterm_state_identifier(machine->states[i]) == identifier) {
            return machine->states[i];
        }
    }
    return NULL;
}

