# Apes Collecting Bananas in a Maze Simulation

## Project Overview

A multi-process, multi-threaded simulation of ape families collecting bananas in a maze using POSIX threads and shared memory under Unix/Linux.

## Features

- **Multi-process Architecture**: Each family runs as a separate process
- **Multi-threaded Design**: Female, Male, and Baby apes run as separate threads within each family process
- **Shared Memory IPC**: Maze and global state shared between processes
- **Semaphore Synchronization**: Thread-safe access to shared resources
- **Configurable Parameters**: All simulation values read from config file
- **Real-time Display**: Live visualization of simulation state

## Simulation Rules

### Female Apes
- Navigate the maze collecting bananas
- Return to basket after collecting a target amount
- Fight other females when meeting in the same cell (winner takes bananas)
- Rest when energy drops below threshold

### Male Apes
- Protect the family basket
- Fight neighboring male apes (probability increases with basket size)
- Winner takes loser's entire basket
- Family withdraws when male energy is critical

### Baby Apes
- Wait for opportunities (when dad is fighting)
- Steal bananas from other family baskets
- Either eat stolen bananas or give to dad's basket

### Termination Conditions
1. Too many families have withdrawn
2. A family's basket reaches the winning threshold
3. A baby has eaten too many bananas
4. Simulation time exceeded

## Project Structure

```
apes_simulation/
├── include/
│   ├── config.h        # Configuration structures
│   ├── shared_data.h   # Shared memory structures
│   ├── maze.h          # Maze operations
│   ├── family.h        # Family/thread logic
│   └── utils.h         # Utility functions
├── src/
│   ├── main.c          # Main coordinator process
│   ├── config.c        # Config file parser
│   ├── maze.c          # Maze generation/operations
│   ├── family.c        # Thread implementations
│   └── utils.c         # Utility implementations
├── simulation.conf     # Configuration file
├── Makefile           # Build system
└── README.md          # This file
```

## Building

```bash
# Build the project
make

# Build with debug symbols (for gdb)
make debug

# Clean build artifacts
make clean
```

## Running

```bash
# Run with default config
make run

# Or run directly
./apes_simulation simulation.conf

# Run with custom config
./apes_simulation myconfig.conf
```

## Configuration

Edit `simulation.conf` to customize simulation parameters:

```ini
# Maze size
maze_rows=15
maze_cols=20

# Number of families
num_families=4

# Termination thresholds
winning_basket_threshold=50
max_simulation_time_seconds=120
```

See `simulation.conf` for all available options.

## Debugging

```bash
# Build with debug symbols
make debug

# Run with gdb
gdb ./apes_simulation

# Inside gdb:
(gdb) run simulation.conf
(gdb) bt          # backtrace on crash
(gdb) info threads # show all threads
```

### Clean Shared Memory (after crash)

```bash
make clean-shm
# Or manually:
ipcrm -M 0x1234
```

## Technical Details

### Synchronization Mechanisms

| Resource | Mechanism | Scope |
|----------|-----------|-------|
| Maze cells | `sem_t` (pshared=1) | Inter-process |
| Family baskets | `sem_t` (pshared=1) | Inter-process |
| Global state | `sem_t` (pshared=1) | Inter-process |
| Family local data | `pthread_mutex_t` | Intra-process |
| Fight signals | `pthread_cond_t` | Intra-process |

### Deadlock Prevention

- Basket locks always acquired in family ID order
- Timed waits used for condition variables
- Clean shutdown via signal handlers

## Requirements

- Linux/Unix operating system
- GCC compiler
- POSIX threads library (pthread)
- POSIX real-time library (rt)

