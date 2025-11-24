#include "iterm_parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "iterm_reader.h"
#include "iterm_screen.h"
#include "vt100_control_parser.h"
#include "vt100_csi_parser.h"
#include "vt100_string_parser.h"
#include "vt100_ansi_parser.h"
#include "vt100_osc_parser.h"
#include "vt100_dcs_parser.h"

struct buffer {
    uint8_t *data;
    size_t length;
    size_t capacity;
};

struct iterm_parser {
    struct buffer ascii_buffer;
    iterm_reader reader;
    vt100_control_parser control_parser;
    vt100_csi_parser csi_parser;
    vt100_string_parser string_parser;
    vt100_ansi_parser ansi_parser;
    vt100_osc_parser osc_parser;
    vt100_dcs_parser dcs_parser;
    iterm_screen *screen;
};

static bool
buffer_reserve(struct buffer *buffer, size_t extra)
{
    if (buffer->length + extra <= buffer->capacity) {
        return true;
    }
    size_t new_capacity = buffer->capacity ? buffer->capacity : 64;
    while (new_capacity < buffer->length + extra) {
        new_capacity *= 2;
    }
    uint8_t *new_data = realloc(buffer->data, new_capacity);
    if (!new_data) {
        return false;
    }
    buffer->data = new_data;
    buffer->capacity = new_capacity;
    return true;
}

static bool
buffer_append(struct buffer *buffer, uint8_t byte)
{
    if (!buffer_reserve(buffer, 1)) {
        return false;
    }
    buffer->data[buffer->length++] = byte;
    return true;
}

static void
buffer_reset(struct buffer *buffer)
{
    buffer->length = 0;
}

static void
emit_token(iterm_parser_callback callback,
           void *user_data,
           iterm_token_type type,
           const uint8_t *data,
           size_t length)
{
    if (!callback || type == ITERM_TOKEN_NONE) {
        return;
    }
    iterm_token token;
    iterm_token_init(&token);
    token.type = type;
    if (data && length) {
        if (type == ITERM_TOKEN_CONTROL) {
            token.code = data[0];
        } else {
            iterm_token_set_ascii(&token, data, length);
        }
    }
    callback(&token, user_data);
    iterm_token_free(&token);
}

static void
flush_ascii(iterm_parser *parser, iterm_parser_callback callback, void *user_data)
{
    if (!parser->ascii_buffer.length) {
        return;
    }
    if (parser->screen) {
        char *temp = malloc(parser->ascii_buffer.length + 1);
        memcpy(temp, parser->ascii_buffer.data, parser->ascii_buffer.length);
        temp[parser->ascii_buffer.length] = '\0';
        iterm_screen_put_text(parser->screen, temp);
        free(temp);
    } else {
        emit_token(callback,
                   user_data,
                   ITERM_TOKEN_ASCII,
                   parser->ascii_buffer.data,
                   parser->ascii_buffer.length);
    }
    buffer_reset(&parser->ascii_buffer);
}

iterm_parser *
iterm_parser_new(iterm_screen *screen)
{
    iterm_parser *parser = calloc(1, sizeof(*parser));
    if (!parser) {
        return NULL;
    }
    iterm_reader_init(&parser->reader);
    vt100_control_parser_init(&parser->control_parser);
    vt100_csi_parser_init(&parser->csi_parser);
    vt100_string_parser_init(&parser->string_parser);
    vt100_ansi_parser_init(&parser->ansi_parser);
    vt100_osc_parser_init(&parser->osc_parser);
    vt100_dcs_parser_init(&parser->dcs_parser);
    parser->screen = screen;
    return parser;
}

void
iterm_parser_free(iterm_parser *parser)
{
    if (!parser) {
        return;
    }
    free(parser->ascii_buffer.data);
    iterm_reader_free(&parser->reader);
    vt100_control_parser_reset(&parser->control_parser);
    vt100_csi_parser_reset(&parser->csi_parser);
    vt100_string_parser_reset(&parser->string_parser);
    vt100_ansi_parser_reset(&parser->ansi_parser);
    vt100_osc_parser_reset(&parser->osc_parser);
    vt100_dcs_parser_reset(&parser->dcs_parser);
    free(parser);
}

void
iterm_parser_reset(iterm_parser *parser)
{
    if (!parser) {
        return;
    }
    buffer_reset(&parser->ascii_buffer);
    iterm_reader_reset(&parser->reader);
    vt100_control_parser_reset(&parser->control_parser);
    vt100_csi_parser_reset(&parser->csi_parser);
    vt100_string_parser_reset(&parser->string_parser);
    vt100_ansi_parser_reset(&parser->ansi_parser);
    vt100_osc_parser_reset(&parser->osc_parser);
    vt100_dcs_parser_reset(&parser->dcs_parser);
}

void
iterm_parser_feed(iterm_parser *parser,
                  const uint8_t *bytes,
                  size_t length,
                  iterm_parser_callback callback,
                  void *user_data)
{
    if (!parser || !bytes) {
        return;
    }
    iterm_reader_append(&parser->reader, bytes, length);
    iterm_reader_cursor cursor;
    iterm_reader_cursor_init(&cursor, &parser->reader);
    size_t processed = 0;
    bool need_more_data = false;
    while (cursor.length > 0) {
        uint8_t byte = cursor.data[0];
        bool is_control = (byte < 0x20 || byte == 0x7F);
        if (!is_control &&
            parser->control_parser.support_8bit_controls &&
            byte >= 0x80 && byte <= 0x9F) {
            is_control = true;
        }

        if (is_control) {
            flush_ascii(parser, callback, user_data);
            iterm_token token;
            iterm_token_init(&token);
            size_t consumed = vt100_control_parser_parse(&parser->control_parser,
                                                         &parser->csi_parser,
                                                         &parser->string_parser,
                                                         &parser->ansi_parser,
                                                         &parser->osc_parser,
                                                         &parser->dcs_parser,
                                                         cursor.data,
                                                         cursor.length,
                                                         &token);
            if (consumed == 0) {
                iterm_token_free(&token);
                need_more_data = true;
                break;
            }
            if (parser->screen && token.type == ITERM_TOKEN_ASCII && token.ascii.length > 0) {
                char *temp = malloc(token.ascii.length + 1);
                if (temp) {
                    memcpy(temp, token.ascii.buffer, token.ascii.length);
                    temp[token.ascii.length] = '\0';
                    iterm_screen_put_text(parser->screen, temp);
                    free(temp);
                }
            } else if (token.type != ITERM_TOKEN_NONE && token.type != ITERM_TOKEN_WAIT) {
                callback(&token, user_data);
            }
            iterm_token_free(&token);
            cursor.data += consumed;
            cursor.length -= consumed;
            processed += consumed;
            continue;
        }

        buffer_append(&parser->ascii_buffer, byte);
        cursor.data++;
        cursor.length--;
        processed++;
    }
    iterm_reader_consume(&parser->reader, processed);

    if (!need_more_data) {
        flush_ascii(parser, callback, user_data);
    }
}

