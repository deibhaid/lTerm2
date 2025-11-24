#include "vt100_dcs_parser.h"

#include <stdbool.h>
#include <string.h>

#include "iterm_token_types.h"

void
vt100_dcs_parser_init(vt100_dcs_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
    parser->state = VT100_DCS_STATE_ENTRY;
}

void
vt100_dcs_parser_reset(vt100_dcs_parser *parser)
{
    if (!parser) {
        return;
    }
    parser->state = VT100_DCS_STATE_ENTRY;
}

size_t
vt100_dcs_parser_decode(vt100_dcs_parser *parser,
                        iterm_parser_context *context,
                        iterm_token *token)
{
    if (!parser || !context || !token) {
        return 0;
    }

    size_t consumed = 0;
    bool finished = false;

    while (iterm_parser_can_advance(context)) {
        uint8_t c = iterm_parser_consume(context);
        consumed++;

        if (c == 0x9C) {
            finished = true;
            break;
        }
        if (c == ITERM_CC_ESC) {
            if (!iterm_parser_can_advance(context)) {
                return 0;
            }
            uint8_t next = iterm_parser_consume(context);
            consumed++;
            if (next == '\\') {
                finished = true;
                break;
            }
            if (parser->length < sizeof(parser->buffer) - 1) {
                parser->buffer[parser->length++] = c;
                parser->buffer[parser->length++] = next;
            }
            continue;
        }

        if (parser->length < sizeof(parser->buffer) - 1) {
            parser->buffer[parser->length++] = c;
        }
    }

    if (!finished) {
        parser->length = 0;
        return 0;
    }

    parser->buffer[parser->length] = '\0';
    char *payload = parser->buffer;
    iterm_token_type type = ITERM_TOKEN_DCS;
    const char tmux_prefix[] = "tmux;";
    if (parser->length >= (int)strlen(tmux_prefix) &&
        strncmp(parser->buffer, tmux_prefix, strlen(tmux_prefix)) == 0) {
        type = ITERM_TOKEN_TMUX;
        payload += strlen(tmux_prefix);
        char *exit_marker = strstr(payload, "%exit");
        if (exit_marker) {
            *exit_marker = '\0';
        }
        char *semicolon = strchr(payload, ';');
        if (semicolon) {
            *semicolon = ':';
        }
    }
    token->type = type;
    iterm_token_set_ascii(token, (const uint8_t *)payload, strlen(payload));
    parser->length = 0;
    return consumed;
}

