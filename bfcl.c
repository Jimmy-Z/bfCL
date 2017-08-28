
#include <stdio.h>
#include "ocl.h"

void ocl_test();

void ocl_brute();

int main(int argc, const char *argv[]) {
	if (argc == 2 && !strcmp(argv[1], "info")) {
		cl_uint num_platforms;
		ocl_info(&num_platforms, 1);
	} else if (argc == 2 && !strcmp(argv[1], "console_id")){
		ocl_brute();
	} else if (argc == 1){
		ocl_test();
#ifdef _WIN32
		system("pause");
#endif
	} else {
		printf("invalid args\n");
	}
	return 0;
}

