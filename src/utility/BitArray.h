#ifndef BITARRAY_H
#define BITARRAY_H


typedef struct bit_array *BitArray;

BitArray BITARRAYinit(int size);
void BITARRAYtoggleBit(BitArray ba, int index);
char BITARRAYgetBit(BitArray ba, int index);
void BITARRAYfree(BitArray ba);

#endif