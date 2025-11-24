#include "lterm_pty.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <util.h>
#else
#include <pty.h>
#endif

extern char **environ;

static void
set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void
lterm_pty_init(lterm_pty *pty)
{
    if (!pty) {
        return;
    }
    pty->master_fd = -1;
    pty->child_pid = -1;
}

bool
lterm_pty_spawn(lterm_pty *pty, const char *program, char *const argv[], char *const envp[])
{
    if (!pty || !program) {
        return false;
    }

    int master_fd = -1;
    int slave_fd = -1;
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) != 0) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(master_fd);
        close(slave_fd);
        return false;
    }

    if (pid == 0) {
        setsid();
#ifdef TIOCSCTTY
        ioctl(slave_fd, TIOCSCTTY, 0);
#endif
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > STDERR_FILENO) {
            close(slave_fd);
        }
        close(master_fd);
        char *const *env = envp ? envp : environ;
        execve(program, argv ? argv : (char *const[]){(char *)program, NULL}, env);
        _exit(EXIT_FAILURE);
    }

    close(slave_fd);
    set_nonblock(master_fd);
    pty->master_fd = master_fd;
    pty->child_pid = pid;
    return true;
}

bool
lterm_pty_spawn_shell(lterm_pty *pty, const char *shell_path)
{
    if (!pty) {
        return false;
    }
    const char *shell = (shell_path && shell_path[0]) ? shell_path : getenv("SHELL");
    if (!shell || !shell[0]) {
        shell = "/bin/bash";
    }
    char *const argv[] = {(char *)shell, "-l", NULL};
    return lterm_pty_spawn(pty, shell, argv, NULL);
}

ssize_t
lterm_pty_read(lterm_pty *pty, uint8_t *buffer, size_t length)
{
    if (!pty || pty->master_fd < 0 || !buffer || length == 0) {
        errno = EINVAL;
        return -1;
    }
    ssize_t n = read(pty->master_fd, buffer, length);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return n;
}

ssize_t
lterm_pty_write(lterm_pty *pty, const uint8_t *data, size_t length)
{
    if (!pty || pty->master_fd < 0 || !data || length == 0) {
        errno = EINVAL;
        return -1;
    }
    ssize_t n = write(pty->master_fd, data, length);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return n;
}

int
lterm_pty_get_fd(const lterm_pty *pty)
{
    return (pty && pty->master_fd >= 0) ? pty->master_fd : -1;
}

pid_t
lterm_pty_child_pid(const lterm_pty *pty)
{
    return pty ? pty->child_pid : -1;
}

bool
lterm_pty_is_active(const lterm_pty *pty)
{
    return pty && pty->master_fd >= 0;
}

void
lterm_pty_close(lterm_pty *pty)
{
    if (!pty) {
        return;
    }
    if (pty->master_fd >= 0) {
        close(pty->master_fd);
        pty->master_fd = -1;
    }
    if (pty->child_pid > 0) {
        int status = 0;
        waitpid(pty->child_pid, &status, WNOHANG);
        pty->child_pid = -1;
    }
}


