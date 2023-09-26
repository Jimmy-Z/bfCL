
#pragma once

// a crude cross Windows/POSIX high precision timer
#ifdef _WIN32

#include <Windows.h>
typedef LARGE_INTEGER TimeHP;
#define get_hp_time QueryPerformanceCounter

#define LL "I64"

#else

#include <time.h>
typedef struct timespec TimeHP;
void get_hp_time(TimeHP *pt);

#define LL "ll"

#endif

long long hp_time_diff(TimeHP *pt0, TimeHP *pt1);

int hex2bytes(unsigned char *out, unsigned byte_len, const char *in, int critical);

const char * hexdump(const void *a, unsigned l, int space);

char * read_file(const char *file_name, size_t *p_size);

void read_files(unsigned num_files, const char *file_names[], char *ptrs[], size_t sizes[]);

void dump_to_file(const char *file_name, const void *buf, size_t len);

int cpu_has_rdrand();

int rdrand_fill(unsigned long long *p, size_t size);

char * trim(char *in);

