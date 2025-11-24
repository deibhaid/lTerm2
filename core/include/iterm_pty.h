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
} iterm_pty;

void iterm_pty_init(iterm_pty *pty);
bool iterm_pty_spawn(iterm_pty *pty, const char *program, char *const argv[], char *const envp[]);
bool iterm_pty_spawn_shell(iterm_pty *pty, const char *shell_path);
ssize_t iterm_pty_read(iterm_pty *pty, uint8_t *buffer, size_t length);
ssize_t iterm_pty_write(iterm_pty *pty, const uint8_t *data, size_t length);
int iterm_pty_get_fd(const iterm_pty *pty);
bool iterm_pty_is_active(const iterm_pty *pty);
pid_t iterm_pty_child_pid(const iterm_pty *pty);
void iterm_pty_close(iterm_pty *pty);

#ifdef __cplusplus
}
#endif


