# Cache-Replacement-Policy-LRU-and-Random-Simulation
Implemented LRU(Least Recently Used) and Random Cache Replacement Policies in C++. Tested the code on 28 traces from SPEC CPU 2006.

For LRU implementation, Hashmap and List data structures are used.

INPUT:
The code expects following four command line parameters in the order:
1. cp: the capacity of the cache in kilobytes (an int)
2. assoc: the associativity of the cache (an int)
3. blocksize: the size of a single cache block in bytes (an int)
4. repl: the replacement policy (a char); 'l' means LRU, 'r' means random

The code reads from the standard input. Sample input is in the form:
r 56ecd8
w 47f639

OUTPUT:
The output is line of six numbers separated by spaces representing the following in order as mentioned:
1. The total number of misses,
2. The percentage of misses (i.e. total misses divided by total accesses),
3. The total number of read misses,
4. The percentage of read misses (i.e. total read misses divided by total read accesses),
5. The total number of write misses,
6. The percentage of write misses (i.e. total write misses divided by total write accesses).

Running the code:
To run give command, gzip -dc <filename> | ./a.out cp assoc blocksize repl.
Sample input: gzip -dc 429.mcf-184B.trace.txt.gz | ./cache 2048 64 64 l, where 429.mcf-184B.trace.txt.gz is sample zipped file and ./a.out is binary file generated, 128 16 64 l are sample parameters.
Sample output for the above input: 55752 5.575200% 55703 5.610155% 49 0.689752%
