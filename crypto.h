
#pragma once

// definition in sha1_16.c
void sha1_16(const unsigned char in[16], unsigned char out[16]);

// definition in aes128.c

#define RK_LEN 44 //round key length

#define AES_BLOCK_SIZE 16

void aes_gen_tables(void);

void aes_set_key_enc_128(unsigned int rk[RK_LEN], const unsigned char *key);

void aes_encrypt_128(const unsigned int rk[8], const unsigned char input[16], unsigned char output[16]);

