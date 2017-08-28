
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <immintrin.h>
#include "ocl.h"
#include "crypto.h"
#include "utils.h"

#ifdef __GNUC__
#include <cpuid.h>
#elif _MSC_VER
#include <intrin.h>
#endif

int cpu_has_rdrand() {
#if __GNUC__
	unsigned a = 0, b = 0, c = 0, d = 0;
	__get_cpuid(1, &a, &b, &c, &d);
	return c & bit_RDRND;
#elif _MSC_VER
	int regs[4];
	__cpuid(regs, 1);
	return regs[2] & (1<<30);
#else
	// ICL only?
	return _may_i_use_cpu_feature(_FEATURE_RDRND);
#endif
}

// CAUTION: caller is responsible to free the buf
char * read_file(const char *file_name, size_t *p_size) {
	FILE * f = fopen(file_name, "rb");
	if (f == NULL) {
		printf("can't read file: %s", file_name);
		exit(-1);
	}
	fseek(f, 0, SEEK_END);
	*p_size = ftell(f);
	char * buf = malloc(*p_size);
	fseek(f, 0, SEEK_SET);
	fread(buf, *p_size, 1, f);
	fclose(f);
	return buf;
}

void read_files(unsigned num_files, const char *file_names[], char *ptrs[], size_t sizes[]) {
	for (unsigned i = 0; i < num_files; ++i) {
		ptrs[i] = read_file(file_names[i], &sizes[i]);
	}
}

void dump_to_file(const char *file_name, const void *buf, size_t len) {
	FILE *f = fopen(file_name, "wb");
	if (f == NULL) {
		printf("can't open file to write: %s\n", file_name);
		return;
	}
	fwrite(buf, len, 1, f);
	fclose(f);
}

#define TEST_SHA1_16		1
#define TEST_AES_128_ECB	2

#define BLOCK_SIZE 0x10
#define NUM_BLOCKS (1 << 24)
#define BLOCKS_PER_ITEM 1

void ocl_test(cl_device_id device_id, cl_uchar *buf_in, int test_case) {
	cl_int err;
	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	HP_Time t0, t1;
	long long td;

	const size_t num_items = NUM_BLOCKS / BLOCKS_PER_ITEM;
	const size_t io_buf_len = NUM_BLOCKS * BLOCK_SIZE;

	const char *source_names[] = { "cl/sha1_16.cl", "cl/aes_tables.cl", "cl/aes_128.cl", "cl/kernels.cl" };
	const unsigned num_sources = sizeof(source_names) / sizeof(char *);
	char *sources[sizeof(source_names) / sizeof(char *)];
	size_t source_sizes[sizeof(source_names) / sizeof(char *)];
	read_files(num_sources, source_names, sources, source_sizes);

	get_hp_time(&t0);
	// WTF? GCC complains if I pass char ** in to a function expecting const char **?
	cl_program program = OCL_ASSERT2(clCreateProgramWithSource(context, num_sources, (const char **)sources, source_sizes, &err));
	char options[0x100];
	sprintf(options, "-w -Werror -DBLOCKS_PER_ITEM=%d", BLOCKS_PER_ITEM);
	// printf("compiler options: %s\n", options);
	err = clBuildProgram(program, 0, NULL, options, NULL, NULL);
	get_hp_time(&t1);
	printf("%d microseconds for compile\n", (int)hp_time_diff(&t0, &t1));
	for (unsigned i = 0; i < num_sources; ++i) {
		free(sources[i]);
	}
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

	cl_uchar key[16];
	unsigned int aes_rk[RK_LEN];
	if (test_case == TEST_AES_128_ECB) {
		aes_gen_tables();
		for (unsigned i = 0; i < 16; ++i) {
			key[i] = rand() & 0xff;
		}
		printf("Key: %s\n", hexdump(key, 16, 0));
	}

	cl_mem mem_in = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, io_buf_len, NULL, &err));
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, io_buf_len, NULL, &err));
	cl_mem mem_key;
	if (test_case == TEST_AES_128_ECB) {
		mem_key = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, 16, NULL, &err));
	}

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_in, CL_TRUE, 0, io_buf_len, buf_in, 0, NULL, NULL));
	if (test_case == TEST_AES_128_ECB) {
		OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_key, CL_TRUE, 0, 16, key, 0, NULL, NULL));
	}
	get_hp_time(&t1);
	printf("%d microseconds for data upload\n", (int)hp_time_diff(&t0, &t1));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_out));
	if (test_case == TEST_AES_128_ECB) {
		OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_key));
	}

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	get_hp_time(&t0);
	// apparently, setting local work size to NULL doesn't affect performance, at least in this kind of work
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

	/*
	dump_to_file("r:/test_aes_in.bin", buf_in, io_buf_len);
	dump_to_file("r:/test_aes_out.bin", buf_out, io_buf_len);
	*/

	get_hp_time(&t0);
	if (test_case == TEST_SHA1_16) {
		for (unsigned offset = 0; offset < io_buf_len; offset += BLOCK_SIZE) {
			sha1_16(buf_in + offset, buf_in + offset);
		}
	} else {
		for (unsigned offset = 0; offset < io_buf_len; offset += BLOCK_SIZE) {
			// setting the same key over and over is stupid
			// yet we still do it to keep it in line with the OpenCL port
			// otherwise we can't test the set key in OpenCL
			aes_set_key_enc_128(aes_rk, key);
			aes_encrypt_128(aes_rk, buf_in + offset, buf_in + offset);
		}
	}
	get_hp_time(&t1);
	td = hp_time_diff(&t0, &t1);
	printf("%d microseconds for C(single thread), %.2f MB/s\n", (int)td, io_buf_len * 1.0f / td);

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

	free(buf_out);
	clReleaseMemObject(mem_in);
	clReleaseMemObject(mem_out);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
}

int main(int argc, const char *argv[]) {
	if (argc == 2 && !strcmp(argv[1], "info")) {
		cl_uint num_platforms;
		ocl_info(&num_platforms, 1);
	} else if (argc == 1){
		cl_platform_id platform_id;
		cl_device_id device_id;
		ocl_get_device(&platform_id, &device_id);
		if (platform_id == NULL || device_id == NULL) {
			return -1;
		}

		cl_uchar *buf_in = malloc(BLOCK_SIZE * NUM_BLOCKS);
		srand(2501);
		HP_Time t0, t1; long long td;
		get_hp_time(&t0);
		if(cpu_has_rdrand()){
			// ~190 MB/s @ X230, ~200 without the success check
			printf("randomize source buffer using RDRAND\n");
			unsigned long long *p = (unsigned long long *)buf_in;
			unsigned long long *p_end = (unsigned long long *)(buf_in + BLOCK_SIZE * NUM_BLOCKS);
			int success = 1;
			while (p < p_end) {
				success &= _rdrand64_step(p++);
			}
			if (!success) {
				printf("RDRND failed\n");
				exit(-1);
			}
		}else {
			printf("randomize source buffer using AES OFB\n");
			// rand() & 0xff is about ~60 MB/s @ X230
			// it's worse than that AES single thread C, so OFB it is
			// ~240 MB/s, even faster than RDRAND
			srand(2501);
			unsigned int aes_rk[RK_LEN];
			unsigned char key_iv[16 * 2];
			for (unsigned i = 0; i < 16 * 2; ++i) {
				key_iv[i] = rand() & 0xff;
			}
			aes_set_key_enc_128(aes_rk, key_iv);
			aes_encrypt_128(aes_rk, key_iv + 16, buf_in);
			unsigned char *p_in = buf_in, *p_out = buf_in + 16,
				*p_end = buf_in + BLOCK_SIZE * NUM_BLOCKS;
			while (p_out < p_end) {
				aes_encrypt_128(aes_rk, p_in, p_out);
				p_in = p_out;
				p_out += 16;
			}
		}
		get_hp_time(&t1);
		td = hp_time_diff(&t0, &t1);
		printf("%d microseconds for preparing test data, %.2f MB/s\n",
			(int)td, BLOCK_SIZE * NUM_BLOCKS * 1.0f / td);

		ocl_test(device_id, buf_in, TEST_SHA1_16);
		ocl_test(device_id, buf_in, TEST_AES_128_ECB);

		free(buf_in); 
#ifdef _WIN32
		system("pause");
#endif
	} else {
		printf("invalid args\n");
	}
	return 0;
}

