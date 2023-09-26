
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"
#include "ocl.h"
#include "ocl_brute.h"

int ocl_test();

static inline cl_ushort u16be(const unsigned char *in){
	cl_ushort out;
	unsigned char *out8 = (unsigned char *)&out;
	out8[0] = in[1];
	out8[1] = in[0];
	return out;
}

const char invalid_parameters[] = "invalid parameters\n";

int main(int argc, const char *argv[]) {
	int ret = 0;
	if (argc == 1) {
		ret = ocl_test();
	} else if (argc == 2 && !strcmp(argv[1], "info")) {
		cl_uint num_platforms;
		ocl_info(&num_platforms, 1);
	} else if (argc == 9) {
		unsigned char console_id[8],
			offset0[2], src0[16], ver0[16],
			offset1[2], src1[16], ver1[16];
		hex2bytes(console_id, 8, argv[2], 1);
		hex2bytes(offset0, 2, argv[3], 1);
		hex2bytes(src0, 16, argv[4], 1);
		hex2bytes(ver0, 16, argv[5], 1);
		hex2bytes(offset1, 2, argv[6], 1);
		hex2bytes(src1, 16, argv[7], 1);
		hex2bytes(ver1, 16, argv[8], 1);
		
		if (!strcmp(argv[1], "console_id")) {
			ret = ocl_brute_console_id(console_id, 0,
				u16be(offset0), src0, ver0, u16be(offset1), src1, ver1, NORMAL);
		} else if (!strcmp(argv[1], "console_id_bcd")) {
			ret = ocl_brute_console_id(console_id, 0,
				u16be(offset0), src0, ver0, u16be(offset1), src1, ver1, BCD);
		} else if (!strcmp(argv[1], "console_id_3ds")) {
			ret = ocl_brute_console_id(console_id, 0,
				u16be(offset0), src0, ver0, u16be(offset1), src1, ver1, CTR);
		} else {
			puts(invalid_parameters);
			ret = -1;
		}
	} else if (argc == 7) {
		unsigned char console_id[8], emmc_cid[16], offset[2], src[16], ver[16];
		hex2bytes(console_id, 8, argv[2], 1);
		hex2bytes(emmc_cid, 16, argv[3], 1);
		hex2bytes(offset, 2, argv[4], 1);
		hex2bytes(src, 16, argv[5], 1);
		hex2bytes(ver, 16, argv[6], 1);

		if (!strcmp(argv[1], "console_id")) {
			ret = ocl_brute_console_id(console_id, emmc_cid, u16be(offset), src, ver, 0, 0, 0, NORMAL);
		} else if (!strcmp(argv[1], "console_id_bcd")) {
			ret = ocl_brute_console_id(console_id, emmc_cid, u16be(offset), src, ver, 0, 0, 0, BCD);
		} else if (!strcmp(argv[1], "console_id_3ds")) {
			ret = ocl_brute_console_id(console_id, emmc_cid, u16be(offset), src, ver, 0, 0, 0, CTR);
		} else if (!strcmp(argv[1], "emmc_cid")) {
			ret = ocl_brute_emmc_cid(console_id, emmc_cid, u16be(offset), src, ver);
		} else {
			puts(invalid_parameters);
			ret = -1;
		}
	} else if(argc == 4 && !strcmp(argv[1], "msky")){
		uint32_t msky[4], ver[4];
		hex2bytes((unsigned char*)msky, 16, argv[2], 1);
		hex2bytes((unsigned char*)ver, 16, argv[3], 1);
		ret = ocl_brute_msky(msky, ver);
	} else if(argc == 5 && !strcmp(argv[1], "lfcs")){
		uint32_t lfcs, ver[2];
		uint16_t newflag;
		hex2bytes((unsigned char*)&lfcs, 4, argv[2], 1);
		hex2bytes((unsigned char*)&newflag, 2, argv[3], 1);
		hex2bytes((unsigned char*)ver, 8, argv[4], 1);
		ret = ocl_brute_lfcs(lfcs, newflag, ver);
	} else {
		printf(invalid_parameters);
		ret = -1;
	}
#ifdef _WIN32
	system("pause");
#endif
	return ret;
}

