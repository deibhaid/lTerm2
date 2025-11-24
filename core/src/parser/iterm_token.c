#include "iterm_token.h"

#include <string.h>

static void
ascii_buffer_reset(iterm_ascii_buffer *ascii)
{
    if (!ascii) {
        return;
    }
    if (ascii->buffer && ascii->buffer != ascii->static_buffer) {
        free(ascii->buffer);
    }
    ascii->buffer = NULL;
    ascii->length = 0;
}

static void
free_subtokens(iterm_token_subtoken *subtoken)
{
    while (subtoken) {
        iterm_token_subtoken *next = subtoken->next;
        iterm_token_free(&subtoken->token);
        free(subtoken);
        subtoken = next;
    }
}

static void
copy_screen_chars(iterm_screen_char_array *dest, const iterm_screen_char_array *src)
{
    if (!dest || !src) {
        return;
    }
    iterm_screen_char_array_reset(dest);
    for (size_t i = 0; i < src->length; ++i) {
        iterm_screen_char_array_append(dest, &src->buffer[i]);
    }
}

void
iterm_token_init(iterm_token *token)
{
    if (!token) {
        return;
    }
    memset(token, 0, sizeof(*token));
    token->type = ITERM_CC_NULL;
    token->code = 0;
    token->ascii.buffer = token->ascii.static_buffer;
    token->ascii.length = 0;
    iterm_csi_param_reset(&token->csi);
    iterm_screen_char_array_init(&token->screen_chars);
    token->subtokens = NULL;
    token->saved_data = NULL;
    token->saved_length = 0;
    token->kvp_key = NULL;
    token->kvp_value = NULL;
    token->crlf_count = 0;
}

void
iterm_token_reset(iterm_token *token)
{
    if (!token) {
        return;
    }
    ascii_buffer_reset(&token->ascii);
    token->ascii.buffer = token->ascii.static_buffer;
    token->ascii.length = 0;
    token->type = ITERM_CC_NULL;
    token->code = 0;
    iterm_csi_param_reset(&token->csi);
    iterm_screen_char_array_reset(&token->screen_chars);
    free_subtokens(token->subtokens);
    token->subtokens = NULL;
    free(token->saved_data);
    token->saved_data = NULL;
    token->saved_length = 0;
    free(token->kvp_key);
    free(token->kvp_value);
    token->kvp_key = NULL;
    token->kvp_value = NULL;
    token->crlf_count = 0;
}

void
iterm_token_free(iterm_token *token)
{
    if (!token) {
        return;
    }
    ascii_buffer_reset(&token->ascii);
    iterm_screen_char_array_reset(&token->screen_chars);
    free_subtokens(token->subtokens);
    token->subtokens = NULL;
    free(token->saved_data);
    token->saved_data = NULL;
    token->saved_length = 0;
    free(token->kvp_key);
    free(token->kvp_value);
    token->kvp_key = NULL;
    token->kvp_value = NULL;
    token->crlf_count = 0;
}

void
iterm_token_set_ascii(iterm_token *token, const uint8_t *data, size_t length)
{
    if (!token || !data) {
        return;
    }
    ascii_buffer_reset(&token->ascii);
    if (length > sizeof(token->ascii.static_buffer)) {
        token->ascii.buffer = malloc(length);
    } else {
        token->ascii.buffer = token->ascii.static_buffer;
    }
    if (!token->ascii.buffer) {
        token->ascii.length = 0;
        return;
    }
    memcpy(token->ascii.buffer, data, length);
    token->ascii.length = length;
}

void
iterm_token_set_saved_data(iterm_token *token, const uint8_t *data, size_t length)
{
    if (!token) {
        return;
    }
    free(token->saved_data);
    token->saved_data = NULL;
    token->saved_length = 0;
    if (!data || !length) {
        return;
    }
    token->saved_data = malloc(length);
    if (!token->saved_data) {
        return;
    }
    memcpy(token->saved_data, data, length);
    token->saved_length = length;
}

void
iterm_token_add_subtoken(iterm_token *token, const iterm_token *subtoken)
{
    if (!token || !subtoken) {
        return;
    }
    iterm_token_subtoken *node = malloc(sizeof(*node));
    if (!node) {
        return;
    }
    iterm_token_init(&node->token);
    iterm_token_copy(&node->token, subtoken);
    node->next = token->subtokens;
    token->subtokens = node;
}

void
iterm_token_set_kvp(iterm_token *token, const char *key, const char *value)
{
    if (!token) {
        return;
    }
    free(token->kvp_key);
    free(token->kvp_value);
    token->kvp_key = NULL;
    token->kvp_value = NULL;
    if (key) {
        token->kvp_key = strdup(key);
    }
    if (value) {
        token->kvp_value = strdup(value);
    }
}

void
iterm_token_copy(iterm_token *dest, const iterm_token *src)
{
    if (!dest || !src) {
        return;
    }
    iterm_token_reset(dest);
    dest->type = src->type;
    dest->code = src->code;
    dest->csi = src->csi;
    if (src->ascii.length) {
        iterm_token_set_ascii(dest, src->ascii.buffer, src->ascii.length);
    }
    copy_screen_chars(&dest->screen_chars, &src->screen_chars);
    if (src->saved_length) {
        iterm_token_set_saved_data(dest, src->saved_data, src->saved_length);
    }
    iterm_token_set_kvp(dest, src->kvp_key, src->kvp_value);
    dest->crlf_count = src->crlf_count;
    for (iterm_token_subtoken *sub = src->subtokens; sub; sub = sub->next) {
        iterm_token_add_subtoken(dest, &sub->token);
    }
}

