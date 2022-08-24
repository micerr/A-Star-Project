#ifndef BALANCER_H
#define BALANCER_H

typedef struct mul_hash_ctx {
    EVP_KDF_CTX *kctx;
    OSSL_PARAM *params;
    BIGNUM *num_secret;
} *MulHash_CTX;

MulHash_CTX BALANCERInitMultiplicativeHashing();
int BALANCERMultiplicativeHashing(MulHash_CTX ctx, unsigned long long state);
int BALANCERRandomDispatchment(unsigned long long state);

static void binaryToLongLong(const unsigned char *key, unsigned long long *key_num_ptr);


#endif