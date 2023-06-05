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

1: Z 1: started<br>
2: Z 2: started <br>
3: U 1: started <br>
4: U 1: taking break <br>
5: U 4: started <br>
6: U 4: taking break <br>
7: U 3: started <br>
8: U 3: taking break <br>
9: U 2: started <br>
10: U 2: taking break<br> 
11: Z 1: entering office for a service 2 <br>
12: Z 2: entering office for a service 2 <br>
13: U 2: break finished <br>
14: Z 1: called by office worker <br>
15: U 2: serving a service of type 2 <br>
16: U 3: break finished <br>
17: U 3: serving a service of type 2 <br>
18: Z 2: called by office worker <br>
19: Z 1: going home <br>
20: U 2: service finished <br>
21: U 2: taking break <br>
22: U 3: service finished <br>
23: U 3: taking break <br>
24: Z 2: going home <br>
25: closing <br>
26: U 4: break finished <br>
27: U 4: going home <br>
28: U 1: break finished <br>
29: U 1: going home <br>
30: U 2: break finished <br>
31: U 2: going home <br>
32: U 3: break finished <br>
33: U 3: going home <br>




