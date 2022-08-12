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
    BitArray ba = malloc(size * sizeof(bit_array));
    if(ba == NULL){
        return NULL;
    }

    ba->array = (char*) malloc(size * sizeof(char));
    if(ba->array == NULL){
        return NULL;
    }

    ba->size = size;
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




void main(){
    BitArray bitArr = BITARRAYinit(10);
}

