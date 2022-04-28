#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "genetic_algorithm.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *P, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*P = (int) strtol(argv[3], NULL, 10);

	if (*P == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int start, int end, int sack_capacity)
{
	int weight;
	int profit;
	//printf("%d %d\n", start, end);
	for (int i = start; i <= end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(individual first, individual second)
{
	int i;

	int res = second.fitness - first.fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first.chromosome_length && i < second.chromosome_length; ++i) {
			first_count += first.chromosomes[i];
			second_count += second.chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second.index - first.index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void merge(int start, int middle, int end, individual *current_generation)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int left_length = middle - start + 1;
    int right_length = end - middle;
    individual *left_array = (individual*) calloc(left_length, sizeof(individual));
    individual *right_array = (individual*) calloc(right_length, sizeof(individual));
    
    // copiaza valorile din jumatatea stanga
    for (int i = 0; i < left_length; i ++) {
        left_array[i] = current_generation[start + i];
    }
    
    // copiaza valorile din jumatatea dreapta
    for (int j = 0; j < right_length; j ++) {
        right_array[j] = current_generation[middle + 1 + j];
    }
    
    i = 0;
    j = 0;
    // face merge intr jumatatea din dreapta si cea din stanga
    while (i < left_length && j < right_length) {
        if (cmpfunc(left_array[i], right_array[j]) < 0) {
            current_generation[start + k] = left_array[i];
            i ++;
        } else {
            current_generation[start + k] = right_array[j];
            j ++;
        }
        k ++;
    }
    
    while (i < left_length) {
        current_generation[start + k] = left_array[i];
        k ++;
        i ++;
    }
    while (j < right_length) {
        current_generation[start + k] = right_array[j];
        k ++;
        j ++;
    }
}

void merge_sort(int start, int end, individual *current_generation)
{
    if (start < end) {
        int middle = start + (end - start) / 2;
        merge_sort(start, middle, current_generation);
        merge_sort(middle + 1, end, current_generation);
        merge(start, middle, end, current_generation);
    }
}

void merge_sections_of_array(individual* current_generation, int parts, int width, int object_count, int P) {
	// face merge intre partile sortate de fiecare thread
    for(int i = 0; i < parts; i = i + 2) {
        int start = i * (object_count / P * width);
        int end = min(((i + 2) * object_count / P * width) - 1, object_count-1);
        int middle = start + (object_count / P * width) - 1;
        merge(start, middle, end, current_generation);
    }
    if (parts / 2 >= 1) {
        merge_sections_of_array(current_generation, parts / 2, width * 2, object_count, P);
    }
}

void *thread_function(void *arg)
{

	thread_obj thread_info = *(thread_obj *)arg;
	individual *current_generation = thread_info.current_generation;
	individual *next_generation = thread_info.next_generation;
	individual *tmp = thread_info.tmp;
	int sack_capacity = thread_info.sack_capacity;
	int object_count = thread_info.object_count;
	int generations_count = thread_info.generations_count;
	// calculeaza numarul de obiecte din generatie pe care il ia fiecare thread
	int start = thread_info.id * (object_count / thread_info.P);
    int end = (thread_info.id + 1) * (object_count / thread_info.P) - 1;
    if (thread_info.id == thread_info.P - 1) {
        end += object_count % thread_info.P;
    }

	for (int k = 0; k < generations_count + 1; ++k) {
		// asteapta ca threadul 0 sa termine operatiile mutate si crossover
		pthread_barrier_wait(thread_info.barrier);

		// fiecare thread calculeaza fitness-ul obiectelor sale
		if (thread_info.id == 0)
			*thread_info.cursor = 0;
		compute_fitness_function(thread_info.objects, current_generation, start, end, sack_capacity);
		// asteapta ca toate threadurile sa calculeze fitness-ul
	    merge_sort(start, end, current_generation);
	    // asteapta ca toate threadurile sa isi soreteze obiectele
	    pthread_barrier_wait(thread_info.barrier);
	    // threadul 0 va face merge intre partile tuturor thread-urilor
	    if (thread_info.id == 0)
		    merge_sections_of_array(current_generation, thread_info.P, 1, object_count, thread_info.P);
		// thread-urile asteapta ca thread 0 sa termine merge-ul
		pthread_barrier_wait(thread_info.barrier);
		// cum thread 0 va realiza celelalte operatii, restul threadurilor se vor intoarce la calcularea fitness-ului
	    if (thread_info.id != 0){
			tmp = current_generation;
			current_generation = next_generation;
			next_generation = tmp;
	    	continue;
	    }
	    if (k == generations_count){
	    	break;
	    }

		// keep first 30% children (elite children selection)
		*thread_info.count = object_count * 3 / 10;
		for (int i = 0; i < *thread_info.count; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}

		*thread_info.cursor = *thread_info.count;
		// mutate first 20% children with the first version of bit string mutation
		*thread_info.count = object_count * 2 / 10;
		for (int i = 0; i < *thread_info.count; ++i) {
			copy_individual(current_generation + i, next_generation + *thread_info.cursor + i);
			mutate_bit_string_1(next_generation + *thread_info.cursor + i, k);
		}
		*thread_info.cursor += *thread_info.count;
		// mutate next 20% children with the second version of bit string mutation
		*thread_info.count = object_count * 2 / 10;
		for (int i = 0; i < *thread_info.count; ++i) {
			copy_individual(current_generation + i + *thread_info.count, next_generation + *thread_info.cursor + i);
			mutate_bit_string_2(next_generation + *thread_info.cursor + i, k);
		}
		*thread_info.cursor += *thread_info.count;
		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		*thread_info.count = object_count * 3 / 10;

		if (*thread_info.count % 2 == 1) {
			copy_individual(current_generation + object_count - 1, next_generation + *thread_info.cursor + *thread_info.count - 1);
			(*thread_info.count)--;
		}

		for (int i = 0; i < *thread_info.count; i += 2) {
			crossover(current_generation + i, next_generation + *thread_info.cursor + i, k);
		}
		// switch to new generation
		tmp = current_generation;
		current_generation = next_generation;
		next_generation = tmp;

		for (int i = 0; i < object_count; ++i) {
			current_generation[i].index = i;
		}

		if (k % 5 == 0) {
			print_best_fitness(current_generation);
		}
	}
	if(thread_info.id != 0)
		pthread_exit(NULL);
	print_best_fitness(current_generation);

	pthread_exit(NULL);
}

void run_genetic_algorithm(const sack_object *objects, int object_count, int generations_count, int sack_capacity, int P)
{
	int count, cursor;
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = 0; i < object_count; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}

	thread_obj thread_info[P];
	pthread_t tid[P];
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, P);

	// se creeaza thread-urile si adauga intr-o structura toate variabilele necesare pentru thread
	for (int i = 0; i < P; i++) {
		thread_info[i].count = &count;
		thread_info[i].cursor = &cursor;
		thread_info[i].id = i;
		thread_info[i].P = P;
		thread_info[i].object_count = object_count;
		thread_info[i].generations_count = generations_count;
		thread_info[i].sack_capacity = sack_capacity;
		thread_info[i].current_generation = current_generation;
		thread_info[i].next_generation = next_generation;
		thread_info[i].tmp = tmp;
		thread_info[i].objects = objects;
		thread_info[i].barrier = &barrier;
		pthread_create(&tid[i], NULL, thread_function, &thread_info[i]);
	}

	// se asteapta thread-urile
	for (int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	pthread_barrier_destroy(&barrier);

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);
}