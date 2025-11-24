#include "vt100_ansi_parser.h"

#include <string.h>

void
vt100_ansi_parser_init(vt100_ansi_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

void
vt100_ansi_parser_reset(vt100_ansi_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

size_t
vt100_ansi_parser_decode(vt100_ansi_parser *parser,
                         const uint8_t *data,
                         size_t length,
                         iterm_token *token)
{
    (void)parser;
    if (!data || length < 2 || !token) {
        return 0;
    }
    if (data[0] != ITERM_CC_ESC) {
        return 0;
    }
    if (data[1] == 'c') {
        token->type = ITERM_TOKEN_ANSI_RIS;
        token->code = 'c';
        return 2;
    }
    return 0;
}

