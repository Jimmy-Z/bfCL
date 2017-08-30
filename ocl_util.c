
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <CL/cl_ext.h>
#include "ocl.h"
#include "utils.h"

#define STATIC_ASSERT(c) static_assert(c, #c)
STATIC_ASSERT(sizeof(char) == sizeof(cl_char));
STATIC_ASSERT(sizeof(int) == sizeof(cl_int));
STATIC_ASSERT(sizeof(long long) == sizeof(cl_long));
#undef STATIC_ASSERT

static char ocl_err_msg_buf[0x40];

const char * ocl_err_msg(cl_int error_code) {
	switch (error_code) {
	case CL_INVALID_PLATFORM:
		return "invalid platform";
	case CL_INVALID_DEVICE_TYPE:
		return "invalid device type";
	case CL_INVALID_VALUE:
		return "invalid value";
	case CL_DEVICE_NOT_FOUND:
		return "device not found";
	case CL_OUT_OF_RESOURCES:
		return "out of resources";
	case CL_OUT_OF_HOST_MEMORY:
		return "out of host memory";
	case CL_PLATFORM_NOT_FOUND_KHR:
		return "platform not found";
	case CL_INVALID_WORK_GROUP_SIZE:
		return "invalid work group size";
	default:
		sprintf(ocl_err_msg_buf, "code: %d", error_code);
		return ocl_err_msg_buf;
	}
}

void ocl_assert(cl_int ret, const char * code, const char * file,
	const char * function, unsigned line) {
	if (ret != CL_SUCCESS) {
		printf("%s: %s, function %s, line %u\n\t%s\nerror: %s\n",
			__FUNCTION__, file, function, line, code, ocl_err_msg(ret));
		exit(ret);
	}
}

// CAUTION: the caller is responsible to free them
cl_platform_id *ocl_get_platform_ids(cl_uint * p_num_platforms){
	OCL_ASSERT(clGetPlatformIDs(0, NULL, p_num_platforms));
	cl_platform_id *p_platform_ids = malloc(*p_num_platforms * sizeof(cl_platform_id));
	assert(p_platform_ids != NULL);
	OCL_ASSERT(clGetPlatformIDs(*p_num_platforms, p_platform_ids, NULL));
	return p_platform_ids;
}

void *ocl_get_platform_info(cl_platform_id platform_id, cl_platform_info param_name){
	size_t size;
	OCL_ASSERT(clGetPlatformInfo(platform_id, param_name, 0, NULL, &size));
	void * p_value = malloc(size);
	assert(p_value != NULL);
	OCL_ASSERT(clGetPlatformInfo(platform_id, param_name, size, p_value, NULL));
	return p_value;
}

cl_device_id *ocl_get_device_ids(cl_platform_id platform_id, cl_device_type device_type, cl_uint * p_num_devices){
	OCL_ASSERT(clGetDeviceIDs(platform_id, device_type, 0, NULL, p_num_devices));
	cl_device_id *p_device_ids = malloc(*p_num_devices * sizeof(cl_device_id));
	assert(p_device_ids != NULL);
	OCL_ASSERT(clGetDeviceIDs(platform_id, device_type, *p_num_devices, p_device_ids, NULL));
	return p_device_ids;
}

void *ocl_get_device_info(cl_device_id device_id, cl_device_info param_name) {
	size_t size;
	OCL_ASSERT(clGetDeviceInfo(device_id, param_name, 0, NULL, &size));
	void *p_value = malloc(size);
	assert(p_value != NULL);
	OCL_ASSERT(clGetDeviceInfo(device_id, param_name, size, p_value, NULL));
	return p_value;
}

// TODO: free the tree
OCL_Platform *ocl_info(cl_uint *p_num_platforms, int verbose){
	cl_platform_id *platform_ids = ocl_get_platform_ids(p_num_platforms);
	if(verbose){
		printf("%u platform(s) found:\n", *p_num_platforms);
	}
	OCL_Platform * platforms = malloc(sizeof(OCL_Platform) * (*p_num_platforms));

	for(unsigned i = 0; i < *p_num_platforms; ++i){
		cl_platform_id platform_id = platform_ids[i];
		OCL_Platform *platform = &platforms[i];
		platform->id = platform_id;
		platform->name    = ocl_get_platform_info(platform_id, CL_PLATFORM_NAME);
		platform->vendor  = ocl_get_platform_info(platform_id, CL_PLATFORM_VENDOR);
		platform->profile = ocl_get_platform_info(platform_id, CL_PLATFORM_PROFILE);
		platform->version = ocl_get_platform_info(platform_id, CL_PLATFORM_VERSION);
		if (verbose) {
			printf("=== 0x%08x ===\n", (unsigned)(unsigned long long)platform_id);
			printf("name    : %s\nvendor  : %s\nprofile : %s\nversion : %s\n",
				platform->name, platform->vendor, platform->profile, platform->version);
		}

		cl_uint num_devices;
		cl_device_id *device_ids = ocl_get_device_ids(platform_id, CL_DEVICE_TYPE_ALL, &num_devices);
		platform->num_devices = num_devices;
		if (num_devices == 0) {
			platform->devices = NULL;
			continue;
		} else {
			platform->devices = malloc(sizeof(OCL_Device) * num_devices);
		}
		if (verbose) {
			printf("\t%u device(s) found:\n", num_devices);
		}
		for (unsigned j = 0; j < num_devices; ++j) {
			cl_device_id device_id = device_ids[j];
			OCL_Device *device = &platform->devices[j];
			device->id = device_id;
			device->name = ocl_get_device_info(device_id, CL_DEVICE_NAME);
			device->vendor = ocl_get_device_info(device_id, CL_DEVICE_VENDOR);
			device->version = ocl_get_device_info(device_id, CL_DEVICE_VERSION);
			device->c_version = ocl_get_device_info(device_id, CL_DEVICE_OPENCL_C_VERSION);
#define _M_CLGDI(t, v, K) \
	OCL_ASSERT(clGetDeviceInfo(device_id, CL_DEVICE_ ## K, sizeof(t), &device->v, NULL))
			// if macro can do a case conversion here...
			_M_CLGDI(cl_uint, max_compute_units, MAX_COMPUTE_UNITS);
			_M_CLGDI(size_t, max_work_group_size, MAX_WORK_GROUP_SIZE);
			_M_CLGDI(cl_device_type, type, TYPE);
			_M_CLGDI(cl_bool, avail, AVAILABLE);
			_M_CLGDI(cl_bool, c_avail, COMPILER_AVAILABLE);
			_M_CLGDI(cl_uint, freq, MAX_CLOCK_FREQUENCY);
			_M_CLGDI(cl_ulong, global_mem, GLOBAL_MEM_SIZE);
			_M_CLGDI(cl_ulong, local_mem, LOCAL_MEM_SIZE);
			_M_CLGDI(cl_bool, little, ENDIAN_LITTLE);
#undef _M_CLGDI
			if(verbose){
				printf("\t=== 0x%08x ===\n", (unsigned)(unsigned long long)device_id);
				printf("\tname : %s\n", device->name);
				printf("\tvendor : %s\n", device->vendor);
				printf("\tversion : %s\n", device->version);
				printf("\tC version : %s\n", device->c_version);
				printf("\tmax compute units : %u\n", device->max_compute_units);
				printf("\tmax work group size : %u\n", device->max_work_group_size);
				switch (device->type) {
					case CL_DEVICE_TYPE_CPU: printf("\ttype : CPU\n"); break;
					case CL_DEVICE_TYPE_GPU: printf("\ttype : GPU\n"); break;
					default: printf("\ttype : other\n"); break;
				}
				printf("\tavailable : %s\n", device->avail ? "yes" : "no");
				printf("\tcompiler available : %s\n", device->c_avail ? "yes" : "no");
				printf("\tendian : %s\n", device->little ? "little" : "big");
				printf("\tfrequency : %u\n", device->freq);
				printf("\tglobal memory : %u\n", device->global_mem);
				printf("\tlocal memory : %u\n", device->local_mem);
			}
		}
		free(device_ids);
	}
	free(platform_ids);
	return platforms;
}

void ocl_get_device(cl_platform_id *p_platform_id, cl_device_id *p_device_id) {
	cl_uint num_platforms;
	OCL_Platform * platforms = ocl_info(&num_platforms, 0);
	cl_uint pl_idx = 0;
	cl_uint dev_idx = 0;
	cl_ulong maximum = 0;
	// for simplicity, we choose only one device
	for (cl_uint i = 0; i < num_platforms; ++i) {
		OCL_Device *devices = platforms[i].devices;
		for (cl_uint j = 0; j < platforms[i].num_devices; ++j) {
#ifdef USE_CPU
			if (devices[j].type == CL_DEVICE_TYPE_CPU
#else
			if (devices[j].type == CL_DEVICE_TYPE_GPU
#endif
				&& devices[j].c_avail == CL_TRUE
				&& (cl_ulong)devices[j].max_compute_units * devices[j].max_work_group_size > maximum) {
				maximum = devices[j].max_compute_units;
				pl_idx = i;
				dev_idx = j;
			}
		}
	}
	if (maximum > 0) {
		printf("selected device %s on platform %s\n",
			trim((char*)platforms[pl_idx].devices[dev_idx].name), trim((char*)platforms[pl_idx].name));
		*p_platform_id = platforms[pl_idx].id;
		*p_device_id = platforms[pl_idx].devices[dev_idx].id;
	} else {
		fprintf(stderr, "no openCL capable GPU found\n");
		*p_platform_id = NULL;
		*p_device_id = NULL;
	}
}

cl_program ocl_build_from_sources(
	unsigned num_sources, const char *source_names[],
	cl_context context, cl_device_id device_id, const char * options)
{
	TimeHP t0, t1;
	cl_int err;
	// read sources
	char **sources = malloc(sizeof(char*) * num_sources);
	size_t *source_sizes = malloc(sizeof(size_t) * num_sources);
	read_files(num_sources, source_names, sources, source_sizes);

	// compile
	get_hp_time(&t0);
	// WTF? GCC complains if I pass char ** to a function expecting const char **?
	cl_program program = OCL_ASSERT2(clCreateProgramWithSource(context, num_sources,
		(const char **)sources, source_sizes, &err));
	// printf("compiler options: %s\n", options);
	err = clBuildProgram(program, 0, NULL, options, NULL, NULL);
	get_hp_time(&t1);
	printf("%.3f seconds for OpenCL compiling\n", hp_time_diff(&t0, &t1) / 1000000.0);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "failed to compile program, error: %s, build log:\n", ocl_err_msg(err));
		size_t len;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
		char *buf_log = malloc(len + 1);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buf_log, NULL);
		buf_log[len] = '\0';
		fprintf(stderr, "%s\n", buf_log);
		free(buf_log);
		exit(err);
	}
	for (unsigned i = 0; i < num_sources; ++i) {
		free(sources[i]);
	}
	free(sources);
	free(source_sizes);
	return program;
}

