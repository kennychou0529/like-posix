like-posix
==========

Builds on top of FreeRTOS, and FatFs by ChaN, providing posix style system calls.


relies upon:
 - FatFs by ChaN
 - FreeRTOS
 - cutensils
 - minstdlibs
 - the file likeposix_config.h / modified for specific project


**system calls**

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

**minimally system calls**

 * void _exit(int i)
 * caddr_t _sbrk(int incr)
 * int _link(char *old, char *new)
 * int _execve(char *name, char **argv, char **env)
 * int _fork()
 * int _getpid()
 * int _kill(int pid, int sig)
 * int times(struct tm *buf)
 * int _wait(int *status)

**stdio**
 
Note, these are implemented in the minstdlibs project.
 
 * FILE* fopen(const char * filename, const char * mode);
 * int fclose(FILE* stream);
 * int fprintf(FILE* stream, const char * fmt, ...);
 * int fputc(int character, FILE* stream);
 * int fputs(const char* str, FILE* stream);
 * int fgetc(FILE* stream);
 * char* fgets(char* str, int num, FILE* stream);
 * long int ftell(FILE* stream);
 * int fseek(FILE * stream, long int offset, int origin);

**sdlib**

When linked with like-posix these utilize tyhe FreeRTOS memory API.

 * void* malloc(size_t num);
 * void* calloc(size_t num, size_t size);
 * void* realloc(void* old, size_t newsize);
 * void free(void* ptr);
 

Configuration
-------------

the file likeposix_config.h is required at project level, to configure like-posix.

``` c

/**
 * sample configuration for the freertos-posix-wrapper project.
 */

#ifndef LIKEPOSIX_CONFIG_H_
#define LIKEPOSIX_CONFIG_H_

/**
 * fudge factor for the file table, presently can be any value higher than STDIN_FILENO...
 */
#define FILE_TABLE_OFFSET		10
/**
 * the maximum number of open files/devices
 */
#define FILE_TABLE_LENGTH 		32
/**
 * the maximum number of installed devices, maximum of 255
 */
#define DEVICE_TABLE_LENGTH 	10
/**
 * location where devices get installed to. 
 * this directory is special, dont write regular files here :)
 * it is reserved for devices.
 */
#define DEVICE_INTERFACE_DIRECTORY 	"/dev/"

#endif /* LIKEPOSIX_CONFIG_H_ */

```

Base Filesystem
-----------------

Many projects are better when based on a structured filessystem.
like-posix provides a filesytem template that can be installed on an SD card.
It is loosely structured, similarly to the root filesystem of a posix OS.


 - /dev/
 	- devices such as serial and USB ports installed here, IO may be performed on them just like normal files.
 	- for serial devices the file naming convention will be "ttySx", starting at 0
 - /var/log/
 	- log files are stored here, syslog and errorlog for example
 - /var/lib/httpd/
 	- default location for files served by an http server
 - /etc/network/
 	- network configuration files live here: interface, ntp, resolv...
 - /etc/logging/
 	- logging configuration files live here: logging.conf
 - /home/user/
 	- there is no concept of the "user", but this is a good place to put random files 
 - /tmp/
 	- temporary files are to be created here
 	
 
 	
 	

