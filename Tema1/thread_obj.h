/*#ifndef THREAD_OBJECT_H
#define THREAD_OBJ_H*/
#include "individual.h"
#include "sack_object.h"

typedef struct thread_obj{
	int *count;
	int *cursor;
	int id;
	int P;
	int sack_capacity;
	int object_count;
	int generations_count;
	individual *current_generation;
	individual *next_generation;
	individual *tmp;
	pthread_barrier_t *barrier;
	const sack_object *objects;
} thread_obj;