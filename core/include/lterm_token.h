#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "lterm_csi_param.h"
#include "lterm_token_types.h"
#include "lterm_screen_char.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LTERM_TOKEN_STATIC_SCREEN_CHARS 128

typedef struct {
    uint8_t *buffer;
    size_t length;
    uint8_t static_buffer[LTERM_ASCII_STATIC];
} lterm_ascii_buffer;

typedef struct lterm_token_subtoken lterm_token_subtoken;

typedef struct lterm_token {
    lterm_token_type type;
    int code;
    lterm_ascii_buffer ascii;
    lterm_csi_param csi;
    lterm_screen_char_array screen_chars;
    lterm_token_subtoken *subtokens;
    uint8_t *saved_data;
    size_t saved_length;
    char *kvp_key;
    char *kvp_value;
    int crlf_count;
} lterm_token;

struct lterm_token_subtoken {
    lterm_token_subtoken *next;
    lterm_token token;
};

void lterm_token_init(lterm_token *token);
void lterm_token_reset(lterm_token *token);
void lterm_token_free(lterm_token *token);

void lterm_token_set_ascii(lterm_token *token, const uint8_t *data, size_t length);
void lterm_token_set_saved_data(lterm_token *token, const uint8_t *data, size_t length);
void lterm_token_add_subtoken(lterm_token *token, const lterm_token *subtoken);
void lterm_token_set_kvp(lterm_token *token, const char *key, const char *value);
void lterm_token_copy(lterm_token *dest, const lterm_token *src);

#ifdef __cplusplus
}
#endif

