# C Project

## Overview
This project is a collection of C programs designed to demonstrate various functionalities, algorithms, or system operations. Each file serves a specific purpose and addresses different aspects of programming in C.

## Requirements
- GCC or any other compatible C compiler
- A system running Linux, macOS, or Windows

## How to Compile and Run
1. cd 01_Geting_Started
3. Use the GCC compiler to compile the desired C file.
   ```bash
   gcc -o hello 01_Hello_world.c
   ./hello



   gcc -Wall -Wextra -Werror -o hello 01_Hello_world.c
./hello


To compile the program using clang you can use:
clang -Wall -Wextra -Werror -o hello 01_Hello_world.c
./hello

gcc -std=c90 -o hello 02_OG_Hello_world.c