#include <sys/types.h>

void _exit(int status) {
    while (1) {}  // Trap in an infinite loop
}

void *_sbrk(ptrdiff_t incr) {
    return (void *)-1;  // No heap, return error
}

int _write(int file, char *ptr, int len) {
    return len;  // Stub implementation, assume success
}

int _close(int file) {
    return -1;  // Not implemented
}

int _fstat(int file, struct stat *st) {
    return 0;  // Not implemented, return success
}

int _isatty(int file) {
    return 1;  // Pretend it's a terminal
}

int _lseek(int file, int ptr, int dir) {
    return 0;  // Not implemented
}

int _read(int file, char *ptr, int len) {
    return 0;  // Not implemented
}

int _kill(int pid, int sig) {
    return -1;  // No process control
}

int _getpid(void) {
    return 1;  // Stub process ID
}
