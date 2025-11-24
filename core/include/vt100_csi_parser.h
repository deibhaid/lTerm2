#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"
#include "iterm_csi_param.h"
#include "iterm_parser_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    iterm_csi_param param;
} vt100_csi_parser;

void vt100_csi_parser_init(vt100_csi_parser *parser);
void vt100_csi_parser_reset(vt100_csi_parser *parser);
size_t vt100_csi_parser_decode(vt100_csi_parser *parser,
                               iterm_parser_context *context,
                               iterm_token *token);

#ifdef __cplusplus
}
#endif

