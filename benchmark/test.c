#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "../MyPthread/my_vm.h"

#define DEFAULT_THREAD_NUM 2
#define VECTOR_SIZE 30
pthread_mutex_t   mutex;
#define SIZE 5
int thread_num = 4;
int* counter;
pthread_t *thread;
int r[VECTOR_SIZE];
int s[VECTOR_SIZE];
int res = 0;

void vector_multiply(void* arg) {
    int i = 0;
    int n = *((int*) arg);
    
    for (i = n; i < VECTOR_SIZE; i += thread_num) {
        pthread_mutex_lock(&mutex);
        res += r[i] * s[i];
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void verify() {
    int i = 0;
    res = 0;
    for (i = 0; i < VECTOR_SIZE; i += 1) {
        res += r[i] * s[i];
    }
    printf("verified res is: %d\n", res);
}

int main() {
    /*
    printf("Allocating three arrays of 400 bytes\n");
    void *a = myalloc(100*4);
    int old_a = (int)a;
    void *b = myalloc(100*4);
    void *c = myalloc(100*4);
    int x = 1;
    int y, z;
    int i =0, j=0;
    int address_a = 0, address_b = 0;
    int address_c = 0;

    printf("Addresses of the allocations: %lu, %lu, %lu\n",(unsigned long) a,(unsigned long) b,(unsigned long) c);
    printf("Storing integers to generate a SIZExSIZE matrix\n");
    for (i = 0; i < SIZE; i++) {
//        printf("This is the %d in first loop\n", i);
        for (j = 0; j < SIZE; j++) {
//            printf("This is the %d in second loop\n", j);
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            PutVal((void *)address_a, &x, sizeof(int));
            PutVal((void *)address_b, &x, sizeof(int));
        }
    }

    printf("Fetching matrix elements stored in the arrays\n");

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            address_b = (unsigned int)b + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            GetVal((void *)address_a, &y, sizeof(int));
            GetVal( (void *)address_b, &z, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }

    printf("Performing matrix multiplication with itself!\n");
    MatMult(a, b, SIZE, c);


    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            address_c = (unsigned int)c + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            GetVal((void *)address_c, &y, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }
    printf("Freeing the allocations!\n");
    myfree(a, 100*4);
    myfree(b, 100*4);
    myfree(c, 100*4);

    printf("Checking if allocations were freed!\n");
    a = myalloc(100*4);
    if ((int)a == old_a)
        printf("free function works\n");
    else
        printf("free function does not work\n");
    */
    
    //Test multi-thread;
    int i = 0;
    /*
    if (argc == 1) {
        thread_num = DEFAULT_THREAD_NUM;
    } else {
        if (argv[1] < 1) {
            printf("enter a valid thread number\n");
            return 0;
        } else {
            thread_num = atoi(argv[1]);
        }
    }
     */
    // initialize counter
    counter = (int*)myalloc(thread_num*sizeof(int));
    for (i = 0; i < thread_num; ++i)
        counter[i] = i;

    // initialize pthread_t
    thread = (pthread_t*)myalloc(thread_num*sizeof(pthread_t));

    // initialize data array
    for (i = 0; i < VECTOR_SIZE; ++i) {
        r[i] = i;
        s[i] = i;
    }

    pthread_mutex_init(&mutex, NULL);

    struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

    for (i = 0; i < thread_num; ++i)
        pthread_create(&thread[i], NULL, &vector_multiply, &counter[i]);

    for (i = 0; i < thread_num; ++i)
        pthread_join(thread[i], NULL);

    clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n",
           (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
    printf("res is: %d\n", res);

    pthread_mutex_destroy(&mutex);
    verify();

    // Free memory on Heap
    myfree(thread, thread_num*sizeof(int));
    myfree(counter, thread_num*sizeof(int));
    return 0;

    return 0;
}


