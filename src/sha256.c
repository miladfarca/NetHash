#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

void sha256_compute(const char *str, unsigned char hash[EVP_MAX_MD_SIZE], unsigned int *hash_len) {
  EVP_MD_CTX *mdctx;
  const EVP_MD *md;

  md = EVP_sha256();
  mdctx = EVP_MD_CTX_new();

  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, str, strlen(str));
  EVP_DigestFinal_ex(mdctx, hash, hash_len);

  EVP_MD_CTX_free(mdctx);
}

void sha256_print_hash(unsigned char hash[], unsigned int hash_len) {
  for (unsigned int i = 0; i < hash_len; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");
}
