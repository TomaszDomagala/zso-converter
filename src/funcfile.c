#include "funcfile.h"

#include <stdio.h>
#include <string.h>

#include "fatal.h"

enum f_type func_parser(char* type) {
    if (strcmp(type, "void") == 0) {
        return f_void;
    } else if (strcmp(type, "int") == 0) {
        return f_int;
    } else if (strcmp(type, "uint") == 0) {
        return f_uint;
    } else if (strcmp(type, "long") == 0) {
        return f_long;
    } else if (strcmp(type, "ulong") == 0) {
        return f_ulong;
    } else if (strcmp(type, "longlong") == 0) {
        return f_longlong;
    } else if (strcmp(type, "ulonglong") == 0) {
        return f_ulonglong;
    } else if (strcmp(type, "ptr") == 0) {
        return f_ptr;
    } else {
        fatalf("unknown function return/argument type: %s\n", type);
    }
}

list_t* parse_funcs(char* lines) {
    list_t* funcs = list_create(sizeof(struct f_func));

    char *str1, *str2, *line, *token;
    char *saveptr1, *saveptr2;

    for (str1 = lines;; str1 = NULL) {
        line = strtok_r(str1, "\n", &saveptr1);
        if (line == NULL) {
            break;
        }
        struct f_func func = {0};

        for (str2 = line;; str2 = NULL) {
            token = strtok_r(str2, " \t", &saveptr2);
            if (token == NULL) {
                break;
            }
            if (func.f_name == NULL) {
                func.f_name = malloc(strlen(token) + 1);
                if (func.f_name == NULL) {
                    sysfatal("malloc", "Could not allocate memory for function name");
                }
                strcpy(func.f_name, token);
                continue;
            }
            if (func.f_ret_type == f_notype) {
                func.f_ret_type = func_parser(token);
                continue;
            }
            if (func.f_args_count == 6) {
                fatalf("too many arguments for function %s\n", func.f_name);
            }
            func.f_args_types[func.f_args_count] = func_parser(token);
            func.f_args_count++;
        }

        if (func.f_name != NULL) {
            list_add(funcs, &func);
        }
    }

    return funcs;
}

void f_type_to_str(char* str, enum f_type type) {
    switch (type) {
        case f_void:
            strcpy(str, "void");
            break;
        case f_int:
            strcpy(str, "int");
            break;
        case f_uint:
            strcpy(str, "uint");
            break;
        case f_long:
            strcpy(str, "long");
            break;
        case f_ulong:
            strcpy(str, "ulong");
            break;
        case f_longlong:
            strcpy(str, "longlong");
            break;
        case f_ulonglong:
            strcpy(str, "ulonglong");
            break;
        case f_ptr:
            strcpy(str, "ptr");
            break;
        case f_notype:
            strcpy(str, "notype");
            break;
        default:
            fatalf("unknown function/argument type: %d\n", type);
    }
}

void print_func(struct f_func* func) {
    char type_str[32];
    f_type_to_str(type_str, func->f_ret_type);
    printf("%s %s(", type_str, func->f_name);
    for (int i = 0; i < func->f_args_count -1; i++) {
        f_type_to_str(type_str, func->f_args_types[i]);
        printf("%s, ", type_str);
    }
    if (func->f_args_count > 0) {
        f_type_to_str(type_str, func->f_args_types[func->f_args_count - 1]);
        printf("%s", type_str);
    }
    printf(")\n");
}
