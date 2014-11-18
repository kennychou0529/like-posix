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
 * @addtogroup syscalls
 *
 * @file syscalls.h
 * @{
 */
#ifndef LIKE_POSIX_SYSCALLS_H_
#define LIKE_POSIX_SYSCALLS_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ff.h"
#include "likeposix_config.h"
#include "termios.h"

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef FILE_TABLE_OFFSET
#error FILE_TABLE_OFFSET must be defined - normally defined in likeposix_config.h
#endif
#ifndef FILE_TABLE_LENGTH
#error FILE_TABLE_LENGTH must be defined - normally defined in likeposix_config.h
#endif
#ifndef DEVICE_TABLE_LENGTH
#error DEVICE_TABLE_LENGTH must be defined - normally defined in likeposix_config.h
#endif
#ifndef DEVICE_INTERFACE_DIRECTORY
#error DEVICE_INTERFACE_DIRECTORY must be defined - normally defined in likeposix_config.h
#endif
#ifndef USE_HARDWARE_TIME_DRIVER
#error USE_HARDWARE_TIME_DRIVER must be defined - normally defined in likeposix_config.h
#endif


 typedef struct _dev_ioctl_t dev_ioctl_t;
 /**
  * function pointer to device io control functions
  */
 typedef int(*dev_ioctl_fn_t)(dev_ioctl_t*);

 /**
  *  definition of queue pair, use for device driver communication
  */
 typedef struct {
 	QueueHandle_t write;	///< queue that directs data written from application, to a physical device
 	QueueHandle_t read;		///< queue that directs data written from a physical device, to the application
 } queue_pair_t;

 /**
  * device interface definition, used for device driver interfacing.
  */
 struct _dev_ioctl_t{
 	dev_ioctl_fn_t read_enable;		///< pointer to enable device read function
 	dev_ioctl_fn_t write_enable;	///< pointer to enable device write function
    dev_ioctl_fn_t ioctl;           ///< pointer to device ioctl function
 	dev_ioctl_fn_t open;			///< pointer to open device function
 	dev_ioctl_fn_t close;			///< pointer to close device function
 	void* ctx;						///< a pointer to data that has meaning in the context of the device driver itself.
    struct termios* termios;        ///< a termios structure to define device settings via termios interface.
 	queue_pair_t pipe;
 };

FIL* get_file(int file);
dev_ioctl_t* get_dev_ioctl(int file);
dev_ioctl_t* install_device(char* name,
							void* dev_ctx,
							dev_ioctl_fn_t read_enable,
							dev_ioctl_fn_t write_enable,
                            dev_ioctl_fn_t open_dev,
							dev_ioctl_fn_t close_dev,
							dev_ioctl_fn_t ioctl);

#if USE_HARDWARE_TIME_DRIVER

/**
 * get_hw_time must be defined somewhere in the device drivers.
 */
extern void get_hw_time(unsigned long* secs, unsigned long* usecs);

#else
#define get_hw_time(sec, usec) (void)sec;(void)usec
#endif

#ifdef __cplusplus
 }
#endif

#endif /* LIKE_POSIX_SYSCALLS_H_ */

 /**
  * @}
  */
