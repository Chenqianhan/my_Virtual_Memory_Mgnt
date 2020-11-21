#include "my_vm.h"
unsigned pte_t *PGT;
unsigned pde_t **PGD;
void *PHYMEM;
int *phy_bit_map;
int *vir_bit_map;
int frame_num;
int page_num;

int isInit = 0;
int offset_bits;
int pt_bits;
int pd_bits;

int pt_size;
int pd_size;
/*
Function responsible for allocating and setting your physical memory
*/
void SetPhysicalMem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating


    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
    
    offset_bits = (int)log(PGSIZE)/LOG(2);
    pt_bits = (32 - offset_bits)/2;
    pd_bits = pt_bits;
    
    //Actually the exact number of pages is less than the limitation of address.
    //Right here we have 2^20 pages in total which is equivalent to MAX_MEMSIZE/PGSIZE, namely virtual memory.
    //But we have 1GB physical memory where there are 2^18 pages.
    pt_size = 1<<pt_bits; //pt_size*pd_size==MAX_MEMSIZE/PGSIZE
    pd_size = 1<<pd_bits;
    
    //we allocate 2^30bytes = 1GB physical memory
    PHYMEM = malloc(MEMSIZE);
    
    PGD = (pde_t **)malloc(pd_size * szieof(pde_t));
    //We don't have to allocate complete pagetable at first.
    //We use a vir_bit_map to mark if it is initialized or used.
    /*
    for(pde_t i=0;i<pd_size;i++){
        PGD[i] = (pte_t *)malloc(pt_size * sizeof(pte_t));
    }
    */
    
    //Cuz we have 2^20 pages.
    page_num = (unsign)MAX_MEMSIZE
    vir_bit_map = malloc()
    //Cuz we need (2^30/2^12)bits to mark page frames. And each byte = 8bits.
    frame_num = (unsigned long)(MEMSIZE/PGSIZE)/8;
    phy_bit_map = malloc(frame_num); //There
}

/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address


    //If translation not successfull
    return NULL;
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
PageMap(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    return -1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {

    //Use virtual address bitmap to find the next free page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will
   have to mark which physical pages are used. */

    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/


}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}