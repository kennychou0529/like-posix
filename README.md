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
 
 ** termios calls **
 
 * int tcgetattr(int fildes, struct termios *termios_p)
 * int tcsetattr(int fildes, int when, struct termios *termios_p)
 * speed_t cfgetispeed(const struct termios* termios)
 * speed_t cfgetospeed(const struct termios* termios)
 * int cfsetispeed(struct termios* termios, speed_t ispeed)
 * int cfsetospeed(struct termios* termios, speed_t ospeed)
 * int tcdrain(int file)
 * int tcflow(int file, int flags)
 * int tcflush(int file, int flags)

 **lwip based socket api, when enabled***

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
 * gethostbyname (directly mapped to lwip_gethostbyname)
 * gethostbyname_r (directly mapped to lwip_gethostbyname_r)
 * freeaddrinfo  (directly mapped to lwip_freeaddrinfo)
 * getaddrinfo (directly mapped to lwip_getaddrinfo)
 
**minimal system calls**

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
 * sample configuration for the like-posix project.
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
/**
 * this is a hack that adds an ofset in seconds onto the time returned by time/gettimeofday.
 * corrects time set by NTP for your timezone. 12 hours for NZT
 */
#define TIMEZONE_OFFSET (12 * 60 * 60)
/**
 * enable full integration of lwip sockets in likeposix
 */
#define ENABLE_LIKEPOSIX_SOCKETS    0

#endif /* LIKEPOSIX_CONFIG_H_ */

```

Base Filesystem
-----------------

Many projects are better when based on a structured filessystem.
like-posix provides a filesytem template that can be installed on an SD card.
It is loosely structured, as follows:

 - /dev
    - this directory is generated by the system.
 	- devices such as serial and USB ports installed here, IO may be performed on them just like normal files.
 	- for serial devices the file naming convention will be "ttySx", starting at 0
 - /var/log
 	- log files are stored here, syslog, bootlog and errorlog for example
 - /var/lib/httpd
 	- default location for files served by an http server
 - /etc/network
 	- network configuration files live here: interface, ntp, resolv...
 - /etc/logging
 	- logging configuration files live here: logging.conf
 - /etc/shell
 	- shell server configuration files live here: shelld_config
 - /etc/echo
 	- echo server configuration files live here: echod_config
 - /etc/http
 	- http server configuration files live here: httpd_config
 - /rom
    - firmware images for use by the sdcard bootloader live here

 	
 
 	
 	

