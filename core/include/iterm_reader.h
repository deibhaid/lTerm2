#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t length;
} iterm_reader_cursor;

typedef struct {
    uint8_t *buffer;
    size_t length;
    size_t capacity;
    size_t offset;
} iterm_reader;

typedef struct {
    iterm_reader_cursor cursor;
    size_t consumed;
} iterm_reader_consumer;

void iterm_reader_init(iterm_reader *reader);
void iterm_reader_free(iterm_reader *reader);
void iterm_reader_reset(iterm_reader *reader);
void iterm_reader_append(iterm_reader *reader, const uint8_t *data, size_t length);

void iterm_reader_cursor_init(iterm_reader_cursor *cursor, const iterm_reader *reader);
size_t iterm_reader_cursor_size(const iterm_reader_cursor *cursor);
uint8_t iterm_reader_cursor_peek(const iterm_reader_cursor *cursor);
uint8_t iterm_reader_cursor_peek_offset(const iterm_reader_cursor *cursor, size_t offset);
void iterm_reader_cursor_advance(iterm_reader_cursor *cursor, size_t count);
const uint8_t *iterm_reader_cursor_ptr(const iterm_reader_cursor *cursor);
void iterm_reader_consume(iterm_reader *reader, size_t count);

void iterm_reader_consumer_init(iterm_reader_consumer *consumer, const iterm_reader_cursor *cursor);
void iterm_reader_consumer_reset(iterm_reader_consumer *consumer);
uint8_t iterm_reader_consumer_peek(iterm_reader_consumer *consumer);
int iterm_reader_consumer_try_peek(iterm_reader_consumer *consumer, uint8_t *out);
void iterm_reader_consumer_advance(iterm_reader_consumer *consumer, size_t count);
size_t iterm_reader_consumer_consumed(const iterm_reader_consumer *consumer);
size_t iterm_reader_consumer_remaining(const iterm_reader_consumer *consumer);

#ifdef __cplusplus
}
#endif

