#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "Balancer.h"
#include "Timer.h"


static void binaryToLongLong(const unsigned char *key, unsigned long long *key_num_ptr);


MulHash_CTX BALANCERInitMultiplicativeHashing(){
    //OpenSSL Data Structures for KDF
    MulHash_CTX mul_hash_ctx = (MulHash_CTX) malloc(sizeof(*mul_hash_ctx));

    EVP_KDF *kdf;  
    EVP_KDF_CTX *kctx = NULL;
    OSSL_PARAM *params;
    BIGNUM *num_secret;

    num_secret = BN_new();
    params = (OSSL_PARAM*) malloc(3 * sizeof(OSSL_PARAM));
    
    if((kdf = EVP_KDF_fetch(NULL, "hkdf", NULL)) == NULL){
        printf("kdf error");
        exit(1);
    }
    kctx = EVP_KDF_CTX_new(kdf);

    EVP_KDF_free(kdf);
    if (kctx == NULL) {
        printf("error");
        exit(1);
    }

    params[0] = OSSL_PARAM_construct_utf8_string("digest", "sha256", (size_t)7);


    mul_hash_ctx->kctx = kctx;
    mul_hash_ctx->params = params;
    mul_hash_ctx->num_secret = num_secret;

    return mul_hash_ctx;
}

int BALANCERMultiplicativeHashing(MulHash_CTX ctx, unsigned long long state){
    // M(s) = H(kdf(s))
    // H(kdf) = floor[p * (key * A - floor[key * A])]


    unsigned char derived[8];
    unsigned char *secret;

    int bignum_bytes;
    
    BN_set_word(ctx->num_secret, state);
    bignum_bytes = BN_num_bytes(ctx->num_secret);

    secret = (unsigned char*) malloc(bignum_bytes * sizeof(char));
    BN_bn2bin(ctx->num_secret, secret);

    /*KDF Parameters*/
    ctx->params[1] = OSSL_PARAM_construct_octet_string("key", secret, (size_t)bignum_bytes);
    ctx->params[2] = OSSL_PARAM_construct_end();
    


    if (EVP_KDF_CTX_set_params(ctx->kctx, ctx->params) <= 0) {
        printf("EVP_KDF_CTX_set_params");
    }

    /* Do the derivation */
    if (EVP_KDF_derive(ctx->kctx, derived, sizeof(derived), NULL) <= 0) {
        printf("EVP_KDF_derive");
    }

    /* Use the 8 bytes as a Key*/
    const unsigned char *key = derived;


    //Multiplicative hashing parameters
    long double A = (sqrt(5) - 1) / 2;
    int processors = sysconf(_SC_NPROCESSORS_ONLN);


    unsigned long long *key_num_ptr = (unsigned long long*) malloc(sizeof(unsigned long long));

    binaryToLongLong(key, key_num_ptr);

    long double m1 = (*key_num_ptr) * A;
    long double m2 = floorl(m1);
    long double res = (floorl((long double)processors * (m1 - m2)));


    return (int)res;

}

int BALANCERRandomDispatchment(unsigned long long state){
    int proc = sysconf(_SC_NPROCESSORS_ONLN);
    return state % proc;
}



// void main(){
//     int proc = sysconf(_SC_NPROCESSORS_ONLN);
//     int occurrences[proc];
//     int occurrences_rand[proc];

//     for(int i=0; i<proc; i++){
//         occurrences[i] = 0;
//         occurrences_rand[i] = 0;
//     }


//     srand(time(NULL)); 

//     Timer timer = TIMERinit(1);
    
//     MulHash_CTX ctx = BALANCERInitMultiplicativeHashing();

//     TIMERstart(timer);
//     for(int i=0; i<1000000; i++){
//         unsigned long long r = rand();
//         //unsigned long long r_p = rand() % 1000000;
//         occurrences[BALANCERMultiplicativeHashing(ctx, r)] += 1;
//         //occurrences_rand[r_p%8] += 1;
//     }
//     TIMERstopEprint(timer);


//     printf("KDF occurrences: \n");
//     for(int i=0; i<proc; i++){
//         printf("Proc %d: %d\n", i+1, occurrences[i]);
//     }
//     printf("\n");

//     // printf("Rand occurrences: \n");
//     // for(int i=0; i<proc; i++){
//     //     printf("Proc %d: %d\n", i+1, occurrences_rand[i]);
//     // }
// }


static void binaryToLongLong(const unsigned char *key, unsigned long long *key_num_ptr){
    unsigned long long key_num = 0;
    //Conversion from binary string to long long
    for(int i=7; i>=0; i--){
        key_num += (unsigned long long)key[i];

        if(i != 0){
            key_num <<= 8;
        }
    }

    *key_num_ptr = key_num;
}

