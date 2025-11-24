#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int master_fd;
    pid_t child_pid;
} lterm_pty;

void lterm_pty_init(lterm_pty *pty);
bool lterm_pty_spawn(lterm_pty *pty, const char *program, char *const argv[], char *const envp[]);
bool lterm_pty_spawn_shell(lterm_pty *pty, const char *shell_path);
bool lterm_pty_resize(lterm_pty *pty, size_t rows, size_t cols);
ssize_t lterm_pty_read(lterm_pty *pty, uint8_t *buffer, size_t length);
ssize_t lterm_pty_write(lterm_pty *pty, const uint8_t *data, size_t length);
int lterm_pty_get_fd(const lterm_pty *pty);
bool lterm_pty_is_active(const lterm_pty *pty);
pid_t lterm_pty_child_pid(const lterm_pty *pty);
void lterm_pty_close(lterm_pty *pty);

#ifdef __cplusplus
}
#endif


