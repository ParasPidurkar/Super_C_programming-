
//program 30 
pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp$ gcc P30_userIP_prog_args.c
pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp$ ./a.out -n 2 -m 25 -H -R
total argument: 6
-n
2
-m
25
-H
-R

//program 31
pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp/P31_writing_to_file$ gcc P31_filewrite.c 
pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp/P31_writing_to_file$ ./a.out 
Data was written into a file

//Program 32

pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp/P32_reading_from_file$ gcc P32_read_file.c 
pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp/P32_reading_from_file$ ./a.out 
Reading a file….
fprintf message 0
fputs message


//Program 33

pkpidurkar@3mn10-Win:/mnt/d/Downloads/Camp$ ./a.out 
====concatenating===
result: helloworld


//Program P39 

Static lib build

Firstly, we compile our library, lib.c, to be a static library, called mylib.a
.
$ gcc -c lib.c
$ ar rs mylib.a lib.o
$ gcc -I./ -L./ -o test test.c ./mylib.a
$ ./test


Shared Library

Firstly, we compile our library, lib.c, to be a shared library


export LD_LIBRARY_PATH=/mnt/d/Downloads/Camp/P39_library/lib:$LD_LIBRARY_PATH
gcc -c -fpic lib.c
gcc -shared -o libmylibrary.so lib.o    //imp to use lib infront of library mame
$ gcc -I./ -L./ -o test test.c -lmylibrary
./test


//Program 40 

gcc -pthread -o createthread P1_createthread.c
./createthread

gcc -pthread -o threadid P2_threadid.c
./threadid

gcc -pthread -o threadexit P3_threadexit.c
./threadexit

$ gcc -pthread -o jointhread P5_join_thread.c -lm
$ ./jointhread


//Program 41

$ sudo apt-get install mysql-server mysql-client
$ sudo apt-get install libmysqlclient-dev

$ gcc -c -I/usr/include/mysql P1_demo-mysql-version.c
$ gcc -o P1_demo-mysql-version P1_demo-mysql-version.o -L/usr/lib/mysql -
$ ./P1_demo-mysql-version


