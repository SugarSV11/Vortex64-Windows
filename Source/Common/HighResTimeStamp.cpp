#include <cstring>
#include "HighResTimeStamp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "Platform.h"


HighResTimeStamp::HighResTimeStamp()
{
    m_time = 0;
}

HighResTimeStamp & HighResTimeStamp::SetToNow(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    m_time = (uint64_t)(counter.QuadPart / freq.QuadPart) * 1000000ull + (uint64_t)((counter.QuadPart % freq.QuadPart) * 1000000ull / freq.QuadPart);
#else
    struct timespec now;
    memset(&now, 0, sizeof(now));
    clock_gettime(CLOCK_MONOTONIC, &now);
    m_time = ((uint64_t)now.tv_sec * (uint64_t)1000000l) + (now.tv_nsec / 1000);
#endif
    return *this;
}

uint64_t HighResTimeStamp::GetMicroSeconds(void)
{
    return m_time;
}

void HighResTimeStamp::SetMicroSeconds(uint64_t MicroSeconds)
{
    m_time = MicroSeconds;
}
