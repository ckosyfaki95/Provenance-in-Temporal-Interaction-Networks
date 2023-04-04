/*Chrysanthi Kosyfaki, University of Ioannina, PhD Candidate */

#ifndef __MINHEAP
#define __MINHEAP

#include <stdio.h>
#include <stdlib.h>

// struct for buffered items (to track provenance)
struct BufItem { /* heap element */
	int origin;
	double ts; //timestamp -> KEY!
	double qty; //quantity
};

void enqueue(struct BufItem newitem, struct BufItem **heap, int *num_elems, int *capacity);
void movedown(struct BufItem *heap, int *num_elems);
int dequeue(struct BufItem *el, struct BufItem *heap, int *num_elems);
void print_heap(struct BufItem *heap, int num_elems);
double sum_elems(struct BufItem *heap, int num_elems);
int parent(int posel);

#endif // __MINHEAP
