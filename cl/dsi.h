
// more about this: https://github.com/Jimmy-Z/TWLbf/blob/master/dsi.c

__constant static const u64 DSi_KEY_Y[2] =
	{0xbd4dc4d30ab9dc76ull, 0xe1a00005202ddd1dull};

__constant static const u64 DSi_KEY_MAGIC[2] =
	{0x2a680f5f1a4f3e79ull, 0xfffefb4e29590258ull};

// CAUTION this one doesn't work in-place
inline void byte_reverse_16(u8 *out, const u8 *in){
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

inline void add_128_64(u64 *a, u64 b){
	a[0] += b;
	if(a[0] < b){
		a[1] += 1;
	}
}

inline void sub_128_64(u64 *a, u64 b) {
	if (a[0] < b) {
		a[1] -= 1;
	}
	a[0] -= b;
}

// Answer to life, universe and everything.
inline void rol42_128(u64 *a){
	u64 t = a[1];
	a[1] = (t << 42 ) | (a[0] >> 22);
	a[0] = (a[0] << 42 ) | (t >> 22);
}

// eMMC Encryption for MBR/Partitions (AES-CTR, with console-specific key)
inline void dsi_make_key(u64 *key, u64 console_id){
	u32 h = console_id >> 32, l = (u32)console_id;
#ifdef CTR
	u32 key_x[4] = {(l ^ 0xb358a6af) | 0x80000000, 0x544e494e, 0x4f444e45, h ^ 0x08c267b7};
#else
	u32 key_x[4] = {l, l ^ 0x24ee6906, h ^ 0xe65b601d, h};
#endif
	// Key = ((Key_X XOR Key_Y) + FFFEFB4E295902582A680F5F1A4F3E79h) ROL 42
	// equivalent to F_XY in twltool/f_xy.c
	// xor_128(key, (u64*)key_x, DSi_KEY_Y);
	key[0] = ((u64*)key_x)[0] ^ DSi_KEY_Y[0];
	key[1] = ((u64*)key_x)[1] ^ DSi_KEY_Y[1];
	// add_128(key, DSi_KEY_MAGIC);
	key[0] += DSi_KEY_MAGIC[0];
	if(key[0] < DSi_KEY_MAGIC[0]){
		key[1] += DSi_KEY_MAGIC[1] + 1;
	}else{
		key[1] += DSi_KEY_MAGIC[1];
	}
	rol42_128(key);
}

