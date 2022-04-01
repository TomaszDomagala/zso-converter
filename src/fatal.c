#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _sysfatal(const char *file, int line, const char *func, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr, "%s:%d: ", file, line);
    fprintf(stderr, "%s: %s\n", func, strerror(errno));
    fprintf(stderr, "errno: %d\n", errno);
    exit(errno);
}

void _fatal(const char *file, int line, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "%s:%d: ", file, line);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    exit(1);
}

void _warn(const char *file, int line, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}
