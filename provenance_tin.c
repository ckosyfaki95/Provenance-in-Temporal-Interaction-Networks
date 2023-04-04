/*Chrysanthi Kosyfaki, University of Ioannina, PhD Candidate */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "minheap.h"
#include "fifoqueue.h"

// struct for input interactions
struct Interaction {
	int src;
	int dest;
	double ts; //timestamp
	double qty; //quantity
};

// struct for buffered items (for proportional tracking - no timestamp needed)
struct BufItemProp { 
	int origin;
	double qty; //quantity
};

// struct for paths of bufitems
// used in how-provenance
struct BufItemPath { 
	int pathlen;
	int pathcapacity;
	int *path; //quantity
};



// assume a graph input file of the form:
// <number of nodes>
// <source> <dest> <timestamp> <quantity>
// ....
// file should be sorted by timestamp
// the algorithm reads the interactions line-by-line and updates the relevant buffers
// --------------------
// computes the flow at each node after processing all interactions in time order
// interaction <source> <dest> <timestamp> <quantity> transfers <quantity> 
// from <source> buffer to <dest> buffer
// if buffer has insufficient quantity, then the source node 'bears' the difference


// read graph from file into memory
int readGraph(FILE *f, struct Interaction **inter, int *numinter, int *numnodes)
{
    int i,j,k;
    
    char *line = NULL; // used for fileread
	size_t len = 0; // used for fileread
	ssize_t read; // used for fileread
	const char delim[2] = "\t"; // used for fileread
	char *token; // used for fileread

	/* read first line */
	/* first line should be <numnodes> */
	read = getline(&line, &len, f);
	if (read==-1)
	{
		printf("ERROR: first line is empty. Exiting...\n");
		return -1;
	};
	token = strtok(line,delim);
	(*numnodes) = atoi(token);
	printf("numnodes=%d\n",*numnodes);
	
	/* second line should be <numinter> */
	read = getline(&line, &len, f);
	if (read==-1)
	{
		printf("ERROR: second line is empty. Exiting...\n");
		return -1;
	};
	token = strtok(line,delim);
	(*numinter) = atoi(token);
	printf("numinter=%d\n",*numinter);
	
	*inter = (struct Interaction *)malloc((*numinter)*sizeof(struct Interaction));
    *numinter = 0; //reset for re-counting
		
	// Read interactions from file
	while ((read = getline(&line,&len,f)) != -1)	{
		token = strtok(line,delim);
		(*inter)[(*numinter)].src = atoi(token);
		token = strtok(NULL,delim);
		(*inter)[(*numinter)].dest = atoi(token);
		token = strtok(NULL,delim);
		(*inter)[(*numinter)].ts = atof(token);
		token = strtok(NULL,delim);
		(*inter)[(*numinter)].qty = atof(token);

		(*numinter)++;
	}

    return 0;
}

// interactions are read from memory
int noProvFromMem(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j;

    double *buffer; // array of buffers, one for each node of the Graph
    
	double relayqty;
		    
    buffer = (double *)malloc(numnodes*sizeof(double));
    for(i=0;i<numnodes;i++)
        buffer[i]=0.0; // initially, all buffers are 0
    
	for(i=0;i<numinter;i++)
	{
		relayqty = buffer[inter[i].src]<inter[i].qty ? buffer[inter[i].src]:inter[i].qty; // min(buffer[src],qty)
		buffer[inter[i].src] -= relayqty;
		buffer[inter[i].dest] += inter[i].qty;
		//for(j=0;j<numnodes;j++) printf("%d\t%f\n",j,buffer[j]);
	}

	int stop = 100<numnodes ? 100:numnodes;
	/*
	for(i=0;i<stop;i++)
       printf("node %d buffer = %f\n",i,buffer[i]);
       */
       
	//for(i=0;i<numnodes;i++)
	//	printf("%d\t%f\n",i,buffer[i]);
		
	double sumqty =0.0;
	for(i=0;i<numnodes;i++)
       sumqty+=buffer[i];
    printf("sumqty=%.2f\n",sumqty);

	free(buffer);
	
    return 0;
}

// comparison function for doubles (descending order)
int cmpfunc (const void * a, const void * b)
{
  if (*(double*)a < *(double*)b)
    return 1;
  else if (*(double*)a > *(double*)b)
    return -1;
  else
    return 0;  
}

// same as noProvFromMem, but 
// records in array topk the k vertices with the largest contribution or buffer as origins 
int noProvFromMem2(struct Interaction *inter, int numinter, int numnodes, int *topk, int k, int contr)
{
    int i,j;
    int numelems, posel, p, tmp, swap; // for heap

    double *buffer; // array of buffers, one for each node of the Graph
    double *contributions; // contribution of each node (as origin)
    
    double *comp; // top-k based on comp: either contribution (if contr=1) or sumbuffered (if contr=0)
    
	double relayqty;
		    
    buffer = (double *)malloc(numnodes*sizeof(double));
    contributions = (double *)malloc(numnodes*sizeof(double));
    for(i=0;i<numnodes;i++) {
        buffer[i]=0.0; // initially, all buffers are 0
        contributions[i]=0.0; // initially, all contributions are 0
    }
    
	for(i=0;i<numinter;i++)
	{
		relayqty = buffer[inter[i].src]<inter[i].qty ? buffer[inter[i].src]:inter[i].qty; // min(buffer[src],qty)
		if (inter[i].qty>buffer[inter[i].src]) // vertex bears new qty
			contributions[inter[i].src] += inter[i].qty-buffer[inter[i].src];
		buffer[inter[i].src] -= relayqty;
		buffer[inter[i].dest] += inter[i].qty;
	}
		
	double sumqty =0.0;
	for(i=0;i<numnodes;i++)
       sumqty+=buffer[i];
    printf("sumqty in buffers=%.2f\n",sumqty);

	sumqty =0.0;
	for(i=0;i<numnodes;i++) {
       sumqty+=contributions[i];
       //printf("%d contribution:%.2f\n",i,contributions[i]);
    }
    printf("sumqty in contributions=%.2f\n",sumqty);    
    
    if (contr)
    	comp = contributions;
    else
    	comp = buffer;

	// now find top-k based on comp array
    //initialize minheap
    if (k>=numnodes) {
    	//trivial case
    	for(i=0;i<numnodes;i++)
    		topk[i]=i;
    }
    else {    
		//add first k elements
		numelems = 0;		
		for(i=0;i<k;i++) {
			//enqueue vertex i
			posel = numelems; //last position
			topk[numelems++] = i;
			//newitem = i;

			while(posel >0)
			{
				p=parent(posel);
				if (comp[i]<comp[topk[p]])
				{
			  		/* swap element with its parent */
				  topk[posel] = topk[p];
				  topk[p] = i;

				  posel = parent(posel);
				}
				else break;
			}
		}
		
		//examine remaining elements
		for(;i<numnodes;i++) {
			if (comp[i]>comp[topk[0]]) // i must be added to heap
			{
				topk[0] = i; //place i at the top of the min-heap
				//correct heap (movedown)
				posel = 0;
				while (posel*2+1 < k) /*there exists a left son*/
				{
					if (posel*2+2< k) /*there exists a right son*/
					{
						if(comp[topk[posel*2+1]]<comp[topk[posel*2+2]])
							swap = posel*2+1;
						else
							swap = posel*2+2;
					}
					else
						swap = posel*2+1;

					if (comp[topk[posel]] > comp[topk[swap]]) /*larger than smallest son*/
					{
						/*swap elements*/	
						tmp = topk[swap];
						topk[swap] = topk[posel];
						topk[posel] = tmp;
			
						posel = swap;
					}
					else break;
				}
			}
		}
	}
	
	free(buffer);
	free(contributions);
	
    return 0;
}

// provenance oldest birth first model
// Least Recently Born in paper
int ProvOldestFirst(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItem **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
    
	double relayqty;
	double residueqty;
	int src;
	int dest;
	struct BufItem newentry;
	
	int numtransfers = 0;
	int numrelays = 0;
	
	clock_t t,tnow;
	double cumqty = 0.0; //cumulative quantity
	
	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItem **)malloc(numnodes*sizeof(struct BufItem *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        bufcapacity[i]=3;
        buffer[i] = (struct BufItem *)malloc(bufcapacity[i]*sizeof(struct BufItem));
    }
    
    t = clock(); 
	for(i=0;i<numinter;i++)
	{
		cumqty += inter[i].qty;
		residueqty=inter[i].qty; // remaining quantity to be transferred
		src = inter[i].src;
		dest = inter[i].dest;
		while (residueqty>0 && bufsize[src])
		{
			numtransfers++;
			if (buffer[src][0].qty>residueqty) { // buffer[src][0] is quantity with oldest birth
				newentry.origin = buffer[src][0].origin;
				newentry.ts = buffer[src][0].ts;
				newentry.qty = residueqty;
				enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
				buffer[src][0].qty-=residueqty;
				residueqty = 0;
			}
			else { //entire entry must be relayed from src to dest
				numrelays++;
				dequeue(&newentry,buffer[src],&bufsize[src]);
				enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
				residueqty-=newentry.qty;
			}
		}
		if (residueqty>0) // src did not have enough buffered quantity to relay; give birth to new flow item
		{	
			numtransfers++;
			newentry.origin = src;
			newentry.ts = inter[i].ts;
			newentry.qty = residueqty;
			enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
		}
	}
	
	int sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	int sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
    double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);
    printf("numtransfers=%d\n",numtransfers);
    printf("numrelays=%d\n",numrelays);

    for(i=0;i<numnodes;i++){
		printf("Buffer of vertex %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(o=%d,ts=%.2f,qty=%.2f) ",buffer[i][j].origin,buffer[i][j].ts,buffer[i][j].qty);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	
    return 0;
}

// provenance newest birth first model
// same as oldest first, but all timestamps are made negative
// in order to prioritize newest ones (smallest negative ones)
int ProvNewestFirst(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItem **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
    
	double relayqty;
	double residueqty;
	int src;
	int dest;
	struct BufItem newentry;
	
	int stop = 100<numnodes ? 100:numnodes;
		    
	clock_t t,tnow;
	 
    buffer = (struct BufItem **)malloc(numnodes*sizeof(struct BufItem *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        bufcapacity[i]=3;
        buffer[i] = (struct BufItem *)malloc(bufcapacity[i]*sizeof(struct BufItem));
    }
    
	t = clock();

	for(i=0;i<numinter;i++)
	{
		residueqty=inter[i].qty; // remaining quantity to be transferred
		src = inter[i].src;
		dest = inter[i].dest;
		while (residueqty>0 && bufsize[src])
		{
			if (buffer[src][0].qty>residueqty) { // buffer[src][0] is quantity with oldest birth
				newentry.origin = buffer[src][0].origin;
				newentry.ts = buffer[src][0].ts;
				newentry.qty = residueqty;
				enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
				buffer[src][0].qty-=residueqty;
				residueqty = 0;
			}
			else { //entire entry must be relayed from src to dest
				dequeue(&newentry,buffer[src],&bufsize[src]);
				enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
				residueqty-=newentry.qty;
			}
		}
		if (residueqty>0) // src did not have enough buffered quantity to relay; give birth to new flow item
		{
			newentry.origin = src;
			newentry.ts = -inter[i].ts;
			newentry.qty = residueqty;
			enqueue(newentry,&buffer[dest],&bufsize[dest],&bufcapacity[dest]);
		}

	}
	
	int sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	int sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
	double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);

    for(i=0;i<numnodes;i++){
		printf("Buffer of vertex %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(o=%d,ts=%.2f,qty=%.2f) ",buffer[i][j].origin,buffer[i][j].ts,buffer[i][j].qty);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	
    return 0;
}

// provenance LIFO model
// last-in first out when propagating quantities
// each node's buffer is managed as a stack 
int ProvLIFO(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItem **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
    
	double relayqty;
	double residueqty;
	int src;
	int dest;
	struct BufItem newentry;
	
	int numtransfers = 0;
	
	clock_t t,tnow;

	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItem **)malloc(numnodes*sizeof(struct BufItem *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        bufcapacity[i]=3; //initial capacity per buffer
        buffer[i] = (struct BufItem *)malloc(bufcapacity[i]*sizeof(struct BufItem));
    }

	t = clock();
	    
	for(i=0;i<numinter;i++)
	{
		residueqty=inter[i].qty; // remaining quantity to be transferred
		src = inter[i].src;
		dest = inter[i].dest;
		if (src==dest) continue;
		while (residueqty>0 && bufsize[src])
		{
			numtransfers++;
			if (buffer[src][bufsize[src]-1].qty>residueqty) { // buffer[src][-1] is most recently added quantity (stack's top)
				newentry.origin = buffer[src][bufsize[src]-1].origin;
				newentry.ts = buffer[src][bufsize[src]-1].ts; //not used
				newentry.qty = residueqty;
				if (bufcapacity[dest] <= bufsize[dest]) {
					buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
					bufcapacity[dest] *= 2;
				}
				buffer[dest][bufsize[dest]++] = newentry;
				
				buffer[src][bufsize[src]-1].qty-=residueqty;
				residueqty = 0;
			}
			else { //entire entry must be relayed from src to dest
				if (bufcapacity[dest] <= bufsize[dest]) {
					buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
					bufcapacity[dest] *= 2;
				}
				buffer[dest][bufsize[dest]++] = buffer[src][bufsize[src]-1];
				residueqty-=buffer[src][bufsize[src]-1].qty;
				
				bufsize[src]--; // implicitly removes top element (no need to implement this explicitly)
			}
		}
		if (residueqty>0) // src did not have enough buffered quantity to relay; give birth to new flow item
		{	
			numtransfers++;
			newentry.origin = src;
			newentry.ts = inter[i].ts; // not used
			newentry.qty = residueqty;
			
			if (bufcapacity[dest] <= bufsize[dest]) {
				buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
				bufcapacity[dest] *= 2;
			}
			buffer[dest][bufsize[dest]++] = newentry;
		}
		
	}

	int sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	int sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
    double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);
    printf("numtransfers=%d\n",numtransfers);


    for(i=0;i<numnodes;i++){
		printf("Buffer of vertex %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(o=%d,qty=%.2f) ",buffer[i][j].origin,buffer[i][j].qty);
		printf("\n");
	}


    for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	
    return 0;
}

// provenance LIFO model
// last-in first out when propagating quantities
// each node's buffer is managed as a stack
// tracks paths of buffered quantities 
int ProvLIFOPaths(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItem **buffer; // array of buffers, one for each node of the Graph
    struct BufItemPath **bufferpath; // array of buffer paths, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
    
	double relayqty;
	double residueqty;
	int src;
	int dest;
	struct BufItem newentry;
	struct BufItemPath newpathentry;
	
	int numtransfers = 0;
	
	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItem **)malloc(numnodes*sizeof(struct BufItem *));
    bufferpath = (struct BufItemPath **)malloc(numnodes*sizeof(struct BufItemPath *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        bufcapacity[i]=3; //initial capacity per buffer
        buffer[i] = (struct BufItem *)malloc(bufcapacity[i]*sizeof(struct BufItem));
        bufferpath[i] = (struct BufItemPath *)malloc(bufcapacity[i]*sizeof(struct BufItemPath));
    }
    
	for(i=0;i<numinter;i++)
	{
		residueqty=inter[i].qty; // remaining quantity to be transferred
		src = inter[i].src;
		dest = inter[i].dest;
		if (src==dest) continue;
		while (residueqty>0 && bufsize[src])
		{
			numtransfers++;
			if (buffer[src][bufsize[src]-1].qty>residueqty) { // buffer[src][-1] is most recently added quantity (stack's top)
				newentry.origin = buffer[src][bufsize[src]-1].origin;
				newentry.ts = buffer[src][bufsize[src]-1].ts; //not used
				newentry.qty = residueqty;
				newpathentry.pathlen = bufferpath[src][bufsize[src]-1].pathlen+1;
				newpathentry.pathcapacity = bufferpath[src][bufsize[src]-1].pathlen+1;
				newpathentry.path = (int *)malloc(newpathentry.pathcapacity*sizeof(int));
				for(j=0;j<bufferpath[src][bufsize[src]-1].pathlen;j++) // copy existing path from source
					newpathentry.path[j] = bufferpath[src][bufsize[src]-1].path[j];
				newpathentry.path[bufferpath[src][bufsize[src]-1].pathlen]=src; // add source node to path
				if (bufcapacity[dest] <= bufsize[dest]) {
					buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
					bufferpath[dest] = (struct BufItemPath *)realloc(bufferpath[dest], bufcapacity[dest]*2*sizeof(struct BufItemPath));
					bufcapacity[dest] *= 2;
				}
				buffer[dest][bufsize[dest]] = newentry;
				bufferpath[dest][bufsize[dest]++] = newpathentry;
				buffer[src][bufsize[src]-1].qty-=residueqty;
				residueqty = 0;
			}
			else { //entire entry must be relayed from src to dest
				if (bufcapacity[dest] <= bufsize[dest]) {
					buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
					bufferpath[dest] = (struct BufItemPath *)realloc(bufferpath[dest], bufcapacity[dest]*2*sizeof(struct BufItemPath));
					bufcapacity[dest] *= 2;
				}
				buffer[dest][bufsize[dest]] = buffer[src][bufsize[src]-1];
				bufferpath[dest][bufsize[dest]] = bufferpath[src][bufsize[src]-1];
				if (bufferpath[dest][bufsize[dest]].pathcapacity <=  bufferpath[dest][bufsize[dest]].pathlen) {
					bufferpath[dest][bufsize[dest]].path = (int *)realloc(bufferpath[dest][bufsize[dest]].path,bufferpath[dest][bufsize[dest]].pathcapacity*2*sizeof(int));
					bufferpath[dest][bufsize[dest]].pathcapacity *= 2;
				}
				bufferpath[dest][bufsize[dest]].path[bufferpath[dest][bufsize[dest]].pathlen++]=src;
				bufsize[dest]++;
				
				residueqty-=buffer[src][bufsize[src]-1].qty;				
				bufsize[src]--; // implicitly removes top element (no need to implement this explicitly - MEMORY LEAK)
			}
		}
		if (residueqty>0) // src did not have enough buffered quantity to relay; give birth to new flow item
		{	
			numtransfers++;
			newentry.origin = src;
			newentry.ts = inter[i].ts; // not used
			newentry.qty = residueqty;
			newpathentry.pathlen = 0; // new path (origin always defines first node of path)
			newpathentry.pathcapacity = 3;
			newpathentry.path = (int *)malloc(newpathentry.pathcapacity*sizeof(int));
			if (bufcapacity[dest] <= bufsize[dest]) {
				buffer[dest] = (struct BufItem *)realloc(buffer[dest], bufcapacity[dest]*2*sizeof(struct BufItem));
				bufferpath[dest] = (struct BufItemPath *)realloc(bufferpath[dest], bufcapacity[dest]*2*sizeof(struct BufItemPath));
				bufcapacity[dest] *= 2;
			}
			buffer[dest][bufsize[dest]] = newentry;
			bufferpath[dest][bufsize[dest]++] = newpathentry;
		}
		
	}

	int sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	int sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
    double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);
    printf("numtransfers=%d\n",numtransfers);


	// print paths
    for(i=0;i<numnodes;i++){
		printf("Buffer of vertex %d:\n",i);
		for(j=0;j<bufsize[i];j++) {
			printf("(o=%d,qty=%.2f) ",buffer[i][j].origin,buffer[i][j].qty);
			printf("path: %d->",buffer[i][j].origin);
			for(k=0;k<bufferpath[i][j].pathlen;k++)
				printf("%d->",bufferpath[i][j].path[k]);
			printf("%d\n",i);
		}
		//printf("\n");
	}


	// count path info
	int totalpathinfo = 0; //total length of all paths
    for(i=0;i<numnodes;i++){
		for(j=0;j<bufsize[i];j++) {
			totalpathinfo += bufferpath[i][j].pathlen;
		}
	}
    printf("total number of path nodeids held=%d\n",totalpathinfo);	
    
    for(i=0;i<numnodes;i++) {
		free(buffer[i]);
		for(j=0;j<bufsize[i];j++)
			free(bufferpath[i][j].path);
		free(bufferpath[i]);
	}
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	free(bufferpath);
	
    return 0;
}


// provenance FIFO model
// first-in first out when propagating quantities
// each node's buffer is managed as a FIFO queue 
int ProvFIFO(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItem **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0

	int *buffirst; // position to 1st item in buffer[i], initially 0
	int *buflast; // position to last item in buffer[i], initially 0
    
	double relayqty;
	double residueqty;
	int src;
	int dest;
	struct BufItem newentry;
	
	int numtransfers = 0;
	
	clock_t t,tnow;
	
	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItem **)malloc(numnodes*sizeof(struct BufItem *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    buffirst = (int *)calloc(numnodes,sizeof(int));
    buflast = (int *)calloc(numnodes,sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        bufcapacity[i]=3; //initial capacity per buffer
        buffer[i] = (struct BufItem *)malloc(bufcapacity[i]*sizeof(struct BufItem));
    }
    
    t = clock();
	    
	for(i=0;i<numinter;i++)
	{
		residueqty=inter[i].qty; // remaining quantity to be transferred
		src = inter[i].src;
		dest = inter[i].dest;
		while (residueqty>0 && bufsize[src])
		{
			numtransfers++;
			if (buffer[src][buflast[src]].qty>residueqty) { // buffer[src][buffirst[src]] is least recently added quantity (fifo queue first)
				//printf("---------------\n");
				//print_queue(buffer[src],bufsize[src],buffirst[src],buflast[src],bufcapacity[src]);
				newentry.origin = buffer[src][buflast[src]].origin;
				newentry.ts = buffer[src][buflast[src]].ts; //not used
				newentry.qty = residueqty;
				//printf("new: %d, %f, %f\n",newentry.origin,newentry.ts,newentry.qty);
				fifoenqueue(newentry,&buffer[dest],&bufsize[dest],&buffirst[dest],&buflast[dest],&bufcapacity[dest]);
				buffer[src][buflast[src]].qty-=residueqty;
				residueqty = 0;
			}
			else { //entire entry must be relayed from src to dest
				fifodequeue(&newentry,buffer[src],&bufsize[src],&buflast[src],bufcapacity[src]);
				fifoenqueue(newentry,&buffer[dest],&bufsize[dest],&buffirst[dest],&buflast[dest],&bufcapacity[dest]);
				residueqty-=newentry.qty;
			}
		}
		if (residueqty>0.0000001) // src did not have enough buffered quantity to relay; give birth to new flow item
		{	
			numtransfers++;
			newentry.origin = src;
			newentry.ts = inter[i].ts;
			newentry.qty = residueqty;
			fifoenqueue(newentry,&buffer[dest],&bufsize[dest],&buffirst[dest],&buflast[dest],&bufcapacity[dest]);
		}

	}

	int sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	int sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
    double sumqty =0;
	for(i=0;i<numnodes;i++)
		sumqty+=sum_fifoelems(buffer[i], bufsize[i], buflast[i], bufcapacity[i]);
    printf("sumqty=%.2f\n",sumqty);
    printf("numtransfers=%d\n",numtransfers);

    for(i=0;i<numnodes;i++){
		printf("Buffer of vertex %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(o=%d,qty=%.2f) ",buffer[i][j].origin,buffer[i][j].qty);
		printf("\n");
	}
    
    for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	free(buffirst);
	free(buflast);
	
    return 0;
}


// add new item to buffer
// if item with same origin exists, update quantity
// fix order of buffer items (based on origin) using an insertion-sort order
int addnewitem(struct BufItemProp *buffer, int *bufsize, struct BufItemProp newentry)
{
	int low=0, hi = (*bufsize)-1;
	int key = newentry.origin;
	int pos;
	
	// binary search for newentry
	while (low <= hi){
      int middle = (low + hi)/2;
      if (buffer[middle].origin == key) {
      	buffer[middle].qty+=newentry.qty;
      	return 0;
      }
      if (buffer[middle].origin < key)
         low = middle + 1;
      else
         hi = middle - 1;
   	}
   	
	// not found -> must be inserted
	(*bufsize)++;
	pos = (*bufsize)-2;
	while (pos>=0 && buffer[pos].origin>key) {
		buffer[pos+1]=buffer[pos];
		pos--;
	}
	buffer[pos+1] = newentry;
	
	return 1;
}

// provenance proportional origin model
// if transferred quantity is lower than buffered quantity 
// then origins are picked proportionally
// creation timestamps are ignored
// Prov Sparse in paper
int ProvProportional(struct Interaction *inter, int numinter, int numnodes)
{
    int i,j,k;

    struct BufItemProp **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
	double *sumbuffered; // total quantity buffered at node i  
	double qty;

	// for merging    
	int a,b,newcap,numelem;
	double transqty;
	struct BufItemProp *newbuffer;

	//double relayqty;
	//double bufferedqty;
	int src;
	int dest;
	struct BufItemProp newentry;
	
	int sumsize,sumcap;
	
	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItemProp **)malloc(numnodes*sizeof(struct BufItemProp *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    sumbuffered = (double *)malloc(numnodes*sizeof( double)); 
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        sumbuffered[i]=0;
        bufcapacity[i]=3;
        buffer[i] = (struct BufItemProp *)malloc(bufcapacity[i]*sizeof(struct BufItemProp));
    }

	clock_t t,tnow;

	t = clock();
	    
	for(i=0;i<numinter;i++)
	{

		if (inter[i].qty>=sumbuffered[inter[i].src]) {
			// case 1: just transfer everything to dest
			if (bufsize[inter[i].dest]==0) {
				// first, check if enough capacity at destination node
				if (bufcapacity[inter[i].dest] < bufsize[inter[i].src]+1) {
					bufcapacity[inter[i].dest] = bufsize[inter[i].src]+1;
					buffer[inter[i].dest] = (struct BufItemProp *)realloc(buffer[inter[i].dest], bufcapacity[inter[i].dest]*sizeof(struct BufItemProp));
				}
				//now copy all to dest
				for(j=0;j<bufsize[inter[i].src];j++)
					buffer[inter[i].dest][j] = buffer[inter[i].src][j];
				bufsize[inter[i].dest] = bufsize[inter[i].src];
			}
			else
			{
				// merge buffer[inter[i].src] into buffer[inter[i].dest]
				
				// 1. calc. size of newbuffer
				a = b = 0;
				numelem = 0;
				while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
					if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
						a++;
					}
					else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
						b++;
					}
					else {
						a++; b++;
					}
					numelem++;						
				}
				if (a<bufsize[inter[i].src]) {
					numelem+=bufsize[inter[i].src]-a;
				}
				else {
					numelem+=bufsize[inter[i].dest]-b;
				}
				
				// 2. do the merge-join
				newcap = numelem+1;
				//newcap = bufsize[inter[i].dest]+bufsize[inter[i].src]+1; 
				newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
				numelem = 0;
				a = b = 0;
				while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
					if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].src][a];
						a++;
					}
					else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].dest][b];
						b++;
					}
					else {
						newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
						newbuffer[numelem++].qty = buffer[inter[i].src][a].qty+buffer[inter[i].dest][b].qty;
						a++; b++;
					}						
				}
				while (a<bufsize[inter[i].src]) {
					newbuffer[numelem++] = buffer[inter[i].src][a];
					a++;
				}
				while (b<bufsize[inter[i].dest]) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				free(buffer[inter[i].dest]);
				buffer[inter[i].dest] = newbuffer;
				bufcapacity[inter[i].dest] = newcap;
				bufsize[inter[i].dest] = numelem;
			}	
				
			bufsize[inter[i].src]=0;
			qty = inter[i].qty-sumbuffered[inter[i].src];
			if (qty>0.00000001) {
					// src did not have enough buffered quantity to relay; give birth to new flow item
					newentry.origin = inter[i].src;
					newentry.qty = qty;
					//printf("newentry.qty=%.2f\n",newentry.qty);
					if (bufsize[inter[i].dest]==0)
						buffer[inter[i].dest][bufsize[inter[i].dest]++] = newentry;
					else
						addnewitem(buffer[inter[i].dest], &bufsize[inter[i].dest], newentry);
					//buffer[inter[i].dest][bufsize[inter[i].dest]++] = newentry;
			}
			sumbuffered[inter[i].src] = 0;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		else {
			// proportional case			

			// 1. calc. size of newbuffer
			a = b = 0;
			numelem = 0;
			while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
				if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
					a++;
				}
				else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
					b++;
				}
				else {
					a++; b++;
				}
				numelem++;						
			}
			if (a<bufsize[inter[i].src]) {
				numelem+=bufsize[inter[i].src]-a;
			}
			else {
				numelem+=bufsize[inter[i].dest]-b;
			}
			
			// 2. do the merge-join
			newcap = numelem+1;			
			newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
			numelem = 0;
			a = b = 0;
			while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
				if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
					newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty;
					buffer[inter[i].src][a].qty -= transqty;
					a++;
				}
				else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				else {
					newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty+buffer[inter[i].dest][b].qty;
					buffer[inter[i].src][a].qty -= transqty;
					a++; b++;
				}						
			}
			while (a<bufsize[inter[i].src]) {
				newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
				transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
				newbuffer[numelem++].qty = transqty;
				buffer[inter[i].src][a].qty -= transqty;
				a++;
			}
			while (b<bufsize[inter[i].dest]) {
				newbuffer[numelem++] = buffer[inter[i].dest][b];
				b++;
			}
			
				
			free(buffer[inter[i].dest]);
			buffer[inter[i].dest] = newbuffer;
			bufcapacity[inter[i].dest] = newcap;
			bufsize[inter[i].dest] = numelem;
			

			sumbuffered[inter[i].src] -= inter[i].qty;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
				
	}

       
	sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
	double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);
    
    
	for(i=0;i<numnodes;i++) {
		printf("Node %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(origin: %d, qty: %.2f) ",buffer[i][j].origin,buffer[i][j].qty);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	free(sumbuffered);
		
    return 0;
}

// Sameas ProvProportional but
// keeps a window of provenance info up to 2*W interactions back
// next window is initiated every W interactions
int ProvProportionalWindow(struct Interaction *inter, int numinter, int numnodes, int W)
{
    int i,j,k;

    struct BufItemProp **buffer; // array of buffers, one for each node of the Graph
    int *bufcapacity; // capacity of each buffer, initially 3
    int *bufsize; // number of items in each buffer, initially 0
	double *sumbuffered; // total quantity buffered at node i  
	double qty;

	// this is for odd windows
    struct BufItemProp **oddbuffer; // array of buffers, one for each node of the Graph
    int *oddbufcapacity; // capacity of each buffer, initially 3
    int *oddbufsize; // number of items in each buffer, initially 0

	// for merging    
	int a,b,newcap,numelem;
	double transqty;
	struct BufItemProp *newbuffer;

	//double relayqty;
	//double bufferedqty;
	int src;
	int dest;
	struct BufItemProp newentry;
	
	int sumsize,sumcap;
	
	int stop = 100<numnodes ? 100:numnodes;

    sumbuffered = (double *)malloc(numnodes*sizeof(double)); 
		    
    buffer = (struct BufItemProp **)malloc(numnodes*sizeof(struct BufItemProp *));
    bufcapacity = (int *)malloc(numnodes*sizeof(int));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        sumbuffered[i]=0;
        bufcapacity[i]=3;
        buffer[i] = (struct BufItemProp *)malloc(bufcapacity[i]*sizeof(struct BufItemProp));
    }

    oddbuffer = (struct BufItemProp **)malloc(numnodes*sizeof(struct BufItemProp *));
    oddbufcapacity = (int *)malloc(numnodes*sizeof(int));
    oddbufsize = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
        oddbufsize[i]=0;
        oddbufcapacity[i]=3;
        oddbuffer[i] = (struct BufItemProp *)malloc(oddbufcapacity[i]*sizeof(struct BufItemProp));
    }

	clock_t t,tnow;

	t = clock();
	    
	for(i=0;i<numinter;i++)
	{		
		if(!(i%W)) {
			if (!(i%(W*2))) {
				//reset buffers
				for(j=0;j<numnodes;j++) {
					if (sumbuffered[j]>0) {
						buffer[j][0].origin = -1;
						buffer[j][0].qty = sumbuffered[j];
						bufsize[j]=1;						
					}
				}
			}
			else
			{
				//reset oddbuffers
				for(j=0;j<numnodes;j++) {
					if (sumbuffered[j]>0) {
						oddbuffer[j][0].origin = -1;
						oddbuffer[j][0].qty = sumbuffered[j];
						oddbufsize[j]=1;		
					}
				}
			}
		}

		//printf("buffqty:%.2f\n",bufferedqty);
		if (inter[i].qty>=sumbuffered[inter[i].src]) {
			// case 1: just transfer everything to dest
			if (bufsize[inter[i].dest]==0) {
				// first, check if enough capacity at destination node
				if (bufcapacity[inter[i].dest] < bufsize[inter[i].src]+1) {
					bufcapacity[inter[i].dest] = bufsize[inter[i].src]+1;
					buffer[inter[i].dest] = (struct BufItemProp *)realloc(buffer[inter[i].dest], bufcapacity[inter[i].dest]*sizeof(struct BufItemProp));
				}
				//now copy all to dest
				for(j=0;j<bufsize[inter[i].src];j++)
					buffer[inter[i].dest][j] = buffer[inter[i].src][j];
				bufsize[inter[i].dest] = bufsize[inter[i].src];
			}
			else
			{
				// merge buffer[inter[i].src] into buffer[inter[i].dest]
				
				// 1. calc. size of newbuffer
				a = b = 0;
				numelem = 0;
				while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
					if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
						a++;
					}
					else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
						b++;
					}
					else {
						a++; b++;
					}
					numelem++;						
				}
				if (a<bufsize[inter[i].src]) {
					numelem+=bufsize[inter[i].src]-a;
				}
				else {
					numelem+=bufsize[inter[i].dest]-b;
				}
				
				// 2. do the merge-join
				newcap = numelem+1;
				//newcap = bufsize[inter[i].dest]+bufsize[inter[i].src]+1; 
				newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
				numelem = 0;
				a = b = 0;
				while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
					if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].src][a];
						a++;
					}
					else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].dest][b];
						b++;
					}
					else {
						newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
						newbuffer[numelem++].qty = buffer[inter[i].src][a].qty+buffer[inter[i].dest][b].qty;
						a++; b++;
					}						
				}
				while (a<bufsize[inter[i].src]) {
					newbuffer[numelem++] = buffer[inter[i].src][a];
					a++;
				}
				while (b<bufsize[inter[i].dest]) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				free(buffer[inter[i].dest]);
				buffer[inter[i].dest] = newbuffer;
				bufcapacity[inter[i].dest] = newcap;
				bufsize[inter[i].dest] = numelem;
			}	
			
			// repeat for oddbuffers
			
			if (oddbufsize[inter[i].dest]==0) {
				// first, check if enough capacity at destination node
				if (oddbufcapacity[inter[i].dest] < oddbufsize[inter[i].src]+1) {
					oddbufcapacity[inter[i].dest] = oddbufsize[inter[i].src]+1;
					oddbuffer[inter[i].dest] = (struct BufItemProp *)realloc(oddbuffer[inter[i].dest], oddbufcapacity[inter[i].dest]*sizeof(struct BufItemProp));
				}
				//now copy all to dest
				for(j=0;j<oddbufsize[inter[i].src];j++)
					oddbuffer[inter[i].dest][j] = oddbuffer[inter[i].src][j];
				oddbufsize[inter[i].dest] = oddbufsize[inter[i].src];
			}
			else
			{
				// merge buffer[inter[i].src] into buffer[inter[i].dest]
				
				// 1. calc. size of newbuffer
				a = b = 0;
				numelem = 0;
				while (a<oddbufsize[inter[i].src] && b<oddbufsize[inter[i].dest]) {
					if (oddbuffer[inter[i].src][a].origin<oddbuffer[inter[i].dest][b].origin) {
						a++;
					}
					else if (oddbuffer[inter[i].src][a].origin>oddbuffer[inter[i].dest][b].origin) {
						b++;
					}
					else {
						a++; b++;
					}
					numelem++;						
				}
				if (a<oddbufsize[inter[i].src]) {
					numelem+=oddbufsize[inter[i].src]-a;
				}
				else {
					numelem+=oddbufsize[inter[i].dest]-b;
				}
				
				// 2. do the merge-join
				newcap = numelem+1;
				//newcap = bufsize[inter[i].dest]+bufsize[inter[i].src]+1; 
				newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
				numelem = 0;
				a = b = 0;
				while (a<oddbufsize[inter[i].src] && b<oddbufsize[inter[i].dest]) {
					if (oddbuffer[inter[i].src][a].origin<oddbuffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = oddbuffer[inter[i].src][a];
						a++;
					}
					else if (oddbuffer[inter[i].src][a].origin>oddbuffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = oddbuffer[inter[i].dest][b];
						b++;
					}
					else {
						newbuffer[numelem].origin = oddbuffer[inter[i].dest][b].origin;
						newbuffer[numelem++].qty = oddbuffer[inter[i].src][a].qty+oddbuffer[inter[i].dest][b].qty;
						a++; b++;
					}						
				}
				while (a<oddbufsize[inter[i].src]) {
					newbuffer[numelem++] = oddbuffer[inter[i].src][a];
					a++;
				}
				while (b<oddbufsize[inter[i].dest]) {
					newbuffer[numelem++] = oddbuffer[inter[i].dest][b];
					b++;
				}
				free(oddbuffer[inter[i].dest]);
				oddbuffer[inter[i].dest] = newbuffer;
				oddbufcapacity[inter[i].dest] = newcap;
				oddbufsize[inter[i].dest] = numelem;
			}
				
			bufsize[inter[i].src]=0;
			oddbufsize[inter[i].src]=0;
			qty = inter[i].qty-sumbuffered[inter[i].src];
			if (qty>0.00000001) {
					// src did not have enough buffered quantity to relay; give birth to new flow item
					newentry.origin = inter[i].src;
					newentry.qty = qty;
					if (bufsize[inter[i].dest]==0)
						buffer[inter[i].dest][bufsize[inter[i].dest]++] = newentry;
					else
						addnewitem(buffer[inter[i].dest], &bufsize[inter[i].dest], newentry);
	
					// src did not have enough buffered quantity to relay; give birth to new flow item
					if (oddbufsize[inter[i].dest]==0)
						oddbuffer[inter[i].dest][oddbufsize[inter[i].dest]++] = newentry;
					else
						addnewitem(oddbuffer[inter[i].dest], &oddbufsize[inter[i].dest], newentry);
			}

			sumbuffered[inter[i].src] = 0;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		else {
			// proportional case			

			// 1. calc. size of newbuffer
			a = b = 0;
			numelem = 0;
			while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
				if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
					a++;
				}
				else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
					b++;
				}
				else {
					a++; b++;
				}
				numelem++;						
			}
			if (a<bufsize[inter[i].src]) {
				numelem+=bufsize[inter[i].src]-a;
			}
			else {
				numelem+=bufsize[inter[i].dest]-b;
			}
			
			// 2. do the merge-join
			newcap = numelem+1;			
			//newcap = bufsize[inter[i].dest]+bufsize[inter[i].src]; 
			newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
			numelem = 0;
			a = b = 0;
			while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
				if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
					newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty;
					buffer[inter[i].src][a].qty -= transqty;
					a++;
				}
				else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				else {
					newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty+buffer[inter[i].dest][b].qty;
					buffer[inter[i].src][a].qty -= transqty;
					a++; b++;
				}						
			}
			while (a<bufsize[inter[i].src]) {
				newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
				transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
				newbuffer[numelem++].qty = transqty;
				buffer[inter[i].src][a].qty -= transqty;
				a++;
			}
			while (b<bufsize[inter[i].dest]) {
				newbuffer[numelem++] = buffer[inter[i].dest][b];
				b++;
			}
			
				
			free(buffer[inter[i].dest]);
			buffer[inter[i].dest] = newbuffer;
			bufcapacity[inter[i].dest] = newcap;
			bufsize[inter[i].dest] = numelem;
			

			// proportional case	
			// repeat for odd buffers		

			// 1. calc. size of newbuffer
			a = b = 0;
			numelem = 0;
			while (a<oddbufsize[inter[i].src] && b<oddbufsize[inter[i].dest]) {
				if (oddbuffer[inter[i].src][a].origin<oddbuffer[inter[i].dest][b].origin) {
					a++;
				}
				else if (oddbuffer[inter[i].src][a].origin>oddbuffer[inter[i].dest][b].origin) {
					b++;
				}
				else {
					a++; b++;
				}
				numelem++;						
			}
			if (a<oddbufsize[inter[i].src]) {
				numelem+=oddbufsize[inter[i].src]-a;
			}
			else {
				numelem+=oddbufsize[inter[i].dest]-b;
			}

			
			// 2. do the merge-join
			newcap = numelem+1;			
			newbuffer = (struct BufItemProp *)malloc(newcap*sizeof(struct BufItemProp));
			numelem = 0;
			a = b = 0;
			while (a<oddbufsize[inter[i].src] && b<oddbufsize[inter[i].dest]) {
				if (oddbuffer[inter[i].src][a].origin<oddbuffer[inter[i].dest][b].origin) {
					newbuffer[numelem].origin = oddbuffer[inter[i].src][a].origin;
					transqty = inter[i].qty*oddbuffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty;
					oddbuffer[inter[i].src][a].qty -= transqty;
					a++;
				}
				else if (oddbuffer[inter[i].src][a].origin>oddbuffer[inter[i].dest][b].origin) {
					newbuffer[numelem++] = oddbuffer[inter[i].dest][b];
					b++;
				}
				else {
					newbuffer[numelem].origin = oddbuffer[inter[i].dest][b].origin;
					transqty = inter[i].qty*oddbuffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty+oddbuffer[inter[i].dest][b].qty;
					oddbuffer[inter[i].src][a].qty -= transqty;
					a++; b++;
				}						
			}
			while (a<oddbufsize[inter[i].src]) {
				newbuffer[numelem].origin = oddbuffer[inter[i].src][a].origin;
				transqty = inter[i].qty*oddbuffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
				newbuffer[numelem++].qty = transqty;
				oddbuffer[inter[i].src][a].qty -= transqty;
				a++;
			}
			while (b<oddbufsize[inter[i].dest]) {
				newbuffer[numelem++] = oddbuffer[inter[i].dest][b];
				b++;
			}
			
			free(oddbuffer[inter[i].dest]);
			oddbuffer[inter[i].dest] = newbuffer;
			oddbufcapacity[inter[i].dest] = newcap;
			oddbufsize[inter[i].dest] = numelem;


			sumbuffered[inter[i].src] -= inter[i].qty;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}

		
	}
       
	sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

	sumcap =0;
	for(i=0;i<numnodes;i++)
       sumcap+=bufcapacity[i];
    printf("sumcapacity=%d\n",sumcap);
    
	double sumqty =0;
	for(i=0;i<numnodes;i++)
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
    printf("sumqty=%.2f\n",sumqty);
    
	for(i=0;i<numnodes;i++) {
		printf("Node %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(origin: %d, qty: %.2f) ",buffer[i][j].origin,buffer[i][j].qty);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(bufcapacity);
	free(buffer);
	free(sumbuffered);
		
    return 0;
}

int cmpbyqtydesc(const void *i1, const void *i2)
{
	struct BufItemProp *a = (struct BufItemProp *)i1;
    struct BufItemProp *b = (struct BufItemProp *)i2;
    
	if (a->qty < b->qty)
		return 1;
	else if (a->qty > b->qty)
		return -1;
	else
		return 0;  
}

int cmpbyorigin (const void *i1, const void *i2) {
	struct BufItemProp *a = (struct BufItemProp *)i1;
    struct BufItemProp *b = (struct BufItemProp *)i2;
    
    return(a->origin - b->origin);
}

// shrink a sparse proportional buffer to keep only the top-reducedsize elements 
// puts total deleted qty in -1 (artificial) vertex
int shrinkbuffer(struct BufItemProp *buffer, int bufsize, int reducedsize)
{
	int i,j;
	double residueqty;
	qsort(buffer, bufsize, sizeof(struct BufItemProp), cmpbyqtydesc);
	residueqty = 0.0;
	for (i=reducedsize; i<bufsize; i++)
		residueqty += buffer[i].qty;
	for (i=0; i<reducedsize; i++)
		if (buffer[i].origin == -1)
			break;
	if (i<reducedsize) // broken => found
		buffer[i].qty += residueqty;
	else {
		buffer[reducedsize].origin = -1;
		buffer[reducedsize++].qty = residueqty;
	}
	qsort(buffer, reducedsize, sizeof(struct BufItemProp), cmpbyorigin);
	return reducedsize;
}

// shrink a sparse proportional buffer to keep only the top-reducedsize elements 
// puts total deleted qty in selfid vertex
int shrinkbufferselfid(struct BufItemProp *buffer, int bufsize, int selfid, int reducedsize)
{
	int i,j;
	double residueqty;
	qsort(buffer, bufsize, sizeof(struct BufItemProp), cmpbyqtydesc);
	residueqty = 0.0;
	for (i=reducedsize; i<bufsize; i++)
		residueqty += buffer[i].qty;
	for (i=0; i<reducedsize; i++)
		if (buffer[i].origin == selfid)
			break;
	if (i<reducedsize) // broken => found
		buffer[i].qty += residueqty;
	else {
		buffer[reducedsize].origin = selfid;
		buffer[reducedsize++].qty = residueqty;
	}
	qsort(buffer, reducedsize, sizeof(struct BufItemProp), cmpbyorigin);
	return reducedsize;
}

// same as ProvProportional, but takes as input a budget B for the buffer vector of each node
// if the budget is reached then (budget-reduced) of the buffer entries with the smallest quantities are
// assumed to originate from the vertex itself, in order to make room 
int ProvProportionalBudget(struct Interaction *inter, int numinter, int numnodes, int budget, int reducedsize)
{
    int i,j,k;

    struct BufItemProp **buffer; // array of buffers, one for each node of the Graph
    int *bufsize; // number of items in each buffer, initially 0
	double *sumbuffered; // total quantity buffered at node i  
	double qty;

	int *lastbufshrink; // marks last time each provenance info is shrunk at each node   
	int *numbufshrinks; // marks number of times provenance info is shrunk at each node   

	// for merging    
	int a,b,newcap,numelem;
	double transqty;
	struct BufItemProp *newbuffer;

	//double relayqty;
	//double bufferedqty;
	int src;
	int dest;
	struct BufItemProp newentry;
	
	int sumsize,sumcap;
	
	int stop = 100<numnodes ? 100:numnodes;
		    
    buffer = (struct BufItemProp **)malloc(numnodes*sizeof(struct BufItemProp *));
    bufsize = (int *)malloc(numnodes*sizeof(int));
    lastbufshrink = (int *)calloc(numnodes,sizeof(int)); // reset to 0 for all nodes
    numbufshrinks = (int *)calloc(numnodes,sizeof(int)); // reset to 0 for all nodes
    sumbuffered = (double *)malloc(numnodes*sizeof(double)); 
    for(i=0;i<numnodes;i++) {
        bufsize[i]=0;
        sumbuffered[i]=0;
        buffer[i] = (struct BufItemProp *)malloc(budget*sizeof(struct BufItemProp));
    }

	newbuffer = (struct BufItemProp *)malloc((2*budget+1)*sizeof(struct BufItemProp));

	clock_t t,tnow;

	t = clock();
	    
	for(i=0;i<numinter;i++)
	{
		if (inter[i].qty>=sumbuffered[inter[i].src]) {
			// case 1: just transfer everything to dest
			if (bufsize[inter[i].dest]==0) {
				if (bufsize[inter[i].src]+1 > budget) {
					// copy to new buffer
					for(j=0;j<bufsize[inter[i].src];j++)
						newbuffer[j] = buffer[inter[i].src][j];
					
					// shrink newbuffer by keeping top-reducedsize quantities and put total residue to inter[i].dest 
					numelem = shrinkbuffer(newbuffer,bufsize[inter[i].src],reducedsize);
					lastbufshrink[inter[i].dest]=i; // mark time of buffer shrinking (i.e., info loss)
					numbufshrinks[inter[i].dest]++; 
					
					for(j=0;j<numelem;j++)
						buffer[inter[i].dest][j] = newbuffer[j];
					bufsize[inter[i].dest] = numelem;
				}
				else {
					//just copy all to dest
					for(j=0;j<bufsize[inter[i].src];j++)
						buffer[inter[i].dest][j] = buffer[inter[i].src][j];
					bufsize[inter[i].dest] = bufsize[inter[i].src];
				}
			}
			else
			{
				// merge buffer[inter[i].src] into buffer[inter[i].dest]
				numelem = 0;
				a = b = 0;
				while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
					if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].src][a];
						a++;
					}
					else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
						newbuffer[numelem++] = buffer[inter[i].dest][b];
						b++;
					}
					else {
						newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
						newbuffer[numelem++].qty = buffer[inter[i].src][a].qty+buffer[inter[i].dest][b].qty;
						a++; b++;
					}						
				}
				while (a<bufsize[inter[i].src]) {
					newbuffer[numelem++] = buffer[inter[i].src][a];
					a++;
				}
				while (b<bufsize[inter[i].dest]) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				
				if (numelem>budget-1)
				{
					numelem = shrinkbuffer(newbuffer,numelem,reducedsize);
					lastbufshrink[inter[i].dest]=i; // mark time of buffer shrinking (i.e., info loss)
					numbufshrinks[inter[i].dest]++; 	
				}
				
				for(j=0;j<numelem;j++)
					buffer[inter[i].dest][j] = newbuffer[j];
				bufsize[inter[i].dest] = numelem;
			}	
				
			bufsize[inter[i].src]=0;
			qty = inter[i].qty-sumbuffered[inter[i].src];
			if (qty>0.00000001) {
					// src did not have enough buffered quantity to relay; give birth to new flow item
					newentry.origin = inter[i].src;
					newentry.qty = qty;
					if (bufsize[inter[i].dest]==0)
						buffer[inter[i].dest][bufsize[inter[i].dest]++] = newentry;
					else
						addnewitem(buffer[inter[i].dest], &bufsize[inter[i].dest], newentry);
			}
			sumbuffered[inter[i].src] = 0;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		else {
			// proportional case			
			// merge into dest buffer
			numelem = 0;
			a = b = 0;
			while (a<bufsize[inter[i].src] && b<bufsize[inter[i].dest]) {
				if (buffer[inter[i].src][a].origin<buffer[inter[i].dest][b].origin) {
					newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty;
					buffer[inter[i].src][a].qty -= transqty;
					a++;
				}
				else if (buffer[inter[i].src][a].origin>buffer[inter[i].dest][b].origin) {
					newbuffer[numelem++] = buffer[inter[i].dest][b];
					b++;
				}
				else {
					newbuffer[numelem].origin = buffer[inter[i].dest][b].origin;
					transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
					newbuffer[numelem++].qty = transqty+buffer[inter[i].dest][b].qty;
					buffer[inter[i].src][a].qty -= transqty;
					a++; b++;
				}						
			}
			//printf("a=%d,b=%d\n",a,b);
			while (a<bufsize[inter[i].src]) {
				newbuffer[numelem].origin = buffer[inter[i].src][a].origin;
				transqty = inter[i].qty*buffer[inter[i].src][a].qty/sumbuffered[inter[i].src];
				newbuffer[numelem++].qty = transqty;
				buffer[inter[i].src][a].qty -= transqty;
				a++;
			}
			while (b<bufsize[inter[i].dest]) {
				newbuffer[numelem++] = buffer[inter[i].dest][b];
				b++;
			}
			
			if (numelem>budget)
			{
				// shrink newbuffer by keeping top-reducedsize quantities and put total residue to inter[i].dest 
				numelem = shrinkbuffer(newbuffer,numelem,reducedsize);
				lastbufshrink[inter[i].dest]=i; // mark time of buffer shrinking (i.e., info loss)
				numbufshrinks[inter[i].dest]++; 
			}
			
			for(j=0;j<numelem;j++)
				buffer[inter[i].dest][j] = newbuffer[j];
			bufsize[inter[i].dest] = numelem;
			
			sumbuffered[inter[i].src] -= inter[i].qty;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		
	}


	sumsize =0;
	for(i=0;i<numnodes;i++)
       sumsize+=bufsize[i];
    printf("sumsize=%d\n",sumsize);

    int nonemptybufs = 0;
    int totshrinking = 0;
    int numshrunk = 0;
	double sumqty =0;
	for(i=0;i<numnodes;i++) {
		for(j=0;j<bufsize[i];j++)
       		sumqty+=buffer[i][j].qty;
       	if(bufsize[i]>0) {
       		nonemptybufs++;
       		totshrinking += numbufshrinks[i];
       		if (numbufshrinks[i]) numshrunk++;
       	}
    }
    printf("sumqty=%.2f\n",sumqty);
    printf("number of non-empty buffers=%d\n",nonemptybufs);
    printf("number of nodes whose buffer was shrunk at least once=%d\n",numshrunk);
    printf("average number of shrinks at non-empty buffers=%.2f\n",(double)totshrinking/nonemptybufs);
    
	for(i=0;i<numnodes;i++) {
		printf("Node %d: ",i);
		for(j=0;j<bufsize[i];j++)
			printf("(origin: %d, qty: %.2f) ",buffer[i][j].origin,buffer[i][j].qty);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(bufsize);
	free(buffer);
	free(sumbuffered);
	free(newbuffer);
	free(lastbufshrink);
	free(numbufshrinks);
    return 0;
}


// provenance proportional origin model
// Proportional Dense model
// works for selected origins only
// to use for all vertices, select all of them in array
// if transferred quantity is lower than buffered quantity 
// then origins are picked proportionally
// creation timestamps are ignored
int ProvProportionalSel(struct Interaction *inter, int numinter, int numnodes, int *selectednodes, int numselected)
{
    int i,j,k;

    double **buffer; // array of buffers, one for each node of the Graph
	double *nonselectedqty; // quantity originating from non-selected nodes  
	double *sumbuffered; // total quantity buffered at node i  
	int *map; // map[i] is position of node i in selectednodes (-1 if it is not there)
	
	double qty; 
	    
	int stop = 100<numnodes ? 100:numnodes;

    buffer = (double **)malloc(numnodes*sizeof( double *)); 
    sumbuffered = (double *)malloc(numnodes*sizeof( double)); 
    nonselectedqty = (double *)malloc(numnodes*sizeof(double));
    
    map = (int *)malloc(numnodes*sizeof(int));
    for(i=0;i<numnodes;i++) {
    	map[i]=-1;
		// each buffer has exactly numselected entries plus one quantity originating from non-selected nodes   
        buffer[i] = (double *)malloc((numselected)*sizeof(double));
        for(j=0;j<numselected;j++)
        	buffer[i][j] = 0.0;
        sumbuffered[i]=0.0;
        nonselectedqty[i] = 0.0;
    }
    
    // map each node-id to a position in selectednodes or to -1 if it does not exist there
	for(i=0; i<numselected; i++)
    	map[selectednodes[i]]=i;    
        	
	for(i=0;i<numinter;i++)
	{
		if (inter[i].qty>=sumbuffered[inter[i].src]) {
			// case 1: just transfer everything to dest
			for(j=0;j<numselected;j++) {
				buffer[inter[i].dest][j] += buffer[inter[i].src][j];
				buffer[inter[i].src][j] = 0.0;
			}
			nonselectedqty[inter[i].dest] += nonselectedqty[inter[i].src];
			nonselectedqty[inter[i].src] = 0.0;
			qty = inter[i].qty-sumbuffered[inter[i].src];
			if (qty>0) {
				if (map[inter[i].src]!=-1) // inter[i].src in selected
					buffer[inter[i].dest][map[inter[i].src]] += qty;
				else
					nonselectedqty[inter[i].dest] += qty;
			}
			sumbuffered[inter[i].src] = 0;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		else { //inter[i].qty < sumbuffered[inter[i].src]
			for(j=0;j<numselected;j++)
			{
				qty = inter[i].qty*buffer[inter[i].src][j]/sumbuffered[inter[i].src];
				buffer[inter[i].dest][j] += qty;
				buffer[inter[i].src][j] -= qty;
			}
			qty = inter[i].qty*nonselectedqty[inter[i].src]/sumbuffered[inter[i].src];
			nonselectedqty[inter[i].dest] += qty;
			nonselectedqty[inter[i].src] -= qty;
			
			sumbuffered[inter[i].src] -= inter[i].qty;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
	}

	// correctness check to compare with noProv and see whether the final total quantities 
	// at the buffers are the same
	       
	double sumqty =0;
	for(i=0;i<numnodes;i++) {
		for(j=0;j<numselected;j++)
       		sumqty+=buffer[i][j];
       	sumqty+=nonselectedqty[i];
    }   	
    printf("sumqty=%.2f\n",sumqty);

	for(i=0;i<numnodes;i++) {
		printf("Node %d: ",i);
		for(j=0;j<numselected;j++)
			printf("%.2f ",buffer[i][j]);
		printf("\n");
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(buffer);
	free(sumbuffered);
	free(nonselectedqty);
	free(map);
	
    return 0;
}


// provenance proportional origin model
// if transferred quantity is lower than buffered quantity 
// then origins are picked proportionally
// creation timestamps are ignored
// assumes that vertices are partitioned to groups (clusters?)
// measures provenance from each group 
// map[i] = group id whereto vertex i is mapped
int ProvProportionalGroup(struct Interaction *inter, int numinter, int numnodes, int *map, int numgroups)
{
    int i,j,k;

    double **buffer; // array of buffers, one for each group
	double *sumbuffered; // total quantity buffered at node i  
	
	double qty; 
	    
	int stop = 100<numnodes ? 100:numnodes;

	//buffer[i][j] is the buffered qty at node i originating from group j	
    buffer = (double **)malloc(numnodes*sizeof( double *)); 
    //sumbuffered[i] is total qty buffered at node i
    sumbuffered = (double *)malloc(numnodes*sizeof( double)); 
    //nonselectedqty = (double *)malloc(numnodes*sizeof(double));
    
    for(i=0;i<numnodes;i++) {
		// each buffer has exactly numgroups entries    
        buffer[i] = (double *)malloc((numgroups)*sizeof(double));
        for(j=0;j<numgroups;j++)
        	buffer[i][j] = 0.0;
        sumbuffered[i]=0.0;
    }

	for(i=0;i<numinter;i++)
	{
		if (inter[i].qty>=sumbuffered[inter[i].src]) {
			// case 1: just transfer everything to dest
			for(j=0;j<numgroups;j++) {
				buffer[inter[i].dest][j] += buffer[inter[i].src][j];
				buffer[inter[i].src][j] = 0.0;
			}
			qty = inter[i].qty-sumbuffered[inter[i].src];
			if (qty>0)
				buffer[inter[i].dest][map[inter[i].src]] += qty;
			sumbuffered[inter[i].src] = 0;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
		else { //inter[i].qty < sumbuffered[inter[i].src]
			//proportional transfer case
			for(j=0;j<numgroups;j++)
			{
				qty = inter[i].qty*buffer[inter[i].src][j]/sumbuffered[inter[i].src];
				buffer[inter[i].dest][j] += qty;
				buffer[inter[i].src][j] -= qty;
			}
			
			sumbuffered[inter[i].src] -= inter[i].qty;
			sumbuffered[inter[i].dest] += inter[i].qty;
		}
	}
       
	double sumqty =0;
	for(i=0;i<numnodes;i++) {
		for(j=0;j<numgroups;j++)
       		sumqty+=buffer[i][j];
    }   	
    printf("sumqty=%.2f\n",sumqty);
	
	
	for(i=0;i<numnodes;i++) {
		if (sumbuffered[i]>0) {
			printf("%d: ",i);
			for(j=0;j<numgroups;j++)
				printf("%.2f ",buffer[i][j]);
			printf("\n");
		}
	}
	
	for(i=0;i<numnodes;i++)
		free(buffer[i]);
	free(buffer);
	free(sumbuffered);
	
    return 0;
}

int main(int argc, char **argv)
{
	int i,j,k;
	int W; // window size
	int budget, reduction; // for budgetProvProp
	FILE *f; // graph input file

    struct Interaction *inter = NULL;
	int numnodes=0;
	int numinter=0;
	
	int *selected = NULL;
	int numselected;
	
    clock_t t;
    double time_taken;
    
	int numgroups; // number of groups whereto vertices are assigned
    int *map; //maps vertex-ids to groups (group-ids should be from 0 to numgroups-1)
    
    if (argc < 3) {
    	//printf("arguments: <graph file> <k for topk origin provenance OR numgroups> <Window size (for sliding prov.)> <budget for BudgetProv> <reduction for BudgetProv>\n");
    	printf("arguments: <graph file> <method> (method arguments)\n");
    	return -1;
    }
    
    f = fopen(argv[1],"r");

	if ((readGraph(f, &inter, &numinter, &numnodes)))
	{
		printf("something went wrong while reading graph file\n");
    	return -1;
    }
    
    int method = atoi(argv[2]);
    
    switch(method)
    {
    	case 0:
    	printf("\nNoProvenance model starts\n"); 
		t = clock(); 
		if (noProvFromMem(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("NoProvenance: Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 1:
		printf("\nProvOldestFirst (least recently born) model starts\n"); 
		t = clock(); 
		if (ProvOldestFirst(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvOldestFirst (least recently born): Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 2:
		printf("\nProvNewestFirst (most recently born) model starts\n"); 
		t = clock(); 
		if (ProvNewestFirst(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvNewestFirst (most recently born): Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 3:
		printf("\nProvLIFO model starts\n"); 
		t = clock(); 
		if (ProvLIFO(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvLIFO: Total time of execution: %f seconds\n", time_taken);
		break;
			
		case 4:
		printf("\nProvFIFO model starts\n"); 
		t = clock(); 
		if (ProvFIFO(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvFIFO: Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 31:
		printf("\nProvLIFOPaths model starts\n"); 
		t = clock(); 
		if (ProvLIFOPaths(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvLIFOPaths: Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 100:
		// use ProvProportionalGroup to simulate ProvPropDense
		// if size of each group is one then each group is one vertex
		printf("\nProvProportional model starts\n");
		//initialize groups
		map = (int *)malloc(numnodes*sizeof(int));	
		numgroups = numnodes;
		//assignment by a simple hash function (should be replaced by something else)
		for(i=0;i<numnodes;i++)
			map[i] = i % numgroups;
		t = clock(); 
		if (ProvProportionalGroup(inter, numinter, numnodes, map, numgroups)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportional: Total time of execution: %f seconds\n", time_taken);
		free(map);
		break;

		case 101:
		printf("\nProvProportional Sparse model starts\n"); 
		t = clock(); 
		if (ProvProportional(inter, numinter, numnodes)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportional Sparse: Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 110:
		if (argc != 4) {
			printf("arguments: <graph file> <method> <numselected>\n");
    		return -1;
		}
		numselected = atoi(argv[3]); // number of selected vertices
		selected = (int *)malloc(numselected*sizeof(int));
		for (i=0; i<numselected; i++) // first numselected vertices are selected
			selected[i] = i;
		printf("\nProvProportional Selective model starts\n"); 
		t = clock(); 
		if (ProvProportionalSel(inter, numinter, numnodes, selected, numselected)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportionalSel: Total time of execution: %f seconds\n", time_taken);
		break;
		
		case 111:
		printf("\nProvProportional Grouping model starts\n"); 
		if (argc != 4) {
			printf("arguments: <graph file> <method> <numgroups>\n");
    		return -1;
		}
		numgroups = atoi(argv[3]); 
		//initialize groups
		map = (int *)malloc(numgroups*sizeof(int));	
		//assignment by a simple hash function 
		for(i=0;i<numnodes;i++)
			map[i] = i % numgroups;
		t = clock(); 
		if (ProvProportionalGroup(inter, numinter, numnodes, map, numgroups)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportionalGroup: Total time of execution: %f seconds\n", time_taken);
		free(map);
		break;
	
		case 120:
		if (argc != 4) {
			printf("arguments: <graph file> <method> <W>\n");
    		return -1;
		}
		W = atoi(argv[3]); 
		printf("\nProvProportional Window model starts\n"); 
		t = clock(); 
		if (ProvProportionalWindow(inter, numinter, numnodes,W)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportionalWindow: Total time of execution: %f seconds\n", time_taken);
		break;

		case 121:
		if (argc != 5) {
			printf("arguments: <graph file> <method> <budget> <reduction>\n");
    		return -1;
		}
		budget = atoi(argv[3]); 
		reduction = atoi(argv[4]); 
		printf("\nProvProportional Budget model starts\n"); 
		t = clock(); 
		if (ProvProportionalBudget(inter, numinter, numnodes, budget, reduction)==-1)
			return -1;
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("ProvProportionalBudget: Total time of execution: %f seconds\n", time_taken);
		break;
		
		default:
		printf("Invalid method. Choices are:\n");
		printf("0: no provenance\n");
		printf("1: least recently born\n");
		printf("2: most recently born\n");
		printf("3: LIFO\n");
		printf("4: FIFO\n");
		printf("31: LIFO with path tracking\n");
		printf("100: proportional (dense vectors)\n");
		printf("101: proportional (sparse vectors)\n");
		printf("110: proportional (from selected vertices)\n");
		printf("111: proportional (from groups of vertices)\n");
		printf("120: proportional (window-based)\n");
		printf("121: proportional (budget-based)\n");
    	return -1;
    }
    

	if (inter!=NULL) 
		free(inter);

	if (selected!=NULL) 
		free(selected);

	return 0;
}
