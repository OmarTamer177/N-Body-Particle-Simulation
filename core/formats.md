# File Formats

This document defines the shared text formats used by:

- `sequential/`
- `openmp/`
- `mpi/`

The current codebase uses one input format for simulation and one summary format for benchmarking output.

## 1. Simulation Input Format

Use a simple text file with:

1. one header line
2. one line per particle

### Header

```text
N iterations dt G softening
```

Where:

- `N`: number of particles
- `iterations`: number of timesteps
- `dt`: timestep size
- `G`: gravitational constant used by the simulation
- `softening`: small value to reduce instability when particles become very close

### Particle Line

```text
id mass pos_x pos_y vel_x vel_y
```

### Example

```text
4 1000 0.01 1.0 1e-9
0 10.0 -1.0  0.0  0.0  0.2
1 10.0  1.0  0.0  0.0 -0.2
2 20.0  0.0  1.0 -0.2  0.0
3 20.0  0.0 -1.0  0.2  0.0
```

## 2. Summary Output Format

Each executable can optionally write a plain-text summary file.

### Summary Contents

```text
implementation sequential
particles 1000
iterations 500
dt 0.01
gravitational_constant 1.0
softening 1e-9
elapsed_seconds 1.234567
threads 1
processes 1
```

For OpenMP:

- `implementation openmp`
- `threads <thread_count>`

For MPI:

- `implementation mpi`
- `processes <process_count>`

## 3. Why This Helps

- the same input files can be used by all three implementations
- benchmark output stays simple and easy to compare
- the report can use summary files directly for timing tables
- there is no extra output overhead unless `--output` is requested
