#ifndef __TIME_UTILS_H__
#define __TIME_UTILS_H__

#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
    uint64_t gettickcount();  // ms
    int64_t gettickspan(uint64_t _old_tick);    // ms
    uint64_t timeMs();
    uint64_t clock_app_monotonic();  // ms
#ifdef __cplusplus
}
#endif


#endif
