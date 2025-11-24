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
} lterm_parser_context;

static inline lterm_parser_context
lterm_parser_context_make(uint8_t *data, int length)
{
    lterm_parser_context ctx = {
        .data = data,
        .length = length,
        .consumed = 0,
    };
    return ctx;
}

static inline int
lterm_parser_can_advance(const lterm_parser_context *ctx)
{
    return ctx->length > 0;
}

static inline uint8_t
lterm_parser_peek(const lterm_parser_context *ctx)
{
    return ctx->data[0];
}

static inline int
lterm_parser_try_peek(const lterm_parser_context *ctx, uint8_t *out)
{
    if (!lterm_parser_can_advance(ctx)) {
        return 0;
    }
    if (out) {
        *out = lterm_parser_peek(ctx);
    }
    return 1;
}

static inline void
lterm_parser_advance(lterm_parser_context *ctx)
{
    ctx->data++;
    ctx->length--;
    ctx->consumed++;
}

static inline void
lterm_parser_advance_multiple(lterm_parser_context *ctx, int count)
{
    ctx->data += count;
    ctx->length -= count;
    ctx->consumed += count;
}

static inline int
lterm_parser_try_advance(lterm_parser_context *ctx)
{
    if (!lterm_parser_can_advance(ctx)) {
        return 0;
    }
    lterm_parser_advance(ctx);
    return 1;
}

static inline int
lterm_parser_bytes_consumed(const lterm_parser_context *ctx)
{
    return ctx->consumed;
}

static inline uint8_t
lterm_parser_consume(lterm_parser_context *ctx)
{
    uint8_t value = ctx->data[0];
    lterm_parser_advance(ctx);
    return value;
}

static inline int
lterm_parser_try_consume(lterm_parser_context *ctx, uint8_t *out)
{
    if (!lterm_parser_can_advance(ctx)) {
        return 0;
    }
    if (out) {
        *out = lterm_parser_consume(ctx);
    } else {
        lterm_parser_consume(ctx);
    }
    return 1;
}

static inline void
lterm_parser_backtrack_by(lterm_parser_context *ctx, int count)
{
    ctx->data -= count;
    ctx->length += count;
    ctx->consumed -= count;
}

static inline void
lterm_parser_backtrack(lterm_parser_context *ctx)
{
    lterm_parser_backtrack_by(ctx, ctx->consumed);
}

static inline int
lterm_parser_bytes_until(lterm_parser_context *ctx, uint8_t target)
{
    uint8_t *ptr = memchr(ctx->data, target, ctx->length);
    if (!ptr) {
        return -1;
    }
    return (int)(ptr - ctx->data);
}

static inline int
lterm_parser_length(const lterm_parser_context *ctx)
{
    return ctx->length;
}

static inline uint8_t *
lterm_parser_peek_bytes(const lterm_parser_context *ctx, int length)
{
    if (ctx->length < length) {
        return NULL;
    }
    return ctx->data;
}

static inline int
lterm_parser_consume_integer(lterm_parser_context *ctx, int *out, int *overflow)
{
    int digits = 0;
    int value = 0;
    int local_overflow = 0;
    while (lterm_parser_can_advance(ctx)) {
        uint8_t c = lterm_parser_peek(ctx);
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
        lterm_parser_advance(ctx);
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

