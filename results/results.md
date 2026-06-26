# Experimental Timing Results

This file contains the timing data collected from the current benchmark summary files.

## N = 32

| Implementation | Configuration | Time (s) |
|---|---|---:|
| Sequential | 1 process | 0.0024447 |
| OpenMP | 2 threads | 0.0183113 |
| OpenMP | 4 threads | 0.0226908 |
| MPI | 2 processes | 0.0027808 |
| MPI | 4 processes | 0.0023888 |

## N = 128

| Implementation | Configuration | Time (s) |
|---|---|---:|
| Sequential | 1 process | 0.0427318 |
| OpenMP | 2 threads | 0.0388238 |
| OpenMP | 4 threads | 0.0404547 |
| MPI | 2 processes | 0.0269722 |
| MPI | 4 processes | 0.0200022 |

## N = 512

| Implementation | Configuration | Time (s) |
|---|---|---:|
| Sequential | 1 process | 0.692378 |
| OpenMP | 2 threads | 0.474365 |
| OpenMP | 4 threads | 0.383889 |
| MPI | 2 processes | 0.379497 |
| MPI | 4 processes | 0.286057 |

These values come from the generated `case*_summary.txt` files in this folder.

