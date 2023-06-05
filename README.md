# vut-ios-project-2 - synchronizácia procesov

Simulation of a post office system using POSIX threads (`pthread.h`), which are synchronized with POSIX semaphores (`semaphore.h`).

## Description

The program aims to simulate a post office system with multiple customers and officers. The customers will enter the post office, wait in queues for different services, and be served by the officers. The officers will provide services for letters, packages, and money operations.

## Usage

1. Compile the code with the provided makefile:

make po-sim

2. Execution syntax:

./po-sim <NC> <NO> <TC> <TO> <PT>


These values represent the following:

- `NC`: Number of customers
- `NO`: Number of officers
- `TC`: Maximum time (in milliseconds) a customer waits before entering the post office
- `TO`: Maximum length of an officer's break (in milliseconds)
- `PT`: Maximum time (in milliseconds) after which the post office is closed for new customers

3. The program will simulate the post office system and export the output into the proj2.out file. If it doesn't exist, it will be created.


## Example input and output(in proj2.out)

./po-sim 2 4 10 50 50

1: Z 1: started
2: Z 2: started
3: U 1: started
4: U 1: taking break
5: U 4: started
6: U 4: taking break
7: U 3: started
8: U 3: taking break
9: U 2: started
10: U 2: taking break
11: Z 1: entering office for a service 2
12: Z 2: entering office for a service 2
13: U 2: break finished
14: Z 1: called by office worker
15: U 2: serving a service of type 2
16: U 3: break finished
17: U 3: serving a service of type 2
18: Z 2: called by office worker
19: Z 1: going home
20: U 2: service finished
21: U 2: taking break
22: U 3: service finished
23: U 3: taking break
24: Z 2: going home
25: closing
26: U 4: break finished
27: U 4: going home
28: U 1: break finished
29: U 1: going home
30: U 2: break finished
31: U 2: going home
32: U 3: break finished
33: U 3: going home



