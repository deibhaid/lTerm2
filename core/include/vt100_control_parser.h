#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int support_8bit_controls;
    int dcs_hooked;
} vt100_control_parser;

void vt100_control_parser_init(vt100_control_parser *parser);
void vt100_control_parser_reset(vt100_control_parser *parser);

struct vt100_csi_parser;
struct vt100_string_parser;
struct vt100_ansi_parser;
struct vt100_osc_parser;
struct vt100_dcs_parser;

size_t vt100_control_parser_parse(vt100_control_parser *parser,
                                  struct vt100_csi_parser *csi_parser,
                                  struct vt100_string_parser *string_parser,
                                  struct vt100_ansi_parser *ansi_parser,
                                  struct vt100_osc_parser *osc_parser,
                                  struct vt100_dcs_parser *dcs_parser,
                                  const uint8_t *data,
                                  size_t length,
                                  iterm_token *token);

#ifdef __cplusplus
}
#endif

