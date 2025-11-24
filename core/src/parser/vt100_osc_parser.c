#include "vt100_osc_parser.h"

#include <stdbool.h>
#include <string.h>

#include "iterm_token_types.h"

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

void
vt100_osc_parser_init(vt100_osc_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

void
vt100_osc_parser_reset(vt100_osc_parser *parser)
{
    if (!parser) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

static int
parse_code(const uint8_t *data, size_t length, size_t *payload_offset)
{
    size_t i = 0;
    int code = 0;
    bool has_digits = false;
    while (i < length && data[i] >= '0' && data[i] <= '9') {
        has_digits = true;
        code = code * 10 + (data[i] - '0');
        i++;
    }
    if (has_digits && i < length && data[i] == ';') {
        if (payload_offset) {
            *payload_offset = i + 1;
        }
        return code;
    }
    if (payload_offset) {
        *payload_offset = 0;
    }
    return -1;
}

typedef struct {
    const char *prefix;
    iterm_token_type type;
} osc_mapping;

static const osc_mapping kOscMappings[] = {
    { "0", ITERM_TOKEN_XTERM_WINICON_TITLE },
    { "1", ITERM_TOKEN_XTERM_ICON_TITLE },
    { "2", ITERM_TOKEN_XTERM_WIN_TITLE },
    { "52", ITERM_TOKEN_CLIPBOARD_CONTROL },
};

static const char *
skip_code_prefix(const uint8_t *data, size_t length, iterm_token_type *out_type)
{
    for (size_t i = 0; i < ARRAY_LENGTH(kOscMappings); ++i) {
        size_t prefix_len = strlen(kOscMappings[i].prefix);
        if (length > prefix_len && memcmp(data, kOscMappings[i].prefix, prefix_len) == 0 && data[prefix_len] == ';') {
            if (out_type) {
                *out_type = kOscMappings[i].type;
            }
            return (const char *)(data + prefix_len + 1);
        }
    }
    if (out_type) {
        *out_type = ITERM_TOKEN_OSC;
    }
    return (const char *)data;
}

size_t
vt100_osc_parser_decode(vt100_osc_parser *parser,
                        const uint8_t *data,
                        size_t length,
                        iterm_token *token)
{
    (void)parser;
    if (!data || !token) {
        return 0;
    }

    iterm_token_type inferred = ITERM_TOKEN_OSC;
    const char *payload = skip_code_prefix(data, length, &inferred);
    size_t payload_len = (payload >= (const char *)data && payload <= (const char *)(data + length))
                             ? (data + length) - (const uint8_t *)payload
                             : length;

    token->type = inferred;
    if (payload_len > 0) {
        iterm_token_set_ascii(token, (const uint8_t *)payload, payload_len);
    }
    return length;
}

