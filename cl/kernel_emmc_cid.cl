
inline void memcpy64_from_constant(u64 *out, __constant const u64 *in, unsigned count) {
	for (unsigned i = 0; i < count; ++i) {
		*out++ = *in++;
	}
}

__kernel void test_emmc_cid(
	__constant u64 constant_aes_rk[RK_LEN >> 1],
	u64 emmc_cid_l, u64 emmc_cid_h,
	u64 offset,
	u64 xor_l, u64 xor_h,
	__global u64 *out)
{
	if (*out) {
		return;
	}
	u8 emmc_cid[16];
	*(u64*)emmc_cid = emmc_cid_l;
	*(u64*)(emmc_cid + 8) = emmc_cid_h;
	*(u32*)(emmc_cid + 1) |= get_global_id(0);
	u8 ctr[16];
	dsi_make_ctr(ctr, emmc_cid, offset);

	u32 aes_rk[RK_LEN];
	memcpy64_from_constant((u64*)aes_rk, constant_aes_rk, RK_LEN >> 1);

	aes_encrypt_128(aes_rk, (u32*)ctr);

	if (xor_l == *(u64*)ctr && xor_h == *(u64*)(ctr + 8)) {
		*out = get_global_id(0);
	}
}

