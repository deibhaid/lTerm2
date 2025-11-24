#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"
#include "iterm_parser_context.h"
#include "iterm_token_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int dummy;
} vt100_string_parser;

void vt100_string_parser_init(vt100_string_parser *parser);
void vt100_string_parser_reset(vt100_string_parser *parser);
size_t vt100_string_parser_decode(vt100_string_parser *parser,
                                  iterm_parser_context *context,
                                  iterm_token *token,
                                  iterm_token_type type);

#ifdef __cplusplus
}
#endif

