#ifndef CONVERTER_LIST_H
#define CONVERTER_LIST_H

#include <stdlib.h>

typedef struct list_node_s list_node_t;

typedef struct list_s list_t;

list_t *list_create(size_t element_size);

list_node_t *list_add(list_t *list, void *element);

size_t list_size(list_t *list);

void list_remove(list_t *list, list_node_t *node);

void *list_element(list_node_t *node);

void list_free(list_t *list);

list_node_t *list_head(list_t *list);

list_node_t *list_tail(list_t *list);

list_node_t *list_next(list_node_t *node);

list_node_t *list_prev(list_node_t *node);

void list_remove_all(list_t *list);

#endif //CONVERTER_LIST_H