#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char buffer[4096];
} vt100_osc_parser;

void vt100_osc_parser_init(vt100_osc_parser *parser);
void vt100_osc_parser_reset(vt100_osc_parser *parser);
size_t vt100_osc_parser_decode(vt100_osc_parser *parser,
                               const uint8_t *data,
                               size_t length,
                               iterm_token *token);

#ifdef __cplusplus
}
#endif

