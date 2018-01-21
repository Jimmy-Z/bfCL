
__kernel void test_msky(
	u32 k0, u32 k1, u32 k2, u32 k3,
	u32 v0, u32 v1, u32 v2, u32 v3,
	__global u32 *out)
{
	if (*out) {
		return;
	}

	k2 |= get_global_id(0);
	u32 io[4] = { k0, k1, k2, k3 };

	sha256_16((u8*)io);

	if (io[0] == v0 && io[1] == v1 && io[2] == v2 && io[3] == v3){
		*out = k2;
	}
}

