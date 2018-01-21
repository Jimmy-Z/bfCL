#pragma once

typedef enum {
	NORMAL = 1,
	BCD = 2,
	CTR = 3
} ocl_brute_mode;

int ocl_brute_console_id(const cl_uchar *console_id, const cl_uchar *emmc_cid,
	cl_uint offset0, const cl_uchar *src0, const cl_uchar *ver0,
	cl_uint offset1, const cl_uchar *src1, const cl_uchar *ver1,
	ocl_brute_mode mode);

int ocl_brute_emmc_cid(const cl_uchar *console_id, cl_uchar *emmc_cid,
	cl_uint offset, const cl_uchar *src, const cl_uchar *ver);

int ocl_brute_msky(const cl_uint *msky, const cl_uint *ver);
