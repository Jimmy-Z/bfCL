
#include <stdio.h>
#include <mbedtls/config.h>
#include <mbedtls/version.h>
#include <mbedtls/aes.h>
#include <mbedtls/aesni.h>
#include "crypto.h"

static mbedtls_aes_context ctx;

static int (*p_aes_crypt_ecb)(mbedtls_aes_context*, int, const unsigned char *, unsigned char *) = NULL;

static void (*p_aes_set_key_enc_128)(const unsigned char *key) = NULL;

static void (*p_aes_set_key_dec_128)(const unsigned char *key) = NULL;

#define AES_KEY_LEN 128
#define NR 10

// I hope eliminating the AESNI check can make it a bit faster
static void aes_set_key_enc_128_aesni(const unsigned char *key){
	// mbedtls_aes_setkey_enc(&ctx, key, 128);
	ctx.nr = NR;
	ctx.rk = ctx.buf;
	mbedtls_aesni_setkey_enc((unsigned char *)ctx.rk, key, AES_KEY_LEN);
}

static void aes_set_key_dec_128_aesni(const unsigned char *key) {
	mbedtls_aes_context cty;
	cty.nr = NR;
	cty.rk = cty.buf;
	mbedtls_aesni_setkey_enc((unsigned char *)cty.rk, key, AES_KEY_LEN);
	ctx.nr = cty.nr;
	ctx.rk = ctx.buf;
	mbedtls_aesni_inverse_key((unsigned char *)ctx.rk, (const unsigned char *)cty.rk, ctx.nr);
}

static void aes_set_key_enc_128_c(const unsigned char *key) {
	mbedtls_aes_setkey_enc(&ctx, key, AES_KEY_LEN);
}

static void aes_set_key_dec_128_c(const unsigned char *key) {
	mbedtls_aes_setkey_dec(&ctx, key, AES_KEY_LEN);
}

void aes_init(){
	fputs(MBEDTLS_VERSION_STRING_FULL, stdout);
	mbedtls_aes_init(&ctx);
	// prevent runtime checks
	if(mbedtls_aesni_has_support(MBEDTLS_AESNI_AES)){
		puts(", AES-NI supported");
		p_aes_crypt_ecb = mbedtls_aesni_crypt_ecb;
		p_aes_set_key_enc_128 = aes_set_key_enc_128_aesni;
		p_aes_set_key_dec_128 = aes_set_key_dec_128_aesni;
	}else {
		puts(", AES-NI not supported");
		p_aes_crypt_ecb = mbedtls_aes_crypt_ecb;
		p_aes_set_key_enc_128 = aes_set_key_enc_128_c;
		p_aes_set_key_dec_128 = aes_set_key_dec_128_c;
	}
#ifndef MBEDTLS_AES_ROM_TABLES
	// it will error out but also get aes_gen_tables done
	mbedtls_aes_setkey_enc(&ctx, NULL, 0);
#endif
}

void aes_set_key_enc_128(const unsigned char *key) {
	p_aes_set_key_enc_128(key);
}

void aes_set_key_dec_128(const unsigned char *key) {
	p_aes_set_key_dec_128(key);
}

void aes_encrypt_128(const unsigned char *in, unsigned char *out){
	p_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
}

void aes_decrypt_128(const unsigned char *in, unsigned char *out){
	p_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, in, out);
}

void aes_encrypt_128_bulk(const unsigned char *in, unsigned char *out, unsigned len){
	len >>= 4;
	for(unsigned i = 0; i < len; ++i){
		p_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
		in += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
	}
}

