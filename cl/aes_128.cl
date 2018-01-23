
/* AES 128 ECB dug out from mbed TLS 2.5.1
 * https://github.com/ARMmbed/mbedtls/blob/development/include/mbedtls/aes.h
 * https://github.com/ARMmbed/mbedtls/blob/development/library/aes.c
 *
 * C style comments are mbed TLS comments
 * C++ style comments are mine
 */

// x86/x86_64 CPU's are little endian
// popular OpenCL platforms(AMD, NVIDIA) are all little endian too
// so GET/PUT_UINIT32_LE is omitted

// did a little counting to understand why buf is [68]
// in set key, they generated:
//     128 bits key: 10 rounds of += 4, plus 4 after, 44
//     192 bits key: 8 rounds of += 6, plus 6 after, 56
//     256 bits key: 7 rounds of += 8, plus 8 after, 64
// and in ecb encrypt, it used:
//     4 + 4 * 2 * 4 + 4 + 4 "++"s, 44
//     4 + 4 * 2 * 5 + 4 + 4 "++"s, 52
//     4 + 4 * 2 * 6 + 4 + 4 "++"s, 60
// so they generated several bytes more in 192 and 256 modes to simplify the loop
// now "able to hold 32 extra bytes" in their comment makes senses now
#define RK_LEN 44

// the caller is responsible to put the key in rk
void aes_set_key_enc_128(uint32_t rk[RK_LEN]) {
	uint32_t *RK = (uint32_t *)rk;

	for (unsigned i = 0; i < 10; ++i, RK += 4) {
		RK[4] = RK[0] ^ RCON[i] ^
			((uint32_t)FSb[(RK[3] >> 8) & 0xFF]) ^
			((uint32_t)FSb[(RK[3] >> 16) & 0xFF] << 8) ^
			((uint32_t)FSb[(RK[3] >> 24) & 0xFF] << 16) ^
			((uint32_t)FSb[(RK[3]) & 0xFF] << 24);

		RK[5] = RK[1] ^ RK[4];
		RK[6] = RK[2] ^ RK[5];
		RK[7] = RK[3] ^ RK[6];
	}
}

void aes_set_key_dec_128(uint32_t rk[RK_LEN]) {
	uint32_t sk[RK_LEN];
	uint32_t *RK =  (uint32_t *)rk, *SK =  (uint32_t *)sk;

	*SK++ = *RK++;
	*SK++ = *RK++;
	*SK++ = *RK++;
	*SK++ = *RK++;

	aes_set_key_enc_128(sk);

	RK =  (uint32_t *)rk;
	SK = sk + 40;

	*RK++ = *SK++;
	*RK++ = *SK++;
	*RK++ = *SK++;
	*RK++ = *SK++;

	int i, j;
	for (i = 9, SK -= 8; i > 0; i--, SK -= 8) {
		for (j = 0; j < 4; j++, SK++) {
			*RK++ = RT0[FSb[(*SK) & 0xFF]] ^
				RT1[FSb[(*SK >> 8) & 0xFF]] ^
				RT2[FSb[(*SK >> 16) & 0xFF]] ^
				RT3[FSb[(*SK >> 24) & 0xFF]];
		}
	}

	*RK++ = *SK++;
	*RK++ = *SK++;
	*RK++ = *SK++;
	*RK++ = *SK++;
}

#define AES_FROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
	X0 = *RK++ ^ FT0[ ( Y0       ) & 0xFF ] ^   \
				 FT1[ ( Y1 >>  8 ) & 0xFF ] ^   \
				 FT2[ ( Y2 >> 16 ) & 0xFF ] ^   \
				 FT3[ ( Y3 >> 24 ) & 0xFF ];    \
												\
	X1 = *RK++ ^ FT0[ ( Y1       ) & 0xFF ] ^   \
				 FT1[ ( Y2 >>  8 ) & 0xFF ] ^   \
				 FT2[ ( Y3 >> 16 ) & 0xFF ] ^   \
				 FT3[ ( Y0 >> 24 ) & 0xFF ];    \
												\
	X2 = *RK++ ^ FT0[ ( Y2       ) & 0xFF ] ^   \
				 FT1[ ( Y3 >>  8 ) & 0xFF ] ^   \
				 FT2[ ( Y0 >> 16 ) & 0xFF ] ^   \
				 FT3[ ( Y1 >> 24 ) & 0xFF ];    \
												\
	X3 = *RK++ ^ FT0[ ( Y3       ) & 0xFF ] ^   \
				 FT1[ ( Y0 >>  8 ) & 0xFF ] ^   \
				 FT2[ ( Y1 >> 16 ) & 0xFF ] ^   \
				 FT3[ ( Y2 >> 24 ) & 0xFF ];    \
}

#define AES_RROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
	X0 = *RK++ ^ RT0[ ( Y0       ) & 0xFF ] ^   \
				 RT1[ ( Y3 >>  8 ) & 0xFF ] ^   \
				 RT2[ ( Y2 >> 16 ) & 0xFF ] ^   \
				 RT3[ ( Y1 >> 24 ) & 0xFF ];    \
												\
	X1 = *RK++ ^ RT0[ ( Y1       ) & 0xFF ] ^   \
				 RT1[ ( Y0 >>  8 ) & 0xFF ] ^   \
				 RT2[ ( Y3 >> 16 ) & 0xFF ] ^   \
				 RT3[ ( Y2 >> 24 ) & 0xFF ];    \
												\
	X2 = *RK++ ^ RT0[ ( Y2       ) & 0xFF ] ^   \
				 RT1[ ( Y1 >>  8 ) & 0xFF ] ^   \
				 RT2[ ( Y0 >> 16 ) & 0xFF ] ^   \
				 RT3[ ( Y3 >> 24 ) & 0xFF ];    \
												\
	X3 = *RK++ ^ RT0[ ( Y3       ) & 0xFF ] ^   \
				 RT1[ ( Y2 >>  8 ) & 0xFF ] ^   \
				 RT2[ ( Y1 >> 16 ) & 0xFF ] ^   \
				 RT3[ ( Y0 >> 24 ) & 0xFF ];    \
}

void aes_encrypt_128(const uint32_t rk[RK_LEN], uint32_t *io) {
	const uint32_t *RK =  (uint32_t *)rk;
	uint32_t Y0, Y1, Y2, Y3,
		X0 = io[0] ^ *RK++,
		X1 = io[1] ^ *RK++,
		X2 = io[2] ^ *RK++,
		X3 = io[3] ^ *RK++;

	// loop unrolled
	AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);

	X0 = *RK++ ^ \
		((uint32_t)FSb[(Y0) & 0xFF]) ^
		((uint32_t)FSb[(Y1 >> 8) & 0xFF] << 8) ^
		((uint32_t)FSb[(Y2 >> 16) & 0xFF] << 16) ^
		((uint32_t)FSb[(Y3 >> 24) & 0xFF] << 24);

	X1 = *RK++ ^ \
		((uint32_t)FSb[(Y1) & 0xFF]) ^
		((uint32_t)FSb[(Y2 >> 8) & 0xFF] << 8) ^
		((uint32_t)FSb[(Y3 >> 16) & 0xFF] << 16) ^
		((uint32_t)FSb[(Y0 >> 24) & 0xFF] << 24);

	X2 = *RK++ ^ \
		((uint32_t)FSb[(Y2) & 0xFF]) ^
		((uint32_t)FSb[(Y3 >> 8) & 0xFF] << 8) ^
		((uint32_t)FSb[(Y0 >> 16) & 0xFF] << 16) ^
		((uint32_t)FSb[(Y1 >> 24) & 0xFF] << 24);

	// removed a ++ here
	X3 = *RK ^ \
		((uint32_t)FSb[(Y3) & 0xFF]) ^
		((uint32_t)FSb[(Y0 >> 8) & 0xFF] << 8) ^
		((uint32_t)FSb[(Y1 >> 16) & 0xFF] << 16) ^
		((uint32_t)FSb[(Y2 >> 24) & 0xFF] << 24);

	io[0] = X0;
	io[1] = X1;
	io[2] = X2;
	io[3] = X3;
}

void aes_decrypt_128(const uint32_t rk[RK_LEN], uint32_t *io) {
	const uint32_t *RK =  (uint32_t *)rk;
	uint32_t Y0, Y1, Y2, Y3,
		X0 = io[0] ^ *RK++,
		X1 = io[1] ^ *RK++,
		X2 = io[2] ^ *RK++,
		X3 = io[3] ^ *RK++;

	AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
	AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
	AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);

	X0 = *RK++ ^ \
		((uint32_t)RSb[(Y0) & 0xFF]) ^
		((uint32_t)RSb[(Y3 >> 8) & 0xFF] << 8) ^
		((uint32_t)RSb[(Y2 >> 16) & 0xFF] << 16) ^
		((uint32_t)RSb[(Y1 >> 24) & 0xFF] << 24);

	X1 = *RK++ ^ \
		((uint32_t)RSb[(Y1) & 0xFF]) ^
		((uint32_t)RSb[(Y0 >> 8) & 0xFF] << 8) ^
		((uint32_t)RSb[(Y3 >> 16) & 0xFF] << 16) ^
		((uint32_t)RSb[(Y2 >> 24) & 0xFF] << 24);

	X2 = *RK++ ^ \
		((uint32_t)RSb[(Y2) & 0xFF]) ^
		((uint32_t)RSb[(Y1 >> 8) & 0xFF] << 8) ^
		((uint32_t)RSb[(Y0 >> 16) & 0xFF] << 16) ^
		((uint32_t)RSb[(Y3 >> 24) & 0xFF] << 24);

	X3 = *RK ^ \
		((uint32_t)RSb[(Y3) & 0xFF]) ^
		((uint32_t)RSb[(Y2 >> 8) & 0xFF] << 8) ^
		((uint32_t)RSb[(Y1 >> 16) & 0xFF] << 16) ^
		((uint32_t)RSb[(Y0 >> 24) & 0xFF] << 24);

	io[0] = X0;
	io[1] = X1;
	io[2] = X2;
	io[3] = X3;
}
