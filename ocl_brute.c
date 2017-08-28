
#include <stdio.h>
#include <stdint.h>
#include "utils.h"
#include "crypto.h"
#include "ocl.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// more info: https://github.com/Jimmy-Z/TWLbf/blob/master/dsi.c

const u64 DSi_KEY_Y[2] =
	{0xbd4dc4d30ab9dc76ull, 0xe1a00005202ddd1dull};

const u64 DSi_KEY_MAGIC[2] =
	{0x2a680f5f1a4f3e79ull, 0xfffefb4e29590258ull};

static inline u64 u64be(const u8 *in){
	u64 out;
	u8 *out8 = (u8*)&out;
	out8[0] = in[7];
	out8[1] = in[6];
	out8[2] = in[5];
	out8[3] = in[4];
	out8[4] = in[3];
	out8[5] = in[2];
	out8[6] = in[1];
	out8[7] = in[0];
	return out;
}

static inline u32 u32be(const u8 *in){
	u32 out;
	u8 *out8 = (u8*)&out;
	out8[0] = in[3];
	out8[1] = in[2];
	out8[2] = in[1];
	out8[3] = in[0];
	return out;
}

static inline u16 u16be(const u8 *in){
	u16 out;
	u8 *out8 = (u8*)&out;
	out8[0] = in[1];
	out8[1] = in[0];
	return out;
}

// CAUTION this one doesn't work in-place
static inline void byte_reverse_16(u8 *out, const u8 *in){
	out[0] = in[15];
	out[1] = in[14];
	out[2] = in[13];
	out[3] = in[12];
	out[4] = in[11];
	out[5] = in[10];
	out[6] = in[9];
	out[7] = in[8];
	out[8] = in[7];
	out[9] = in[6];
	out[10] = in[5];
	out[11] = in[4];
	out[12] = in[3];
	out[13] = in[2];
	out[14] = in[1];
	out[15] = in[0];
}

static inline void xor_128(u64 *x, const u64 *a, const u64 *b){
	x[0] = a[0] ^ b[0];
	x[1] = a[1] ^ b[1];
}

static inline void add_128(u64 *a, const u64 *b){
	a[0] += b[0];
	if(a[0] < b[0]){
		a[1] += b[1] + 1;
	}else{
		a[1] += b[1];
	}
}

static inline void add_128_64(u64 *a, u64 b){
	a[0] += b;
	if(a[0] < b){
		a[1] += 1;
	}
}

// Answer to life, universe and everything.
static inline void rol42_128(u64 *a){
	u64 t = a[1];
	a[1] = (t << 42 ) | (a[0] >> 22);
	a[0] = (a[0] << 42 ) | (t >> 22);
}

// eMMC Encryption for MBR/Partitions (AES-CTR, with console-specific key)
static inline void dsi_make_key(u64 *key, u64 console_id){
	u32 h = console_id >> 32, l = (u32)console_id;
	u32 key_x[4] = {l, l ^ 0x24ee6906, h ^ 0xe65b601d, h};
	// Key = ((Key_X XOR Key_Y) + FFFEFB4E295902582A680F5F1A4F3E79h) ROL 42
	// equivalent to F_XY in twltool/f_xy.c
	xor_128(key, (u64*)key_x, DSi_KEY_Y);
	add_128(key, DSi_KEY_MAGIC);
	rol42_128(key);
}

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

void ocl_brute(const cl_uchar *console_id, const cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver, int bcd)
{
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
		source_names, context, device_id, bcd ? "-w -Werror -DBCD" : "-w -Werror");

	cl_kernel kernel = OCL_ASSERT2(clCreateKernel(program, "test_console_id", &err));

	size_t local;
	OCL_ASSERT(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL));
	printf("local work size: %u\n", (unsigned)local);

	// preparing test data
	cl_ulong xor_l, xor_h, console_id_template, out;
	cl_uint ctr[4];
	cl_int success = 0;
	{
		u8 target_xor[16];
		xor_128((u64*)target_xor, (u64*)src, (u64*)ver);
		xor_l = u64be(target_xor + 8);
		xor_h = u64be(target_xor);

		u8 emmc_cid_sha1_16[16];
		sha1_16(emmc_cid, emmc_cid_sha1_16);
		add_128_64((u64*)emmc_cid_sha1_16, u16be(offset));
		byte_reverse_16((u8*)ctr, emmc_cid_sha1_16);
		console_id_template = u64be(console_id);
	}
	/*
	printf("xor_l : %s\n", hexdump(&xor_l, 8, 0));
	printf("xor_h : %s\n", hexdump(&xor_h, 8, 0));
	printf("templ : %s\n", hexdump(&console_id_template, 8, 0));
	printf("ctr_l : %s\n", hexdump(ctr, 8, 0));
	printf("ctr_h : %s\n", hexdump(ctr + 2, 8, 0));
	*/

	cl_mem mem_success =
		OCL_ASSERT2(clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int), NULL, &err));
	cl_mem mem_out =
		OCL_ASSERT2(clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, &err));

	OCL_ASSERT(clEnqueueWriteBuffer(command_queue, mem_success, CL_TRUE, 0, sizeof(cl_int), &success, 0, NULL, NULL));

	unsigned template_bits;
	cl_ulong total;
	unsigned group_bits;
	size_t num_items;
	unsigned steps;
	if (bcd) {
		// 0x08a15????????1?? as example, 10 digit to brute, beware the known digit near the end
		// 5 BCD digits, used to calculate template mask
		template_bits = 20;
		// I wish we could use 1e10, counting 0 is not good to your eye
		total = from_bcd(1ull << 40);
		// work items variations on lower bits, 8 + 1 digits a group
		group_bits = 36;
		// work items per enqueue, don't count the known digit here
		num_items = from_bcd(1ull << (group_bits - 4));
		// between the template bits and group bits, it's the loop bits
		steps = from_bcd(1 << (64 - template_bits - group_bits));
	} else {
		template_bits = 32;
		total = 1ull << (64 - template_bits);
		group_bits = 28;
		num_items = 1ull << group_bits; // 268435456
		steps = 1 << (64 - template_bits - group_bits);
	}
	// beware in BCD mode this is not the same to num_items
	const cl_ulong template_mask = ((cl_ulong)-1ll) << (64 - template_bits);
	// printf("total: %I64u, 0x%I64x\n", total, total);
	// printf("num_items: %I64u, 0x%I64x\n", num_items, num_items);
	// printf("steps: %u, 0x%x\n", steps, steps);
	get_hp_time(&t0);
	for (unsigned i = 0; i < steps; ++i) {
		if (bcd) {
			console_id_template = (console_id_template & template_mask)
				| (to_bcd(i) << group_bits);
		} else {
			console_id_template = (console_id_template & template_mask)
				| (i << group_bits);
		}
		printf("%" LL "x\n", console_id_template);
		OCL_ASSERT(clSetKernelArg(kernel, 0, sizeof(cl_ulong), &xor_l));
		OCL_ASSERT(clSetKernelArg(kernel, 1, sizeof(cl_ulong), &xor_h));
		OCL_ASSERT(clSetKernelArg(kernel, 2, sizeof(cl_ulong), &console_id_template));
		OCL_ASSERT(clSetKernelArg(kernel, 3, sizeof(cl_uint), &ctr[0]));
		OCL_ASSERT(clSetKernelArg(kernel, 4, sizeof(cl_uint), &ctr[1]));
		OCL_ASSERT(clSetKernelArg(kernel, 5, sizeof(cl_uint), &ctr[2]));
		OCL_ASSERT(clSetKernelArg(kernel, 6, sizeof(cl_uint), &ctr[3]));
		OCL_ASSERT(clSetKernelArg(kernel, 7, sizeof(cl_mem), &mem_success));
		OCL_ASSERT(clSetKernelArg(kernel, 8, sizeof(cl_mem), &mem_out));

		if (num_items % local) {
			num_items = (num_items / local + 1) * local;
		}
		OCL_ASSERT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &num_items, &local, 0, NULL, NULL));
		clFinish(command_queue);

		OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_success, CL_TRUE, 0, sizeof(cl_int), &success, 0, NULL, NULL));
		// printf("out : %s\n", hexdump(&out, 8, 0));
		if (success) {
			// TODO: if success, the (total / td) speed measurement is invalid
			OCL_ASSERT(clEnqueueReadBuffer(command_queue, mem_out, CL_TRUE, 0, sizeof(cl_ulong), &out, 0, NULL, NULL));
			get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
			printf("got a hit in %.2f seconds: %"LL"x\n", td / 1000000.0, out);
			// also write to a file
			dump_to_file(hexdump(emmc_cid, 16, 0), &out, 8);
			break;
		}
	}
	if (!success) {
		get_hp_time(&t1); td = hp_time_diff(&t0, &t1);
		printf("%.2f seconds, %.2f M/s\nno hit", td / 1000000.0, (total) * 1.0 / td);
	}

	clReleaseKernel(kernel);
}
