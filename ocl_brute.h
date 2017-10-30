#pragma once

typedef enum {
	NORMAL = 1,
	BCD = 2,
	CTR = 3
} ocl_brute_mode;

int ocl_brute_console_id(const cl_uchar *console_id, const cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver, ocl_brute_mode mode);

int ocl_brute_emmc_cid(const cl_uchar *console_id, cl_uchar *emmc_cid,
	const cl_uchar *offset, const cl_uchar *src, const cl_uchar *ver);
