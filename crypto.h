
#pragma once

// definition in sha1_16.c
void sha1_16(const unsigned char in[16], unsigned char out[16]);

// definition in aes128.c

#define RK_LEN 44 //round key length

#define AES_BLOCK_SIZE 16

void aes_gen_tables(void);

// only half the table is used in encrypt
typedef struct {
	unsigned char FSb[256];
	unsigned int FT0[256];
	unsigned int FT1[256];
	unsigned int FT2[256];
	unsigned int FT3[256];
	/*
	unsigned char RSb[256];
	unsigned int RT0[256];
	unsigned int RT1[256];
	unsigned int RT2[256];
	unsigned int RT3[256];
	*/
	unsigned int RCON[10];
} AES_Tables;

void aes_set_key_enc_128(unsigned int rk[RK_LEN], const unsigned char *key);

void aes_encrypt_128(const unsigned int rk[8], const unsigned char input[16], unsigned char output[16]);
