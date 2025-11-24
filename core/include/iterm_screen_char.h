#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ITERM_ASCII_STATIC
#define ITERM_ASCII_STATIC 128
#endif

typedef struct {
    uint32_t code;
    uint8_t complex;
    uint8_t bold;
    uint8_t faint;
    uint8_t italic;
    uint8_t blink;
    uint8_t underline;
    uint8_t undercurl;
    uint8_t double_width;
    uint8_t double_height;
    uint8_t invisible;
    uint8_t strikethrough;
    uint8_t image;
} iterm_screen_char;

typedef struct {
    iterm_screen_char *buffer;
    size_t length;
    size_t capacity;
    iterm_screen_char static_buffer[ITERM_ASCII_STATIC];
} iterm_screen_char_array;

static inline void
iterm_screen_char_array_init(iterm_screen_char_array *array)
{
    if (!array) {
        return;
    }
    array->buffer = array->static_buffer;
    array->length = 0;
    array->capacity = sizeof(array->static_buffer) / sizeof(array->static_buffer[0]);
    memset(array->static_buffer, 0, sizeof(array->static_buffer));
}

static inline void
iterm_screen_char_array_reset(iterm_screen_char_array *array)
{
    if (!array) {
        return;
    }
    if (array->buffer && array->buffer != array->static_buffer) {
        free(array->buffer);
    }
    array->buffer = array->static_buffer;
    array->length = 0;
    array->capacity = sizeof(array->static_buffer) / sizeof(array->static_buffer[0]);
    memset(array->static_buffer, 0, sizeof(array->static_buffer));
}

static inline void
iterm_screen_char_array_append(iterm_screen_char_array *array, const iterm_screen_char *ch)
{
    if (!array || !ch) {
        return;
    }
    if (array->length == array->capacity) {
        size_t new_capacity = array->capacity * 2;
        iterm_screen_char *new_buffer = malloc(new_capacity * sizeof(iterm_screen_char));
        if (!new_buffer) {
            return;
        }
        memcpy(new_buffer, array->buffer, array->length * sizeof(iterm_screen_char));
        if (array->buffer != array->static_buffer) {
            free(array->buffer);
        }
        array->buffer = new_buffer;
        array->capacity = new_capacity;
    }
    array->buffer[array->length++] = *ch;
}

#ifdef __cplusplus
}
#endif

