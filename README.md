# ICDE-2022 Provenance in Temporal Interaction Networks (code)

Find the paper here: https://www.cs.uoi.gr/~nikos/icde22.pdf

Find our bitcoin dataset here: https://www.kaggle.com/chrysanthikosyfaki/datasets

Instructions for compiling and running the code

1) run make at your terminal to compile the code (compilation is done using gcc with -O3 flag)

2) To use the code, you will need a directed graph file in the following format:
- numberofvertices
- numberofinteractions
- src1 dest1 time1 flow1...
- src2 dest2 time2 flow2...
- src3 dest3 time3 flow3...
...

An example graph file (graph.txt) is given in this distribution. In addition, you can use a real dataset we used in our experimental analysis (taxis_sort.txt).

3) Running ./provenance_tin <graph file> <method> (method arguments)
- the following algorithms are run and their provenance information is shown at the output
  
  
| Parameters | Algorithms |
| ------ | ------ |
| 0 | No Provenance (baseline) |
| 1 |	Least Recently Born |
| 2 |	Most Recently Born |
| 4 |	FIFO |
| 31 |	LIFO with path tracking |
| 100 |	Proportional (Dense Vectors) |
| 101 |	Proportional (Sparse Vectors) | 
| 110 |	Proportional (From Selected Vertices) |
| 111 |	Proportional (From Groups of Vertices) |
| 120 |	Window-based Proportional |
| 121 |	Budget-based Proportional |

Example of execution:
- ` ./provenance_tin graph.txt 0 `
- ` ./provenance_tin graph.txt 1 `
- ` ./provenance_tin graph.txt 2 `
- ` ./provenance_tin graph.txt 3 `
- ` ./provenance_tin graph.txt 4 `
- ` ./provenance_tin graph.txt 31 `
- ` ./provenance_tin graph.txt 100 `
- ` ./provenance_tin graph.txt 101 `
- ` ./provenance_tin graph.txt 110 2 `
- ` ./provenance_tin graph.txt 111 2 `
- ` ./provenance_tin graph.txt 120 3 `
- ` ./provenance_tin graph.txt 121 3 2 `
