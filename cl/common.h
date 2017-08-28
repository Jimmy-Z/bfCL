
typedef unsigned int uint32_t;

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long u64;

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

// OpenCL has these fancy address space qualifiers that can't be cast without
#define GET_UINT32_LE(n, b, i) \
	(n) = *(uint32_t*)(b + i)
#define GET_UINT32_LE_G(n, b, i) \
	(n) = *(__global uint32_t*)(b + i)
#define GET_UINT32_LE_C(n, b, i) \
	(n) = *(__constant uint32_t*)(b + i)
#define PUT_UINT32_LE(n, b, i) \
	*(uint32_t*)(b + i) = (n)
#define PUT_UINT32_LE_G(n, b, i) \
	*(__global uint32_t*)(b + i) = (n)

