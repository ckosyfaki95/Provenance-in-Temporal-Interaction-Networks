/*Chrysanthi Kosyfaki, University of Ioannina, PhD Candidate */

#ifndef __FIFOQUEUE
#define __FIFOQUEUE

#include <stdio.h>
#include <stdlib.h>
#include "minheap.h" // to obtain struct BufItem definition

void fifoenqueue(struct BufItem newitem, struct BufItem **queue, int *num_elems, int *first, int *last, int *capacity);
int fifodequeue(struct BufItem *el, struct BufItem *queue, int *num_elems, int *last, int capacity);
void print_queue(struct BufItem *queue, int num_elems, int first, int last, int capacity);
double sum_fifoelems(struct BufItem *queue, int num_elems, int last, int capacity);

#endif // __FIFOQUEUE
