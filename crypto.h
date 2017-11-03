
#pragma once

// definition in sha1_16.c
void sha1_16(const unsigned char in[16], unsigned char out[16]);

// definition in aes.c

#define AES_BLOCK_SIZE 16

void aes_init(void);

void aes_set_key_enc_128(const unsigned char *key);

void aes_encrypt_128(const unsigned char input[16], unsigned char output[16]);

void aes_set_key_dec_128(const unsigned char *key);

void aes_decrypt_128(const unsigned char input[16], unsigned char output[16]);

