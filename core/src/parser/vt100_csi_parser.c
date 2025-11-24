#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lterm_csi_param.h"
#include "lterm_parser_context.h"
#include "vt100_csi_parser.h"
#include "lterm_token_types.h"

static void
reset_param(lterm_csi_param *param)
{
    lterm_csi_param_reset(param);
}

void
vt100_csi_parser_init(vt100_csi_parser *parser)
{
    if (!parser) {
        return;
    }
    reset_param(&parser->param);
}

void
vt100_csi_parser_reset(vt100_csi_parser *parser)
{
    if (!parser) {
        return;
    }
    reset_param(&parser->param);
}

static lterm_token_type map_csi_type(uint8_t final)
{
    switch (final) {
        case 'A': return LTERM_TOKEN_CSI_CUU;
        case 'B': return LTERM_TOKEN_CSI_CUD;
        case 'C': return LTERM_TOKEN_CSI_CUF;
        case 'D': return LTERM_TOKEN_CSI_CUB;
        case 'H':
        case 'f': return LTERM_TOKEN_CSI_CUP;
        case 'J': return LTERM_TOKEN_CSI_ED;
        case 'K': return LTERM_TOKEN_CSI_EL;
        case 'm': return LTERM_TOKEN_CSI_SGR;
        default:
            return LTERM_TOKEN_CSI;
    }
}

static bool
parse_parameters(lterm_parser_context *context, lterm_csi_param *param)
{
    int current = 0;
    bool have_value = false;
    int count = 0;

    while (lterm_parser_can_advance(context)) {
        uint8_t c = lterm_parser_peek(context);
        if (c >= '0' && c <= '9') {
            have_value = true;
            current = current * 10 + (c - '0');
            lterm_parser_advance(context);
            continue;
        }
        if (c == ';') {
            if (count < LTERM_CSI_PARAM_MAX) {
                param->p[count++] = have_value ? current : -1;
            }
            current = 0;
            have_value = false;
            lterm_parser_advance(context);
            continue;
        }
        if (c == ':') {
            return false;
        }
        break;
    }

    if (have_value || count > 0) {
        if (count < LTERM_CSI_PARAM_MAX) {
            param->p[count++] = have_value ? current : -1;
        }
    }

    param->count = count;
    return true;
}

size_t
vt100_csi_parser_decode(vt100_csi_parser *parser,
                        lterm_parser_context *context,
                        lterm_token *token)
{
    if (!parser || !context || !token) {
        return 0;
    }

    reset_param(&parser->param);
    uint8_t prefix = 0;
    uint8_t intermediate = 0;

    if (lterm_parser_can_advance(context)) {
        uint8_t c = lterm_parser_peek(context);
        if (c == '<' || c == '=' || c == '>' || c == '?') {
            prefix = c;
            lterm_parser_advance(context);
        }
    }

    if (!parse_parameters(context, &parser->param)) {
        return 0;
    }

    while (lterm_parser_can_advance(context)) {
        uint8_t c = lterm_parser_peek(context);
        if (c >= 0x20 && c <= 0x2F) {
            intermediate = c;
            lterm_parser_advance(context);
            continue;
        }
        break;
    }

    if (!lterm_parser_can_advance(context)) {
        return 0;
    }

    uint8_t final = lterm_parser_peek(context);
    if (final < 0x40 || final > 0x7E) {
        return 0;
    }
    lterm_parser_advance(context);

    token->type = map_csi_type(final);
    token->code = final;
    token->csi = parser->param;
    token->csi.cmd = LTERM_PACKED_CSI(prefix, intermediate, final);

    size_t consumed = (size_t)lterm_parser_bytes_consumed(context);
    return consumed;
}

