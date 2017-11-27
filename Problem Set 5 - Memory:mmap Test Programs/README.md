Test programs that help answer the following questions: <br />

A: "When one has mapped a file for read-only access, but attempts to write to that mapped area, what signal is generated?" <br />
B: "If one maps a file with MAP_SHARED and then writes to the mapped memory, is that update visible when accessing the file through the traditional lseek(2)/read(2) system calls?" <br />
C: Same question as above, except for MAP_PRIVATE. <br />
D: "Say a pre-existing file of a certain size which is not an exact multiple of the page size is mapped with MAP_SHARED and read/write access, and one writes to the mapped memory just beyond the byte corresponding to the last byte of the existing file. Does the size of the file through the traditional interface (e.g. stat(2)) change?" <br />
E: "Let us say that after performing the aforementioned memory write, one then increased the size of the file beyond the written area, without over-writing the intervening contents (e.g. by using lseek(2) and then write(2) with a size of 1), thus creating a ’hole’ in the file. What happens to the data that had been written to this hole previously via the memory-map? Are they visible in the file?" <br />
F: "Let’s say there is an existing small file (say 10 bytes). Can you establish a valid mmap region two pages (8192 bytes) long? If so, what signal is delivered when attempting to access memory in the second page? What about the first page?" <br />
