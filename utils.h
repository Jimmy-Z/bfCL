
#pragma once

// a crude cross Windows/POSIX high precision timer
#ifdef _WIN32
#include <Windows.h>
typedef LARGE_INTEGER HPTime;
#else
#include <sys/time.h>
typedef struct timeval HPTime;
#endif

#include "common.h"

#ifdef _WIN32
#define get_hp_time QueryPerformanceCounter
#else
void get_hp_time(HPTime *pt);
#endif

long long hp_time_diff(HPTime *pt0, HPTime *pt1);

int hex2bytes(u8 *out, unsigned byte_len, const char *in, int critical);

const char * hexdump(const void *a, unsigned l, int space);
