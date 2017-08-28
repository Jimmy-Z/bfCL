
#include <stdio.h>
#include "utils.h"
#include "crypto.h"
#include "ocl.h"

void ocl_brute() {
	TimeHP t0, t1; long long td;

	cl_int err;
	cl_platform_id platform_id;
	cl_device_id device_id;
	ocl_get_device(&platform_id, &device_id);
	if (platform_id == NULL || device_id == NULL) {
		return;
	}


	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	const char *source_names[] = {
		"cl/common.h",
		"cl/aes_tables.cl",
		"cl/aes_128.cl",
		"cl/dsi.cl",
		"cl/kernel_console_id.cl" };
	cl_program program = ocl_build_from_sources(sizeof(source_names) / sizeof(char *),
		source_names, context, device_id, NULL /* "-w -Werror" */);

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, "test_console_id", &err));

	cl_ulong xor_l, xor_h, console_id_template = 0x08a1522617110100ull, out;
	cl_uint ctr0, ctr1, ctr2, ctr3;
	cl_int success = 0;

	cl_mem mem_success =
		OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int), NULL, &err));
	cl_mem mem_out =
		OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, &err));

	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_success, CL_TRUE, 0, sizeof(cl_int), &success, 0, NULL, NULL));

	OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_ulong), &xor_l));
	OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_ulong), &xor_h));
	OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_ulong), &console_id_template));
	OCL_ASSERT(clSetKernelArg(kernel, 3, sizeof(cl_uint), &ctr0));
	OCL_ASSERT(clSetKernelArg(kernel, 4, sizeof(cl_uint), &ctr1));
	OCL_ASSERT(clSetKernelArg(kernel, 5, sizeof(cl_uint), &ctr2));
	OCL_ASSERT(clSetKernelArg(kernel, 6, sizeof(cl_uint), &ctr3));
	OCL_ASSERT(clSetKernelArg(kernel, 7, sizeof(cl_mem), &mem_success));
	OCL_ASSERT(clSetKernelArg(kernel, 8, sizeof(cl_mem), &mem_out));

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	size_t num_items = 0x100;

	get_hp_time(&t0);
	OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
	clFinish(command_queue);
	get_hp_time(&t1); td = hp_time_diff(&t0, &t1);

	OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_success, CL_TRUE, 0, sizeof(cl_int), &success, 0, NULL, NULL));
	if (success) {
		// if success, the speed measurement is invalid
		printf("got a hit in %d microseconds\n", (int)td);
		OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));
		printf("%08x%08x\n", (unsigned)(out >> 32), (unsigned)(out|0xffffffffu));
	} else {
		printf("%d microseconds, %.2f M/s\n", (int)td, num_items * 1.0f / td);
		printf("sorry, no hit\n");
	}

	clReleaseKernel(kernel);
}