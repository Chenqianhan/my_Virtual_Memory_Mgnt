#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>
#include <float.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4

// Maximum size of your memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024 //4GB

#define MEMSIZE 1024*1024*1024

// Represents a page table entry
typedef void* pte_t;

// Represents a page directory entry
typedef pte_t* pde_t;

#define TLB_SIZE 120

//Structure to represents TLB

struct tlbentry {
    void *pa;
    void *va;
};

struct tlb {

    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    // You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes
    struct tlbentry entries[TLB_SIZE];
};
struct tlb tlb_store;


void SetPhysicalMem(); //mmap -> size, (id, start address),
pte_t* Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void* pa);
void *myalloc(unsigned int num_bytes);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();
pte_t* check_TLB(void *target_va);
int add_TLB(void *target_va, void *target_pa);

void setBit(unsigned long *bit_map, unsigned long bit);
unsigned long getBit(unsigned long *bit_map, unsigned long bit);
void removeBit(unsigned long *bit_map, unsigned long bit);

#endif
