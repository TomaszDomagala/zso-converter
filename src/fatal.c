#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _sysfatal(const char *func, const char *file, int line, const char *sys_func, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr, "system fatal error %s:%d in %s:\n", file, line, func);
    fprintf(stderr, "%s: %s\n", sys_func, strerror(errno));
    fprintf(stderr, "errno: %d\n", errno);
    exit(errno);
}

void _fatal(const char *func, const char *file, int line, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "fatal error %s:%d in %s:\n", file, line, func);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    exit(1);
}

void _warn(const char *func, const char *file, int line, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    fprintf(stderr, "warning %s:%d in %s:\n", file, line, func);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}
