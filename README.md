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

## Purpose

The **Pthread-Benchmark** aims to:
1. Serve as a resource for research on concurrency bugs, specifically data races.
2. Enable the development and evaluation of tools and models for fault detection, localization, and program repair in multithreaded programming.
3. Provide a foundational dataset for studying mutex locking and unlocking patterns in Pthreads.

## Citation

If you use this benchmark in your research or projects, please cite this repository:

[Your Repository Name](https://github.com/your-repository-link)

### References

This benchmark was inspired by or built upon work from the following repository:
- [Referenced Repository Name](https://github.com/referenced-repository-link)

