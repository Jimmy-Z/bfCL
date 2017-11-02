
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// more info: https://github.com/Jimmy-Z/TWLbf/blob/master/dsi.c

static const u64 DSi_KEY_Y[2] =
	{0xbd4dc4d30ab9dc76ull, 0xe1a00005202ddd1dull};

static const u64 DSi_KEY_MAGIC[2] =
	{0x2a680f5f1a4f3e79ull, 0xfffefb4e29590258ull};

static inline u64 u64be(const u8 *in){
	u64 out;
	u8 *out8 = (u8*)&out;
	out8[0] = in[7];
	out8[1] = in[6];
	out8[2] = in[5];
	out8[3] = in[4];
	out8[4] = in[3];
	out8[5] = in[2];
	out8[6] = in[1];
	out8[7] = in[0];
	return out;
}

static inline u32 u32be(const u8 *in){
	u32 out;
	u8 *out8 = (u8*)&out;
	out8[0] = in[3];
	out8[1] = in[2];
	out8[2] = in[1];
	out8[3] = in[0];
	return out;
}

// CAUTION this one doesn't work in-place
static inline void byte_reverse_16(u8 *out, const u8 *in){
	out[0] = in[15];
	out[1] = in[14];
	out[2] = in[13];
	out[3] = in[12];
	out[4] = in[11];
	out[5] = in[10];
	out[6] = in[9];
	out[7] = in[8];
	out[8] = in[7];
	out[9] = in[6];
	out[10] = in[5];
	out[11] = in[4];
	out[12] = in[3];
	out[13] = in[2];
	out[14] = in[1];
	out[15] = in[0];
}

static inline void xor_128(u64 *x, const u64 *a, const u64 *b){
	x[0] = a[0] ^ b[0];
	x[1] = a[1] ^ b[1];
}

static inline void add_128(u64 *a, const u64 *b){
	a[0] += b[0];
	if(a[0] < b[0]){
		a[1] += b[1] + 1;
	}else{
		a[1] += b[1];
	}
}

static inline void add_128_64(u64 *a, u64 b){
	a[0] += b;
	if(a[0] < b){
		a[1] += 1;
	}
}

// Answer to life, universe and everything.
static inline void rol42_128(u64 *a){
	u64 t = a[1];
	a[1] = (t << 42 ) | (a[0] >> 22);
	a[0] = (a[0] << 42 ) | (t >> 22);
}

// eMMC Encryption for MBR/Partitions (AES-CTR, with console-specific key)
static inline void dsi_make_key(u8 *key, u64 console_id){
	u32 h = console_id >> 32, l = (u32)console_id;
	u32 key_x[4] = {l, l ^ 0x24ee6906, h ^ 0xe65b601d, h};
	// Key = ((Key_X XOR Key_Y) + FFFEFB4E295902582A680F5F1A4F3E79h) ROL 42
	// equivalent to F_XY in twltool/f_xy.c
	xor_128((u64*)key_x, (u64*)key_x, DSi_KEY_Y);
	add_128((u64*)key_x, DSi_KEY_MAGIC);
	rol42_128((u64*)key_x);
	byte_reverse_16(key, (u8*)key_x);
}

static inline void dsi_make_ctr(u8 *ctr, const u8 *emmc_cid, u64 offset) {
	u8 emmc_cid_sha1_16[16];
	sha1_16(emmc_cid, emmc_cid_sha1_16);
	add_128_64((u64*)emmc_cid_sha1_16, offset);
	byte_reverse_16(ctr, emmc_cid_sha1_16);
}

static inline void dsi_make_xor(u8 *xor, const u8 *src, const u8 *ver) {
	u8 target_xor[16];
	xor_128((u64*)target_xor, (u64*)src, (u64*)ver);
	byte_reverse_16(xor, target_xor);
}

