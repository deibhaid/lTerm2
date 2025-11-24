#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "iterm_csi_param.h"
#include "iterm_token_types.h"
#include "iterm_screen_char.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITERM_TOKEN_STATIC_SCREEN_CHARS 128

typedef struct {
    uint8_t *buffer;
    size_t length;
    uint8_t static_buffer[ITERM_ASCII_STATIC];
} iterm_ascii_buffer;

typedef struct iterm_token_subtoken iterm_token_subtoken;

typedef struct iterm_token {
    iterm_token_type type;
    int code;
    iterm_ascii_buffer ascii;
    iterm_csi_param csi;
    iterm_screen_char_array screen_chars;
    iterm_token_subtoken *subtokens;
    uint8_t *saved_data;
    size_t saved_length;
    char *kvp_key;
    char *kvp_value;
    int crlf_count;
} iterm_token;

struct iterm_token_subtoken {
    iterm_token_subtoken *next;
    iterm_token token;
};

void iterm_token_init(iterm_token *token);
void iterm_token_reset(iterm_token *token);
void iterm_token_free(iterm_token *token);

void iterm_token_set_ascii(iterm_token *token, const uint8_t *data, size_t length);
void iterm_token_set_saved_data(iterm_token *token, const uint8_t *data, size_t length);
void iterm_token_add_subtoken(iterm_token *token, const iterm_token *subtoken);
void iterm_token_set_kvp(iterm_token *token, const char *key, const char *value);
void iterm_token_copy(iterm_token *dest, const iterm_token *src);

#ifdef __cplusplus
}
#endif

