
#include <stdio.h>
#include "utils.h"
#include "crypto.h"
#include "ocl.h"
#include "dsi.h"

// 123 -> 0x123
static cl_ulong to_bcd(cl_ulong i) {
	cl_ulong o = 0;
	unsigned shift = 0;
	while (i) {
		o |= (i % 10) << (shift++ << 2);
		i /= 10;
	}
	return o;
}

// 0x123 -> 123
static cl_ulong from_bcd(cl_ulong i) {
	cl_ulong o = 0;
	cl_ulong pow = 1;
	while (i) {
		o += (i & 0xf) * pow;
		i >>= 4;
		pow *= 10;
	}
	return o;
}

int ocl_brute_console_id(const cl_uchar *console_id, const cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver, int bcd)
{
	TimeHP t0, t1; long long td = 0;

	cl_int err;
	cl_platform_id platform_id;
	cl_device_id device_id;
	ocl_get_device(&platform_id, &device_id);
	if (platform_id == NULL || device_id == NULL) {
		return -1;
	}

	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	const char *source_names[] = {
		"cl/common.h",
		"cl/sha1_16.cl",
		"cl/aes_tables.cl",
		"cl/aes_128.cl",
		"cl/dsi.cl",
		"cl/kernel_console_id.cl" };
	cl_program program = ocl_build_from_sources(sizeof(source_names) / sizeof(char *),
		source_names, context, device_id, bcd ? "-w -Werror -DBCD" : "-w -Werror");

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, "test_console_id", &err));

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	// preparing args
	cl_ulong console_id_template = u64be(console_id);
	cl_ulong xor[2];
	dsi_make_xor((u8*)xor, src, ver);
	cl_uint ctr[4];
	dsi_make_ctr((u8*)ctr, emmc_cid, u16be(offset));
	cl_ulong out = 0;
#if DEBUG
	{
		printf("XOR     : %s\n", hexdump(xor, 16, 0));
		u8 aes_key[16];
		dsi_make_key(aes_key, u64be(console_id));
		printf("AES KEY : %s\n", hexdump(aes_key, 16, 0));
		cl_uint aes_rk[RK_LEN];
		aes_gen_tables();
		aes_set_key_enc_128(aes_rk, aes_key);
		printf("AES RK  : %s\n", hexdump(aes_rk, 48, 0));
		printf("CTR     : %s\n", hexdump(ctr, 16, 0));
		aes_encrypt_128(aes_rk, (u8*)ctr, (u8*)xor);
		printf("XOR TRY : %s\n", hexdump(xor, 16, 0));
		// exit(1);
	}
#endif

	// there's no option to create it zero initialized
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &err));
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));

	unsigned template_bits;
	cl_ulong total;
	unsigned group_bits;
	size_t num_items;
	unsigned loops;
	if (bcd) {
		// 0x08a15????????1?? as example, 10 digit to brute, beware the known digit near the end
		// 5 BCD digits, used to calculate template mask
		template_bits = 20;
		// I wish we could use 1e10 in C, counting 0 is not good to your eye
		total = from_bcd(1ull << 40);
		// work items variations on lower bits per enqueue, 8 + 1 digits, including the known digit
		group_bits = 36;
		// work items per enqueue, don't count the known digit here
		num_items = from_bcd(1ull << (group_bits - 4));
		// between the template bits and group bits, it's the loop bits
		loops = from_bcd(1 << (64 - template_bits - group_bits));
	} else {
		template_bits = 32;
		total = 1ull << (64 - template_bits);
		group_bits = 28;
		num_items = 1ull << group_bits; // 268435456
		loops = 1 << (64 - template_bits - group_bits);
	}
	// it needs to be aligned, a little overhead hurts nobody
	if (num_items % local) {
		// printf("alignment: %"LL"d -> ", num_items);
		num_items = (num_items / local + 1) * local;
		// printf("%"LL"d\n", num_items);
	}
	console_id_template &= ((cl_ulong)-1ll) << (64 - template_bits);
	// printf("total: %I64u, 0x%I64x\n", total, total);
	// printf("num_items: %I64u, 0x%I64x\n", num_items, num_items);
	// printf("steps: %u, 0x%x\n", steps, steps);
	get_hp_time(&t0);
	for (unsigned i = 0; i < loops; ++i) {
		cl_ulong console_id = console_id_template;
		if (bcd) {
			console_id |= to_bcd(i) << group_bits;
		} else {
			console_id |= i << group_bits;
		}
		printf("%016"LL"x\n", console_id);
		OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_ulong), &console_id));
		OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_uint), &ctr[0]));
		OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_uint), &ctr[1]));
		OCL_ASSERT(clSetKernelArg(kernel, 3, sizeof(cl_uint), &ctr[2]));
		OCL_ASSERT(clSetKernelArg(kernel, 4, sizeof(cl_uint), &ctr[3]));
		OCL_ASSERT(clSetKernelArg(kernel, 5, sizeof(cl_ulong), &xor[0]));
		OCL_ASSERT(clSetKernelArg(kernel, 6, sizeof(cl_ulong), &xor[1]));
		OCL_ASSERT(clSetKernelArg(kernel, 7, sizeof(cl_mem), &mem_out));

		OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
		clFinish(command_queue);

		OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));
		if (out) {
			get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
			printf("got a hit: %016"LL"x\n", out);
			// also write to a file
			dump_to_file(hexdump(emmc_cid, 16, 0), &out, 8);
			break;
		}
	}

	u64 tested = 0;
	if (!out) {
		tested = total;
		get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	} else {
		tested = out - console_id_template;
		if (bcd) {
			tested = from_bcd(((tested & ~0xfff) >> 4) | (tested & 0xff));
		}
	}
	printf("%.2f seconds, %.2f M/s\n", td / 1000000.0, tested * 1.0 / td);

	clReleaseKernel(kernel);
	return !out;
}

int ocl_brute_emmc_cid(const cl_uchar *console_id, cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver)
{
	TimeHP t0, t1; long long td = 0;

	cl_int err;
	cl_platform_id platform_id;
	cl_device_id device_id;
	ocl_get_device(&platform_id, &device_id);
	if (platform_id == NULL || device_id == NULL) {
		return -1;
	}

	cl_context context = OCL_ASSERT2(clCreateContext(0, 1, &device_id, NULL, NULL, &err));
	cl_command_queue command_queue = OCL_ASSERT2(clCreateCommandQueue(context, device_id, 0, &err));

	const char *source_names[] = {
		"cl/common.h",
		"cl/sha1_16.cl",
		"cl/aes_tables.cl",
		"cl/aes_128.cl",
		"cl/dsi.cl",
		"cl/kernel_emmc_cid.cl" };
	cl_program program = ocl_build_from_sources(sizeof(source_names) / sizeof(char *),
		source_names, context, device_id, "-w -Werror");

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, "test_emmc_cid", &err));

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	// preparing args
	cl_uint aes_rk[RK_LEN];
	u8 aes_key[16];
	dsi_make_key(aes_key, u64be(console_id));
	aes_gen_tables();
	aes_set_key_enc_128(aes_rk, aes_key);
	cl_ulong u_offset = u16be(offset);
	cl_ulong xor[2];
	dsi_make_xor((u8*)xor, src, ver);
	cl_ulong out = 0;
#ifdef DEBUG
	{
		printf("XOR     : %s\n", hexdump(xor, 16, 0));
		printf("AES KEY : %s\n", hexdump(aes_key, 16, 0));
		printf("AES RK  : %s\n", hexdump(aes_rk, 48, 0));
		u8 ctr[16];
		dsi_make_ctr(ctr, emmc_cid, u_offset);
		printf("CTR     : %s\n", hexdump(ctr, 16, 0));
		aes_encrypt_128(aes_rk, ctr, (u8*)xor);
		printf("XOR TRY : %s\n", hexdump(xor, 16, 0));
		// exit(1);
	}
#endif

	cl_mem mem_rk = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(aes_rk), NULL, &err));
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_rk, CL_TRUE, 0, sizeof(aes_rk), aes_rk, 0, NULL, NULL));
	// there's no option to create it zero initialized
	cl_mem mem_out = OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &err));
	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));

	unsigned brute_bits = 32;
	unsigned group_bits = 28;
	unsigned loop_bits = brute_bits - group_bits;
	unsigned loops = 1ull << loop_bits;
	size_t num_items = 1ull << group_bits;
	// it needs to be aligned, a little overhead hurts nobody
	if (num_items % local) {
		num_items = (num_items / local + 1) * local;
	}
	get_hp_time(&t0);
	for (unsigned i = 0; i < loops; ++i) {
		*(u32*)(emmc_cid + 1) = i << group_bits;
		puts(hexdump(emmc_cid, 16, 0));
		OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_rk));
		OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_ulong), emmc_cid));
		OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_ulong), emmc_cid + 8));
		OCL_ASSERT(clSetKernelArg(kernel, 3, sizeof(cl_ulong), &u_offset));
		OCL_ASSERT(clSetKernelArg(kernel, 4, sizeof(cl_ulong), &xor[0]));
		OCL_ASSERT(clSetKernelArg(kernel, 5, sizeof(cl_ulong), &xor[1]));
		OCL_ASSERT(clSetKernelArg(kernel, 6, sizeof(cl_mem), &mem_out));

		OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
		clFinish(command_queue);

		OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));
		if (out) {
			get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
			*(u32*)(emmc_cid + 1) |= out;
			printf("got a hit: %s\n", hexdump(emmc_cid, 16, 0));
			// also write to a file
			dump_to_file(hexdump(console_id, 8, 0), emmc_cid, 16);
			break;
		}
	}

	u64 tested = 0;
	if (!out) {
		tested = 1ull << 32;
		get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
	} else {
		tested = *(u32*)(emmc_cid + 1);
	}
	printf("%.2f seconds, %.2f M/s\n", td / 1000000.0, tested * 1.0 / td);

	clReleaseKernel(kernel);
	return !out;
}
