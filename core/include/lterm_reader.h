#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t length;
} lterm_reader_cursor;

typedef struct {
    uint8_t *buffer;
    size_t length;
    size_t capacity;
    size_t offset;
} lterm_reader;

typedef struct {
    lterm_reader_cursor cursor;
    size_t consumed;
} lterm_reader_consumer;

void lterm_reader_init(lterm_reader *reader);
void lterm_reader_free(lterm_reader *reader);
void lterm_reader_reset(lterm_reader *reader);
void lterm_reader_append(lterm_reader *reader, const uint8_t *data, size_t length);

void lterm_reader_cursor_init(lterm_reader_cursor *cursor, const lterm_reader *reader);
size_t lterm_reader_cursor_size(const lterm_reader_cursor *cursor);
uint8_t lterm_reader_cursor_peek(const lterm_reader_cursor *cursor);
uint8_t lterm_reader_cursor_peek_offset(const lterm_reader_cursor *cursor, size_t offset);
void lterm_reader_cursor_advance(lterm_reader_cursor *cursor, size_t count);
const uint8_t *lterm_reader_cursor_ptr(const lterm_reader_cursor *cursor);
void lterm_reader_consume(lterm_reader *reader, size_t count);

void lterm_reader_consumer_init(lterm_reader_consumer *consumer, const lterm_reader_cursor *cursor);
void lterm_reader_consumer_reset(lterm_reader_consumer *consumer);
uint8_t lterm_reader_consumer_peek(lterm_reader_consumer *consumer);
int lterm_reader_consumer_try_peek(lterm_reader_consumer *consumer, uint8_t *out);
void lterm_reader_consumer_advance(lterm_reader_consumer *consumer, size_t count);
size_t lterm_reader_consumer_consumed(const lterm_reader_consumer *consumer);
size_t lterm_reader_consumer_remaining(const lterm_reader_consumer *consumer);

#ifdef __cplusplus
}
#endif

