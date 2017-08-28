
// AES 128 ECB adapted for OpenCL, see "aes_128.c" for more info

#define RK_LEN 44

// the caller is responsible to put the key in rk
void aes_set_key_enc_128(uint32_t rk[RK_LEN])
{
	uint32_t *RK = rk;

	for (unsigned i = 0; i < 10; ++i, RK += 4) {
		RK[4]  = RK[0] ^ RCON[i] ^
		( (uint32_t) FSb[ ( RK[3] >>  8 ) & 0xFF ]       ) ^
		( (uint32_t) FSb[ ( RK[3] >> 16 ) & 0xFF ] <<  8 ) ^
		( (uint32_t) FSb[ ( RK[3] >> 24 ) & 0xFF ] << 16 ) ^
		( (uint32_t) FSb[ ( RK[3]       ) & 0xFF ] << 24 );

		RK[5]  = RK[1] ^ RK[4];
		RK[6]  = RK[2] ^ RK[5];
		RK[7]  = RK[3] ^ RK[6];
	}
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

void aes_encrypt_128(const uint32_t rk[RK_LEN],
	const uint32_t *in, uint32_t *out)
{
	const uint32_t *RK = rk;
	uint32_t X0 = in[0], X1 = in[1], X2 = in[2], X3 = in[3],
		Y0, Y1, Y2, Y3;

	X0 ^= *RK++;
	X1 ^= *RK++;
	X2 ^= *RK++;
	X3 ^= *RK++;

	AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
	AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
	AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
	AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
	AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
	AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
	AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
	AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
	AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );

	X0 = *RK++ ^ \
			( (uint32_t) FSb[ ( Y0       ) & 0xFF ]       ) ^
			( (uint32_t) FSb[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSb[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSb[ ( Y3 >> 24 ) & 0xFF ] << 24 );

	X1 = *RK++ ^ \
			( (uint32_t) FSb[ ( Y1       ) & 0xFF ]       ) ^
			( (uint32_t) FSb[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSb[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSb[ ( Y0 >> 24 ) & 0xFF ] << 24 );

	X2 = *RK++ ^ \
			( (uint32_t) FSb[ ( Y2       ) & 0xFF ]       ) ^
			( (uint32_t) FSb[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSb[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSb[ ( Y1 >> 24 ) & 0xFF ] << 24 );

	X3 = *RK ^ \
			( (uint32_t) FSb[ ( Y3       ) & 0xFF ]       ) ^
			( (uint32_t) FSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
			( (uint32_t) FSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
			( (uint32_t) FSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );

	out[0] = X0;
	out[1] = X1;
	out[2] = X2;
	out[3] = X3;
}
