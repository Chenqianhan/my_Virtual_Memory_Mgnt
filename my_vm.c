#include "my_vm.h"
pte_t *PGT;
pde_t **PGD;
unsigned long *PHYMEM;
unsigned long *phy_bit_map;
unsigned long *vir_bit_map;
unsigned long frame_num;
unsigned long page_num;

unsigned isInit = 0;
unsigned long offset_bits;
unsigned long pt_bits; //number of bits in address
unsigned long pd_bits;

unsigned long pt_size; //number of entries in each pageTable
unsigned long pd_size;
/*
Function responsible for allocating and setting your physical memory
*/
void SetPhysicalMem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating


    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
    
    offset_bits = (unsigned long)(log(PGSIZE)/log(2));
    pt_bits = (32 - offset_bits)/2;
    pd_bits = 32 - pt_bits - offset_bits;
    
    //Actually the exact number of pages is less than the limitation of address.
    //Right here we have 2^20 pages in total which is equivalent to MAX_MEMSIZE/PGSIZE, namely virtual memory.
    //But we have 1GB physical memory where there are 2^18 pages.
    pt_size = 1<<pt_bits; //pt_size*pd_size==MAX_MEMSIZE/PGSIZE
    pd_size = 1<<pd_bits;
    
    //we allocate 2^30bytes = 1GB physical memory
    PHYMEM = (unsigned long*)malloc(MEMSIZE);
    
    PGD = (pde_t **)malloc(pd_size * sizeof(pde_t));

    //We use a vir_bit_map to mark if it is initialized or used.
    for(unsigned long i=0;i<pd_size;i++){
        //PGD[i] = (pte_t *)malloc(pt_size * sizeof(pte_t));
        PGD[i] = NULL;
    }
    
    //Cuz we have 2^20 pages.
    page_num = (unsigned long)(MAX_MEMSIZE/PGSIZE);
    vir_bit_map = (unsigned long *)malloc(page_num/8);
    //memset(vir_bit_map, 0, page_num);
    
    //Cuz we need (2^30/2^12)bits to mark page frames. And each byte = 8bits.
    frame_num = (unsigned long)(MEMSIZE/PGSIZE);
    phy_bit_map = (unsigned long *)malloc(frame_num/8);
    //memset(phy_bit_map, 0, frame_num);
}

/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address
    
    //Decode virtual address
    unsigned long address = (unsigned long)va;
    unsigned long offset = address & ((1 << offset_bits)-1);
    address = address >> offset_bits;
    unsigned long pt_index = address & ((1<< pt_bits)-1);
    unsigned long pd_index = address >> pt_bits;
    
    //If translation not successfull
    if(getBit(vir_bit_map, address) == 0){
        printf("No such bit for this va");
        return NULL;
    }
    
    unsigned long pa = (unsigned long)pgdir[pd_index][pt_index];
    pa += offset;
    
    return (pte_t *)pa;
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
    unsigned long address = (unsigned long)va;
    unsigned long offset = address & ((1 << offset_bits)-1);
    address = address >> offset_bits;
    unsigned long pt_index = address & ((1<< pt_bits)-1);
    unsigned long pd_index = address >> pt_bits;
    
    if(PGD[pd_index] == NULL){
        PGD[pd_index] = (pte_t *)malloc(pt_size * sizeof(pte_t));
    }
    
    //Means this va is not initialized
    /*
    for(int i=0;i<page_num;i++){
        if(getBit(vir_bit_map, address) == 0){
     int carry = (pt_index + i)/pt_size;
     PGD[pd_index+carry][(pt_index+i)%pt_size] = (void *)((pa >> offset_bits) << offset_bits);
            setBit(vir_bit_map, address+i);
        }else{
            for(int j=i-1;j>=0;j--){
                int c = (pt_index + j)/pt_size;
                PGD[pd_index+c][(pt_index+j)%pt_size] = NULL;
                removeBit(vir_bit_map, address+j);
            }
            return 0;
        }
    }
    */
    
    if(getBit(vir_bit_map, address) == 0){
        //---------------------
        PGD[pd_index][pt_index] = (void *)pa;
        
        //PGD[pd_index][pt_index] = ((unsigned long)pa >> offset_bits) << offset_bits;
        //setBit(vir_bit_map, address);
        return 1;
    }
    
    //If the virtual address that va point to is already used to map physical map, we return 0;

    return 0;
}


/*Function that gets the next available page
*/
/*
void *get_next_avail_phy(int num_pages) {
    unsigned long *pointer = PHYMEM;
    //Use virtual address bitmap to find the next free page
    unsigned long template = (1 << num_pages)-1;
    for(int i = 0; i<frame_num; i++){
        //Unsure!!!!!!!!
        if(*phy_bit_map & template > 0){
            template << 1;
        }else{
            return (unsigned long*)(pointer + i*PAGESIZE/4);
        }
    }
    
    return NULL; //Not enough physical
}
*/
void *get_next_avail_vir(int num_pages){
    //unsigned long *pointer = 0;
    unsigned long template = (1 << num_pages)-1;
    for(unsigned long i = 0; i<page_num; i++){
        if(*vir_bit_map & template == 0){
            return (unsigned long)i;
        }
    }
    return NULL;
}
//Only return the index of frame.
void *get_next_avail_phy(int num_pages) {
    
    unsigned long template = (1 << num_pages)-1;
    for(unsigned long i = 0; i<frame_num; i++){
        if(*vir_bit_map & template == 0){
            return (unsigned long)i;
        }
    }
    return NULL;
}

void *get_next_avail(int num_pages){
    //unsigned long template = (1 << num_pages) - 1;
    unsigned long virtual_start = 0;
    int cnt = 0;
    for(unsigned long i = 0; i<page_num; i++){
        if(getBit(vir_bit_map, i) == 0) cnt++;
        else{
            cnt = 0;
        }
        
        if(cnt == num_pages){
            virtual_start = i;
            break;
        }
        
        if(i == page_num - num_pages){
            return NULL;
        }
    }
    
    unsigned long* phy_candidate = malloc(num_pages * sizeof(unsigned long));
    cnt = 0;
    for(unsigned long i = 0; i<frame_num; i++){
        if(getBit(phy_bit_map, i) == 0) phy_candidate[cnt++] = i;
        
        if(cnt == num_pages) break;
        if(i == frame_num) return NULL;
    }
    
    virtual_start *= PGSIZE;
    for(int i=0;i<num_pages;i++){
        unsigned long va = virtual_start + i*PGSIZE;
        unsigned long pa = (unsigned long)PHYMEM + phy_candidate[i]*PGSIZE;
        //unsigned long phy_offset = pa & ((1<<offset_bits)-1);
        //va |= phy_offset;
        if(PageMap(PGD, (void *)va, (void*)pa) == 0) return NULL;
        setBit(vir_bit_map, (virtual_start >> offset_bits)+i);
        setBit(phy_bit_map, phy_candidate[i]);
        
        //printf("Page Maping: va=%lu, pa=%lu\n",va,pa);
    }
    /*
    virtual_start |= ((unsigned long)PHYMEM + phy_candidate[0]*PGSIZE) & ((1<<offset_bits)-1);
    */
    return virtual_start;
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
    if(!isInit){
        SetPhysicalMem();
        isInit = 1;
    }
    
    int num_pages = num_bytes/PGSIZE;
    if(num_bytes%PGSIZE > 0) num_pages++;
    
    unsigned long va = get_next_avail(num_pages);
    if(va == NULL){
        //Not enough space
        return NULL;
    }
    
    return (void *)va;
        
    /*
    int num_pages = num_bytes/PGSIZE;
    if(num_bytes%PGSIZE > 0) num_pages++;
    
    unsigned long frame_index = get_next_avail_phy(num_pages);
    unsigned long page_index = get_next_avail_vir(num_pages);
    
    for(unsigned i=0; i<num_pages; i++){
        setBit(phy_bit_map, frame_index + i);
        setBit(vir_bit_map, page_index + i);
    }
    
    unsigned long* pa = (unsigned long *)(PHYMEM + frame_index*1024);
    unsigned long offset = (unsigned long)pa & ((1<<offset_bits)-1);
    
    unsigned pd_index = page_index/pt_size;
    unsigned pt_index = page_index%pt_size;
    unsigned long va = pd_index << pt_bits;
    va |= pt_index;
    va << offset_bits;
    va |= offset;
    
    if(PageMap(PGD, (unsigned long*)va, pa) == 0) return NULL;
     */
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
    unsigned long* pa = (unsigned long*)Translate(PGD, va);
    int num_pages = size/PGSIZE;
    if(size%PGSIZE > 0) num_pages++;
    
    for(int i=0;i<frame_num;i++){
        if((unsigned long)PHYMEM + i*1024 == (unsigned long)pa){
            for(int j=0;j<num_pages;j++){
                removeBit(phy_bit_map, i+j);
            }
        }
    }
    
    unsigned long index = (unsigned long)va >> offset_bits;
    for(int j=0;j<num_pages;j++){
        removeBit(vir_bit_map, index+j);
    }
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/
    //printf("Translation pa: %lu\n", (unsigned long)Translate(PGD, va));
    memcpy((unsigned long *)Translate(PGD, va), val, size);
}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */

    memcpy(val, (unsigned long *)Translate(PGD, va), size);
}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
//void MatMult(void *a, void *b, int SIZE, void *c) {
void MatMult(void *mat1, void *mat2, int size, void *answer) {
    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/
    
    /*
    int address_a = 0, address_b = 0, address_c = 0;
    int* temp_a = malloc(sizeof(int)), temp_b = malloc(sizeof(int));
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int temp = 0;
            for (int t = 0; t < SIZE; t++) {
                address_a = (unsigned int) a + ((i * SIZE * sizeof(int))) + (t * sizeof(int));
                address_b = (unsigned int) b + ((t * SIZE * sizeof(int))) + (j * sizeof(int));
                GetVal(address_a, temp_a, sizeof(int));
                GetVal(address_b, temp_b, sizeof(int));
                temp += (*temp_a) * (*temp_b);
            }
            address_c = (unsigned int) c + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            PutVal(&temp, &address_c, sizeof(int));
            //printf("%d ", y);
        }
        //printf("\n");
    }
     */
    int sum = 0;
        int m1 = 0, m2 = 0, ans = 0;
        int a, b;
        int i, j, k;
        for(i = 0; i < size; i++ ){
            for(j = 0; j < size; j++){
                sum = 0;
                for(k = 0; k < size; k++){
                    /*Get address of next value in matrices*/
                    m1 = (unsigned int)mat1 + ((i * size * sizeof(int)) + (k * sizeof(int)));
                    m2 = (unsigned int)mat2 + ((k * size * sizeof(int)) + (j * sizeof(int)));
                    /*Get value from address in matrices*/
                    GetVal((void*)m1, &a, sizeof(int));
                    GetVal((void*)m2, &b, sizeof(int));
                    
                    sum += a * b;
                }
                /*Get address of answer matrix*/
                ans = (unsigned int)answer + ((i * size * sizeof(int)) + (j * sizeof(int)));
                /*Store sum in answer matrix*/
                PutVal((void*)ans, &sum, sizeof(int));
            }
        }

}

// (virtual_address - 0)/PGSIZE = THE FIRST (32 - OFFSETS_BITS)
void setBit(unsigned long *bit_map, unsigned long bit){
    unsigned long offset = bit%32;
    unsigned long frame = bit/32;
    //
    bit_map[frame] |= 1 << offset;
}

// (physical_address - physical_starting)/PGSIZE
unsigned long getBit(unsigned long *bit_map, unsigned long bit){
    unsigned long offset = bit%32;
    unsigned long frame = bit/32;
    
    return bit_map[frame] & (1 << offset);
}

void removeBit(unsigned long *bit_map, unsigned long bit){
    unsigned long offset = bit%32;
    unsigned long frame = bit/32;
    
    bit_map[frame] &= ~(1 << offset);
}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
/*Part 2 HINT: Add a virtual to physical page translation to the TLB */
/*
int
add_TLB(void *va, void *pa)
{


    return -1;
}
*/

/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
/* Part 2: TLB lookup code here */
/*
pte_t *
check_TLB(void *va) {

    

}
*/

/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */

/*Part 2 Code here to calculate and print the TLB miss rate*/
/*
void
print_TLB_missrate()
{
    double miss_rate = 0;

    




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}
*/
