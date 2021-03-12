# my_Virtual_Memory_Mgnt
User-level virtual memory mgnt
 
Please use the given Makefile for compiling. Before compiling the benchmark, you have to compile the virtual memory code first. 

For 32-bit addressing, compile the code with -m32 flag on gcc. For ease of testing later on, keep your page sizes as a macro definition (#define PAGETABLE 4096) in your library.