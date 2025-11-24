#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "iterm_core.h"
#include "iterm_parser.h"

typedef struct {
    iterm_token tokens[32];
    uint8_t storage[32][128];
    size_t count;
} token_log;

static void
log_token(const iterm_token *token, void *user_data)
{
    token_log *log = user_data;
    if (!token || log->count >= 32) {
        return;
    }
    log->tokens[log->count] = *token;
    size_t to_copy = token->length > sizeof(log->storage[0])
                         ? sizeof(log->storage[0])
                         : token->length;
    memcpy(log->storage[log->count], token->data, to_copy);
    log->tokens[log->count].data = log->storage[log->count];
    log->tokens[log->count].length = to_copy;
    log->count++;
}

static void
test_ascii_and_csi(void)
{
    iterm_parser *parser = iterm_parser_new();
    assert(parser);

    const uint8_t sample[] = "abc\x1b[31mdef";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);

    assert(log.count == 3);
    assert(log.tokens[0].type == ITERM_TOKEN_ASCII);
    assert(log.tokens[0].length == 3);
    assert(memcmp(log.tokens[0].data, "abc", 3) == 0);

    assert(log.tokens[1].type == ITERM_TOKEN_CSI);
    assert(memcmp(log.tokens[1].data, "\x1b[31m", 5) == 0);

    assert(log.tokens[2].type == ITERM_TOKEN_ASCII);
    assert(memcmp(log.tokens[2].data, "def", 3) == 0);

    iterm_parser_free(parser);
}

static void
test_osc_termination(void)
{
    iterm_parser *parser = iterm_parser_new();
    assert(parser);
    const uint8_t sample[] = "\x1b]0;hello world\x07";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);
    assert(log.count == 1);
    assert(log.tokens[0].type == ITERM_TOKEN_OSC);
    assert(memcmp(log.tokens[0].data, sample, sizeof(sample) - 1) == 0);
    iterm_parser_free(parser);
}

static void
test_osc_st_termination(void)
{
    iterm_parser *parser = iterm_parser_new();
    assert(parser);
    const uint8_t sample[] = "\x1b]1;title\x1b\\";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);
    assert(log.count == 1);
    assert(log.tokens[0].type == ITERM_TOKEN_OSC);
    assert(memcmp(log.tokens[0].data, sample, sizeof(sample) - 1) == 0);
    iterm_parser_free(parser);
}

int
main(void)
{
    const iterm_core_version *version = iterm_core_get_version();
    assert(version != NULL);

    test_ascii_and_csi();
    test_osc_termination();
    test_osc_st_termination();
    printf("parser tests passed\n");
    return 0;
}

