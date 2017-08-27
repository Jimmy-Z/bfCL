
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
	if (f == NULL) {
		printf("can't read file: %s", file_name);
		exit(-1);
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	char * buf = malloc(size + 1);
	fseek(f, 0, SEEK_SET);
	fread(buf, size, 1, f);
	fclose(f);
	buf[size] = '\0';
	return buf;
}

#define TEST_SHA1_16		1
#define TEST_AES_128_ECB	2

void ocl_test(int test_case) {
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
	long long td;

#define BLOCK_SIZE 0x10
#define NUM_BLOCKS (1 << 24)
#define BLOCKS_PER_ITEM 1

	const size_t num_items = NUM_BLOCKS / BLOCKS_PER_ITEM;
	const size_t io_buf_len = NUM_BLOCKS * BLOCK_SIZE;

	char *source = read_source("kernel.c");
	get_hp_time(&t0);
	cl_program program = OCL_ASSERT2(clCreateProgramWithSource(context, 1, (const char**)&source, NULL, &err));
	char options[0x100];
	sprintf(options, "-DBLOCKS_PER_ITEM=%d", BLOCKS_PER_ITEM);
	printf("compiler options: %s\n", options);
	err = clBuildProgram(program, 0, NULL, options, NULL, NULL);
	get_hp_time(&t1);
	printf("%d microseconds for compile\n", (int)hp_time_diff(&t0, &t1));
	free(source);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "failed to build program, error: %s, build log:\n", ocl_err_msg(err));
		size_t len;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
		char *buf_log = malloc(len + 1);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buf_log, NULL);
		buf_log[len] = '\0';
		fprintf(stderr, "%s\n", buf_log);
		free(buf_log);
		exit(err);
	}

	const char * test_name;
	switch (test_case) {
		case TEST_SHA1_16: test_name = "sha1_16_test"; break;
		case TEST_AES_128_ECB: test_name = "aes_128_ecb_test"; break;
		default: exit(-1);
	}

	printf("%s on %u bytes\n", test_name, (unsigned)io_buf_len);

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, test_name, &err));

	cl_uchar *buf_in = malloc(io_buf_len);
	cl_uchar key[16];
	unsigned int aes_rk[RK_LEN];
	get_hp_time(&t0);
	srand(2501);
	for (unsigned i = 0; i < io_buf_len; ++i) {
		buf_in[i] = ((unsigned)rand() << 8) / RAND_MAX;
	}
	if (test_case == TEST_AES_128_ECB) {
		aes_gen_tables();
		for (unsigned i = 0; i < 16; ++i) {
			key[i] = ((unsigned)rand() << 8) / RAND_MAX;
		}
		aes_set_key_enc_128(aes_rk, key);
	}
	get_hp_time(&t1);
	printf("%d microseconds for preparing test data\n", (int)hp_time_diff(&t0, &t1));

	cl_mem mem_in = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, io_buf_len, NULL, &err));
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, io_buf_len, NULL, &err));
	cl_mem mem_tables, mem_key;
	if (test_case == TEST_AES_128_ECB) {
		mem_tables = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(AES_Tables), NULL, &err));
		mem_key = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, 16, NULL, &err));
	}

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_in, CL_TRUE, 0, io_buf_len, buf_in, 0, NULL, NULL));
	if (test_case == TEST_AES_128_ECB) {
		OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_tables, CL_TRUE, 0, sizeof(AES_Tables), &AES_tables, 0, NULL, NULL));
		OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_key, CL_TRUE, 0, 16, key, 0, NULL, NULL));
	}
	get_hp_time(&t1);
	printf("%d microseconds for data upload\n", (int)hp_time_diff(&t0, &t1));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_out));
	if (test_case == TEST_AES_128_ECB) {
		OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_tables));
		OCL_ASSERT(clSetKernelArg(kernel, 3, sizeof(cl_mem), &mem_key));
	}

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	get_hp_time(&t0);
	// appearantly, settiing local work size to NULL doesn't affect performance, at least in this kind of work
	OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
	// OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, NULL, 0, NULL, NULL));
	clFinish(command_queue);
	get_hp_time(&t1);
	td = hp_time_diff(&t0, &t1);
	printf("%d microseconds for OpenCL, %.2f MB/s\n", (int)td, io_buf_len * 1.0f / td);

	cl_uchar *buf_out = malloc(io_buf_len);
	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, io_buf_len, buf_out, 0, NULL, NULL));
	get_hp_time(&t1);
	printf("%d microseconds for data download\n", (int)hp_time_diff(&t0, &t1));

	get_hp_time(&t0);
	if (test_case == TEST_SHA1_16) {
		for (unsigned offset = 0; offset < io_buf_len; offset += BLOCK_SIZE) {
			sha1_16(buf_in + offset, buf_in + offset);
		}
	} else {
		for (unsigned offset = 0; offset < io_buf_len; offset += BLOCK_SIZE) {
			aes_encrypt_128(aes_rk, buf_in + offset, buf_in + offset);
		}
	}
	get_hp_time(&t1);
	td = hp_time_diff(&t0, &t1);
	printf("%d microseconds for C(single thread), %.3f MB/s\n", (int)td, io_buf_len * 1.0f / td);

	if (memcmp(buf_in, buf_out, io_buf_len)) {
		printf("%s: verification failed\n", test_name);
		for (unsigned offset = 0; offset < io_buf_len; offset += BLOCK_SIZE) {
			if (memcmp(buf_in + offset, buf_out + offset, BLOCK_SIZE)) {
				printf("first difference @ 0x%08x/0x%08x:\n", offset, (unsigned)num_items );
				printf("\t%s\n", hexdump(buf_in + offset, BLOCK_SIZE, 0));
				printf("\t%s\n", hexdump(buf_out + offset, BLOCK_SIZE, 0));
				break;
			}
		}
	} else {
		printf("%s: succeed\n", test_name);
	}

	free(buf_in); free(buf_out);
	clReleaseMemObject(mem_in);
	clReleaseMemObject(mem_out);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
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
		ocl_test(TEST_SHA1_16);
		ocl_test(TEST_AES_128_ECB);
#ifdef _WIN32
		system("pause");
#endif
	} else {
		printf("invalid args\n");
	}
	return 0;
}

