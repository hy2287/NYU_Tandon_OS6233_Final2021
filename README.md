# CS-GY 6233 Final Project

Dec 2021 by Jian Ruan and Mark Yam

## Getting Started

### Compiles everything
```
./build
```

## Executing program

### Q1. Basics

* Read from a file and print out the xor value of that file
```
./run <filename> -r <block_size> <block_count>
```
* Write to a file
```
./run <filename> -w <block_size> <block_count>
```

### Q2. Measurement
* For a particular blocksize, find a file size which can be read in a "reasonable" time (5-15 sec)
```
./findFileSize <block_size>
```

### Q3. Raw Performance
* For uncached read:
```
./measure_performance_uncached
```

### Q4. Caching
* For cached read:
```
./measure_performance_cached
```

### Q5. System calls
* Measure how many system calls you can do per second
```
./syscallPerformance
```

### Q6. Optimized raw performance
* Read from the file as fast as possible
```
./fast <filename>
```