#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"
#include "iterm_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iterm_parser iterm_parser;
iterm_parser *iterm_parser_new(iterm_screen *screen);
void iterm_parser_free(iterm_parser *parser);
void iterm_parser_reset(iterm_parser *parser);
typedef void (*iterm_parser_callback)(const iterm_token *token, void *user_data);
void iterm_parser_feed(iterm_parser *parser,
                       const uint8_t *bytes,
                       size_t length,
                       iterm_parser_callback callback,
                       void *user_data);

#ifdef __cplusplus
}
#endif

