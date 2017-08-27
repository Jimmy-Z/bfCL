
// see sha1_16.c and aes128.c

typedef unsigned int uint32_t;

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
	(n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
		| ( (uint32_t) (b)[(i) + 1] << 16 )             \
		| ( (uint32_t) (b)[(i) + 2] <<  8 )             \
		| ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
	(b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
	(b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
	(b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
	(b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

void sha1_16(const uint32_t *in, uint32_t *out){
	const uint32_t
		h0 = 0x67452301,
		h1 = 0xEFCDAB89,
		h2 = 0x98BADCFE,
		h3 = 0x10325476,
		h4 = 0xC3D2E1F0;

	uint32_t temp, W[16],
		A = h0, B = h1, C = h2, D = h3, E = h4;

	W[0] = in[0]; W[1] = in[1]; W[2] = in[2]; W[3] = in[3];
	W[4] = 0x80000000u; W[5] = 0; W[6] = 0; W[7] = 0;
	W[8] = 0; W[9] = 0; W[10] = 0; W[11] = 0;
	W[12] = 0; W[13] = 0; W[14] = 0; W[15] = 0x80u;

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
	temp = W[( t -  3 ) & 0x0F] ^ W[( t - 8 ) & 0x0F] ^ \
		   W[( t - 14 ) & 0x0F] ^ W[  t       & 0x0F],  \
	( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
	e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

	P( A, B, C, D, E, W[0]  );
	P( E, A, B, C, D, W[1]  );
	P( D, E, A, B, C, W[2]  );
	P( C, D, E, A, B, W[3]  );
	P( B, C, D, E, A, W[4]  );
	P( A, B, C, D, E, W[5]  );
	P( E, A, B, C, D, W[6]  );
	P( D, E, A, B, C, W[7]  );
	P( C, D, E, A, B, W[8]  );
	P( B, C, D, E, A, W[9]  );
	P( A, B, C, D, E, W[10] );
	P( E, A, B, C, D, W[11] );
	P( D, E, A, B, C, W[12] );
	P( C, D, E, A, B, W[13] );
	P( B, C, D, E, A, W[14] );
	P( A, B, C, D, E, W[15] );
	P( E, A, B, C, D, R(16) );
	P( D, E, A, B, C, R(17) );
	P( C, D, E, A, B, R(18) );
	P( B, C, D, E, A, R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

	P( A, B, C, D, E, R(20) );
	P( E, A, B, C, D, R(21) );
	P( D, E, A, B, C, R(22) );
	P( C, D, E, A, B, R(23) );
	P( B, C, D, E, A, R(24) );
	P( A, B, C, D, E, R(25) );
	P( E, A, B, C, D, R(26) );
	P( D, E, A, B, C, R(27) );
	P( C, D, E, A, B, R(28) );
	P( B, C, D, E, A, R(29) );
	P( A, B, C, D, E, R(30) );
	P( E, A, B, C, D, R(31) );
	P( D, E, A, B, C, R(32) );
	P( C, D, E, A, B, R(33) );
	P( B, C, D, E, A, R(34) );
	P( A, B, C, D, E, R(35) );
	P( E, A, B, C, D, R(36) );
	P( D, E, A, B, C, R(37) );
	P( C, D, E, A, B, R(38) );
	P( B, C, D, E, A, R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

	P( A, B, C, D, E, R(40) );
	P( E, A, B, C, D, R(41) );
	P( D, E, A, B, C, R(42) );
	P( C, D, E, A, B, R(43) );
	P( B, C, D, E, A, R(44) );
	P( A, B, C, D, E, R(45) );
	P( E, A, B, C, D, R(46) );
	P( D, E, A, B, C, R(47) );
	P( C, D, E, A, B, R(48) );
	P( B, C, D, E, A, R(49) );
	P( A, B, C, D, E, R(50) );
	P( E, A, B, C, D, R(51) );
	P( D, E, A, B, C, R(52) );
	P( C, D, E, A, B, R(53) );
	P( B, C, D, E, A, R(54) );
	P( A, B, C, D, E, R(55) );
	P( E, A, B, C, D, R(56) );
	P( D, E, A, B, C, R(57) );
	P( C, D, E, A, B, R(58) );
	P( B, C, D, E, A, R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

	P( A, B, C, D, E, R(60) );
	P( E, A, B, C, D, R(61) );
	P( D, E, A, B, C, R(62) );
	P( C, D, E, A, B, R(63) );
	P( B, C, D, E, A, R(64) );
	P( A, B, C, D, E, R(65) );
	P( E, A, B, C, D, R(66) );
	P( D, E, A, B, C, R(67) );
	P( C, D, E, A, B, R(68) );
	P( B, C, D, E, A, R(69) );
	P( A, B, C, D, E, R(70) );
	P( E, A, B, C, D, R(71) );
	P( D, E, A, B, C, R(72) );
	P( C, D, E, A, B, R(73) );
	P( B, C, D, E, A, R(74) );
	P( A, B, C, D, E, R(75) );
	P( E, A, B, C, D, R(76) );
	P( D, E, A, B, C, R(77) );
	P( C, D, E, A, B, R(78) );
	P( B, C, D, E, A, R(79) );

#undef K
#undef F

#undef S
#undef R
#undef P

	A += h0;
	B += h1;
	C += h2;
	D += h3;

	out[0] = A;
	out[1] = B;
	out[2] = C;
	out[3] = D;
}

// cl doesn't have include, keep this identical to crypto.h
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
// I hope this doesn't induce any performance penalties
#define FSb p_tables->FSb
#define FT0 p_tables->FT0
#define FT1 p_tables->FT1
#define FT2 p_tables->FT2
#define FT3 p_tables->FT3
/*
#define RSb p_tables->RSb
#define RT0 p_tables->RT0
#define RT1 p_tables->RT1
#define RT2 p_tables->RT2
#define RT3 p_tables->RT3
*/
#define RCON p_tables->RCON

/* OpenCL doesn't allow this kind of pointer cast
#define GET_UINT32_LE(n, b, i) \
	(n) = *(uint32_t*)(b + i)
#define PUT_UINT32_LE(n, b, i) \
	*(uint32_t*)(b + i) = (n)
*/

#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ]       )             \
        | ( (uint32_t) (b)[(i) + 1] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 2] << 16 )             \
        | ( (uint32_t) (b)[(i) + 3] << 24 );            \
}
#endif

#ifndef PUT_UINT32_LE
#define PUT_UINT32_LE(n,b,i)                                    \
{                                                               \
    (b)[(i)    ] = (unsigned char) ( ( (n)       ) & 0xFF );    \
    (b)[(i) + 1] = (unsigned char) ( ( (n) >>  8 ) & 0xFF );    \
    (b)[(i) + 2] = (unsigned char) ( ( (n) >> 16 ) & 0xFF );    \
    (b)[(i) + 3] = (unsigned char) ( ( (n) >> 24 ) & 0xFF );    \
}
#endif

#define RK_LEN 44

void aes_set_key_enc_128(__global const AES_Tables *p_tables,
	uint32_t rk[RK_LEN]
){
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

inline void aes_encrypt_128(__global const AES_Tables *p_tables,
	const uint32_t rk[RK_LEN],
	const uint32_t *in, uint32_t *out
){
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

__kernel void sha1_16_test(
	__global const unsigned char *in,
	__global unsigned char *out
){
	unsigned offset = get_global_id(0) * BLOCKS_PER_ITEM * 16;
#if BLOCKS_PER_ITEM != 1
	for(unsigned i = 0; i < BLOCKS_PER_ITEM; ++i){
#endif
		uint32_t local_buf[4];
		GET_UINT32_BE(local_buf[0], in, offset);
		GET_UINT32_BE(local_buf[1], in, offset + 4);
		GET_UINT32_BE(local_buf[2], in, offset + 8);
		GET_UINT32_BE(local_buf[3], in, offset + 12);
		sha1_16(local_buf, local_buf);
		PUT_UINT32_BE(local_buf[0], out, offset);
		PUT_UINT32_BE(local_buf[1], out, offset + 4);
		PUT_UINT32_BE(local_buf[2], out, offset + 8);
		PUT_UINT32_BE(local_buf[3], out, offset + 12);
#if BLOCKS_PER_ITEM != 1
		offset += 16;
	}
#endif
}
	
#define AES_BLOCK_SIZE 16;

__kernel void aes_128_ecb_test(
	__global const unsigned char *in,
	__global unsigned char *out,
	__global const AES_Tables *p_tables,
	__global const unsigned char *key
){
	uint32_t rk[RK_LEN];
	GET_UINT32_LE(rk[0], key, 0);
	GET_UINT32_LE(rk[1], key, 4);
	GET_UINT32_LE(rk[2], key, 8);
	GET_UINT32_LE(rk[3], key, 12);
	aes_set_key_enc_128(p_tables, rk);
	unsigned offset = get_global_id(0) * BLOCKS_PER_ITEM * AES_BLOCK_SIZE;
#if BLOCKS_PER_ITEM != 1
	for (unsigned i = 0; i < BLOCKS_PER_ITEM; ++i) {
#endif
		uint32_t local_buf[4];
		GET_UINT32_LE(local_buf[0], in, offset);
		GET_UINT32_LE(local_buf[1], in, offset + 4);
		GET_UINT32_LE(local_buf[2], in, offset + 8);
		GET_UINT32_LE(local_buf[3], in, offset + 12);
		aes_encrypt_128(p_tables, rk, local_buf, local_buf);
		PUT_UINT32_LE(local_buf[0], out, offset);
		PUT_UINT32_LE(local_buf[1], out, offset + 4);
		PUT_UINT32_LE(local_buf[2], out, offset + 8);
		PUT_UINT32_LE(local_buf[3], out, offset + 12);
#if BLOCKS_PER_ITEM != 1
		offset += AES_BLOCK_SIZE;
	}
#endif
}
