/**
 * Author: Paul Dagnelie
 * Modified by: Michael Arntzenius
 * To build: gcc -O3 -pthread hasher.c skein.c skein_block.c -o hasher
 */

#include "skein.h"
#include "SHA3api_ref.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

u08b_t desired[] = {0x5b, 0x4d, 0xa9, 0x5f, 0x5f, 0xa0, 0x82, 0x80, 0xfc, 0x98, 0x79, 0xdf,
                    0x44, 0xf4, 0x18, 0xc8, 0xf9, 0xf1, 0x2b, 0xa4, 0x24, 0xb7, 0x75, 0x7d,
                    0xe0, 0x2b, 0xbd, 0xfb, 0xae, 0x0d, 0x4c, 0x4f, 0xdf, 0x93, 0x17, 0xc8,
                    0x0c, 0xc5, 0xfe, 0x04, 0xc6, 0x42, 0x90, 0x73, 0x46, 0x6c, 0xf2, 0x97,
                    0x06, 0xb8, 0xc2, 0x59, 0x99, 0xdd, 0xd2, 0xf6, 0x54, 0x0d, 0x44, 0x75,
                    0xcc, 0x97, 0x7b, 0x87, 0xf4, 0x75, 0x7b, 0xe0, 0x23, 0xf1, 0x9b, 0x8f,
                    0x40, 0x35, 0xd7, 0x72, 0x28, 0x86, 0xb7, 0x88, 0x69, 0x82, 0x6d, 0xe9,
                    0x16, 0xa7, 0x9c, 0xf9, 0xc9, 0x4c, 0xc7, 0x9c, 0xd4, 0x34, 0x7d, 0x24,
                    0xb5, 0x67, 0xaa, 0x3e, 0x23, 0x90, 0xa5, 0x73, 0xa3, 0x73, 0xa4, 0x8a,
                    0x5e, 0x67, 0x66, 0x40, 0xc7, 0x9c, 0xc7, 0x01, 0x97, 0xe1, 0xc5, 0xe7,
                    0xf9, 0x02, 0xfb, 0x53, 0xca, 0x18, 0x58, 0xb6};

int default_best = 1000;
char *interstring = "tai";
int NUM_THREADS = 16;
int tid = 0;

typedef union {
    char str[25];
    struct {
        unsigned long a;
        char seed[8];
        unsigned long b;
        char zero;
    } foo;
} block_t;

void fitasciia(block_t* block) {
    block->foo.a &= 0x7F7F7F7F7F7F7F7FL;
    for (int i=0;i<8;i++) {
        if (block->str[i] < 0x20) {
            block->str[i] = (char) (block->str[i] + 0x20);
        }
    }
}

int fitasciib(block_t* block) {
    for (int i=0;i<8;i++) {
        if (block->str[16+i] < 0x21) {
            for (int j=i; j<8; j++) {
                block->str[16+j] = '!';
            }
            return 1;
        }
        if ((unsigned char) (block->str[16+i]) > 0x7E) {
            // little endian
            for (int j=i; j>=0; j--) {
                block->str[16+j] = '!';
            }
            if (i < 7) {
                for (int j=i+1; j<8; j++) {
                    block->str[16+j] = (char) (block->str[16+j] + 0x01);
                    if (block->str[16+j] == (char) 0x7F) {
                        block->str[16+j] = '!';
                    } else {
                        return 1;
                    }
                }
            } else {
                return 0;
            }
        }
    }
    return 1;
}

int popcnt(u08b_t a){
    int ret = 0;
    while(a!=0){
        if(a&1)
            ret++;
        a>>=1;
    }
    return ret;
}

int hash(u08b_t *arg,size_t len,u08b_t *res){
    Skein1024_Ctxt_t ctx;
    Skein1024_Init(&ctx,1024);
    Skein1024_Update(&ctx,arg,len);
    Skein1024_Final(&ctx,res);
/*    for(int i=0;i<128;i++){
        u08b_t j = res[i];
        printf("%02X",j);
    }
    printf("\n");*/

    int dif = 0;
    for(int i=0;i<32;i++){
        unsigned int xor = ((unsigned int *)res)[i]^((unsigned int *)desired)[i];
        dif+=__builtin_popcount(xor);
    }
//    printf("diff: %d\n",dif);
    return dif;
}

void thsmain(block_t* block) {
    int best = default_best;
    u08b_t res[128];
    while (fitasciib(block)) {
        for (size_t len = 24; len > 16; len--) {
            int res2 = hash(block->str,len,res);
            if(res2 < best) {
                best = res2;
                printf("%d %.*s\n",best, len, block->str);
                fflush(stdout);
            }
            if (block->str[len-1] != '!') break;
        }
        block->foo.b += 1L;
    }
}

/*
void *thrmain(void *arg){
    unsigned long arg2 = (unsigned long)arg;
    printf("Ehlo from thread %lu\n",arg2);
    u08b_t *res = calloc(128,sizeof(u08b_t));

    char *foo = calloc(100,sizeof(char));
    int best = default_best;
    char *beststr = calloc(100,sizeof(char));
    for(unsigned long int j=arg2;j<arg2+(ULONG_MAX/NUM_THREADS);j++){
        for(unsigned long int i=0;i<ULONG_MAX;i++){
            size_t len = sprintf(foo,"%lu%s%lu",i,interstring,j);
            int res2 = hash(foo,len,res);
            if(res2<best){
                best=res2;
                memset(beststr,0,100);
                strcpy(beststr,foo);
                printf("%d %s\n",best,beststr);
                fflush(stdout);
            }
        }
    }

    free(foo);
    free(res);
    return beststr;
}
*/

void *thrmain(void *arg){
    block_t block;
    block.foo.a = 0LU;
    block.foo.seed[0] = ' ';
    block.foo.seed[1] = ' ';
    block.foo.seed[2] = ' ';
    block.foo.seed[3] = ' ';
    block.foo.seed[4] = ' ';
    block.foo.seed[5] = ' ';
    block.foo.seed[6] = ' ';
    block.foo.seed[7] = ' ';
    block.foo.zero = '\0';

    printf("Starting with max=%d, seed=%.8s\n", default_best, interstring);
    snprintf(block.str+5, 4, "%03d", tid);
    snprintf(block.foo.seed, 9, "%s", interstring);
    if (strlen(interstring) < 8) {
        block.foo.seed[strlen(interstring)] = ' ';
    }
    block.foo.a += time(NULL) & 0x7F7F7F7F7F;
    block.foo.b = 0x2121212121212121LU;
    fitasciia(&block);
    fitasciib(&block);

    printf("Starting value: %s\n", block.str);
    fflush(stdout);
    thsmain(&block);
}

/*
int main(int argc, char **argv){
    block_t block;
    block.foo.a = 0LU;
    block.foo.seed[0] = ' ';
    block.foo.seed[1] = ' ';
    block.foo.seed[2] = ' ';
    block.foo.seed[3] = ' ';
    block.foo.seed[4] = ' ';
    block.foo.seed[5] = ' ';
    block.foo.seed[6] = ' ';
    block.foo.seed[7] = ' ';
    block.foo.zero = '\0';

    if(argc==5){
        sscanf(argv[1],"%d",&default_best);
        interstring = argv[2];
        sscanf(argv[3], "%d", &NUM_THREADS);
        sscanf(argv[4], "%d", &tid);
        printf("Starting with max=%d, seed=%.8s\n", default_best, interstring);
        snprintf(block.str+5, 4, "%03d", tid);
        snprintf(block.foo.seed, 9, "%s", interstring);
        if (strlen(interstring) < 8) {
            block.foo.seed[strlen(interstring)] = ' ';
        }
        block.foo.a += time(NULL) & 0x7F7F7F7F7F;
        block.foo.b = 0x2121212121212121LU;
        fitasciia(&block);
        fitasciib(&block);
        printf("Starting value: %s\n", block.str);
        //printf("%016lx\n%02x%02x%02x%02x%02x%02x%02x%02x\n%016lx\n", block.foo.a,
        //       block.foo.seed[0], block.foo.seed[1], block.foo.seed[2],
        //       block.foo.seed[3], block.foo.seed[4], block.foo.seed[5],
        //       block.foo.seed[6], block.foo.seed[7], block.foo.b);
        fflush(stdout);
        thsmain(&block);
    }

    //thrmain();
    return 0;
}
*/

// argv should be:
// max_best seed_string num_threads thread_id
int main(int argc, char **argv) {
    if (argc >= 2) {
        sscanf(argv[1],"%d",&default_best);
    }
    if (argc >= 3) {
        interstring = argv[2];
    }
    if (argc >= 4) {
        sscanf(argv[3], "%d", &NUM_THREADS);
    }
    if (argc >= 5) {
        sscanf(argv[4], "%d", &tid);
    }

    for(int i=0;i<NUM_THREADS-1;i++){
        pthread_t foo;
        pthread_create(&foo,NULL,thrmain,(void *)((ULONG_MAX/NUM_THREADS)*i));
    }
    thrmain((void *)((ULONG_MAX/NUM_THREADS)*(NUM_THREADS-1)));
    return 0;
}

