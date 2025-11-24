#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*iterm_state_action)(uint8_t ch, void *user_data);

typedef struct iterm_state iterm_state;
typedef struct iterm_state_machine iterm_state_machine;

iterm_state *iterm_state_create(const char *name, uintptr_t identifier);
void iterm_state_destroy(iterm_state *state);

void iterm_state_set_entry_action(iterm_state *state, iterm_state_action action, void *user_data);
void iterm_state_set_exit_action(iterm_state *state, iterm_state_action action, void *user_data);

void iterm_state_add_transition(iterm_state *state,
                                uint32_t character,
                                iterm_state *to_state,
                                iterm_state_action action,
                                void *user_data);
void iterm_state_add_transition_range(iterm_state *state,
                                      uint32_t start,
                                      uint32_t length,
                                      iterm_state *to_state,
                                      iterm_state_action action,
                                      void *user_data);

uintptr_t iterm_state_identifier(const iterm_state *state);
const char *iterm_state_name(const iterm_state *state);

iterm_state_machine *iterm_state_machine_create(void);
void iterm_state_machine_destroy(iterm_state_machine *machine);
void iterm_state_machine_add_state(iterm_state_machine *machine, iterm_state *state);
void iterm_state_machine_set_ground_state(iterm_state_machine *machine, iterm_state *state);
void iterm_state_machine_handle_char(iterm_state_machine *machine, uint8_t ch);
iterm_state *iterm_state_machine_find_state(const iterm_state_machine *machine, uintptr_t identifier);

#ifdef __cplusplus
}
#endif

