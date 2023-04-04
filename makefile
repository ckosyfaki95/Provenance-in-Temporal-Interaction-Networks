CC       = gcc
CCOPTS   = -c -O3
LINK     = gcc

.c.o: 
	$(CC) $(CCOPTS) $<

all: provenance_tin

minheap.o: minheap.c

fifoqueue.o: fifoqueue.c

provenance_tin.o: provenance_tin.c

provenance_tin: provenance_tin.o minheap.o fifoqueue.o
	$(LINK) -o provenance_tin provenance_tin.o minheap.o fifoqueue.o
clean:
	rm *o provenance_tin

