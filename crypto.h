
#pragma once

#include "common.h"

// definition in sha1_16.c
void sha1_16(u8 out[16], const u8 in[16]);

// definition in aes128.c

#define RK_LEN 44 //round key length

void aes_gen_tables(void);

typedef struct {
	unsigned char FSb[256];
	uint32_t FT0[256];
	uint32_t FT1[256];
	uint32_t FT2[256];
	uint32_t FT3[256];
	unsigned char RSb[256];
	uint32_t RT0[256];
	uint32_t RT1[256];
	uint32_t RT2[256];
	uint32_t RT3[256];
	uint32_t RCON[10];
} AES_Tables;

void aes_set_key_enc_128(uint32_t rk[RK_LEN], const unsigned char *key);

void aes_encrypt(const uint32_t rk[8], const unsigned char input[16], unsigned char output[16]);
