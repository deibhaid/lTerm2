#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"
#include "iterm_parser_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VT100_DCS_STATE_ENTRY,
    VT100_DCS_STATE_PARAM,
    VT100_DCS_STATE_INTERMEDIATE,
    VT100_DCS_STATE_PASSTHROUGH,
    VT100_DCS_STATE_ESCAPE,
} vt100_dcs_state;

typedef struct {
    vt100_dcs_state state;
    int support_8bit_controls;
    char buffer[4096];
    size_t length;
} vt100_dcs_parser;

void vt100_dcs_parser_init(vt100_dcs_parser *parser);
void vt100_dcs_parser_reset(vt100_dcs_parser *parser);
size_t vt100_dcs_parser_decode(vt100_dcs_parser *parser,
                               iterm_parser_context *context,
                               iterm_token *token);

#ifdef __cplusplus
}
#endif

