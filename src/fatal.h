#ifndef FATAL_H
#define FATAL_H

void _fatal(const char * func, const char *file, int line, const char *format, ...) __attribute__((noreturn));
void _sysfatal(const char * func, const char *file, int line, const char *sys_func, const char *format, ...) __attribute__((noreturn));
void _warn(const char * func, const char *file, int line, const char *format, ...);

#define fatalf(format, ...) _fatal(__PRETTY_FUNCTION__, __FILE__, __LINE__, format, __VA_ARGS__)
#define fatal(format) _fatal(__PRETTY_FUNCTION__, __FILE__, __LINE__, format)
#define sysfatalf(func, format, ...) _sysfatal(__PRETTY_FUNCTION__, __FILE__, __LINE__, func, format, __VA_ARGS__)
#define sysfatal(func, format) _sysfatal(__PRETTY_FUNCTION__, __FILE__, __LINE__, func, format)
#define warnf(format, ...) _warn(__PRETTY_FUNCTION__, __FILE__, __LINE__, format, __VA_ARGS__)
#define warn(format) _warn(__PRETTY_FUNCTION__, __FILE__, __LINE__, format)

#endif
