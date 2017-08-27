
#include <stdint.h>
#include "crypto.h"

/* AES 128 ECB dug out from mbed TLS 2.5.1
 * https://github.com/ARMmbed/mbedtls/blob/development/include/mbedtls/aes.h
 * https://github.com/ARMmbed/mbedtls/blob/development/library/aes.c
 *
 * C style comments are mbed TLS comments
 * C++ style comments are mine
 */

// x86/x86_64 CPU's are little endian
// popular OpenCL platforms(AMD, NVIDIA) are all little endian too
// so pointer cast instead of bit operations

#define GET_UINT32_LE(n, b, i) \
	(n) = *(uint32_t*)(b + i)
#define PUT_UINT32_LE(n, b, i) \
	*(uint32_t*)(b + i) = (n)

// packed into a struct to be easier to pass to OpenCL
// it's interesting they mix unsigned char with uint32_t
AES_Tables AES_tables;
// and aliases
static unsigned char *FSb = AES_tables.FSb;
static uint32_t *FT0 = AES_tables.FT0;
static uint32_t *FT1 = AES_tables.FT1;
static uint32_t *FT2 = AES_tables.FT2;
static uint32_t *FT3 = AES_tables.FT3;
static unsigned char *RSb = AES_tables.RSb;
static uint32_t *RT0 = AES_tables.RT0;
static uint32_t *RT1 = AES_tables.RT1;
static uint32_t *RT2 = AES_tables.RT2;
static uint32_t *RT3 = AES_tables.RT3;
static uint32_t *RCON = AES_tables.RCON;

/*
 * Tables generation code
 */
#define ROTL8(x) ( ( x << 8 ) & 0xFFFFFFFF ) | ( x >> 24 )
#define XTIME(x) ( ( x << 1 ) ^ ( ( x & 0x80 ) ? 0x1B : 0x00 ) )
#define MUL(x,y) ( ( x && y ) ? pow[(log[x]+log[y]) % 255] : 0 )

void aes_gen_tables( void )
{
    int i, x, y, z;
    int pow[256];
    int log[256];

    /*
     * compute pow and log tables over GF(2^8)
     */
    for( i = 0, x = 1; i < 256; i++ )
    {
        pow[i] = x;
        log[x] = i;
        x = ( x ^ XTIME( x ) ) & 0xFF;
    }

    /*
     * calculate the round constants
     */
    for( i = 0, x = 1; i < 10; i++ )
    {
        RCON[i] = (uint32_t) x;
        x = XTIME( x ) & 0xFF;
    }

    /*
     * generate the forward and reverse S-boxes
     */
    FSb[0x00] = 0x63;
    RSb[0x63] = 0x00;

    for( i = 1; i < 256; i++ )
    {
        x = pow[255 - log[i]];

        y  = x; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
        x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
        x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
        x ^= y; y = ( ( y << 1 ) | ( y >> 7 ) ) & 0xFF;
        x ^= y ^ 0x63;

        FSb[i] = (unsigned char) x;
        RSb[x] = (unsigned char) i;
    }

    /*
     * generate the forward and reverse tables
     */
    for( i = 0; i < 256; i++ )
    {
        x = FSb[i];
        y = XTIME( x ) & 0xFF;
        z =  ( y ^ x ) & 0xFF;

        FT0[i] = ( (uint32_t) y       ) ^
                 ( (uint32_t) x <<  8 ) ^
                 ( (uint32_t) x << 16 ) ^
                 ( (uint32_t) z << 24 );

        FT1[i] = ROTL8( FT0[i] );
        FT2[i] = ROTL8( FT1[i] );
        FT3[i] = ROTL8( FT2[i] );

        x = RSb[i];

        RT0[i] = ( (uint32_t) MUL( 0x0E, x )       ) ^
                 ( (uint32_t) MUL( 0x09, x ) <<  8 ) ^
                 ( (uint32_t) MUL( 0x0D, x ) << 16 ) ^
                 ( (uint32_t) MUL( 0x0B, x ) << 24 );

        RT1[i] = ROTL8( RT0[i] );
        RT2[i] = ROTL8( RT1[i] );
        RT3[i] = ROTL8( RT2[i] );
    }
}

#define RK_LEN 44

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

void aes_set_key_enc_128(uint32_t rk[RK_LEN], const unsigned char *key) {
	uint32_t *RK = rk;

	GET_UINT32_LE(RK[0], key, 0);
	GET_UINT32_LE(RK[1], key, 4);
	GET_UINT32_LE(RK[2], key, 8);
	GET_UINT32_LE(RK[3], key, 12);

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

void aes_encrypt_128( const uint32_t rk[RK_LEN],
                                  const unsigned char input[16],
                                  unsigned char output[16] )
{
    uint32_t X0, X1, X2, X3, Y0, Y1, Y2, Y3;
    const uint32_t *RK = rk;

    GET_UINT32_LE( X0, input,  0 );
    GET_UINT32_LE( X1, input,  4 );
    GET_UINT32_LE( X2, input,  8 );
    GET_UINT32_LE( X3, input, 12 );

	X0 ^= *RK++;
	X1 ^= *RK++;
	X2 ^= *RK++;
	X3 ^= *RK++;

	// loop unrolled
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

	// removed a ++ here
    X3 = *RK ^ \
            ( (uint32_t) FSb[ ( Y3       ) & 0xFF ]       ) ^
            ( (uint32_t) FSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
            ( (uint32_t) FSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
            ( (uint32_t) FSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );

    PUT_UINT32_LE( X0, output,  0 );
    PUT_UINT32_LE( X1, output,  4 );
    PUT_UINT32_LE( X2, output,  8 );
    PUT_UINT32_LE( X3, output, 12 );
}
