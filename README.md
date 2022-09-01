# A-Star Project

## Introduction
A* is a widely used pathfinding algorithm for traversing Graphs; it extends Dijkstra's algorithm by introducing the concept of **Heuristic Function** which plays a key role in making more optimal decisions at each step.


The project was developed in **C** on a **Unix-like** architecture and implements several versions of this algorithm, both **sequential** and **parallel** ones, with the goal of testing one against the other and getting a final result that summarises their overall performances.

## Compilation Guide
The code can be compiled by means of a single **Makefile** in four different ways:

1. **make**: Standard compilation that produces an executable named **AStar** useful for executing the different versions of the algorithm and visualizing the paths computed by the latter.  

2. **make debug**: Produces an executable file named **AStar_debug** optimal for debugging purposes as during execution it will print on screen the most important steps performed by the algorithm.

3. **make time**: Produces an executable file named **AStar_time** very similar to **AStar** one; in addition to the latter it also shows the total time of execution of the different versions of the algorithm.

4. **make test**: Will produce an executable named **Test** used to compare one another completion times, correctness and several other benchmarks resulting from the execution of all the implementations of A*.


## Execution Guide
When executed, **AStar**, **AStar_debug** and **AStar_time**, will show the following menu.

```
Operations on weighted directed graphs
===============
1. Load graph from file (sequential)
2. Load graph from file (parallel)
---
3. Shortest path with sequential A*
4. Shortest path with SPA*
5. Shortest path with SPA* Version 2
6. Shortest path with HDA (Courirer Master)
7. Shortest path with HDA (NO Courirer Master)
8. Free graph and exit
===============
Enter your choice :
```

### Graph Loading
At first it will be neccessary to load a Graph into memory; this is made possible by **options 1 and 2** that will ask for the **.bin** file to be loaded and also if we want to start the **numbering of nodes from 0 or 1**. In addition **option 2** will also ask for the number of threads that are to be used for the load of the graph.

### Graph .bin Files format
In order for a .bin file to be used for the representation of a graph, it has to follow specific format rules defined by us:

- **First row** must be an **integer N** (4 Bytes) that specifies the total amount of nodes in the graph

- The following **N rows** have to define the **cordinates of each node in this way**:
    + one **float** (4 Bytes) for the first cordinate.
    + one **float** (4 Bytes) for the second cordinate.


-  Succeeding the list of nodes, it has to be represented the **list of edges** that will reach until the end of the file; **edges must be represented in the following way**:

    + one **int** (4 Bytes) for the precursor of the edge
    + one **int** (4 Bytes) for the successor of the edge
    + one **float** (4 Bytes) for the weight of the edge

### Execution of a version of A*
Once a Graph has been loaded it will be possible to choose among one of the A* versions to be executed (from **option 3 to option 7**); each version will require some specific settings to be chosen by the user, prior to the execution of the algorithm.

### Settings of A* implementation
The available settings introduced above will be now described thoroughly:

- **Start** and **End** nodes: respectively the nodes for which a path has to be searched.
```
Enter your choice : 4

Insert starting node = 2
Insert destination node = 10
```

- **Number of threads**: parameter required only for the concurrent implementations of A* (**options from 4 to 7**), it defines the number of threads to be used for the path finding.

```
Insert number of threads: 4
```

- **Type of search**: this parameter defines the type of search that will be performed by the algorithm inside the builtin **priority queue**.
    
    + **LINEAR SEARCH**: available in all versions, performs a single-threaded search in O(n)
    + **CONSTANT SEARCH** available in all versions, performs a search in O(1) but it requires an additional in-memory data structure.  

    + **PARALLEL SEARCH**: available only in sequential versions of the algorithm (**option 3**) performs a multi-threaded search, the amount of threads generated will be the same as the amount of cores available in the machine.

    ```
    Which kind of search do you want to perform?
        1 -> LINEAR SEARCH
        2 -> CONSTANT SEARCH
        3 -> PARALLEL SEARCH
        Enter your choice :
    ```

- **Heuristic Function**: defines the function to be used in order to compute the distance from a generic node to the destination.

    + **Dijkstra emulator**: Simulates **Dijkstra** shortest path algorithm behaviour which doesn't take into account any Heuristic Function, hence it always **returns 0**.

    + **Euclidean Distance**: Computes the **Euclidean Distance** between two points on a plane (optimal when using **Cartesian cordinates**).

    + **Haversine Distance**: Computes the angular distance between two points on a sphere (optimal when using **Geographic cordinates**).

    ```
    Insert the heuristic function h(x) to use:
        1: Dijkstra emulator
        2: Euclidean distance
        3: Haversine formula
        Enter your choice :
    ```


- **Hash Function**: choosing an hash function is required when executing the two **HDA versions** of A* (**options 6 and 7**); each of them will be explained in detail in the project documentation.

    + **Random Hash**

    + **Multiplicative Hash**

    + **Zobrist Hash**

    + **Abstract (State) Zobrist Hash**

    + **Abstract (Feature) Zobsrist Hash**

    ```
    Insert the hash function hash(x) to use:
        1: Random Hash
        2: Multiplicative Hash
        3: Zobrist Hash
        4: Abstract (State) Zobrist Hash
        5: Abstract (Feature) Zobrist Hash
        Enter your choice : 
    ```




