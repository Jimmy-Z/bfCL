
#include <stdio.h>
#include "utils.h"
#include "ocl.h"

void ocl_test();

void ocl_brute(const cl_uchar *console_id, const cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver, int mode);

int main(int argc, const char *argv[]) {
	if (argc == 2 && !strcmp(argv[1], "info")) {
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
			ocl_brute(console_id, emmc_cid, offset, src, ver, 0);
		}else if(!strcmp(argv[1], "console_id_bcd")){
			ocl_brute(console_id, emmc_cid, offset, src, ver, 1);
		}else{
			puts("invalid parameters\n");
		}
	} else if (argc == 1){
		ocl_test();
#ifdef _WIN32
		system("pause");
#endif
	} else {
		printf("invalid parameters\n");
	}
	return 0;
}

