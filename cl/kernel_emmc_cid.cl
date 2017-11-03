
__kernel void test_emmc_cid(
	u64 emmc_cid_l, u64 emmc_cid_h,
	u64 sha1_16_l, u64 sha1_16_h,
	__global u64 *out)
{
	if (*out) {
		return;
	}
	u64 emmc_cid[2] = { emmc_cid_l, emmc_cid_h };
	*(u32*)(((u8*)emmc_cid) + 1) |= get_global_id(0);

	sha1_16((u8*)emmc_cid);

	if (sha1_16_l == emmc_cid[0] && sha1_16_h == emmc_cid[1]) {
		*out = get_global_id(0);
	}
}

