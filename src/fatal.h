void _fatal(const * file, int line, const char *format, ...) __attribute__((noreturn));
void _sysfatal(const * file, int line, const char *func, const char *format, ...) __attribute__((noreturn));
void _warn(const * file, int line, const char *format, ...);

#define fatalf(format, ...) _fatal(__FILE__, __LINE__, format, __VA_ARGS__)
#define fatal(format) _fatal(__FILE__, __LINE__, format)
#define sysfatalf(func, format, ...) _sysfatal(__FILE__, __LINE__, func, format, __VA_ARGS__)
#define sysfatal(func, format) _sysfatal(__FILE__, __LINE__, func, format)
#define warnf(format, ...) _warn(__FILE__, __LINE__, format, __VA_ARGS__)
#define warn(format) _warn(__FILE__, __LINE__, format)
