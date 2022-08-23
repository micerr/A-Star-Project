#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>


#include <stdio.h>
#include <stdlib.h>



void test(void){
    EVP_KDF *kdf;
    EVP_KDF_CTX *kctx = NULL;
    unsigned char derived[32];
    OSSL_PARAM params[5], *p = params;

    /* Find and allocate a context for the HKDF algorithm */
    if ((kdf = EVP_KDF_fetch(NULL, "hkdf", NULL)) == NULL) {
        printf("EVP_KDF_fetch");
    }
    kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);    /* The kctx keeps a reference so this is safe */
    if (kctx == NULL) {
        printf("error");
    }

}

