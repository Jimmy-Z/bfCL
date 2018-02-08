
__kernel void test_lfcs(
	u32 lfcs, u16 newflag,
	u32 v0, u32 v1,
	__global u32 *out)
{
	if (*out) {
		return;
	}

	u32 gid = (u32)get_global_id(0);
	// u32 gid = 0x5128;

	u32 io[3];
	io[0] = lfcs + (gid >> 16); // lfcs += gid_h
	io[1] = ((u32)newflag) | (gid << 16); // rand = gid_l
	io[2] = 0;

	sha256_12((u8*)io);

	// *out = io[1] - v1 + 1;
	if (io[0] == v0 && io[1] == v1){
		*out = gid;
	}
}

