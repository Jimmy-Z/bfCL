
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ocl.h"
#include "crypto.h"
#include "utils.h"

extern AES_Tables AES_tables;

// CAUTION: caller is responsible to free the buf
char * read_source(const char *file_name) {
	FILE * f = fopen(file_name, "rb");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	char * buf = malloc(size + 1);
	fseek(f, 0, SEEK_SET);
	fread(buf, size, 1, f);
	fclose(f);
	buf[size] = '\0';
	return buf;
}

void sha1_16_test() {
	cl_platform_id platform_id;
	cl_device_id device_id;
	ocl_get_device(&platform_id, &device_id);
	if (platform_id == NULL || device_id == NULL) {
		return;
	}
	cl_int err;
	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	HP_Time t0, t1;

	char *source = read_source("sha1_16.cl");
	get_hp_time(&t0);
	cl_program program = OCL_ASSERT2(clCreateProgramWithSource(context, 1, (const char**)&source, NULL, &err));
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	get_hp_time(&t1);
	printf("%d microseconds for build & compile\n", (int)hp_time_diff(&t0, &t1));
	free(source);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "failed to build program, error: %s, build log:\n", ocl_err_msg(err));
		size_t len;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
		char *buf_log = malloc(len + 1);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buf_log, NULL);
		buf_log[len] = '\0';
		fprintf(stderr, "%s\n", buf_log);
		exit(err);
	}

	cl_kernel kern_sha1_16 = OCL_ASSERT2(clCreateKernel(program, "sha1_16", &err));

#define ITEM_SIZE 0x10
	const size_t item_size = ITEM_SIZE;
	const size_t num_items = 1 << 24;
	const size_t io_buf_len = num_items * item_size;
	cl_uchar *buf_in = malloc(io_buf_len);
	srand(2501);
	get_hp_time(&t0);
	for (unsigned i = 0; i < io_buf_len; ++i) {
		buf_in[i] = ((unsigned)rand() << 8) / RAND_MAX;
	}
	get_hp_time(&t1);
	printf("%d microseconds for preparing test data\n", (int)hp_time_diff(&t0, &t1));

	cl_mem mem_in = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, io_buf_len, NULL, &err));
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, io_buf_len, NULL, &err));

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_in, CL_TRUE, 0, io_buf_len, buf_in, 0, NULL, NULL));
	get_hp_time(&t1);
	printf("%d microseconds for data upload\n", (int)hp_time_diff(&t0, &t1));

	OCL_ASSERT(clSetKernelArg(kern_sha1_16, 0, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kern_sha1_16, 1, sizeof(cl_mem), &mem_out));

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kern_sha1_16, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kern_sha1_16, 1, NULL, &num_items, &local, 0, NULL, NULL));
	clFinish(command_queue);
	get_hp_time(&t1);
	printf("%d microseconds for OpenCL\n", (int)hp_time_diff(&t0, &t1));

	cl_uchar *buf_out = malloc(io_buf_len);
	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, io_buf_len, buf_out, 0, NULL, NULL));
	get_hp_time(&t1);
	printf("%d microseconds for data download\n", (int)hp_time_diff(&t0, &t1));

	get_hp_time(&t0);
	for (unsigned offset = 0; offset < io_buf_len; offset += item_size) {
		// sha1_16 can works in place
		sha1_16(buf_in + offset, buf_in + offset);
	}
	get_hp_time(&t1);
	printf("%d microseconds for C(single thread)\n", (int)hp_time_diff(&t0, &t1));

	if (memcmp(buf_in, buf_out, io_buf_len)) {
		printf("%s: verification failed\n", __FUNCTION__);
		for (unsigned offset = 0; offset < io_buf_len; offset += item_size) {
			if (memcmp(buf_in + offset, buf_out + offset, item_size)) {
				printf("first difference @ 0x%08x/0x%08x:\n", offset, (unsigned)num_items );
				printf("\t%s\n", hexdump(buf_in + offset, (unsigned)item_size, 0));
				printf("\t%s\n", hexdump(buf_out + offset, (unsigned)item_size, 0));
				break;
			}
		}
	} else {
		printf("%s: succeed\n", __FUNCTION__);
	}

	free(buf_in); free(buf_out);
	clReleaseMemObject(mem_in);
	clReleaseMemObject(mem_out);
	clReleaseProgram(program);
	clReleaseKernel(kern_sha1_16);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
}

void aes128_test() {
	// TODO: test against OpenSSL results
	unsigned char test_key[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8 };
	unsigned char test_src[32] = { 8, 7, 6, 5, 4, 3, 2, 1, 1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1 };
	unsigned char test_out[32];

	aes_gen_tables();

	unsigned aes_rk[RK_LEN];

	aes_set_key_enc_128(aes_rk, test_key);
	aes_encrypt_128(aes_rk, test_src, test_out);
	aes_encrypt_128(aes_rk, test_src + 16, test_out + 16);
	puts(hexdump(test_out, 32, 1));
}

int main(int argc, const char *argv[]) {
	if (argc == 2 && !strcmp(argv[1], "info")) {
		cl_uint num_platforms;
		ocl_info(&num_platforms, 1);
	}else if (argc == 2 && !strcmp(argv[1], "aes128ecb")) {
		aes128_test();
	} else if (argc == 1){
		sha1_16_test();
#ifdef _WIN32
		system("pause");
#endif
	} else {
		printf("invalid args\n");
	}
	return 0;
}

