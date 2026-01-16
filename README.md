# Apes Collecting Bananas in a Maze Simulation

A multi-process, multi-threaded simulation demonstrating concurrent programming concepts using POSIX threads, shared memory IPC, and semaphore synchronization. Each ape family runs as a separate process with multiple threads (female, male, babies) competing to collect bananas in a maze.

## Features

- **Multi-Process Architecture**: Each family runs as an isolated process using `fork()`
- **Multi-Threaded Design**: Female, male, and baby apes operate as separate threads within each process
- **Inter-Process Communication**: Shared memory with semaphore-based synchronization
- **Real-time Visualization**: Live display of maze state and simulation events
- **Configurable Parameters**: All simulation values customizable via config file

## Technical Highlights

- **Synchronization**: Semaphores for inter-process coordination, mutexes/condition variables for intra-process thread coordination
- **Deadlock Prevention**: Lock ordering strategies to prevent circular dependencies
- **Cross-Platform**: Supports both Linux (unnamed semaphores) and macOS (named semaphores)
- **Error Handling**: Graceful cleanup of shared resources on termination

## Simulation Mechanics

- **Female Apes**: Navigate maze, collect bananas, fight other females, rest when energy depleted
- **Male Apes**: Protect basket, fight neighbors, signal babies during fights
- **Baby Apes**: Opportunistically steal bananas when dad is fighting

## Technologies

- C programming language
- POSIX threads (pthread)
- System V shared memory (shmget/shmat)
- POSIX semaphores
- Signal handling for cleanup

## Build & Run

make
./apes_simulation simulation.conf
