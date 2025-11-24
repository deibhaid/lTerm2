#include "vt100_string_parser.h"

#include <stdbool.h>
#include <string.h>

void
vt100_string_parser_init(vt100_string_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

void
vt100_string_parser_reset(vt100_string_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

size_t
vt100_string_parser_decode(vt100_string_parser *parser,
                           lterm_parser_context *context,
                           lterm_token *token,
                           lterm_token_type type)
{
    (void)parser;
    if (!context || !token) {
        return 0;
    }

    uint8_t *payload_start = context->data;
    int remaining = context->length;
    size_t payload_len = 0;
    size_t consumed = 0;
    bool finished = false;

    while (remaining > 0) {
        uint8_t c = payload_start[consumed];
        consumed++;
        remaining--;
        if (c == 0x07) {
            finished = true;
            break;
        }
        if (c == LTERM_CC_ESC) {
            if (remaining == 0) {
                return 0;
            }
            uint8_t next = payload_start[consumed];
            consumed++;
            remaining--;
            if (next == '\\') {
                finished = true;
                break;
            }
            payload_len += 2;
            continue;
        }
        payload_len++;
    }

    if (!finished) {
        return 0;
    }

    lterm_parser_advance_multiple(context, (int)consumed);
    token->type = type;
    if (payload_len > 0) {
        lterm_token_set_ascii(token, payload_start, payload_len);
    }
    return consumed;
}

