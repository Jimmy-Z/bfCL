
__kernel void test_emmc_cid(
	u64 emmc_cid_l, u64 emmc_cid_h,
	u64 sha1_16_l, u64 sha1_16_h,
	__global u64 *out)
{
	if (*out) {
		return;
	}
	u8 emmc_cid[16];
	*(u64*)emmc_cid = emmc_cid_l;
	*(u64*)(emmc_cid + 8) = emmc_cid_h;
	*(u32*)(emmc_cid + 1) |= get_global_id(0);

	sha1_16((u32*)emmc_cid);

	if (sha1_16_l == *(u64*)emmc_cid && sha1_16_h == *(u64*)(emmc_cid + 8)) {
		*out = get_global_id(0);
	}
}

