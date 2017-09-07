
#include <stdio.h>
#include "utils.h"
#include "ocl.h"
#include "ocl_brute.h"

int ocl_test();

int main(int argc, const char *argv[]) {
	int ret = 0;
	if (argc == 1) {
		ret = ocl_test();
	} else if (argc == 2 && !strcmp(argv[1], "info")) {
		cl_uint num_platforms;
		ocl_info(&num_platforms, 1);
	} else if (argc == 7){
		cl_uchar console_id[8], emmc_cid[16], src[16], ver[16], offset[2];
		hex2bytes(console_id, 8, argv[2], 1);
		hex2bytes(emmc_cid, 16, argv[3], 1);
		hex2bytes(offset, 2, argv[4], 1);
		hex2bytes(src, 16, argv[5], 1);
		hex2bytes(ver, 16, argv[6], 1);

		if(!strcmp(argv[1], "console_id")){
			ret = ocl_brute_console_id(console_id, emmc_cid, offset, src, ver, NORMAL);
		}else if(!strcmp(argv[1], "console_id_bcd")){
			ret = ocl_brute_console_id(console_id, emmc_cid, offset, src, ver, BCD);
		}else if(!strcmp(argv[1], "console_id_3ds")){
			ret = ocl_brute_console_id(console_id, emmc_cid, offset, src, ver, CTR);
		}else if(!strcmp(argv[1], "emmc_cid")){
			ret = ocl_brute_emmc_cid(console_id, emmc_cid, offset, src, ver);
		}else{
			puts("invalid parameters\n");
			ret = -1;
		}
	} else {
		printf("invalid parameters\n");
		ret = -1;
	}
#ifdef _WIN32
		system("pause");
#endif
	return ret;
}

