#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lterm_state_action)(uint8_t ch, void *user_data);

typedef struct lterm_state lterm_state;
typedef struct lterm_state_machine lterm_state_machine;

lterm_state *lterm_state_create(const char *name, uintptr_t identifier);
void lterm_state_destroy(lterm_state *state);

void lterm_state_set_entry_action(lterm_state *state, lterm_state_action action, void *user_data);
void lterm_state_set_exit_action(lterm_state *state, lterm_state_action action, void *user_data);

void lterm_state_add_transition(lterm_state *state,
                                uint32_t character,
                                lterm_state *to_state,
                                lterm_state_action action,
                                void *user_data);
void lterm_state_add_transition_range(lterm_state *state,
                                      uint32_t start,
                                      uint32_t length,
                                      lterm_state *to_state,
                                      lterm_state_action action,
                                      void *user_data);

uintptr_t lterm_state_identifier(const lterm_state *state);
const char *lterm_state_name(const lterm_state *state);

lterm_state_machine *lterm_state_machine_create(void);
void lterm_state_machine_destroy(lterm_state_machine *machine);
void lterm_state_machine_add_state(lterm_state_machine *machine, lterm_state *state);
void lterm_state_machine_set_ground_state(lterm_state_machine *machine, lterm_state *state);
void lterm_state_machine_handle_char(lterm_state_machine *machine, uint8_t ch);
lterm_state *lterm_state_machine_find_state(const lterm_state_machine *machine, uintptr_t identifier);

#ifdef __cplusplus
}
#endif

