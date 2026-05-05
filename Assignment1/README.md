# Assignment 1 — B-Tree Index Implementation & Performance Comparison

## Overview

Implements and benchmarks three B-Tree variants as in-memory index structures over a dataset of 100,000 student records.

- **B-Tree** — classic B-Tree storing records at all node levels
- **B+Tree** — keys stored only at leaf nodes; leaves linked for efficient range scans
- **B\*Tree** — variant with a higher minimum occupancy requirement, reducing splits

Each tree is evaluated across order values from 3 to 100, measuring insert time, structural efficiency, search performance, and deletion cost.

## Directory Structure

```
Assignment1/
├── Makefile
├── Assignment1_Manual.pdf     # assignment specification
├── Assignment1_report.pdf     # submitted report
├── src/
│   ├── storage.cc             # CSV loader and Student record store
│   ├── btree.cc               # B-Tree implementation
│   ├── bplus_tree.cc          # B+Tree implementation
│   └── bstar_tree.cc          # B*Tree implementation
├── experiments/
│   └── experiment.cc          # benchmark driver
└── data/
    └── student.csv            # 100,000 student records
```

## Build & Run

```bash
cd Assignment1
make
cd experiments && ./experiment
```
Results saved to experiments/result.csv

## Output

`experiments/result.csv` contains one row per (tree type, order) combination with the following metrics:

| Column | Description |
|--------|-------------|
| `tree` | Tree type (`btree`, `bplus`, `bstar`) |
| `order` | Tree order (3–100) |
| `insert_ms` | Total insertion time (ms) |
| `splits` | Number of node splits during insertion |
| `nodes` | Total node count after insertion |
| `util_pct` | Average node key utilization (%) |
| `memory_kb` | Estimated memory usage (KB) |
| `search_us` | Average point search time (µs) |
| `range_ms` | Range query time over 100,000 IDs (ms) |
| `del_r05`–`del_r50` | Cumulative deletion time after removing 5%–50% of records (ms) |
