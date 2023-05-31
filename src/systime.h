#pragma once

#include "stdint.h"
#include <zephyr/kernel.h>

//rt_tick_get

#define SYSTIME_SECOND(s) (s * 1000UL) 

// extern volatile uint32_t g_sysTime;

// static inline void SysTimeInc(uint32_t inc)
// {
//     g_sysTime += inc;
// }

static inline uint32_t SysTimeSpan(uint32_t base)
{
    uint32_t sysTime = k_uptime_get_32();

    if (sysTime >= base)
        return sysTime - base;
    else
        return UINT32_MAX - base + sysTime + 1;
}

static inline uint32_t GetSysTime(void)
{
    return k_uptime_get_32();
}

