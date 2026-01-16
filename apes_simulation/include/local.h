/*
 * local.h
 * Common includes for the Apes Simulation project
 * Include this file in all source files for standard headers
 */

#ifndef LOCAL_H
#define LOCAL_H

/* ==================== Standard C Library Headers ==================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

/* ==================== POSIX Headers ==================== */
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>

/* ==================== Project Headers ==================== */
#include "shared_data.h"
#include "config.h"
#include "utils.h"
#include "maze.h"
#include "family.h"
#include "sem_wrapper.h"

#endif /* LOCAL_H */

