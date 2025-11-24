#include "iterm_reader.h"

#include <stdlib.h>
#include <string.h>

#define READER_DEFAULT_CAPACITY 65536

void
iterm_reader_init(iterm_reader *reader)
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
iterm_reader_free(iterm_reader *reader)
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
iterm_reader_reset(iterm_reader *reader)
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
iterm_reader_compact(iterm_reader *reader)
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
iterm_reader_append(iterm_reader *reader, const uint8_t *data, size_t length)
{
    if (!reader || !data || !length) {
        return;
    }
    size_t remaining_space = reader->capacity - reader->length;
    if (remaining_space < length) {
        iterm_reader_compact(reader);
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
iterm_reader_cursor_init(iterm_reader_cursor *cursor, const iterm_reader *reader)
{
    if (!cursor || !reader) {
        return;
    }
    cursor->data = reader->buffer + reader->offset;
    cursor->length = reader->length - reader->offset;
}

size_t
iterm_reader_cursor_size(const iterm_reader_cursor *cursor)
{
    return cursor ? cursor->length : 0;
}

uint8_t
iterm_reader_cursor_peek(const iterm_reader_cursor *cursor)
{
    return (cursor && cursor->length) ? cursor->data[0] : 0;
}

uint8_t
iterm_reader_cursor_peek_offset(const iterm_reader_cursor *cursor, size_t offset)
{
    if (!cursor || offset >= cursor->length) {
        return 0;
    }
    return cursor->data[offset];
}

void
iterm_reader_cursor_advance(iterm_reader_cursor *cursor, size_t count)
{
    if (!cursor || count > cursor->length) {
        return;
    }
    cursor->data += count;
    cursor->length -= count;
}

const uint8_t *
iterm_reader_cursor_ptr(const iterm_reader_cursor *cursor)
{
    return cursor ? cursor->data : NULL;
}

void
iterm_reader_consume(iterm_reader *reader, size_t count)
{
    if (!reader || count > reader->length - reader->offset) {
        return;
    }
    reader->offset += count;
}

void
iterm_reader_consumer_init(iterm_reader_consumer *consumer, const iterm_reader_cursor *cursor)
{
    if (!consumer || !cursor) {
        return;
    }
    consumer->cursor = *cursor;
    consumer->consumed = 0;
}

void
iterm_reader_consumer_reset(iterm_reader_consumer *consumer)
{
    if (!consumer) {
        return;
    }
    consumer->consumed = 0;
}

uint8_t
iterm_reader_consumer_peek(iterm_reader_consumer *consumer)
{
    return consumer ? iterm_reader_cursor_peek(&consumer->cursor) : 0;
}

int
iterm_reader_consumer_try_peek(iterm_reader_consumer *consumer, uint8_t *out)
{
    if (!consumer || iterm_reader_cursor_size(&consumer->cursor) == 0) {
        return 0;
    }
    if (out) {
        *out = consumer->cursor.data[0];
    }
    return 1;
}

void
iterm_reader_consumer_advance(iterm_reader_consumer *consumer, size_t count)
{
    if (!consumer || count > consumer->cursor.length) {
        return;
    }
    iterm_reader_cursor_advance(&consumer->cursor, count);
    consumer->consumed += count;
}

size_t
iterm_reader_consumer_consumed(const iterm_reader_consumer *consumer)
{
    return consumer ? consumer->consumed : 0;
}

size_t
iterm_reader_consumer_remaining(const iterm_reader_consumer *consumer)
{
    return consumer ? consumer->cursor.length : 0;
}

