
#include <stdio.h>
#include "utils.h"
#include "ocl.h"
#include "crypto.h"

#define BLOCK_SIZE 0x10
#define NUM_BLOCKS (1 << 23)
#define BLOCKS_PER_ITEM 1
#define NUM_ITEMS (NUM_BLOCKS / BLOCKS_PER_ITEM)
#define BUF_SIZE (BLOCK_SIZE * NUM_BLOCKS)

void ocl_test_run_and_read(const char * test_name, cl_kernel kernel,
	cl_device_id device_id, cl_command_queue command_queue,
	cl_mem mem_out, cl_uchar *buf_out)
{
	printf("# %s on %u MB\n", test_name, (unsigned)BUF_SIZE >> 20);
	TimeHP t0, t1; long long td;
	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	size_t num_items = NUM_ITEMS;

	get_hp_time(&t0);
	// apparently, setting local work size to NULL doesn't affect performance, at least in this kind of work
	OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
	// OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, NULL, 0, NULL, NULL));
	clFinish(command_queue);
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for OpenCL, %.2f MB/s\n", td / 1000000.0, BUF_SIZE * 1.0 / td);

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, BUF_SIZE, buf_out, 0, NULL, NULL));
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for data download, %.2f MB/s\n", td / 1000000.0, BUF_SIZE * 1.0 / td);

	clReleaseKernel(kernel);
}

int verify(const char *test_name, cl_uchar *buf_in, cl_uchar *buf_out, cl_uchar *buf_verify){
	if (memcmp(buf_verify, buf_out, BUF_SIZE)) {
		printf("%s: verification failed\n", test_name);
		unsigned count = 5;
		for (unsigned offset = 0; offset < BUF_SIZE; offset += BLOCK_SIZE) {
			if (memcmp(buf_verify + offset, buf_out + offset, BLOCK_SIZE)) {
				printf("difference @ 0x%08x/0x%08x:\n", offset, (unsigned)NUM_BLOCKS);
				printf("\t%s\n", hexdump(buf_in + offset, BLOCK_SIZE, 1));
				printf("\t%s\n", hexdump(buf_out + offset, BLOCK_SIZE, 1));
				printf("\t%s\n", hexdump(buf_verify + offset, BLOCK_SIZE, 1));
				if (!--count) {
					break;
				}
			}
		}
		return 1;
	} else {
		printf("%s: succeed\n", test_name);
		return 0;
	}
}

int ocl_test() {
	TimeHP t0, t1; long long td;

	cl_int err;
	cl_platform_id platform_id;
	cl_device_id device_id;
	ocl_get_device(&platform_id, &device_id);
	if (platform_id == NULL || device_id == NULL) {
		return -1;
	}

	cl_uchar *buf_in = malloc(BUF_SIZE);
	cl_uchar *buf_out = malloc(BUF_SIZE);
	cl_uchar *buf_verify = malloc(BUF_SIZE);

	srand(2501);
	cl_uchar key[16];
	unsigned int aes_rk[RK_LEN];
	aes_gen_tables();
	for (unsigned i = 0; i < 16; ++i) {
		key[i] = rand() & 0xff;
	}
	printf("AES Key: %s\n", hexdump(key, 16, 0));
	get_hp_time(&t0);
	if(cpu_has_rdrand()){
		// ~190 MB/s @ X230, ~200 without the success check
		printf("randomize source buffer using RDRAND\n");
		if (!rdrand_fill((cl_ulong*)buf_in, BUF_SIZE >> 3)) {
			printf("RDRND failed\n");
			exit(-1);
		}
	}else {
		printf("randomize source buffer using AES OFB\n");
		// rand() & 0xff is about ~60 MB/s @ X230
		// it's worse than that AES single thread C, so OFB it is
		// ~240 MB/s, even faster than RDRAND
		unsigned int aes_rk[RK_LEN];
		unsigned char key_iv[16 * 2];
		for (unsigned i = 0; i < 16 * 2; ++i) {
			key_iv[i] = rand() & 0xff;
		}
		aes_set_key_enc_128(aes_rk, key_iv);
		aes_encrypt_128(aes_rk, key_iv + 16, buf_in);
		unsigned char *p_in = buf_in, *p_out = buf_in + 16,
			*p_end = buf_in + BUF_SIZE;
		while (p_out < p_end) {
			aes_encrypt_128(aes_rk, p_in, p_out);
			p_in = p_out;
			p_out += 16;
		}
	}
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for preparing test data, %.2f MB/s\n",
		td / 1000000.0, BUF_SIZE * 1.0 / td);

	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	const char *source_names[] = {
		"cl/common.h",
		"cl/sha1_16.cl",
		"cl/aes_tables.cl",
		"cl/aes_128.cl",
		"cl/kernel_tests.cl" };
	char options[0x100];
	sprintf(options, "-w -Werror -DBLOCKS_PER_ITEM=%u", BLOCKS_PER_ITEM);
	cl_program program = ocl_build_from_sources(sizeof(source_names) / sizeof(char *),
		source_names, context, device_id, options);

	// create buffer and upload data
	cl_mem mem_key = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, 16, NULL, &err));
	cl_mem mem_in = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, BUF_SIZE, NULL, &err));
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, BUF_SIZE, NULL, &err));

	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_key, CL_TRUE, 0, 16, key, 0, NULL, NULL));
	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_in, CL_TRUE, 0, BUF_SIZE, buf_in, 0, NULL, NULL));
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for data upload, %.2f MB/s\n", td / 1000000.0, BUF_SIZE * 1.0 / td);

	// SHA1_16 test
	const char * test_name = "sha1_16_test";

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, test_name, &err));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_out));

	ocl_test_run_and_read(test_name, kernel, device_id, command_queue, mem_out, buf_out);

	get_hp_time(&t0);
	for (unsigned offset = 0; offset < BUF_SIZE; offset += BLOCK_SIZE) {
		sha1_16(buf_in + offset, buf_verify + offset);
	}
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for reference C(single thread), %.2f MB/s\n", td / 1000000.0, BUF_SIZE * 1.0 / td);

	int succeed = verify(test_name, buf_in, buf_out, buf_verify);

	// AES encrypt test
	test_name = "aes_enc_128_test";

	kernel = OCL_ASSERT2(clCreateKernel(program, test_name, &err));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_key));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_out));

	ocl_test_run_and_read(test_name, kernel, device_id, command_queue, mem_out, buf_out);

	get_hp_time(&t0);
	for (unsigned offset = 0; offset < BUF_SIZE; offset += BLOCK_SIZE) {
		// setting the same key over and over is stupid
		// we do this to make the results comparable
		aes_set_key_enc_128(aes_rk, key);
		aes_encrypt_128(aes_rk, buf_in + offset, buf_verify + offset);
	}
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	printf("%.3f seconds for reference C(single thread), %.2f MB/s\n", td / 1000000.0, BUF_SIZE * 1.0f / td);
	// dump_to_file("r:/test_aes_in.bin", buf_in, BUF_SIZE);
	// dump_to_file("r:/test_aes_out.bin", buf_out, BUF_SIZE);
	// dump_to_file("r:/test_aes_verify.bin", buf_verify, BUF_SIZE);

	succeed |= verify(test_name, buf_in, buf_out, buf_verify);

	// AES decrypt test
	test_name = "aes_dec_128_test";
	// use the encrypt output as input
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_in, CL_TRUE, 0, BUF_SIZE, buf_out, 0, NULL, NULL));

	kernel = OCL_ASSERT2(clCreateKernel(program, test_name, &err));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_key));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_in));
	OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_out));

	// read out to buf_verify
	ocl_test_run_and_read(test_name, kernel, device_id, command_queue, mem_out, buf_verify);

	// verify against buf_in
	succeed |= verify(test_name, buf_out, buf_verify, buf_in);

	// cleanup
	free(buf_in); free(buf_out); free(buf_verify);
	clReleaseMemObject(mem_in);
	clReleaseMemObject(mem_out);
	clReleaseMemObject(mem_key);
	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	return succeed;
}

