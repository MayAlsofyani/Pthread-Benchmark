# Pthread-Benchmark

## Overview

**Pthread-Benchmark** is a curated dataset designed to analyze and evaluate concurrency practices in Pthreads programs, specifically focusing on mutex synchronization patterns. Pthreads and other parallel programming languages enable the execution of multiple threads across various processing units while offering mechanisms to manage data sharing and synchronization. This repository provides a collection of faulty and fixed source code files for studying data races, a common data race bug.

## About Data Race Bugs

Data race bugs occur when two or more threads access shared variables concurrently without proper synchronization. This can lead to data corruption or unpredictable behavior. Mutexes (`pthread_mutex`) are crucial for preventing such issues, as they allow threads to coordinate access to shared resources.

- The `pthread_mutex_lock()` function locks a mutex object, granting the calling thread exclusive ownership.
- Other threads attempting to lock an already-locked mutex must wait until it is released using `pthread_mutex_unlock()`.

## Dataset Details

This benchmark consists of **two main folders**: `faulty` and `fixed`, containing source code files extracted from open-source projects on GitHub. These files focus on mutex lock and unlock synchronization patterns.

### Faulty and Fixed Folders

To create a balanced dataset:
- Faults were introduced by removing specific `pthread_mutex_lock()` and `pthread_mutex_unlock()` function calls.
- Each faulty file contains one or more types of data races related to mutex synchronization:
  - **OneBug Folder:** Faulty files containing a single data race.
  - **ManyBugs Folder:** Faulty files containing two or more data races (up to a maximum of 4 bugs per file).
- Fixed files include proper implementations of `pthread_mutex_lock()` and `pthread_mutex_unlock()` to ensure safe access to shared variables.

### Dataset Composition
- **Faulty Files:** 
  - 22 source code files with a single data race issue (OneBug folder).
  - 22 source code files with multiple data race bugs (ManyBugs folder, up to 4 bugs per file).
- **Fixed Files:** 44 source code files with proper mutex synchronization.
  
### Code Characteristics
- The source code files in this benchmark range from a **minimum of ~50 lines** to a **maximum of ~200 lines**.
- This range ensures a manageable size for analysis while retaining realistic complexity for concurrency studies.
- The table below provides examples of statistics from **Benchmark 1 (One Bug)** on whether the shared variables are used directly in the code between the mutex acquisition and release or if they involve more elaborate access patterns with function nesting.

| File                        | Mutex (memory address for actual variable)   |   Direct Access Count |   Function Call Count | Function Calls                                              |
|:----------------------------|:---------------------------------------------|----------------------:|----------------------:|:------------------------------------------------------------|
| thread_with_conditions.c    | &count_mutex                                 |                     3 |                     7 | inc_count, printf, if, pthread_cond_signal                  |
| thread_with_conditions.c    | &count_mutex                                 |                     3 |                    12 | printf, pthread_cond_wait, watch_count, while               |
| zad_dom1.c                  | &mutex                                       |                     1 |                     5 | pop_f, printf, if, display                                  |
| zad_dom1.c                  | &mutex                                       |                     0 |                     3 | push_f, printf, display                                     |
| 06_thread_cond_var.c        | &count_mutex                                 |                     3 |                     7 | inc_count, printf, if, pthread_cond_signal                  |
| 06_thread_cond_var.c        | &count_mutex                                 |                     3 |                    12 | printf, pthread_cond_wait, watch_count, while               |
| udp_server.c                | &mutex1                                      |                     1 |                     0 | nan                                                         |
| udp_server.c                | &mutex1                                      |                     0 |                     0 | nan                                                         |
| 023_sync_mutex.c            | &my_mutex                                    |                     2 |                     5 | if, strcpy                                                  |
| 023_sync_mutex.c            | &my_mutex                                    |                     0 |                     2 | memcpy, sizeof                                              |
| 02test.c                    | &mutex                                       |                     3 |                     4 | for, printf, setbuf, pthread_self                           |
| 02test.c                    | &mutex                                       |                     3 |                     4 | for, printf, setbuf, pthread_self                           |
| 02.c                        | &mutex                                       |                     3 |                     4 | printf, pthread_cond_wait, while                            |
| 02.c                        | &mutex                                       |                     4 |                     6 | malloc, sizeof, printf, pthread_cond_signal, memset         |
| 06test_pro_con.c            | &mutex                                       |                     1 |                     1 | printf                                                      |
| 06test_pro_con.c            | &mutex                                       |                     2 |                     4 | printf, pthread_cond_wait, while                            |
| 02_condition_modify.c       | &mutex                                       |                     2 |                     2 | printf, pthread_self                                        |
| 02_condition_modify.c       | &mutex                                       |                     3 |                     5 | printf, free, if, pthread_cond_wait, pthread_self           |
| 11-14UseConditionVariable.c | &count_mutex                                 |                     2 |                     7 | inc_count, printf, if, pthread_cond_signal                  |
| 11-14UseConditionVariable.c | &count_mutex                                 |                     1 |                     8 | printf, pthread_cond_wait, watch_count, while               |
| copy_deamon.c               | &offsetMutex                                 |                     1 |                     3 | pread, if, sizeof                                           |
| copy_deamon.c               | &offsetMutex                                 |                     0 |                     4 | recv, sendMessage, sizeof, sprintf                          |
| concurio.c                  | &timerlock                                   |                     2 |                     2 | pthread_cond_timedwait, while                               |
| concurio.c                  | &worker_sync_lock                            |                     0 |                     0 | nan                                                         |
| concurio.c                  | &worker_sync_lock                            |                     0 |                     2 | pthread_cond_wait, while                                    |
| concurio.c                  | &worker_sync_lock                            |                     1 |                     1 | pthread_cond_broadcast                                      |
| concurio.c                  | &worker_sync_lock                            |                     0 |                     0 | nan                                                         |
| pth_pool.c                  | &mutex                                       |                     3 |                     3 | for, pthread_cond_wait, while                               |
| pth_pool.c                  | &mutex                                       |                     1 |                     2 | pthread_cond_wait, while                                    |
| PThread-synchronization.c   | &mutex                                       |                     0 |                     3 | usleep, if, printf                                          |
| PThread-synchronization.c   | &mutex                                       |                     0 |                     3 | usleep, if, printf                                          |
| employee_with_mutex.c       | &lock                                        |                     0 |                     1 | copy_employee                                               |
| employee_with_mutex.c       | &lock                                        |                     1 |                    24 | strcmp, s, printf, exit, if, d                              |
| hot_plate_barriers.c        | &runnable                                    |                     2 |                     0 | nan                                                         |
| hot_plate_barriers.c        | &critical_begin_end                          |                     4 |                     3 | pthread_create, malloc, sizeof                              |
| 6.c                         | &count_mutex                                 |                     2 |                     4 | inc_count, pthread_cond_broadcast, if                       |
| 6.c                         | &count_mutex                                 |                     3 |                     7 | pthread_cond_wait, watch_count, while                       |
| helper.c                    | &m                                           |                     5 |                    11 | fprintf, gettimeofday, pthread_cond_timedwait, strerror, if |



## Purpose
The **Pthread-Benchmark** aims to:
1. Serve as a resource for research on concurrency bugs, specifically data races.
2. Enable the development and evaluation of tools and models for fault detection, localization, and program repair in multithreaded programming.
3. Provide a foundational dataset for studying mutex locking and unlocking patterns in Pthreads.

## Citation

If you use this benchmark in your research or projects, please cite this repository:
https://github.com/MayAlsofyani/Pthread-Benchmark

### References

This benchmark was inspired by or built upon work from the following repository:
- https://github.com/tarcisiodarocha/ExemplosPthreads
- https://github.com/coapp-packages/pthreads
- https://github.com/Veinin/POSIX-threads-programming-tutorials
- https://github.com/zly5/Parallel-Computing-Lab
- https://github.com/junstar92/parallel_programming_study
- https://github.com/JunhuaiYang/PthreadPool
- https://github.com/manasesjesus/pthreads
- https://github.com/imsure/parallel-programming
- https://github.com/ishanthilina/Pthreads-Monte-Carlo-Pi-Calculation
- https://github.com/snikulov/prog_posix_threads
- https://github.com/hailinzeng/Programming-POSIX-Threads
  

