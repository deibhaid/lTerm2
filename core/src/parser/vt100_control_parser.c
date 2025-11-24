#include "vt100_control_parser.h"

#include <stdbool.h>
#include <string.h>

#include "lterm_parser_context.h"
#include "vt100_csi_parser.h"
#include "vt100_string_parser.h"
#include "vt100_ansi_parser.h"
#include "vt100_osc_parser.h"
#include "vt100_dcs_parser.h"
#include "lterm_token_types.h"

void
vt100_control_parser_init(vt100_control_parser *parser)
{
    if (!parser) {
        return;
    }
    parser->support_8bit_controls = 0;
    parser->dcs_hooked = 0;
}

void
vt100_control_parser_reset(vt100_control_parser *parser)
{
    if (!parser) {
        return;
    }
    parser->dcs_hooked = 0;
}

static size_t
consume_run(uint8_t value, const uint8_t *data, size_t length)
{
    size_t consumed = 0;
    while (consumed < length && data[consumed] == value) {
        consumed++;
    }
    return consumed;
}

static bool
should_emit_control(uint8_t byte, bool support8)
{
    if (byte < 0x20 || byte == 0x7F) {
        return true;
    }
    if (support8 && byte >= 0x80 && byte <= 0x9F) {
        return true;
    }
    return false;
}

size_t
vt100_control_parser_parse(vt100_control_parser *parser,
                           vt100_csi_parser *csi_parser,
                           vt100_string_parser *string_parser,
                           vt100_ansi_parser *ansi_parser,
                           vt100_osc_parser *osc_parser,
                           vt100_dcs_parser *dcs_parser,
                           const uint8_t *data,
                           size_t length,
                           lterm_token *token)
{
    if (!parser || !data || !length || !token) {
        return 0;
    }

    uint8_t byte = data[0];
    bool support8 = parser->support_8bit_controls;
    if (dcs_parser) {
        dcs_parser->support_8bit_controls = support8;
    }

    if (byte == LTERM_CC_NULL) {
        size_t consumed = consume_run(LTERM_CC_NULL, data, length);
        token->type = LTERM_TOKEN_SKIP;
        token->code = LTERM_CC_NULL;
        return consumed;
    }

    if (byte == LTERM_CC_ESC || (support8 && (byte == LTERM_CC_C1_CSI || byte == LTERM_CC_C1_OSC || byte == LTERM_CC_C1_DCS))) {
        if (byte == LTERM_CC_ESC && length == 1) {
            token->type = LTERM_TOKEN_WAIT;
            return 0;
        }

        size_t prefix = (data[0] == LTERM_CC_ESC) ? 2 : 1;
        if (length < prefix) {
            token->type = LTERM_TOKEN_WAIT;
            return 0;
        }

        uint8_t indicator = (data[0] == LTERM_CC_ESC) ? data[1] : data[0];
        const uint8_t *payload = data + prefix;
        size_t payload_len = (length > prefix) ? length - prefix : 0;

        if (indicator == '[' || indicator == LTERM_CC_C1_CSI) {
            lterm_parser_context ctx = lterm_parser_context_make((uint8_t *)payload, (int)payload_len);
            size_t consumed = vt100_csi_parser_decode(csi_parser, &ctx, token);
            if (consumed == 0) {
                token->type = LTERM_TOKEN_WAIT;
                return 0;
            }
            return consumed + prefix;
        }

        if (indicator == ']' || indicator == LTERM_CC_C1_OSC) {
            lterm_parser_context ctx = lterm_parser_context_make((uint8_t *)payload, (int)payload_len);
            lterm_token temp;
            lterm_token_init(&temp);
            size_t consumed = vt100_string_parser_decode(string_parser, &ctx, &temp, LTERM_TOKEN_OSC);
            if (consumed == 0) {
                lterm_token_free(&temp);
                token->type = LTERM_TOKEN_WAIT;
                return 0;
            }
            size_t parsed = vt100_osc_parser_decode(osc_parser, temp.ascii.buffer, temp.ascii.length, token);
            lterm_token_free(&temp);
            if (parsed == 0) {
                token->type = LTERM_TOKEN_WAIT;
                return 0;
            }
            token->type = LTERM_TOKEN_OSC;
            return consumed + prefix;
        }

        if (indicator == 'P' || indicator == LTERM_CC_C1_DCS) {
            lterm_parser_context ctx = lterm_parser_context_make((uint8_t *)payload, (int)payload_len);
            size_t consumed = vt100_dcs_parser_decode(dcs_parser, &ctx, token);
            if (consumed == 0) {
                token->type = LTERM_TOKEN_WAIT;
                return 0;
            }
            return consumed + prefix;
        }

        if (indicator == '(' || indicator == ')' || indicator == '*' || indicator == '+') {
            size_t consumed = vt100_ansi_parser_decode(ansi_parser, data, length, token);
            if (consumed == 0) {
                token->type = LTERM_TOKEN_WAIT;
                return 0;
            }
            return consumed;
        }

        token->type = LTERM_TOKEN_WAIT;
        return 0;
    }

    if (should_emit_control(byte, support8)) {
        token->type = byte;
        token->code = byte;
        return 1;
    }

    token->type = LTERM_TOKEN_NONE;
    return 0;
}

