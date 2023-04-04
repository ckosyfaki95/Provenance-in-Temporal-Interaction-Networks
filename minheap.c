/*Chrysanthi Kosyfaki, University of Ioannina, PhD Candidate */

/*implementation of a minheap (priority queue) especially for buffered items*/
/*used by provenance.c*/
/*heap capacity is dynamic*/
#include <time.h>
#include "minheap.h"

/*returns the parent of a heap position*/
int parent(int posel)
{
  if (posel%2) /*odd*/
        return posel/2;
  else
        return (posel-1)/2;
}

/*enqueues element*/
/*heap[0] is the element with the smallest value*/
/*every element is greater than or equal to its parent*/
void enqueue(struct BufItem newitem, struct BufItem **heap, int *num_elems, int *capacity)
//void enqueue(e_type el, int idx, elem *heap, int *num_elems)
{
    struct BufItem tmp;
    int p;
    int posel;

	if (*capacity <= *num_elems) {
		*heap = (struct BufItem *)realloc(*heap, (*capacity)*2*sizeof(struct BufItem));
    	(*capacity) *= 2;
    }
    posel = *num_elems; //last position
    (*heap)[(*num_elems)++] = newitem;

    while(posel >0)
    {
        p=parent(posel);
        if (newitem.ts<(*heap)[p].ts)
        {
	  /* swap element with its parent */
          tmp = (*heap)[p];
          (*heap)[p] = newitem;
          (*heap)[posel] = tmp;

          posel = parent(posel);
        }
        else break;
    }
}

/* moves down the root element */
/* used by dequeue (see below) */
void movedown(struct BufItem *heap, int *num_elems)
{
    struct BufItem tmp;
    int posel = 0; //root
    int swap;
    /*while posel is not a leaf and heap[posel].ts > any of childen*/
    while (posel*2+1 < *num_elems) /*there exists a left son*/
    {
        if (posel*2+2< *num_elems) /*there exists a right son*/
        {
            if(heap[posel*2+1].ts<heap[posel*2+2].ts)
                swap = posel*2+1;
            else
                swap = posel*2+2;
        }
        else
            swap = posel*2+1;

        if (heap[posel].ts > heap[swap].ts) /*larger than smallest son*/
        {
	    /*swap elements*/
            tmp = heap[swap];
            heap[swap] = heap[posel];
            heap[posel] = tmp;
            
	        posel = swap;
        }
        else break;
    }
}

/* returns the root element, puts the last element as root and moves it down */
int dequeue(struct BufItem *el, struct BufItem *heap, int *num_elems)
{
    if ((*num_elems)==0) /* empty queue */
        return 0;

    *el = heap[0];
    heap[0] = heap[(*num_elems)-1];
    (*num_elems)--;
    movedown(heap, num_elems);
    return 1;
}

void print_heap(struct BufItem *heap, int num_elems) {
  int i;

  printf("Heap contents:\n");
  for (i=0; i<num_elems; i++)
    printf("%d %.2f %.2f\n", heap[i].origin, heap[i].ts, heap[i].qty);
}

double sum_elems(struct BufItem *heap, int num_elems) {
	double s = 0;
	for (int i=0; i<num_elems; i++)
		s += heap[i].qty;
	return s;
}




