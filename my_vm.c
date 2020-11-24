#include "my_vm.h"
pte_t *PGT;
pde_t **PGD;
unsigned long *PHYMEM;//starting address of the physical address
unsigned long *phy_bit_map;
unsigned long *vir_bit_map;
unsigned long frame_num;//num of physical page frames
unsigned long page_num; // num of virtual pages

unsigned isInit = 0;
unsigned long offset_bits;// offset_bits based on PGSIZE
unsigned long pt_bits; //second level number of bits in address
unsigned long pd_bits;// first level number of bits in address

unsigned long pt_size; //= 1 << pt_bits number of entries in each pageTable
unsigned long pd_size;// = 1 << pd_bits

pthread_mutex_t mutex;
struct tlb *TLB;
double tlb_search_cnt;
double tlb_miss_cnt;
/**
 * Function responsible for allocating and setting your physical memory
 * 0. calculate values like pt_bits, pd_bits etc
 * 1. Allocate physical memory using malloc
 * 2. calculate the number of physical and virtual pages
 *    2.1 num_of_virt_pages = MAX_MEMSIZE / PGSIZE
 *    2.2 num_of_phys_pages = MEMSIZE / PGSIZE
 * 3. allocate virtual and physical bitmaps and initialize them
 *    3.1 virt_bitmap is of size num_of_virt_pages
 *    3.2 phys_bitmap is of size num_of_phys_pages
 * 4. Initialise page directory
 */
void SetPhysicalMem() {

    //pthread_mutex_lock(&mutex);
    //step 0
    offset_bits = (unsigned long)(log(PGSIZE)/log(2));
    pt_bits = (32 - offset_bits)/2;
    pd_bits = 32 - pt_bits - offset_bits;
    //Actually the exact number of pages is less than the limitation of address.
    //Right here we have 2^20 pages in total which is equivalent to MAX_MEMSIZE/PGSIZE, namely virtual memory.
    //But we have 1GB physical memory where there are 2^18 pages.
    pt_size = 1<<pt_bits;
    pd_size = 1<<pd_bits;

    // step 1: we allocate 2^30bytes = 1GB physical memory
    PHYMEM = (unsigned long*)malloc(MEMSIZE);

    //step 2 & 3
    page_num = (unsigned long)(MAX_MEMSIZE/PGSIZE);
    //since malloc use byte as input, so we need to divide 8;
    vir_bit_map = (unsigned long *)malloc(page_num/8);
    frame_num = (unsigned long)(MEMSIZE/PGSIZE);
    //since malloc use byte as input, so we need to divide 8;
    phy_bit_map = (unsigned long *)malloc(frame_num/8);
    
    //step 4
    PGD = (pde_t **)malloc(pd_size * sizeof(pde_t));
    for(unsigned long i=0;i<pd_size;i++){
        PGD[i] = NULL;
    }
    
    //step 5 Initialize TLB
    TLB = malloc(sizeof(struct tlb));
    
    //pthread_mutex_unlock(&mutex);
}

/**
 * The function takes a virtual address and page directories starting address and
 * performs translation to return the physical address
 */
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address
    //Decode virtual address
    //pthread_mutex_lock(&mutex);
    //If translation not successful
    if(getBit(vir_bit_map, address) == 0){
        //printf("No such bit for this va");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    
    pte_t *pa = check_TLB(va);
    tlb_search_cnt++;
    
    if(tlb_search_cnt == LDBL_MAX){
        tlb_search_cnt = 0;
        tlb_miss_cnt = 0;
    }
    
    //Check TLB first
    if(pa == NULL){
        unsigned long address = (unsigned long)va;
        unsigned long offset = address & ((1 << offset_bits)-1);
        address = address >> offset_bits;
        unsigned long pt_index = address & ((1<< pt_bits)-1);
        unsigned long pd_index = address >> pt_bits;
        
        pa = (unsigned long)pgdir[pd_index][pt_index] + offset;
        
        add_TLB(va, pa);
        tlb_miss_cnt++;
        
        //pthread_mutex_unlock(&mutex);
        return (pte_t *)pa;
        
    }else{
        //pthread_mutex_unlock(&mutex);
        return pa;
    }
}

/**
 * The function takes a page directory address, virtual address, physical address
 * as an argument, and sets a page table entry. This function will walk the page
 * directory to see if there is an existing mapping for a virtual address. If the
 * virtual address is not present, then a new entry will be added.
 * Note: for pa, we assume it's the starting address of the corresponding physical page, since we only call this function in that situation
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
    
    //pthread_mutex_lock(&mutex);
    if(PGD[pd_index] == NULL){
        PGD[pd_index] = (pte_t *)malloc(pt_size * sizeof(pte_t));
    }

    if(getBit(vir_bit_map, address) == 0){
        PGD[pd_index][pt_index] = (void *)pa;
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    //pthread_mutex_unlock(&mutex);
    //If the virtual address that va point to is already used to map physical map, we return 0;

    return 0;
}

/**
 * this function is responsible for the following tasks
 * 1. find whether there's enough continuous pages in virtual memory, if not return null
 * 2. find whether there's enough pages in physical memory, it can be separate or continuous pages. If failed ,return null
 * 3. map each virtual page to the corresponding physical page and mark virtual bitmap and physical bitmap accordingly
 * @param num_pages, which means how many pages we should allocate
 * @return null if failed, starting address of virtual pages allovcated if successful
 */
void *get_next_avail(int num_pages){
    //unsigned long template = (1 << num_pages) - 1;
    unsigned long virtual_start = 0;
    int cnt = 0;
    pthread_mutex_lock(&mutex);
    
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
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }
    
    unsigned long* phy_candidate = malloc(num_pages * sizeof(unsigned long));
    cnt = 0;
    for(unsigned long i = 0; i<frame_num; i++){
        if(getBit(phy_bit_map, i) == 0) phy_candidate[cnt++] = i;
        
        if(cnt == num_pages) break;
        if(i == frame_num){
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }
    
    virtual_start *= PGSIZE;
    for(int i=0;i<num_pages;i++){
        unsigned long va = virtual_start + i*PGSIZE;
        unsigned long pa = (unsigned long)PHYMEM + phy_candidate[i]*PGSIZE;
        if(PageMap(PGD, (void *)va, (void*)pa) == 0){
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        setBit(vir_bit_map, (virtual_start >> offset_bits)+i);
        setBit(phy_bit_map, phy_candidate[i]);
        //printf("Page Maping: va=%lu, pa=%lu\n",va,pa);
    }
    pthread_mutex_unlock(&mutex);

    return virtual_start;
}

/**
 * Function responsible for allocating pages
 * and used by the benchmark
 * 1. If the lock is not initialised, initialise it here
 * 2. SetPhysicalMem if hasn't done so
 * 3. Allocate enough pages according to num_bytes. All allocations are at a page granularity
 * 4. If failed to find enough pages then return NULL. Else, return the starting virtual address of the first virtual page.
 */
void *myalloc(unsigned int num_bytes) {
   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will
   have to mark which physical pages are used. */

    //##### SideNode: Our page directory is initialized in SetPhysicalMem() function.
    if(pthread_mutex_init(&mutex, NULL) != 0){
        printf("Error initializing mutex: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&mutex);
    if(!isInit){
        SetPhysicalMem();
        isInit = 1;
    }
    
    int num_pages = num_bytes/PGSIZE;
    if(num_bytes%PGSIZE > 0) num_pages++;
    
    unsigned long va = get_next_avail(num_pages);
    if(va == NULL){
        //Not enough space
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);
    return (void *)va;
}

/**
 * Responsible for releasing one or more memory pages using virtual address (va)
 * 1. unmark corresponding bits in virtual bitmap
 * 2. unmark corresponding bits in physical bitmap
 * 3. remove the mappings of each virtual page to the corresponding physical page frame
 *    i.e. set PGD[pd_t][pt_t] to NULL.
 */
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
    //unsigned long* pa = (unsigned long*)Translate(PGD, va);
    int num_pages = size/PGSIZE;
    if(size%PGSIZE > 0) num_pages++;
    unsigned long index = (unsigned long)va >> offset_bits;
    unsigned long v = (unsigned long)va;
    
    pthread_mutex_lock(&mutex);
    for(int j=0;j<num_pages;j++){
        unsigned long* pa = (unsigned long*)Translate(PGD, v + j*PGSIZE);
        unsigned long p_index = ((unsigned long)pa - (unsigned long)PHYMEM) >> offset_bits;
        
        removeBit(vir_bit_map, index);
        removeBit(phy_bit_map, p_index);
        
        unsigned long pt_t = index & ((1<<pt_bits)-1);
        unsigned long pd_t = index >> pt_bits;
        
        PGD[pd_t][pt_t] = NULL;
    }
    pthread_mutex_unlock(&mutex);
    
}

/**
 * The function copies data pointed by "val" to physical memory pages using virtual address (va)
 * Note: The "size" value can be larger than one page.
 * Therefore, we find multiple pages using Translate() function and then memcpy to  each corresponding physical page.
 * 1. If the size is smaller than the rest of space of the virtual page starting at vo, then memcpy just once
 * 2. If the size is largers than the rest of space of the virtual page starting at vo, then memcpy each physical page
 *    until all size has been memcopied.
 */
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/
    unsigned long va_offset = (unsigned long)va & ((1<<offset_bits)-1);
    pthread_mutex_lock(&mutex);
    if ( PGSIZE -  va_offset >= size) {
        memcpy((unsigned long *)Translate(PGD, va), val, size);
    } else {
        unsigned long starting_address_va = (unsigned long) va;
        unsigned long starting_address_val = (unsigned long) val;
        memcpy((unsigned long *)Translate(PGD, starting_address_va), starting_address_val, PGSIZE -  va_offset);
        size -= PGSIZE -  va_offset;
        starting_address_val += PGSIZE - va_offset;
        while (size >= PGSIZE) {
            starting_address_va = ((starting_address_va>> offset_bits) + 1) << offset_bits;
            memcpy((unsigned long *)Translate(PGD, starting_address_va), starting_address_val, PGSIZE);
            size -= PGSIZE;
            starting_address_val += PGSIZE;
        }
        if (size > 0) {
            starting_address_va = ((starting_address_va>> offset_bits) + 1) << offset_bits;
            memcpy((unsigned long *)Translate(PGD, starting_address_va), starting_address_val, size);
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * The function copies the contents of the page to val
 * Note: The "size" value can be larger than one page.
 * Therefore, we find multiple pages using Translate() function and then memcpy to corresponding val address.
 * 1. If the size is smaller than the rest of space of the virtual page starting at vo, then memcpy just once
 * 2. If the size is larger than the rest of space of the virtual page starting at vo, then memcpy each physical page
 *    until all size has been memcopied.
 */
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */
    pthread_mutex_lock(&mutex);
    unsigned long va_offset = (unsigned long)va & ((1<<offset_bits)-1);
    if ( PGSIZE -  va_offset >= size) {
        memcpy(val, (unsigned long *)Translate(PGD, va), size);
    } else {
        unsigned long starting_address_va = (unsigned long) va;
        unsigned long starting_address_val = (unsigned long) val;
        memcpy(starting_address_val, (unsigned long *)Translate(PGD, starting_address_va), PGSIZE -  va_offset);
        size -= PGSIZE -  va_offset;
        starting_address_val += PGSIZE -  va_offset;
        while (size >= PGSIZE) {
            starting_address_va = ((starting_address_va>> offset_bits) + 1) << offset_bits;
            memcpy(starting_address_val, (unsigned long *)Translate(PGD, starting_address_va), PGSIZE);
            size -= PGSIZE;
            starting_address_val += PGSIZE;
        }
        if (size > 0) {
            starting_address_va = ((starting_address_va>> offset_bits) + 1) << offset_bits;
            memcpy(starting_address_val, (unsigned long *)Translate(PGD, starting_address_va),size);
        }
    }
    pthread_mutex_unlock(&mutex);
}


/**
 * This function receives two matrices a and b as an argument with size
 * argument representing the number of rows and columns. After performing matrix
 * multiplication, copy the result to c.
 */
void MatMult(void *a, void *b, int SIZE, void *c) {
//void MatMult(void *mat1, void *mat2, int size, void *answer) {
    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/
    int address_a = 0, address_b = 0, address_c = 0;
    int temp_a , temp_b ;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int temp = 0;
            for (int t = 0; t < SIZE; t++) {
                address_a = (unsigned int) a + ((i * SIZE * sizeof(int))) + (t * sizeof(int));
                address_b = (unsigned int) b + ((t * SIZE * sizeof(int))) + (j * sizeof(int));
                GetVal(address_a, &temp_a, sizeof(int));
                GetVal(address_b, &temp_b, sizeof(int));
                temp += temp_a*temp_b;
            }
            address_c = (unsigned int) c + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            PutVal((void*) address_c, &temp, sizeof(int));
        }
    }
}

// (virtual_address - 0)/PGSIZE = THE FIRST (32 - OFFSETS_BITS)
//here bit means the index of the pages that need to be marked, like the first page, the second page ..etc
void setBit(unsigned long *bit_map, unsigned long bit){

    unsigned long offset = bit%32;
    unsigned long frame = bit/32;
    bit_map[frame] |= 1 << offset;


}

// (physical_address - physical_starting)/PGSIZE
//here bit means the index of the pages that need to see whether marked, like the first page, the second page ..etc
unsigned long getBit(unsigned long *bit_map, unsigned long bit){

    unsigned long offset = bit%32;
    unsigned long frame = bit/32;

    return bit_map[frame] & (1 << offset);
}

//here bit means the index of the pages that need to be unmarked, like the first page, the second page ..etc
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

int
add_TLB(void *target_va, void *target_pa)
{
    //printf("adding TLB");
    static int head;
    //static int size;

    unsigned long offset = (unsigned long)target_va & ((1<<offset_bits)-1);
    target_va = (void *)(((unsigned long)target_va >> offset_bits) << offset_bits);
    
    target_pa = (void *)((unsigned long)target_pa - offset);
    
    TLB->entries[head].va = target_va;
    TLB->entries[head].pa = target_pa;
    head = (head+1) % TLB_SIZE;
    /*
    if(size < TLB_SIZE){
        
        size++;
    }else{
        TLB.entries[head]->va = target_va;
        TLB.entries[head]->pa = target_pa;
        head = (head+1) % TLB_SIZE;
    }
    */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
/* Part 2: TLB lookup code here */

pte_t *
check_TLB(void *target_va) {
    //printf("Checking TLB");
    unsigned long offset = (unsigned long)target_va & ((1<<offset_bits)-1);
    target_va = (void *)(((unsigned long)target_va >> offset_bits) << offset_bits) ;
    
    for(int i=0;i<TLB_SIZE;i++){
        if(TLB->entries[i].va == NULL || TLB->entries[i].va != target_va) continue;
        
        unsigned long output = (unsigned long)TLB->entries[i].pa + offset;
        return (pte_t *)output;
    }
    
    return NULL;
}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */

/*Part 2 Code here to calculate and print the TLB miss rate*/

void
print_TLB_missrate()
{
    double miss_rate = 0;
    miss_rate = tlb_miss_cnt/tlb_search_cnt;
    
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

