
__kernel void sha1_16_test(
	__global const unsigned char *in,
	__global unsigned char *out)
{
	unsigned offset = get_global_id(0) * BLOCKS_PER_ITEM * 16;
#if BLOCKS_PER_ITEM != 1
	for(unsigned i = 0; i < BLOCKS_PER_ITEM; ++i){
#endif
		uint32_t buf[4];
		GET_UINT32_BE(buf[0], in, offset);
		GET_UINT32_BE(buf[1], in, offset + 4);
		GET_UINT32_BE(buf[2], in, offset + 8);
		GET_UINT32_BE(buf[3], in, offset + 12);
		sha1_16(buf);
		PUT_UINT32_BE(buf[0], out, offset);
		PUT_UINT32_BE(buf[1], out, offset + 4);
		PUT_UINT32_BE(buf[2], out, offset + 8);
		PUT_UINT32_BE(buf[3], out, offset + 12);
#if BLOCKS_PER_ITEM != 1
		offset += 16;
	}
#endif
}
	
#define AES_BLOCK_SIZE 16

__kernel void aes_128_ecb_test(
	__global const uint32_t *key,
	__global const uint32_t *in,
	__global uint32_t *out)
{
	uint32_t rk[RK_LEN];
	rk[0] = key[0]; rk[1] = key[1]; rk[2] = key[2]; rk[3] = key[3];
	aes_set_key_enc_128(rk);
	unsigned offset = get_global_id(0) * BLOCKS_PER_ITEM * AES_BLOCK_SIZE / 4;
	in += offset; out += offset;
#if BLOCKS_PER_ITEM != 1
	for (unsigned i = 0; i < BLOCKS_PER_ITEM; ++i) {
#endif
		uint32_t buf[4];
		buf[0] = in[0]; buf[1] = in[1]; buf[2] = in[2]; buf[3] = in[3];
		aes_encrypt_128(rk, buf);
		out[0] = buf[0]; out[1] = buf[1]; out[2] = buf[2]; out[3] = buf[3];
#if BLOCKS_PER_ITEM != 1
		offset += AES_BLOCK_SIZE / 4;
	}
#endif
}
