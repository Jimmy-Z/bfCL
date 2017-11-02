
// dark_samus3's brilliant idea, doesn't need the EMMC CID beforehand
// instead, use two known sectors to verify against each other
// https://gbatemp.net/threads/twlbf-a-tool-to-brute-force-dsi-console-id-or-emmc-cid.481732/page-4#post-7661355
// the caller should feed two target xor pad byte reversed as two uint64_t
__kernel void test_console_id_ds3(
	u64 console_id_template,
	u32 offset0, u64 xor0_l, u64 xor0_h,
	u32 offset1, u64 xor1_l, u64 xor1_h,
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
	aes_set_key_dec_128(aes_rk);

	u64 xor0[2] = { xor0_l, xor0_h },
		xor1[2] = { xor1_l, xor1_h };

	aes_decrypt_128(aes_rk, (u32*)xor0);
	aes_decrypt_128(aes_rk, (u32*)xor1);

	u64 ctr0[2], ctr1[2];
	byte_reverse_16((u8*)ctr0, (u8*)xor0);
	byte_reverse_16((u8*)ctr1, (u8*)xor1);

	sub_128_64(ctr0, offset0);
	sub_128_64(ctr1, offset1);

	if(ctr0[0] == ctr1[0] && ctr0[1] == ctr1[1]){
		*out = console_id;
	}
}
