#include "lterm_parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lterm_reader.h"
#include "lterm_screen.h"
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

struct lterm_parser {
    struct buffer ascii_buffer;
    lterm_reader reader;
    vt100_control_parser control_parser;
    vt100_csi_parser csi_parser;
    vt100_string_parser string_parser;
    vt100_ansi_parser ansi_parser;
    vt100_osc_parser osc_parser;
    vt100_dcs_parser dcs_parser;
    lterm_screen *screen;
};

static int csi_param_value(const lterm_csi_param *param, int index, int fallback)
{
    if (!param || index < 0 || index >= param->count) {
        return fallback;
    }
    int value = param->p[index];
    return value == -1 ? fallback : value;
}

static bool apply_token_to_screen(lterm_parser *parser, const lterm_token *token)
{
    if (!parser || !parser->screen || !token) {
        return false;
    }
    switch (token->type) {
        case LTERM_TOKEN_CSI_CUU: {
            int count = csi_param_value(&token->csi, 0, 1);
            lterm_screen_move_cursor(parser->screen, -count, 0);
            return true;
        }
        case LTERM_TOKEN_CSI_CUD: {
            int count = csi_param_value(&token->csi, 0, 1);
            lterm_screen_move_cursor(parser->screen, count, 0);
            return true;
        }
        case LTERM_TOKEN_CSI_CUF: {
            int count = csi_param_value(&token->csi, 0, 1);
            lterm_screen_move_cursor(parser->screen, 0, count);
            return true;
        }
        case LTERM_TOKEN_CSI_CUB: {
            int count = csi_param_value(&token->csi, 0, 1);
            lterm_screen_move_cursor(parser->screen, 0, -count);
            return true;
        }
        case LTERM_TOKEN_CSI_CUP: {
            int row = csi_param_value(&token->csi, 0, 1);
            int col = csi_param_value(&token->csi, 1, 1);
            size_t target_row = row > 0 ? (size_t)(row - 1) : 0;
            size_t target_col = col > 0 ? (size_t)(col - 1) : 0;
            lterm_screen_set_cursor(parser->screen, target_row, target_col);
            return true;
        }
        case LTERM_TOKEN_CSI_ED: {
            int mode = csi_param_value(&token->csi, 0, 0);
            lterm_screen_clear_screen(parser->screen, mode);
            return true;
        }
        case LTERM_TOKEN_CSI_EL: {
            int mode = csi_param_value(&token->csi, 0, 0);
            lterm_screen_clear_line(parser->screen, mode);
            return true;
        }
        case LTERM_TOKEN_CSI_SGR:
            lterm_screen_apply_sgr(parser->screen, &token->csi);
            return true;
        default:
            return false;
    }
}


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
emit_token(lterm_parser_callback callback,
           void *user_data,
           lterm_token_type type,
           const uint8_t *data,
           size_t length)
{
    if (!callback || type == LTERM_TOKEN_NONE) {
        return;
    }
    lterm_token token;
    lterm_token_init(&token);
    token.type = type;
    if (data && length) {
        if (type == LTERM_TOKEN_CONTROL) {
            token.code = data[0];
        } else {
            lterm_token_set_ascii(&token, data, length);
        }
    }
    callback(&token, user_data);
    lterm_token_free(&token);
}

static void
flush_ascii(lterm_parser *parser, lterm_parser_callback callback, void *user_data)
{
    if (!parser->ascii_buffer.length) {
        return;
    }
    if (parser->screen) {
        char *temp = malloc(parser->ascii_buffer.length + 1);
        memcpy(temp, parser->ascii_buffer.data, parser->ascii_buffer.length);
        temp[parser->ascii_buffer.length] = '\0';
        lterm_screen_put_text(parser->screen, temp);
        free(temp);
    } else {
        emit_token(callback,
                   user_data,
                   LTERM_TOKEN_ASCII,
                   parser->ascii_buffer.data,
                   parser->ascii_buffer.length);
    }
    buffer_reset(&parser->ascii_buffer);
}

lterm_parser *
lterm_parser_new(lterm_screen *screen)
{
    lterm_parser *parser = calloc(1, sizeof(*parser));
    if (!parser) {
        return NULL;
    }
    lterm_reader_init(&parser->reader);
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
lterm_parser_free(lterm_parser *parser)
{
    if (!parser) {
        return;
    }
    free(parser->ascii_buffer.data);
    lterm_reader_free(&parser->reader);
    vt100_control_parser_reset(&parser->control_parser);
    vt100_csi_parser_reset(&parser->csi_parser);
    vt100_string_parser_reset(&parser->string_parser);
    vt100_ansi_parser_reset(&parser->ansi_parser);
    vt100_osc_parser_reset(&parser->osc_parser);
    vt100_dcs_parser_reset(&parser->dcs_parser);
    free(parser);
}

void
lterm_parser_reset(lterm_parser *parser)
{
    if (!parser) {
        return;
    }
    buffer_reset(&parser->ascii_buffer);
    lterm_reader_reset(&parser->reader);
    vt100_control_parser_reset(&parser->control_parser);
    vt100_csi_parser_reset(&parser->csi_parser);
    vt100_string_parser_reset(&parser->string_parser);
    vt100_ansi_parser_reset(&parser->ansi_parser);
    vt100_osc_parser_reset(&parser->osc_parser);
    vt100_dcs_parser_reset(&parser->dcs_parser);
}

void
lterm_parser_feed(lterm_parser *parser,
                  const uint8_t *bytes,
                  size_t length,
                  lterm_parser_callback callback,
                  void *user_data)
{
    if (!parser || !bytes) {
        return;
    }
    lterm_reader_append(&parser->reader, bytes, length);
    lterm_reader_cursor cursor;
    lterm_reader_cursor_init(&cursor, &parser->reader);
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
            lterm_token token;
            lterm_token_init(&token);
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
                lterm_token_free(&token);
                need_more_data = true;
                break;
            }
            bool token_applied = false;
            if (parser->screen) {
                token_applied = apply_token_to_screen(parser, &token);
            }
            if (!token_applied && token.type != LTERM_TOKEN_NONE && token.type != LTERM_TOKEN_WAIT) {
                callback(&token, user_data);
            }
            lterm_token_free(&token);
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
    lterm_reader_consume(&parser->reader, processed);

    if (!need_more_data) {
        flush_ascii(parser, callback, user_data);
    }
}

