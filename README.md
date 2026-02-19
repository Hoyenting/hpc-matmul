# hpc-matmul

A starter project for matrix multiplication optimization and profiling.

## Structure

- `src/`: source code
- `include/`: headers
- `bench/`: benchmarking code/scripts
- `tests/`: correctness tests
- `results/`: benchmark outputs
- `scripts/`: helper scripts

## Build

```bash
make
```

## Run with arguments

```bash
make run ARGS="1024 1024 1024"
```

## Link extra libraries

```bash
make LDLIBS="-lm"
```
