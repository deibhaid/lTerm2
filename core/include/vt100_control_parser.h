#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lterm_token.h"
#include "vt100_csi_parser.h"
#include "vt100_string_parser.h"
#include "vt100_ansi_parser.h"
#include "vt100_osc_parser.h"
#include "vt100_dcs_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int support_8bit_controls;
    int dcs_hooked;
} vt100_control_parser;

void vt100_control_parser_init(vt100_control_parser *parser);
void vt100_control_parser_reset(vt100_control_parser *parser);

size_t vt100_control_parser_parse(vt100_control_parser *parser,
                                  vt100_csi_parser *csi_parser,
                                  vt100_string_parser *string_parser,
                                  vt100_ansi_parser *ansi_parser,
                                  vt100_osc_parser *osc_parser,
                                  vt100_dcs_parser *dcs_parser,
                                  const uint8_t *data,
                                  size_t length,
                                  lterm_token *token);

#ifdef __cplusplus
}
#endif

