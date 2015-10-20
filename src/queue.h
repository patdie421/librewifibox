/*
 *  queue.h
 *
 *  Created by Patrice Dietsch on 28/07/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __queue_h
#define __queue_h

#include "error.h"

#define QUEUE_ENABLE_INDEX 1

typedef void (*free_data_f)(void *);
typedef int (*compare_data_f)(void *, void *);

struct queue_elem
{
   void *d;
   struct queue_elem *next;
   struct queue_elem *prev;
};

typedef struct
{
   struct queue_elem *first;
   struct queue_elem *last;
   struct queue_elem *current;
   unsigned long nb_elem;
#ifdef QUEUE_ENABLE_INDEX
   void   **index;
   char   index_status; // 1 = utilisable, 0 = non utilisable (doit être reconstruit)
   compare_data_f index_order_f;
#endif
} queue_t;


mea_error_t init_queue(queue_t *queue);
mea_error_t out_queue_elem(queue_t *queue, void **data);
mea_error_t in_queue_elem(queue_t *queue, void *data);
mea_error_t process_all_elem_queue(queue_t *queue, void (*f)(void *));
mea_error_t first_queue(queue_t *queue);
mea_error_t last_queue(queue_t *queue);
mea_error_t next_queue(queue_t *queue);
mea_error_t prev_queue(queue_t *queue);
mea_error_t current_queue(queue_t *queue, void **data);
mea_error_t clean_queue(queue_t *queue,free_data_f f);
mea_error_t remove_current_queue(queue_t *queue);

/* renommage de toutes les fonctions à faire, voir ci-dessous

mea_error_t queue_init(queue_t *queue);
mea_error_t queue_out_elem(queue_t *queue, void **data);
mea_error_t queue_in_elem(queue_t *queue, void *data);
mea_error_t queue_process_all_elem(queue_t *queue, void (*f)(void *));
mea_error_t queue_current_elem(queue_t *queue, void **data);

mea_error_t queue_remove_current_elem(queue_t *queue);

mea_error_t queue_first(queue_t *queue);
mea_error_t queue_last(queue_t *queue);
mea_error_t queue_next(queue_t *queue);
mea_error_t queue_prev(queue_t *queue);

mea_error_t queue_clean(queue_t *queue,free_data_f f);

*/
mea_error_t queue_find_elem(queue_t *queue, compare_data_f, void *data_to_find, void **data);

#ifdef ENABLE_INDEX
mea_error_t queue_create_index(queue_t *queue, compare_data_f f);
mea_error_t queue_recreate_index(queue_t *queue, char force);
mea_error_t queue_remove_index(queue_t *queue);
mea_error_t queue_find_elem_by_index(queue_t *queue, void *data_to_find, void **data);
mea_error_t queue_get_elem_by_index(queue_t *queue, int i, void **data);
char queue_index_status(queue_t *queue);
void *queue_get_data(void *e);

#endif

#endif
