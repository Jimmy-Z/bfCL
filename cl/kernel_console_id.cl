
// the caller should feed the target xor pad byte reversed as two uint64_t
// the ctr from emmc_cid_sha1 byte reversed as 4 uint32_t
__kernel void test_console_id(
	u64 console_id_template,
	u32 ctr0, u32 ctr1, u32 ctr2, u32 ctr3,
	u64 xor_l, u64 xor_h,
	__global u64 *out)
{
	if(*out){
		return;
	}
#ifdef BCD
	u64 console_id = to_dsi_bcd(get_global_id(0)) | console_id_template;
#else
	u64 console_id = get_global_id(0) | console_id_template;
#endif
	u64 dsi_key[2];
	dsi_make_key(dsi_key, console_id);

	u32 aes_rk[RK_LEN];
	byte_reverse_16((u8*)aes_rk, (u8*)dsi_key);
	aes_set_key_enc_128(aes_rk);

	u32 ctr[4] = {ctr0, ctr1, ctr2, ctr3};
	aes_encrypt_128(aes_rk, ctr);

	if(xor_l == *(u64*)ctr && xor_h == *(u64*)(ctr + 2)){
		*out = console_id;
	}
}

