# Sparse Matrix Multiplication

This project demonstrates the performance comparison between different matrix multiplication techniques:
classical multiplication and sparse matrix multiplication using the COO (Coordinate) format.
The program uses multithreading for faster computation in both classical and sparse matrix multiplication.

## Features

- **Matrix Generation**: Generates sparse matrices with random values using a probabilistic approach.
- **Classical Matrix Multiplication**: A basic, single-threaded approach to matrix multiplication.
- **Multithreaded Classical Multiplication**: A multithreaded approach to speed up matrix multiplication using multiple threads.
- **COO Matrix Representation**: Converts matrices into the sparse COO format for more efficient storage and multiplication.
- **Multithreaded COO Multiplication**: A multithreaded approach to sparse matrix multiplication using the COO format.

## Performance

```Choose your matrices size
2500
Generation time: 171 ms
Singlethreaded classical multiplication time: 171,015 ms
Multithreaded classical multiplication time: 67,107 ms
Coo multiplication time: 29 ms
Multithreaded Coo multiplication time: 9 ms
```
This gives us a 19000x speedup on calculation compared to the single-threaded implementation, which gets even more for matrices of bigger sizes.

## What is COO Representation
#### From Wikipedia
*Coordinate list (COO) stores a list of (row, column, value) tuples. Ideally, the entries are sorted first by row index and then by column index, to improve random access times. This is another format that is good for incremental matrix construction.*

In our case we have 3 vectors of rows, columns and values that in a sparse matrix are much less elements to check than in the matrix itself.<br>
Also, considering the number of 0's we have inside the matrix we will avoid trying to make useless calculations.

