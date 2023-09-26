#ifndef _WIN32
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <immintrin.h>
#include "utils.h"

#ifdef __GNUC__
#include <cpuid.h>
#elif _MSC_VER
#include <intrin.h>
#endif

int htoi(char a){
	if(a >= '0' && a <= '9'){
		return a - '0';
	}else if(a >= 'a' && a <= 'f'){
		return a - ('a' - 0xa);
	}else if(a >= 'A' && a <= 'F'){
		return a - ('A' - 0xa);
	}else{
		return -1;
	}
}

int hex2bytes(unsigned char *out, unsigned byte_len, const char *in, int critical){
	if (strlen(in) != byte_len << 1){
		fprintf(stderr, "%s: invalid input length, expecting %u, got %u.\n",
			__FUNCTION__, (unsigned)byte_len << 1, (unsigned)strlen(in));
		assert(!critical);
		return -1;
	}
	for(unsigned i = 0; i < byte_len; ++i){
		int h = htoi(*in++), l = htoi(*in++);
		if(h == -1 || l == -1){
			fprintf(stderr, "%s: invalid input \"%c%c\"\n",
				__FUNCTION__, *(in - 2), *(in - 1));
			assert(!critical);
			return -2;
		}
		*out++ = (h << 4) + l;
	}
	return 0;
}

#ifndef HEXDUMP_BUF_SIZE
#define HEXDUMP_BUF_SIZE 0x100
#endif

static char hexdump_buf[HEXDUMP_BUF_SIZE];
// CAUTION, this always assume the buffer is big enough
const char *hexdump(const void *b, unsigned l, int space){
	const unsigned char *p = (unsigned char*)b;
	char *out = hexdump_buf;
	for(unsigned i = 0; i < l; ++i){
		out += sprintf(out, "%02x", *p);
		++p;
		if(space && i < l - 1){
			*out++ = ' ';
		}
	}
	return hexdump_buf;
}

#ifdef _WIN32

long long hp_time_diff(LARGE_INTEGER *pt0, LARGE_INTEGER *pt1) {
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	long long diff = pt1->QuadPart - pt0->QuadPart;
	diff *= 1000000;
	diff /= freq.QuadPart;
	return diff;
}

#else

void get_hp_time(struct timespec *pt) {
	clock_gettime(CLOCK_MONOTONIC, pt);
}

long long hp_time_diff(struct timespec *pt0, struct timespec *pt1) {
	long long diff = pt1->tv_sec - pt0->tv_sec;
	diff *= 1000000;
	diff += (pt1->tv_nsec - pt0->tv_nsec) / 1000;
	return diff;
}

#endif

// CAUTION: caller is responsible to free the buf
char * read_file(const char *file_name, size_t *p_size) {
	FILE * f = fopen(file_name, "rb");
	if (f == NULL) {
		fprintf(stderr, "can't read file: %s", file_name);
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
		fprintf(stderr, "can't open file to write: %s\n", file_name);
		return;
	}
	fwrite(buf, len, 1, f);
	fclose(f);
}

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

// input must be multiple of uint64_t
int rdrand_fill(unsigned long long *p, size_t size) {
	unsigned long long *p_end = p + size;
	int success = 1;
	while (p < p_end) {
		success &= _rdrand64_step(p++);
	}
	return success;
}

inline int is_white_space(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// the crudest trim
// it works by:
//	setting the char next to the last non ws char to '\0'
//	return the pointer to the first non ws character
// so beware the input string got modified
// and the returned ptr becomes unavailable if the input is gone
// and if the input is malloced, you should save the ptr for free
char * trim(char *in) {
	char *first_non_ws = 0;
	char *last_non_ws = 0;
	while (*in) {
		if (!is_white_space(*in)) {
			if (!first_non_ws) {
				first_non_ws = in;
			}
			last_non_ws = in;
		}
		++in;
	}
	// cut the trailing
	++last_non_ws;
	if (last_non_ws < in) {
		*last_non_ws = '\0';
	}
	return first_non_ws;
}

