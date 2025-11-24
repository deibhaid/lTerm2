#pragma once

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    int length;
    int consumed;
} iterm_parser_context;

static inline iterm_parser_context
iterm_parser_context_make(uint8_t *data, int length)
{
    iterm_parser_context ctx = {
        .data = data,
        .length = length,
        .consumed = 0,
    };
    return ctx;
}

static inline int
iterm_parser_can_advance(const iterm_parser_context *ctx)
{
    return ctx->length > 0;
}

static inline uint8_t
iterm_parser_peek(const iterm_parser_context *ctx)
{
    return ctx->data[0];
}

static inline int
iterm_parser_try_peek(const iterm_parser_context *ctx, uint8_t *out)
{
    if (!iterm_parser_can_advance(ctx)) {
        return 0;
    }
    if (out) {
        *out = iterm_parser_peek(ctx);
    }
    return 1;
}

static inline void
iterm_parser_advance(iterm_parser_context *ctx)
{
    ctx->data++;
    ctx->length--;
    ctx->consumed++;
}

static inline void
iterm_parser_advance_multiple(iterm_parser_context *ctx, int count)
{
    ctx->data += count;
    ctx->length -= count;
    ctx->consumed += count;
}

static inline int
iterm_parser_try_advance(iterm_parser_context *ctx)
{
    if (!iterm_parser_can_advance(ctx)) {
        return 0;
    }
    iterm_parser_advance(ctx);
    return 1;
}

static inline int
iterm_parser_bytes_consumed(const iterm_parser_context *ctx)
{
    return ctx->consumed;
}

static inline uint8_t
iterm_parser_consume(iterm_parser_context *ctx)
{
    uint8_t value = ctx->data[0];
    iterm_parser_advance(ctx);
    return value;
}

static inline int
iterm_parser_try_consume(iterm_parser_context *ctx, uint8_t *out)
{
    if (!iterm_parser_can_advance(ctx)) {
        return 0;
    }
    if (out) {
        *out = iterm_parser_consume(ctx);
    } else {
        iterm_parser_consume(ctx);
    }
    return 1;
}

static inline void
iterm_parser_backtrack_by(iterm_parser_context *ctx, int count)
{
    ctx->data -= count;
    ctx->length += count;
    ctx->consumed -= count;
}

static inline void
iterm_parser_backtrack(iterm_parser_context *ctx)
{
    iterm_parser_backtrack_by(ctx, ctx->consumed);
}

static inline int
iterm_parser_bytes_until(iterm_parser_context *ctx, uint8_t target)
{
    uint8_t *ptr = memchr(ctx->data, target, ctx->length);
    if (!ptr) {
        return -1;
    }
    return (int)(ptr - ctx->data);
}

static inline int
iterm_parser_length(const iterm_parser_context *ctx)
{
    return ctx->length;
}

static inline uint8_t *
iterm_parser_peek_bytes(const iterm_parser_context *ctx, int length)
{
    if (ctx->length < length) {
        return NULL;
    }
    return ctx->data;
}

static inline int
iterm_parser_consume_integer(iterm_parser_context *ctx, int *out, int *overflow)
{
    int digits = 0;
    int value = 0;
    int local_overflow = 0;
    while (iterm_parser_can_advance(ctx)) {
        uint8_t c = iterm_parser_peek(ctx);
        if (!isdigit(c)) {
            break;
        }
        digits++;
        if (value > (INT_MAX - 10) / 10) {
            local_overflow = 1;
        } else {
            value *= 10;
            value += (c - '0');
        }
        iterm_parser_advance(ctx);
    }
    if (out) {
        *out = value;
    }
    if (overflow) {
        *overflow = local_overflow;
    }
    return digits > 0;
}

#ifdef __cplusplus
}
#endif

