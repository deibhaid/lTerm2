#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iterm_csi_param.h"
#include "iterm_parser_context.h"
#include "vt100_csi_parser.h"

static void
reset_param(iterm_csi_param *param)
{
    iterm_csi_param_reset(param);
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

static bool
parse_parameters(iterm_parser_context *context, iterm_csi_param *param)
{
    int current = 0;
    bool have_value = false;
    int count = 0;

    while (iterm_parser_can_advance(context)) {
        uint8_t c = iterm_parser_peek(context);
        if (c >= '0' && c <= '9') {
            have_value = true;
            current = current * 10 + (c - '0');
            iterm_parser_advance(context);
            continue;
        }
        if (c == ';') {
            if (count < ITERM_CSI_PARAM_MAX) {
                param->p[count++] = have_value ? current : -1;
            }
            current = 0;
            have_value = false;
            iterm_parser_advance(context);
            continue;
        }
        if (c == ':') {
            return false;
        }
        break;
    }

    if (have_value || count > 0) {
        if (count < ITERM_CSI_PARAM_MAX) {
            param->p[count++] = have_value ? current : -1;
        }
    }

    param->count = count;
    return true;
}

size_t
vt100_csi_parser_decode(vt100_csi_parser *parser,
                        iterm_parser_context *context,
                        iterm_token *token)
{
    if (!parser || !context || !token) {
        return 0;
    }

    reset_param(&parser->param);
    uint8_t prefix = 0;
    uint8_t intermediate = 0;

    if (iterm_parser_can_advance(context)) {
        uint8_t c = iterm_parser_peek(context);
        if (c == '<' || c == '=' || c == '>' || c == '?') {
            prefix = c;
            iterm_parser_advance(context);
        }
    }

    if (!parse_parameters(context, &parser->param)) {
        return 0;
    }

    while (iterm_parser_can_advance(context)) {
        uint8_t c = iterm_parser_peek(context);
        if (c >= 0x20 && c <= 0x2F) {
            intermediate = c;
            iterm_parser_advance(context);
            continue;
        }
        break;
    }

    if (!iterm_parser_can_advance(context)) {
        return 0;
    }

    uint8_t final = iterm_parser_peek(context);
    if (final < 0x40 || final > 0x7E) {
        return 0;
    }
    iterm_parser_advance(context);

    token->type = ITERM_TOKEN_CSI;
    token->code = final;
    token->csi = parser->param;
    token->csi.cmd = ITERM_PACKED_CSI(prefix, intermediate, final);

    size_t consumed = (size_t)iterm_parser_bytes_consumed(context);
    return consumed;
}
#include "vt100_csi_parser.h"

#include <stdbool.h>
#include <string.h>

#include "iterm_parser_context.h"
#include "iterm_token.h"

