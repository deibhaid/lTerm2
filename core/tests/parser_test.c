#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "iterm_core.h"
#include "iterm_parser.h"

typedef struct {
    iterm_token tokens[32];
    size_t count;
} token_log;

static void
log_token(const iterm_token *token, void *user_data)
{
    token_log *log = user_data;
    if (!token || log->count >= 32) {
        return;
    }
    iterm_token *dest = &log->tokens[log->count];
    iterm_token_init(dest);
    iterm_token_copy(dest, token);
    log->count++;
}

static void
reset_log(token_log *log)
{
    for (size_t i = 0; i < log->count; ++i) {
        iterm_token_free(&log->tokens[i]);
    }
    memset(log, 0, sizeof(*log));
}

static void
test_ascii_and_csi(void)
{
    iterm_parser *parser = iterm_parser_new(NULL);
    assert(parser);

    const uint8_t sample[] = "abc\x1b[31mdef";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);

    assert(log.count == 3);
    assert(log.tokens[0].type == ITERM_TOKEN_ASCII);
    assert(log.tokens[0].ascii.length == 3);
    assert(memcmp(log.tokens[0].ascii.buffer, "abc", 3) == 0);

    assert(log.tokens[1].type == ITERM_TOKEN_CSI);
    assert(log.tokens[1].code == 'm');

    assert(log.tokens[2].type == ITERM_TOKEN_ASCII);
    assert(log.tokens[2].ascii.length == 3);
    assert(memcmp(log.tokens[2].ascii.buffer, "def", 3) == 0);

    reset_log(&log);
    iterm_parser_free(parser);
}

static void
test_osc_termination(void)
{
    iterm_parser *parser = iterm_parser_new(NULL);
    assert(parser);
    const uint8_t sample[] = "\x1b]0;hello world\x07";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);
    assert(log.count == 1);
    assert(log.tokens[0].type == ITERM_TOKEN_OSC);
    assert(log.tokens[0].ascii.length == strlen("hello world"));
    assert(memcmp(log.tokens[0].ascii.buffer, "hello world", log.tokens[0].ascii.length) == 0);
    reset_log(&log);
    iterm_parser_free(parser);
}

static void
test_osc_st_termination(void)
{
    iterm_parser *parser = iterm_parser_new(NULL);
    assert(parser);
    const uint8_t sample[] = "\x1b]1;title\x1b\\";
    token_log log = {0};
    iterm_parser_feed(parser, sample, sizeof(sample) - 1, log_token, &log);
    assert(log.count == 1);
    assert(log.tokens[0].type == ITERM_TOKEN_OSC);
    assert(log.tokens[0].ascii.length == strlen("title"));
    assert(memcmp(log.tokens[0].ascii.buffer, "title", log.tokens[0].ascii.length) == 0);
    reset_log(&log);
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

