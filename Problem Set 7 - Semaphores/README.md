Created a test file (pset7q1.c) that demonstrates errors when adding values in shared memory, and fixing this using a mutex (tas.S). <br />
Created a module sem.c with header file sem.h that implements semaphore operations (verified by pset7q2.c). <br />
Created a fifo module fifo.c with header file fifo.h that maintains a FIFO of unsigned longs using a shared memory data structure that is protected exclusively by the semaphore modules above. <br />
Created a testbed (pset7q3.c and pset7q4.c) of virtual processors to write and read sequentially numbered data from shared memory to verify the fifo. <br />
