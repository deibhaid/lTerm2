#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lterm_token.h"
#include "lterm_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lterm_parser lterm_parser;
lterm_parser *lterm_parser_new(lterm_screen *screen);
void lterm_parser_free(lterm_parser *parser);
void lterm_parser_reset(lterm_parser *parser);
typedef void (*lterm_parser_callback)(const lterm_token *token, void *user_data);
void lterm_parser_feed(lterm_parser *parser,
                       const uint8_t *bytes,
                       size_t length,
                       lterm_parser_callback callback,
                       void *user_data);

#ifdef __cplusplus
}
#endif

