#define SHA256_SIZE 32
#define NETHASH_RAW_SIZE 8
#define NETHASH_SIZE NETHASH_RAW_SIZE + SHA256_SIZE

int nethash_compute(const char* string, unsigned char* nethash_buffer);

