#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "BitArray.h"


struct bit_array {
    char *array;
    int size;
};

BitArray BITARRAYinit(int size){
    //sizeof(*ba)?
    BitArray ba = malloc(size * sizeof(*ba));
    if(ba == NULL){
        return NULL;
    }

    ba->array = (char*) malloc(size * sizeof(char));
    if(ba->array == NULL){
        return NULL;
    }

    ba->size = size;

    return ba;
}

void BITARRAYtoggleBit(BitArray ba, int index){
    ba->array[index / 8] ^= 1 << (index % 8);
}

char BITARRAYgetBit(BitArray ba, int index){
    return 1 & ba->array[index / 8] >> (index % 8);
}

void BITARRAYfree(BitArray ba){
    free(ba->array);
    free(ba);
}




// int main(){
//     BitArray bitArr = BITARRAYinit(10);

//     for(int i=0; i<10; i+=2){
//         BITARRAYtoggleBit(bitArr, i);
//     }


//     for(int i=0; i<10; i++){
//         char bit = BITARRAYgetBit(bitArr, i);
//         printf("%d\n", bit);
//     }

//     for(int i=0; i<10; i++){
//         BITARRAYtoggleBit(bitArr, i);
//     }
//     printf("--------\n");

//     for(int i=0; i<10; i++){
//         char bit = BITARRAYgetBit(bitArr, i);
//         printf("%d\n", bit);
//     }

//     return 0;
// }

