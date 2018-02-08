/* sha256_16/12
 * again specialized to only take 16/12 bytes input and spit out the first 16/last 8 bytes
 * again code dug out from mbed TLS
 * https://github.com/ARMmbed/mbedtls/blob/development/library/sha256.c
 */

__constant const uint32_t K[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

#define  SHR(x,n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define R(t)                                    \
(                                               \
    W[t] = S1(W[t -  2]) + W[t -  7] +          \
           S0(W[t - 15]) + W[t - 16]            \
)

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

#ifdef SHA256_16
void sha256_16(unsigned char *io)
#elif defined SHA256_12
void sha256_12(unsigned char *io)
#endif
{
	uint32_t temp1, temp2, W[64];
	uint32_t A[8] = {
		0x6A09E667,
		0xBB67AE85,
		0x3C6EF372,
		0xA54FF53A,
		0x510E527F,
		0x9B05688C,
		0x1F83D9AB,
		0x5BE0CD19
	};
	unsigned int i;

	// padding and msglen identical/similar to sha1_16
	GET_UINT32_BE(W[0], io, 0);
	GET_UINT32_BE(W[1], io, 4);
	GET_UINT32_BE(W[2], io, 8);
#ifdef SHA256_16
	GET_UINT32_BE(W[3], io, 12);
	W[4] = 0x80000000u;
#elif defined SHA256_12
	W[3] = 0x80000000u;
	W[4] = 0;
#endif
	W[5] = 0; W[6] = 0; W[7] = 0;
	W[8] = 0; W[9] = 0; W[10] = 0; W[11] = 0;
	W[12] = 0; W[13] = 0; W[14] = 0;
#ifdef SHA256_16
	W[15] = 0x80u;
#elif defined SHA256_12
	W[15] = 0x60u;
#endif

	for (i = 0; i < 16; i += 8)
	{
		P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i + 0], K[i + 0]);
		P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i + 1], K[i + 1]);
		P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i + 2], K[i + 2]);
		P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i + 3], K[i + 3]);
		P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i + 4], K[i + 4]);
		P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i + 5], K[i + 5]);
		P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i + 6], K[i + 6]);
		P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i + 7], K[i + 7]);
	}

	for (i = 16; i < 64; i += 8)
	{
		P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(i + 0), K[i + 0]);
		P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(i + 1), K[i + 1]);
		P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(i + 2), K[i + 2]);
		P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(i + 3), K[i + 3]);
		P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(i + 4), K[i + 4]);
		P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(i + 5), K[i + 5]);
		P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(i + 6), K[i + 6]);
		P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(i + 7), K[i + 7]);
	}

#ifdef SHA256_16
	A[0] += 0x6A09E667;
	A[1] += 0xBB67AE85;
	A[2] += 0x3C6EF372;
	A[3] += 0xA54FF53A;

	PUT_UINT32_BE(A[0], io, 0);
	PUT_UINT32_BE(A[1], io, 4);
	PUT_UINT32_BE(A[2], io, 8);
	PUT_UINT32_BE(A[3], io, 12);
#elif defined SHA256_12
	A[6] += 0x1F83D9AB;
	A[7] += 0x5BE0CD19;

	PUT_UINT32_BE(A[6], io, 0);
	PUT_UINT32_BE(A[7], io, 4);
#endif
}
