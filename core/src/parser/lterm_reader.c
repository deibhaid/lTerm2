#include "lterm_reader.h"

#include <stdlib.h>
#include <string.h>

#define READER_DEFAULT_CAPACITY 65536

void
lterm_reader_init(lterm_reader *reader)
{
    if (!reader) {
        return;
    }
    reader->capacity = READER_DEFAULT_CAPACITY;
    reader->buffer = malloc(reader->capacity);
    reader->length = 0;
    reader->offset = 0;
}

void
lterm_reader_free(lterm_reader *reader)
{
    if (!reader) {
        return;
    }
    free(reader->buffer);
    reader->buffer = NULL;
    reader->capacity = 0;
    reader->length = 0;
    reader->offset = 0;
}

void
lterm_reader_reset(lterm_reader *reader)
{
    if (!reader) {
        return;
    }
    reader->length = 0;
    reader->offset = 0;
    if (reader->capacity > READER_DEFAULT_CAPACITY * 2) {
        free(reader->buffer);
        reader->capacity = READER_DEFAULT_CAPACITY;
        reader->buffer = malloc(reader->capacity);
    }
}

static void
lterm_reader_compact(lterm_reader *reader)
{
    if (!reader || reader->offset == 0) {
        return;
    }
    size_t remaining = reader->length - reader->offset;
    memmove(reader->buffer, reader->buffer + reader->offset, remaining);
    reader->length = remaining;
    reader->offset = 0;
}

void
lterm_reader_append(lterm_reader *reader, const uint8_t *data, size_t length)
{
    if (!reader || !data || !length) {
        return;
    }
    size_t remaining_space = reader->capacity - reader->length;
    if (remaining_space < length) {
        lterm_reader_compact(reader);
        remaining_space = reader->capacity - reader->length;
    }
    if (remaining_space < length) {
        size_t new_capacity = reader->capacity;
        while (new_capacity - reader->length < length) {
            new_capacity += READER_DEFAULT_CAPACITY;
        }
        uint8_t *new_buffer = realloc(reader->buffer, new_capacity);
        if (!new_buffer) {
            return;
        }
        reader->buffer = new_buffer;
        reader->capacity = new_capacity;
    }
    memcpy(reader->buffer + reader->length, data, length);
    reader->length += length;
}

void
lterm_reader_cursor_init(lterm_reader_cursor *cursor, const lterm_reader *reader)
{
    if (!cursor || !reader) {
        return;
    }
    cursor->data = reader->buffer + reader->offset;
    cursor->length = reader->length - reader->offset;
}

size_t
lterm_reader_cursor_size(const lterm_reader_cursor *cursor)
{
    return cursor ? cursor->length : 0;
}

uint8_t
lterm_reader_cursor_peek(const lterm_reader_cursor *cursor)
{
    return (cursor && cursor->length) ? cursor->data[0] : 0;
}

uint8_t
lterm_reader_cursor_peek_offset(const lterm_reader_cursor *cursor, size_t offset)
{
    if (!cursor || offset >= cursor->length) {
        return 0;
    }
    return cursor->data[offset];
}

void
lterm_reader_cursor_advance(lterm_reader_cursor *cursor, size_t count)
{
    if (!cursor || count > cursor->length) {
        return;
    }
    cursor->data += count;
    cursor->length -= count;
}

const uint8_t *
lterm_reader_cursor_ptr(const lterm_reader_cursor *cursor)
{
    return cursor ? cursor->data : NULL;
}

void
lterm_reader_consume(lterm_reader *reader, size_t count)
{
    if (!reader || count > reader->length - reader->offset) {
        return;
    }
    reader->offset += count;
}

void
lterm_reader_consumer_init(lterm_reader_consumer *consumer, const lterm_reader_cursor *cursor)
{
    if (!consumer || !cursor) {
        return;
    }
    consumer->cursor = *cursor;
    consumer->consumed = 0;
}

void
lterm_reader_consumer_reset(lterm_reader_consumer *consumer)
{
    if (!consumer) {
        return;
    }
    consumer->consumed = 0;
}

uint8_t
lterm_reader_consumer_peek(lterm_reader_consumer *consumer)
{
    return consumer ? lterm_reader_cursor_peek(&consumer->cursor) : 0;
}

int
lterm_reader_consumer_try_peek(lterm_reader_consumer *consumer, uint8_t *out)
{
    if (!consumer || lterm_reader_cursor_size(&consumer->cursor) == 0) {
        return 0;
    }
    if (out) {
        *out = consumer->cursor.data[0];
    }
    return 1;
}

void
lterm_reader_consumer_advance(lterm_reader_consumer *consumer, size_t count)
{
    if (!consumer || count > consumer->cursor.length) {
        return;
    }
    lterm_reader_cursor_advance(&consumer->cursor, count);
    consumer->consumed += count;
}

size_t
lterm_reader_consumer_consumed(const lterm_reader_consumer *consumer)
{
    return consumer ? consumer->consumed : 0;
}

size_t
lterm_reader_consumer_remaining(const lterm_reader_consumer *consumer)
{
    return consumer ? consumer->cursor.length : 0;
}

