/*Chrysanthi Kosyfaki, University of Ioannina, PhD Candidate */

/*implementation of a FIFO queue for buffered items*/
/*used by provenance.c*/
/*queue capacity is dynamic*/
/*ring queue implementation*/
#include <time.h>
#include "fifoqueue.h"


/*enqueues element*/
/*queue[first] is the first element in queue*/
/*queue[last] is the last element in queue*/
void fifoenqueue(struct BufItem newitem, struct BufItem **queue, int *num_elems, int *first, int *last, int *capacity)
{
    int p;

	if (*capacity <= *num_elems) {
		*queue = (struct BufItem *)realloc(*queue, (*capacity)*2*sizeof(struct BufItem));
    	//move elements 
    	for(p=(*capacity)-1; p>=*first; p--)
    	{
    		(*queue)[(*capacity)+p]=(*queue)[p];
    	}
		(*last)+=(*capacity);
    	(*capacity) *= 2;
    }

	(*queue)[*first]=newitem;

	*first = ((*first)+1) %  (*capacity);
	(*num_elems)++;
}

/* removes and returns the last element  */
int fifodequeue(struct BufItem *el, struct BufItem *queue, int *num_elems, int *last, int capacity)
{
    if ((*num_elems)==0) /* empty queue */
        return 0;

	*el=queue[*last];
	*last = ((*last)+1) % capacity;
	(*num_elems)--;
    return 1;
}

void print_queue(struct BufItem *queue, int num_elems, int first, int last, int capacity) 
{
  int i, inext;

  printf("Queue contents:\n");
  if (num_elems ==0)
  	printf("<empty>\n");
  else {
  	while (num_elems) {
  		printf("%d %.2f %.2f\n", queue[last].origin, queue[last].ts, queue[last].qty);
  		last = (last+1)%capacity;
	  	num_elems--;
  	}
  }
}

double sum_fifoelems(struct BufItem *queue, int num_elems, int last, int capacity)
{
	double s = 0;
	while (num_elems) {
  		s += queue[last].qty;
  		last = (last+1)%capacity;
	  	num_elems--;
  	}
	return s;
}


