#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"
#include "iterm_token_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int unused;
} vt100_ansi_parser;

void vt100_ansi_parser_init(vt100_ansi_parser *parser);
void vt100_ansi_parser_reset(vt100_ansi_parser *parser);
size_t vt100_ansi_parser_decode(vt100_ansi_parser *parser,
                                const uint8_t *data,
                                size_t length,
                                iterm_token *token);

#ifdef __cplusplus
}
#endif

