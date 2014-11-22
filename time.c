/*
 * Copyright (c) 2015 Michael Stuart.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the freertos-posix-wrapper project, <https://github.com/drmetal/freertos-posix-wrapper>
 *
 * Author: Michael Stuart <spaceorbot@gmail.com>
 *
 */

/**
 * @addtogroup syscalls System Calls
 *
 * relies upon:
 * - FreeRTOS (optional)
 *
 * The following functions are available:
 *
 * int gettimeofday(struct timeval *tp, struct timezone *tzp)
 * time_t time(time_t* time)
 * unsigned int sleep(unsigned int secs)
 * int usleep(useconds_t usecs)
 *
 *
 *
 * Note: get_hw_time must be defined somewhere in the device drivers.
 *
\code
  void get_hw_time(unsigned long* secs, unsigned long* usecs);
\endcode
 *
 * @file syscalls.c
 * @{
 */

#include <sys/time.h>
#include <unistd.h>
#include "system.h"

#ifndef USE_FREERTOS
#define USE_FREERTOS 0
#endif

#if USE_POSIX_STYLE_IO
#include "likeposix_config.h"
#endif

#if USE_FREERTOS
#pragma message("building with thread aware sleep")
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#else
#pragma message("building with polled sleep")
#endif

#if USE_DRIVER_SYSTEM_TIMER
#pragma message("building with hardware timer support")
extern void get_hw_time(unsigned long* secs, unsigned long* usecs);
#else
#pragma message("building without hardware timer support")
#define get_hw_time(sec, usec) (void)sec;(void)usec
#endif

#ifndef TIMEZONE_OFFSET
#pragma message("building timezone offset set to 0 - normally defined in likeposix_config.h")
#define TIMEZONE_OFFSET 0
#endif

unsigned int sleep(unsigned int secs)
{
#if USE_FREERTOS
    vTaskDelay((secs*1000)/portTICK_RATE_MS);
#else
    delay(secs*1000);
#endif
    return 0;
}

int usleep(useconds_t usecs)
{
#if USE_FREERTOS
    vTaskDelay((usecs/1000)/portTICK_RATE_MS);
#else
    delay(usecs/1000);
#endif
    return 0;
}

int _gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    (void)tzp;
    get_hw_time((unsigned long*)&tp->tv_sec, (unsigned long*)&tp->tv_usec);
    tp->tv_sec += TIMEZONE_OFFSET;
    return 0;
}

time_t _time(time_t* time)
{
    time_t usec;
    get_hw_time((unsigned long*)time, (unsigned long*)&usec);
    *time += TIMEZONE_OFFSET;
    return *time;
}

/**
 * @}
 */
