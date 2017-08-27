#pragma once

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

typedef struct{
	cl_device_id id;
	cl_char *name;
	cl_char *vendor;
	cl_char *version;
	cl_char *c_version;
	cl_uint max_compute_units;
	cl_uint max_work_group_size;
	cl_device_type type;
	cl_bool avail;
	cl_bool c_avail;
	cl_uint freq;
	cl_uint global_mem;
	cl_uint local_mem;
	cl_bool little;
} OCL_Device;

typedef struct{
	cl_platform_id id;
	cl_uint num_devices;
	OCL_Device *devices;
	cl_char *name;
	cl_char *vendor;
	cl_char *profile;
	cl_char *version;
} OCL_Platform;

// using ocl_ prefix to separate from OpenCL API names

#define OCL_ASSERT(r) \
	ocl_assert(r, #r, __FILE__, __FUNCTION__, __LINE__)

#define OCL_ASSERT2(c) \
	c; ocl_assert(err, #c, __FILE__, __FUNCTION__, __LINE__)

const char * ocl_err_msg(cl_int error_code);

void ocl_assert(cl_int ret, const char * code, const char * file,
	const char * function, unsigned line);

OCL_Platform *ocl_info(cl_uint *p_num_platforms, int verbose);

void ocl_get_device(cl_platform_id *p_platform_id, cl_device_id *p_device_id);
