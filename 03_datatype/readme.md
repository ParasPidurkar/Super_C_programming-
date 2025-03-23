# C Project

## Overview
This project is a collection of C programs designed to demonstrate various functionalities, algorithms, or system operations. Each file serves a specific purpose and addresses different aspects of programming in C.

## Requirements
- GCC or any other compatible C compiler
- A system running Linux, macOS, or Windows

## How to Compile and Run
1. cd 02_comments
3. Use the GCC compiler to compile the desired C file.
   ```bash
   gcc -o main 01_comment_using_prepro.c
   ./main

   gcc -o main 02_comment_MACRO.c
   ./main

   gcc 03_pitfalls.c -trigraphs -> error

   gcc 04_trigraphs.c -trigraphs
   ./a.out