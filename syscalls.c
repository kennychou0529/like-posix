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
 * This file is part of the like-posix project, <https://github.com/drmetal/like-posix>
 *
 * Author: Michael Stuart <spaceorbot@gmail.com>
 *
 */

/**
 * @defgroup syscalls System Calls
 *
 * A set of system calls and support functions, aimed to link up:
 *
 * 	- FreeRTOS
 * 	- ChaNs FatFs
 * 	- LwIP
 *
 * The API given on the newlib site was used as a basis: https://sourceware.org/newlib/
 *
 * The implementation of _realloc_r was taken from Stefano Oliveri's syscalls_minimal.c
 *
 * The result is Device, File and Socket IO, all avaliable under an almost standard C
 * API, including open, close, read, write, fsync, flseek, etc.
 *
 * relies upon:
 * - FreeRTOS
 * - minstlibs
 * - FatFs
 * - cutensils
 *
 * The following system calls are available:
 *
 * int open(const char *name, int flags, int mode)
 * int close(int file)
 * int write(int file, char *buffer, unsigned int count)
 * int read(int file, char *buffer, int count)
 * int fsync(int file)
 * int fstat(int file, struct stat *st)
 * int stat(char *file, struct stat *st)
 * int isatty(int file)
 * int lseek(int file, int offset, int whence)
 * int unlink(char *name)
 * int rename(const char *oldname, const char *newname)
 * void exit(int i)
 * char* getcwd(char* buffer, size_t size)
 * DIR* opendir(const char *name)
 * int closedir(DIR *dirp)
 * struct dirent* readdir(DIR *dirp)
 * int chdir(const char *path)
 * int mkdir(const char *pathname, mode_t mode)
 * int gettimeofday(struct timeval *tp, struct timezone *tzp)
 * time_t time(time_t* time)
 * unsigned int sleep(unsigned int secs)
 * int usleep(useconds_t usecs)
 *
 * termios functions supported:
 *
 * int tcgetattr(int fildes, struct termios *termios_p)
 * int tcsetattr(int fildes, int when, struct termios *termios_p)
 * speed_t cfgetispeed(const struct termios* termios)
 * speed_t cfgetospeed(const struct termios* termios)
 * int cfsetispeed(struct termios* termios, speed_t ispeed)
 * int cfsetospeed(struct termios* termios, speed_t ospeed)
 * int tcdrain(int file)
 * int tcflow(int file, int flags)
 * int tcflush(int file, int flags)
 *
 * socket funcions supported:
 *
 * int socket(int namespace, int style, int protocol);
 * int closesocket(int socket);
 * int accept(int socket, struct sockaddr *addr, socklen_t *length_ptr);
 * int connect(int socket, struct sockaddr *addr, socklen_t length);
 * int bind(int socket, struct sockaddr *addr, socklen_t length);
 * int shutdown(int socket, int how);
 * int getsockname(int socket, struct sockaddr *addr, socklen_t *length);
 * int getpeername(int socket, struct sockaddr *addr, socklen_t *length);
 * int setsockopt(int socket, int level, int optname, void *optval, socklen_t optlen);
 * int getsockopt(int socket, int level, int optname, void *optval, socklen_t *optlen);
 * int listen(int socket, int n);
 * int recv(int socket, void *buffer, size_t size, int flags);
 * int recvfrom(int socket, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *length);
 * int send(int socket, const void *buffer, size_t size, int flags);
 * int sendto(int socket, const void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t length);
 * int ioctlsocket(int socket, int cmd, void* argp);
 *
 * @file syscalls.c
 * @{
 */

#include <sys/socket.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <errno.h>
#include <time.h>
#include <string.h>
#include "syscalls.h"
#include "cutensils.h"
#include "strutils.h"
#include "systime.h"

/**
 *  definition of block structure, copied from heap2 allocator
 */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	///< The next free block in the list
	size_t xBlockSize;						///< The size of the free block
} xBlockLink;

/**
 * filetable entry definition
 */
typedef struct {
	dev_ioctl_t* device;	///< pointer to the device interface
    int mode;               ///< the the mode under which the device was opened
	int flags;				///< the the mode under which the device was opened
	FIL file;				///< regular file, or device interface file
	unsigned int size;		///< size, used only for queues
}filtab_entry_t;

/**
 * file table definition.
 */
typedef struct {
	int count;									///< the number of open files, 0 means nothing open yet
	filtab_entry_t* tab[FILE_TABLE_LENGTH];		///< the file table
	dev_ioctl_t* devtab[DEVICE_TABLE_LENGTH];	///< the device table
	SemaphoreHandle_t lock;                     ///< file table lock.
}_filtab_t;

/**
 * the max number of bytes to read from a device interface file
 */
#define DEVICED_INTERFACE_FILE_SIZE		32

#define DEFAULT_DEVICE_TIMEOUT          1000

#define lock_filtab()                   (xSemaphoreTake(filtab.lock, 2000/portTICK_RATE_MS) == pdTRUE)
#define unlock_filtab()                 xSemaphoreGive(filtab.lock)

#undef errno
extern int errno;
char *__env[1] = {0};
char **__environ = __env;
extern unsigned int _heap;
extern unsigned int _eheap;
caddr_t heap = NULL;
static const unsigned short heapSTRUCT_SIZE	=
 		( sizeof( xBlockLink ) + portBYTE_ALIGNMENT -
 		( sizeof( xBlockLink ) % portBYTE_ALIGNMENT ) );
static _filtab_t filtab;
struct dirent _dirent;

/**
 * to make STDIO work with serial IO,
 * please define "void phy_putc(char c)" somewhere
 */
extern void phy_putc(char c) __attribute__((weak));
extern char phy_getc() __attribute__((weak));

/**
 * initialses likeposix state.
 */
void init_likeposix()
{
    if(filtab.lock == NULL)
    {
        filtab.lock = xSemaphoreCreateMutex();
        assert_true(filtab.lock);
    }
}

/**
 * @param	file is a file pointer to an open device file.
 * @retval 	the file table entry for a given file descriptor,
 * 			or NULL if the file descriptor was not valid, or the file was not open.
 */
inline filtab_entry_t* __get_entry(int file)
{
	if(!filtab.count || (filtab.count > FILE_TABLE_LENGTH))
		return NULL;

	file -= FILE_TABLE_OFFSET;
	if(file >= 0)
	{
		return  filtab.tab[file];
	}
	return NULL;
}

/**
 * deletes the structures of a file table entry...
 */
inline void __delete_filtab_item(filtab_entry_t* fte)
{
    if((fte->mode == S_IFREG) || (fte->mode == S_IFIFO))
    {
        // #1 close the file
        f_close(&fte->file);
        // # 2 remove pipe
        if(fte->device)
        {
            // remove read & write queues
            if(fte->device->pipe.read)
                vQueueDelete(fte->device->pipe.read);
            if(fte->device->pipe.write)
                vQueueDelete(fte->device->pipe.write);
        }
    }
#if ENABLE_LIKEPOSIX_SOCKETS
    else if(fte->mode == S_IFSOCK)
    {
        int fd = (int)fte->device;
        if(fd != -1)
            lwip_close(fd);
    }
#endif
	// #3 delete file table node
	vPortFree(fte);
}

/**
 * create a new file stat structure.
 *
 * does not enter the structure in to the file table...
 *
 * if mode contains S_IFREG, the file number returned operates on a regular file, as per the conditions
 * given in flags.
 *
 * if mode contains S_IFIFO, the file number returned operates on a pair of queues, rather than a file.
 *  - if flags contains FREAD, then a read queue of length bytes becomes available to the read() function.
 *  - if flags contains FWRITE, then a write queue of length bytes becomes available to the write() function.
 *  - the opposing ends of the queues may be interfaced to a device in a device driver module...
 *
 * @param 	fdes is a pointer to a raw file table entry, which doesnt have to be pre initialized.
 * @param	name is the name of the file, or device file to open.
 * @param	flags may be a combination of one of O_RDONLY, O_WRONLY, or O_RDWR,
 * 			and any of O_APPEND | O_CREAT | O_TRUNC | O_NONBLOCK
 * @param 	mode is one of S_IFDIR | S_IFCHR | S_IFBLK | S_IFREG | S_IFLNK | S_IFSOCK | S_IFIFO.
 * 			only S_IFREG and S_IFIFO are supported.
 *
 * @retval 0 on success, -1 on failure.
 */
inline int __create_filtab_item(filtab_entry_t** fdes, const char* name, int flags, int mode, int length)
{
	if(!name)
		return EOF;

	int file = EOF;
	BYTE ff_flags = 0;

	// create new file table node
	filtab_entry_t* fte = (filtab_entry_t*)pvPortMalloc(sizeof(filtab_entry_t));

	if(fte)
	{
		fte->mode = mode;
		fte->device = NULL;
		fte->flags = flags+1;
		fte->size = length;

		/**********************************
		 * create file
		 **********************************/

		if(fte->mode == S_IFREG)
		{
			if(fte->flags&FREAD)
				ff_flags |= FA_READ;
			if(fte->flags&FWRITE)
				ff_flags |= FA_WRITE;

			if(fte->flags&O_CREAT)
			{
				if(fte->flags&O_TRUNC)
					ff_flags |= FA_CREATE_ALWAYS;
				else
					ff_flags |= FA_OPEN_ALWAYS;
			}
			else
				ff_flags |= FA_OPEN_EXISTING;

			// TODO can we used this flag? FA_CREATE_NEW
		}

		if(f_open(&fte->file, (const TCHAR*)name, (BYTE)ff_flags) == FR_OK)
		{
			if(fte->mode == S_IFREG)
			{
				file = 0;
				if(fte->flags&O_APPEND)
					f_lseek(&fte->file, f_size(&fte->file));
			}
			else if(fte->mode == S_IFIFO)
			{
				/**********************************
				 * create data pipe
				 **********************************/

				// fetch device interface index
				unsigned char buf[DEVICED_INTERFACE_FILE_SIZE];
				int devindex;
				unsigned int n = 0;

				// read device index (buf[0])
				f_read(&fte->file, buf, (UINT)DEVICED_INTERFACE_FILE_SIZE, (UINT*)&n);
				devindex = (int)buf[0];

				if(n > 0 && devindex < DEVICE_TABLE_LENGTH)
					fte->device = filtab.devtab[devindex];

				// populate "pipe", timeout values
				if(fte->device)
				{
				    if(fte->flags & O_NONBLOCK)
				        fte->device->timeout = 0;
				    else
				        fte->device->timeout = DEFAULT_DEVICE_TIMEOUT/portTICK_RATE_MS;

					// create write device queue
					char write_q = 1;
					fte->device->pipe.write = NULL;
					if(fte->flags&FWRITE)
					{
						fte->device->pipe.write = xQueueCreate(fte->size, 1);
						write_q = fte->device->pipe.write ? 1 : 0;
					}

					// create read device queue
					char read_q = 1;
					fte->device->pipe.read = NULL;
					if(fte->flags&FREAD)
					{
						fte->device->pipe.read = xQueueCreate(fte->size, 1);
						read_q = fte->device->pipe.read ? 1 : 0;
					}

					if(read_q && write_q)
						file = 0;
				}
			}
		}

		if(file == 0)
		{
			// on success ...
			*fdes = fte;
		}
		else
		{
			// on error...
			__delete_filtab_item(fte);
		}
	}

	return file;
}

/**
 * put a file table entry into file table.
 *
 * @param 	fte is a pointer to a file table entry, which NEEDS to have been pre initialized.
 * @retval 	the file number if successful, or -1 on error.
 */
inline int __insert_entry(filtab_entry_t* fte)
{
	int file;
	for(file = 0; file < FILE_TABLE_LENGTH; file++)
	{
		if(filtab.tab[file] == NULL)
		{
			filtab.tab[file] = fte;
			filtab.count++;
			return file + FILE_TABLE_OFFSET;
		}
	}

	return EOF;
}

/**
 * remove a file table entry from the file table.
 *
 * @param	file is a file pointer to a device file.
 * @retval  0 if successful, or -1 on error.
 */
inline int __remove_entry(int file)
{
	if(!filtab.count || (filtab.count > FILE_TABLE_LENGTH))
		return EOF;

	filtab.tab[file-FILE_TABLE_OFFSET] = NULL;
	filtab.count--;
	return 0;
}

/**
 * determine the mode to open the file with - this is a customization of the mode passed into _open()...
 *
 * returns a combination of the following...
 *  @arg     S_IFDIR - not implemented
 *  @arg     S_IFCHR - not implemented
 *  @arg     S_IFBLK - not implemented
 *  @arg     S_IFREG - signifies regular a file
 *  @arg     S_IFLNK - not implemented
 *  @arg     S_IFSOCK - not implemented
 *  @arg     S_IFIFO - signifies open a FIFO (interfacing a device)
 */
inline int __determine_mode(const char *name)
{
	int x = startswith(name, DEVICE_INTERFACE_DIRECTORY);
	return x == 0 ? S_IFIFO : S_IFREG;
}

/**
 * installs a device for use by the application.
 *
 * an index into filtab.devtab is written into the device file,
 * this index is used by open() to interface a one of filtab.tab to one of filtab.devtab.
 *
 * @param	name is the full path to the file to associate with the device.
 * @param	dev_ctx is a pointer to some data that will be passed to the device driver
 *			driver ioctl functions.
 * @param	read_enable is an ioctl function that can enable a device to read data.
 * @param	write_enable is an ioctl function that can enable a device to write data.
 * @param   open is an ioctl function that can enable a device.
 * @param	close is an ioctl function that can disable a device.
 * @param	ioctl is an ioctl function that can set the hardware settings of a device.
 * @retval	returns a pointer to the created dev_ioctl_t structure, or NULL on error.
 */
dev_ioctl_t* install_device(char* name,
					void* dev_ctx,
					dev_ioctl_fn_t read_enable,
					dev_ioctl_fn_t write_enable,
                    dev_ioctl_fn_t open_dev,
					dev_ioctl_fn_t close_dev,
					dev_ioctl_fn_t ioctl)
{
	FIL f;
	unsigned char buf[DEVICED_INTERFACE_FILE_SIZE];
	int device;
	dev_ioctl_t* ret = NULL;
	unsigned int n = 0;

	log_syslog(NULL, "installing %s...", name);

	// try to create the directory first, if it fails it is already there
	// or, the disk isnt in. the next step will also fail in that case.
	f_mkdir(DEVICE_INTERFACE_DIRECTORY);

    // is it possible to open the device file?
    if(f_open(&f, (const TCHAR*)name, FA_WRITE|FA_OPEN_ALWAYS) == FR_OK)
    {
        for(device = 0; device < DEVICE_TABLE_LENGTH; device++)
        {
            // found an empty slot
            if(filtab.devtab[device] == NULL)
            {
                // todo, add better logic to write the file only if it is out of date/not exists
                // install devno in file
                buf[0] = (unsigned char)device;
                if((f_write(&f, buf, (UINT)1, (UINT*)&n) == FR_OK) && (n == 1))
                {
                    // create device io structure and populate api
                    filtab.devtab[device] = pvPortMalloc(sizeof(dev_ioctl_t));
                    if(filtab.devtab[device])
                    {
                        // note that filtab.devtab[device]->pipe is populated by _open()
                        filtab.devtab[device]->timeout = 0;
                        filtab.devtab[device]->read_enable = read_enable;
                        filtab.devtab[device]->write_enable = write_enable;
                        filtab.devtab[device]->ioctl = ioctl;
                        filtab.devtab[device]->open = open_dev;
                        filtab.devtab[device]->close = close_dev;
                        filtab.devtab[device]->ctx = dev_ctx;
                        filtab.devtab[device]->termios = NULL;
                    }
                    ret = filtab.devtab[device];
                    log_syslog(NULL, "%s OK", name);
                }
                else
                    log_error(NULL, "failed to write device %s", name);

                break;
            }
        }
        f_close(&f);
    }
    else
        log_error(NULL, "failed to open device %s", name);

	return ret;
}

/**
 * system call, 'open'
 *
 * opens a file for disk IO, or a FreeRTOS Queue, for
 * device IO or data transfer/sharing.
 *
 * **this is a non standard implementation**
 *
 * @param	the name of the file to open
 * @param	flags -
 * 			if mode is S_IFREG, may be a combination of:
 *  		one of O_RDONLY, O_WRONLY, or O_RDWR,
 * 			and any of O_APPEND | O_CREAT | O_TRUNC
 * @param	mode - repurposed - in the case of device files, specified the queue length.
 * 			otherwise ignored.
 * @retval	returns a file descriptor, that may be used with
 * 			read(), write(), close(), or -1 if there was an error.
 */
int _open(const char *name, int flags, int mode)
{
    int file = EOF;

	if(filtab.count > FILE_TABLE_LENGTH)
		return EOF;

	filtab_entry_t* fte = NULL;
	int length = mode;

	if(lock_filtab())
	{
        file = __create_filtab_item(&fte, name, flags, __determine_mode(name), length);

        // if we got 0 here it means a file or a queue was made successfully
        // now need to add the file stat struct to the descriptor table
        if(file == 0)
        {
            // add file to table
            file = __insert_entry(fte);
            // add failed, delete
            if(file == EOF)
                __delete_filtab_item(fte);
            else
            {
                // actions to do if everything went right...

                if((fte->mode == S_IFIFO) && fte->device)
                {
                    // call device open
                    if(fte->device->open)
                        fte->device->open(fte->device);
                    // enable reading
                    if((fte->flags & FREAD) && fte->device->read_enable)
                        fte->device->read_enable(fte->device);
                    // writing is enabled in _write()...
                }
            }
        }
        unlock_filtab();
	}

	return file;
}

/**
 * close the specified file descriptor.
 *
 * @param	file is the file descriptor to close.
 * @retval 	0 on success, -1 on error.
 */
int _close(int file)
{
	int res = EOF;
    if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(file);
        if(fte)
        {
            // disable device IO first
            if((fte->mode == S_IFIFO) && fte->device && (fte->device->close))
            {
                // call device close
                fte->device->close(fte->device);
            }
            // then remove the file table entry
            res = __remove_entry(file);
            // then delete all the file structures
            __delete_filtab_item(fte);
        }
        unlock_filtab();
    }
	return res;
}

/**
 * writes a buffer to the file specified.
 *
 * @param	file is a file descriptor, may be the value returned by
 * 			a call to the open() syscall, or STDOUT_FILENO or STDERR_FILENO.
 * @param	a buffer of characters to write.
 * @param	count, the number of characters to write.file
 * @retval	the number of characters written or -1 on error.
 */
int _write(int file, char *buffer, unsigned int count)
{
    unsigned int timeout;
	int n = EOF;

	if(file == STDOUT_FILENO || file == STDERR_FILENO || file == (intptr_t)stdout || file == (intptr_t)stderr)
	{
		for(n = 0; n < (int)count; n++)
			phy_putc(*buffer++);
	}
	else if(lock_filtab())
	{
		filtab_entry_t* fte = __get_entry(file);

		if(fte && (fte->flags & FWRITE))
		{
			if(fte->mode == S_IFREG)
			{
				if(f_write(&fte->file, (void*)buffer, (UINT)count, (UINT*)&n) != FR_OK)
				    n = EOF;
			}
			else if((fte->mode == S_IFIFO) && fte->device)
			{
                timeout = fte->device->timeout;

				for(n = 0; n < (int)count; n++)
				{
					if(xQueueSend(fte->device->pipe.write, buffer++, timeout) != pdTRUE)
						break;
                    timeout = 0;
				}
				// enable the physical device to write
				if(fte->device->write_enable)
					fte->device->write_enable(fte->device);
			}
#if ENABLE_LIKEPOSIX_SOCKETS
            else if(fte->mode == S_IFSOCK)
            {
                n = lwip_write((int)fte->device, buffer, count);
            }
#endif
		}
		unlock_filtab();
	}

	return n;
}

/**
 * reads a number of characters into a buffer from the file specified.
 *
 * @param	file is a file descriptor, may be the value returned by
 * 			a call to the open() syscall, or STDIN_FILENO
 * @param	a buffer for the characters to read.
 * @param	count, the number of characters to read.
 * @retval	the number of characters read or -1 on error.
 */
int _read(int file, char *buffer, int count)
{
    unsigned int timeout;
	int n = EOF;

	if(file == STDIN_FILENO || file == (intptr_t)stdin)
	{
		for(n = 0; n < count; n++)
			*buffer++ = phy_getc();
	}
	else if(lock_filtab())
	{
		filtab_entry_t* fte = __get_entry(file);

		if(fte && (fte->flags & FREAD))
		{
			if(fte->mode == S_IFREG)
			{
				f_read(&fte->file, (void*)buffer, (UINT)count, (UINT*)&n);
			}
			else if((fte->mode == S_IFIFO) && fte->device)
			{
			    timeout = fte->device->timeout;

				for(n = 0; n < count; n++)
				{
					if(xQueueReceive(fte->device->pipe.read, buffer++, timeout) != pdTRUE)
						break;
					timeout = 0;
				}
			}
#if ENABLE_LIKEPOSIX_SOCKETS
            else if(fte->mode == S_IFSOCK)
            {
                n = lwip_read((int)fte->device, buffer, count);
            }
#endif
		}
        unlock_filtab();
	}

	return n;
}

int fsync(int file)
{
	int res = EOF;
	if(file == STDIN_FILENO ||
			file == STDOUT_FILENO ||
			file == STDERR_FILENO ||
			file == (intptr_t)stdout ||
			file == (intptr_t)stderr ||
			file == (intptr_t)stdin)
	{
		res = 0;
	}
	else if(lock_filtab())
	{
		filtab_entry_t* fte = __get_entry(file);

		if(fte)
		{
			if(fte->mode == S_IFREG)
			{
				f_sync(&fte->file);
				res = 0;
			}
		}
        unlock_filtab();
	}

	return res;
}

/**
 * gets the current working directory - follows the GNU version
 * in that id buffer is set to NULL, a buffer of size bytes is allocated
 * to hold the cwd string. it must be freed afterward by the user...
 */
char* getcwd(char* buffer, size_t size)
{
    if(buffer == NULL)
        buffer = malloc(size);

    if(buffer)
    {
        if(f_getcwd((TCHAR*)buffer, (UINT)size) != FR_OK)
        {
            free(buffer);
            buffer = NULL;
        }
    }

    return buffer;
}

/**
 * allocates and populates a DIR info struct.
 * returns NULL if there was no memory allocated or the directory specified didnt exist.
 * the directory must be closed with closedir() by the user.
 */
DIR* opendir(const char *name)
{
    DIR* dir = malloc(sizeof(DIR));

    if(dir)
    {
        if(f_opendir(dir, (const TCHAR*)name) != FR_OK)
        {
            free(dir);
            dir = NULL;
        }
    }

    return dir;
}
/**
 * closes a directory opened with opendir.
 * returns 0 on success, or -1 on error.
 */
int closedir(DIR *dir)
{
    if(dir)
    {
//        f_closedir(dir);
        free(dir);
    }

    return 0;
}

/**
 * reads directory info. returns a pointer to a struct dirent,
 * as long as there are entries in the directory.
 * returns NULL when there are no other entries in the directory.
 */
struct dirent* readdir(DIR *dirp)
{
    FILINFO info;

    info.lfname = _dirent.d_name;
    info.lfsize = sizeof(_dirent.d_name);

    _dirent.d_name[0] = '\0';
    _dirent.d_type = DT_REG;

    if(f_readdir(dirp, &info) != FR_OK || !info.fname[0])
        return NULL;

    if(_dirent.d_name[0] == '\0')
        strcpy(_dirent.d_name, info.fname);

    if(info.fattrib & AM_DIR)
        _dirent.d_type = DT_DIR;

    return &_dirent;
}

int chdir(const char *path)
{
    return f_chdir((TCHAR*)path) == FR_OK ? 0 : -1;
}

int mkdir(const char *pathname, mode_t mode)
{
    (void)mode;
    return f_mkdir(pathname) == FR_OK ? 0 : -1;
}

/**
 * populates a struct stat type with:
 *
 *  - st_size	- the size of the file
 *  - st_mode 	- the mode of the file (S_IFCHR, S_IFREG, S_IFIFO, etc)
 *
 *  ... from an open file.
 */
int _fstat(int file, struct stat *st)
{
	int res = EOF;
	if(file == STDIN_FILENO ||
			file == STDOUT_FILENO ||
			file == STDERR_FILENO ||
			file == (intptr_t)stdout ||
			file == (intptr_t)stderr ||
			file == (intptr_t)stdin)
	{
		if(st)
		{
			st->st_size = 1;
			st->st_mode = S_IFCHR;
		}
		res = 0;
	}
	else if(lock_filtab())
	{
		filtab_entry_t* fte = __get_entry(file);

		if(fte)
		{
			if(fte->mode == S_IFREG)
			{
				if(st)
					st->st_size = f_size(&fte->file);
			}
			if(fte->mode == S_IFIFO)
			{
				if(st)
					st->st_size = fte->size;
			}

			if(st)
				st->st_mode = fte->mode;

			res = 0;
		}
		unlock_filtab();
	}

	return res;
}

long int _ftell(int file)
{
	int res = EOF;

	if(lock_filtab())
	{
        filtab_entry_t* fte = __get_entry(file);

        if(fte)
        {
            if(fte->mode == S_IFREG)
                res = f_tell(&fte->file);
        }
        unlock_filtab();
	}

	return res;
}

/**
 * populates a struct stat type with:
 *
 *  - st_size	- the size of the file
 *  - st_mode 	- the mode of the file (S_IFCHR, S_IFREG, S_IFIFO, etc)
 *
 *  ... from a file that is not already open.
 */
int _stat(char *file, struct stat *st)
{
	int res = EOF;
	int fd = _open(file, O_RDONLY, 0);
	if(fd == EOF)
		return EOF;
	res = _fstat(fd, st);
	_close(fd);
	return res;
}

/**
 * returns 1 if the file is a device, or stdio endpoint, 0 otherwise.
 *
 */
int _isatty(int file)
{
	int res = 0;

	if(file == STDIN_FILENO ||
			file == STDOUT_FILENO ||
			file == STDERR_FILENO ||
			file == (intptr_t)stdout ||
			file == (intptr_t)stderr ||
			file == (intptr_t)stdin)
		res = 1;
	else if(lock_filtab())
	{
		filtab_entry_t* fte = __get_entry(file);
		if(fte && (fte->mode == S_IFIFO))
			res = 1;

		unlock_filtab();
	}
	return res;
}

/**
 * only works for files with mode = S_IFREG (not devices, or stdio's)
 *
 * SEEK_SET 	Offset is to be measured in absolute terms.
 * SEEK_CUR 	Offset is to be measured relative to the current location of the pointer.
 * SEEK_END 	Offset is to be measured back, relative to the end of the file.
 */
int _lseek(int file, int offset, int whence)
{
	int res = EOF;
    if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(file);

        if(fte)
        {
            if(fte->mode == S_IFREG)
            {
                if(whence == SEEK_CUR)
                    offset = f_tell(&fte->file) + offset;
                else if(whence == SEEK_END)
                    offset = f_size(&fte->file) - offset;

                if(f_lseek(&fte->file, offset) == FR_OK)
                    res = 0;
            }
        }
        unlock_filtab();
    }
	return res;
}

int _unlink(char *name)
{
	FRESULT res = f_unlink((const TCHAR*)name);
	return res == FR_OK ? 0 : EOF;
}

int rename(const char *oldname, const char *newname)
{
	FRESULT res = f_rename((const TCHAR*)oldname, (const TCHAR*)newname);
	return res == FR_OK ? 0 : EOF;
}

void _exit(int i)
{
	printf("Program exit with code %d", i);
	while (1);
}

caddr_t _sbrk(int incr)
{
	(void)incr;
    return NULL;
}

int _link(char *old, char *new)
{
	(void)old;
	(void)new;
	errno = EMLINK;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	(void)name;
	(void)argv;
	(void)env;
	errno = ENOMEM;
	return -1;
}

int _fork()
{
	errno = EAGAIN;
	return -1;
}

int _getpid()
{
	return 1;
}

int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	errno = EINVAL;
	return (-1);
}

int times(struct tm *buf)
{
	(void)buf;
	return -1;
}

int _wait(int *status)
{
	(void)status;
	errno = ECHILD;
	return -1;
}

_PTR _realloc_r(struct _reent *re, _PTR oldAddr, size_t newSize)
{
	(void)re;

	xBlockLink *block;
	size_t toCopy;
	void *newAddr;

	newAddr = pvPortMalloc(newSize);

	if (newAddr == NULL)
		return NULL;

	/* We need the block struct pointer to get the current size */
	block = oldAddr;
	block -= heapSTRUCT_SIZE;

	/* determine the size to be copied */
	toCopy = (newSize<block->xBlockSize)?(newSize):(block->xBlockSize);

	/* copy old block into new one */
	memcpy((void *)newAddr, (void *)oldAddr, (size_t)toCopy);

	vPortFree(oldAddr);

	return newAddr;
}

_PTR _calloc_r(struct _reent *re, size_t num, size_t size) {
	(void)re;
	return pvPortMalloc(num*size);
}

_PTR _malloc_r(struct _reent *re, size_t size) {
	(void)re;
	return pvPortMalloc(size);
}

_VOID _free_r(struct _reent *re, _PTR ptr) {
	(void)re;
	vPortFree(ptr);
}

int tcgetattr(int fildes, struct termios *termios_p)
{
    int ret = -1;
    if(termios_p == NULL || isatty(fildes) == 0)
        return ret;

    memset(termios_p, 0, sizeof(struct termios));

    if(fildes == STDOUT_FILENO || fildes == STDERR_FILENO || fildes == (intptr_t)stdout || fildes == (intptr_t)stderr)
    {
        termios_p->c_cflag = B115200|CS8;
        ret = 0;
    }
    else if(fildes == STDIN_FILENO || fildes == (intptr_t)stdin)
    {
        termios_p->c_cflag = B115200|CS8;
        ret = 0;
    }
    else if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(fildes);

        if(fte->device && fte->device->ioctl)
        {
            fte->device->termios = termios_p;
            ret = fte->device->ioctl(fte->device);
            fte->device->termios = NULL;
        }
        unlock_filtab();
    }

    return ret;
}

int tcsetattr(int fildes, int when, const struct termios *termios_p)
{
    (void)when;
    int ret = -1;
    if(termios_p == NULL || isatty(fildes) == 0)
        return ret;

    if(fildes == STDOUT_FILENO || fildes == STDERR_FILENO || fildes == (intptr_t)stdout || fildes == (intptr_t)stderr)
    {

    }
    else if(fildes == STDIN_FILENO || fildes == (intptr_t)stdin)
    {

    }
    else if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(fildes);

        if(fte->device && fte->device->ioctl)
        {
            fte->device->termios = (struct termios *)termios_p;
            ret = fte->device->ioctl(fte->device);
            fte->device->termios = NULL;
        }
        unlock_filtab();
    }

    return ret;
}

speed_t cfgetispeed(const struct termios* termios)
{
    return termios->c_ispeed;
}
speed_t cfgetospeed(const struct termios* termios)
{
    return termios->c_ospeed;
}

int cfsetispeed(struct termios* termios, speed_t ispeed)
{
    termios->c_ispeed = ispeed;
    return 0;
}
int cfsetospeed(struct termios* termios, speed_t ospeed)
{
    termios->c_ospeed = ospeed;
    return 0;
}

int tcdrain(int file)
{
    unsigned long timeout;
    int res = EOF;

    if(file == STDOUT_FILENO || file == STDERR_FILENO || file == (intptr_t)stdout || file == (intptr_t)stderr)
    {
        res = 0;
    }
    else if(file == STDIN_FILENO || file == (intptr_t)stdin)
    {
        res = 0;
    }
    else if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(file);

        if(fte)
        {
            if(fte->mode == S_IFIFO)
            {
                timeout = get_hw_time_ms() + fte->device->timeout;
                while(uxQueueMessagesWaiting(fte->device->pipe.write) > 0 && get_hw_time_ms() < timeout)
                    portYIELD();

                if(get_hw_time_ms() < timeout)
                    res = 0;
            }
        }
        unlock_filtab();
    }

    return res;
}

/**
 * not implemented....
 */
int tcflow(int file, int flags)
{
    int res = EOF;

    if(file == STDOUT_FILENO || file == STDERR_FILENO || file == (intptr_t)stdout || file == (intptr_t)stderr)
    {
        res = 0;
    }
    else if(file == STDIN_FILENO || file == (intptr_t)stdin)
    {
        res = 0;
    }
    else if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(file);

        if(fte)
        {
            if(fte->mode == S_IFIFO)
            {
                (void)flags;
                res = EOF;
            }
        }
        unlock_filtab();
    }
    return res;
}

int tcflush(int file, int flags)
{
    int res = EOF;

    if(file == STDOUT_FILENO || file == STDERR_FILENO || file == (intptr_t)stdout || file == (intptr_t)stderr)
    {
        res = 0;
    }
    else if(file == STDIN_FILENO || file == (intptr_t)stdin)
    {
        res = 0;
    }
    else if(lock_filtab())
    {
        filtab_entry_t* fte = __get_entry(file);

        if(fte)
        {
            if(fte->mode == S_IFIFO)
            {
                if(flags == TCIFLUSH)
                {
                    xQueueReset(fte->device->pipe.read);
                    res = 0;
                }

                else if(flags == TCOFLUSH)
                {
                    xQueueReset(fte->device->pipe.write);
                    res = 0;
                }

                else if(flags == TCIOFLUSH)
                {
                    xQueueReset(fte->device->pipe.write);
                    xQueueReset(fte->device->pipe.read);
                    res = 0;
                }
            }
        }
        unlock_filtab();
    }
    return res;
}

#if ENABLE_LIKEPOSIX_SOCKETS

/**
 * wrapper for interfacing lwiip functions with like-posix
 */
#define SOCKET_WRAPPER(lwip_function, sockfd, ...)                  \
    int res = EOF; if(lock_filtab()){                               \
    filtab_entry_t* fte = __get_entry(sockfd);                      \
    if(fte && (fte->mode == S_IFSOCK))                               \
        res = lwip_function((int)fte->device, __VA_ARGS__);         \
    unlock_filtab();                                                \
} return res;


/**
 * creates anew socket and adds it to the file table.
 *
 * @retval  returns a file descriptor, that may be used with
 *          read(), write(), close(), closesocket(), or -1 if there was an error.
 */
int socket(int namespace, int style, int protocol)
{
    int file = EOF;

    if(filtab.count > FILE_TABLE_LENGTH)
        return EOF;

    // create new file table node
    filtab_entry_t* ftn = (filtab_entry_t*)pvPortMalloc(sizeof(filtab_entry_t));

    if(!ftn)
        return EOF;

    ftn->mode = S_IFSOCK;
    ftn->flags = FWRITE | FREAD;
    ftn->device = NULL;
    ftn->size = 0;

    file = lwip_socket(namespace, style, protocol);

    if(file == -1)
        return file;

    // hack socket file descriptor on as the device
    ftn->device = (dev_ioctl_t*)file;
    file = EOF;

    if(lock_filtab())
    {
        // add file to table
        file = __insert_entry(ftn);
        unlock_filtab();
    }

    if(file == EOF)
        __delete_filtab_item(ftn);

    return file;
}

/**
 * TODO - reuse code from socket in accept.
 */
int accept(int sockfd, struct sockaddr *addr, socklen_t *length_ptr)
{
    if(filtab.count > FILE_TABLE_LENGTH)
        return EOF;

//    if(lock_filtab())
//    {
        filtab_entry_t* fte = __get_entry(sockfd);
        sockfd = EOF;
        if(fte && (fte->mode == S_IFSOCK))
            sockfd = lwip_accept((int)fte->device, addr, length_ptr);
//        unlock_filtab();
//    }

    // sockfd is now the accepted socket fdes

    if(sockfd == -1)
        return EOF;

    // create new file table node
    filtab_entry_t* ftn = (filtab_entry_t*)pvPortMalloc(sizeof(filtab_entry_t));

    if(!ftn)
    {
        lwip_close(sockfd);
        return EOF;
    }

    ftn->mode = S_IFSOCK;
    ftn->flags = FWRITE | FREAD;
    ftn->size = 0;
    // hack sockfd file descriptor on as the device
    ftn->device = (dev_ioctl_t*)sockfd;
    sockfd = EOF;

    if(lock_filtab())
    {
        // add file to table
        sockfd = __insert_entry(ftn);
        // sockfd is now the file table entry fdes
        unlock_filtab();
    }

    if(sockfd == EOF)
        __delete_filtab_item(ftn);

    return sockfd;
}

int connect(int sockfd, struct sockaddr *addr, socklen_t length)
{
    SOCKET_WRAPPER(lwip_connect, sockfd, addr, length);
}

int bind(int sockfd, struct sockaddr *addr, socklen_t length)
{
    SOCKET_WRAPPER(lwip_bind, sockfd, addr, length);
}

int shutdown(int sockfd, int how)
{
    SOCKET_WRAPPER(lwip_shutdown, sockfd, how);
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *length)
{
    SOCKET_WRAPPER(lwip_getsockname, sockfd, addr, length);
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *length)
{
    SOCKET_WRAPPER(lwip_getpeername, sockfd, addr, length);
}

int setsockopt(int sockfd, int level, int optname, void *optval, socklen_t optlen)
{
    SOCKET_WRAPPER(lwip_setsockopt, sockfd, level, optname, optval, optlen);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    SOCKET_WRAPPER(lwip_getsockopt, sockfd, level, optname, optval, optlen);
}

int listen(int sockfd, int n)
{
    SOCKET_WRAPPER(lwip_listen, sockfd, n);
}

int recv(int sockfd, void *buffer, size_t size, int flags)
{
    SOCKET_WRAPPER(lwip_recv, sockfd, buffer, size, flags);
}

int recvfrom(int sockfd, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *length)
{
    SOCKET_WRAPPER(lwip_recvfrom, sockfd, buffer, size, flags, addr, length);
}

int send(int sockfd, const void *buffer, size_t size, int flags)
{
    SOCKET_WRAPPER(lwip_send, sockfd, buffer, size, flags);
}

int sendto(int sockfd, const void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t length)
{
    SOCKET_WRAPPER(lwip_sendto, sockfd, buffer, size, flags, addr, length);
}

/**
 * TODO - finish me
 */
//int select(int nfds, fd_set *read-fds, fd_set *write-fds, fd_set *except-fds, struct timeval *timeout)
//{
//    SOCKET_WRAPPER(lwip_select, sockfd, buffer, size, flags, addr, length);
//}

int ioctlsocket(int sockfd, int cmd, void* argp)
{
    SOCKET_WRAPPER(lwip_ioctl, sockfd, cmd, argp);
}

/**
 * clashes with signature in standard header sys/_default_fcntl.h
 */
//int fcntl(int filedes, int cmd, int val)
//{
//    SOCKET_WRAPPER(lwip_fcntl, filedes, cmd, val);
//}
int closesocket(int socket)
{
    return _close(socket);
}
#endif
/**
 * @}
 */
