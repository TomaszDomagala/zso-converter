#include "list.h"

enum f_type {
    f_notype = 0,  // no type
    f_void,        // void, only for return type
    f_int,       // always 32 bit signed
    f_uint,      // always 32 bit unsigned
    f_long,      // mode dependent, 32 signed in 32 bit mode, 64 signed in 64 bit mode
    f_ulong,     // mode dependent, 32 unsigned in 32 bit mode, 64 unsigned in 64 bit mode
    f_longlong,       // always 64 bit signed
    f_ulonglong,      // always 64 bit unsigned
    f_ptr,      // mode dependent, 32 bit pointer in 32 bit mode, 64 bit pointer in 64 bit mode
};

struct f_func {
    char* f_name;
    enum f_type f_ret_type;
    enum f_type f_args_types[6];
    int f_args_count;
};

list_t* read_funcs(char* filename);
void print_func(struct f_func* func);
