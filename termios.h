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

#ifndef _TERMIOS_H
#define _TERMIOS_H

typedef unsigned char   cc_t;
typedef unsigned int    speed_t;
typedef unsigned int    tcflag_t;


#define NCCS 1
struct termios
  {
    tcflag_t c_iflag;       /* input mode flags */
    tcflag_t c_oflag;       /* output mode flags */
    tcflag_t c_cflag;       /* control mode flags */
    tcflag_t c_lflag;       /* local mode flags */
    cc_t c_line;            /* line discipline */
    cc_t c_cc[NCCS];        /* control characters */
    speed_t c_ispeed;       /* input speed */
    speed_t c_ospeed;       /* output speed */
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
  };

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK  0000001
#define BRKINT  0000002
#define IGNPAR  0000004
#define PARMRK  0000010
#define INPCK   0000020
#define ISTRIP  0000040
#define INLCR   0000100
#define IGNCR   0000200
#define ICRNL   0000400
#define IUCLC   0001000
#define IXON    0002000
#define IXANY   0004000
#define IXOFF   0010000
#define IMAXBEL 0020000
#define IUTF8   0040000

/* c_oflag bits */
#define OPOST   0000001
#define OLCUC   0000002
#define ONLCR   0000004
#define OCRNL   0000010
#define ONOCR   0000020
#define ONLRET  0000040
#define OFILL   0000100
#define OFDEL   0000200
#if defined __USE_MISC || defined __USE_XOPEN
# define NLDLY  0000400
# define   NL0  0000000
# define   NL1  0000400
# define CRDLY  0003000
# define   CR0  0000000
# define   CR1  0001000
# define   CR2  0002000
# define   CR3  0003000
# define TABDLY 0014000
# define   TAB0 0000000
# define   TAB1 0004000
# define   TAB2 0010000
# define   TAB3 0014000
# define BSDLY  0020000
# define   BS0  0000000
# define   BS1  0020000
# define FFDLY  0100000
# define   FF0  0000000
# define   FF1  0100000
#endif

#define VTDLY   0040000
#define   VT0   0000000
#define   VT1   0040000

#ifdef __USE_MISC
# define XTABS  0014000
#endif

/* c_cflag bit meaning */
#ifdef __USE_MISC
# define CBAUD  0010017
#endif
#define  B0 0000000     /* hang up */
#define  B50    0000001
#define  B75    0000002
#define  B110   0000003
#define  B134   0000004
#define  B150   0000005
#define  B200   0000006
#define  B300   0000007
#define  B600   0000010
#define  B1200  0000011
#define  B1800  0000012
#define  B2400  0000013
#define  B4800  0000014
#define  B9600  0000015
#define  B19200 0000016
#define  B38400 0000017
#ifdef __USE_MISC
# define EXTA B19200
# define EXTB B38400
#endif
#define CSIZE   0000060
#define   CS5   0000000
#define   CS6   0000020
#define   CS7   0000040
#define   CS8   0000060
#define CSTOPB  0000100
#define CREAD   0000200
#define PARENB  0000400
#define PARODD  0001000
#define HUPCL   0002000
#define CLOCAL  0004000
#ifdef __USE_MISC
# define CBAUDEX 0010000
#endif
#define  B57600   0010001
#define  B115200  0010002
#define  B230400  0010003
#define  B460800  0010004
#define  B500000  0010005
#define  B576000  0010006
#define  B921600  0010007
#define  B1000000 0010010
#define  B1152000 0010011
#define  B1500000 0010012
#define  B2000000 0010013
#define  B2500000 0010014
#define  B3000000 0010015
#define  B3500000 0010016
#define  B4000000 0010017
#define __MAX_BAUD B4000000
#ifdef __USE_MISC
# define CIBAUD   002003600000      /* input baud rate (not used) */
# define CMSPAR   010000000000      /* mark or space (stick) parity */
# define CRTSCTS  020000000000      /* flow control */
#endif

/* c_lflag bits */
#define ISIG    0000001
#define ICANON  0000002
#if defined __USE_MISC || defined __USE_XOPEN
# define XCASE  0000004
#endif
#define ECHO    0000010
#define ECHOE   0000020
#define ECHOK   0000040
#define ECHONL  0000100
#define NOFLSH  0000200
#define TOSTOP  0000400
#ifdef __USE_MISC
# define ECHOCTL 0001000
# define ECHOPRT 0002000
# define ECHOKE  0004000
# define FLUSHO  0010000
# define PENDIN  0040000
#endif
#define IEXTEN  0100000
#ifdef __USE_BSD
# define EXTPROC 0200000
#endif

/* tcflow() and TCXONC use these */
#define TCOOFF      0
#define TCOON       1
#define TCIOFF      2
#define TCION       3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH    0
#define TCOFLUSH    1
#define TCIOFLUSH   2

/* tcsetattr uses these */
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2


#define _IOT_termios /* Hurd ioctl type field.  */ \
  _IOT (_IOTS (cflag_t), 4, _IOTS (cc_t), NCCS, _IOTS (speed_t), 2)


/* Return the output baud rate stored in *TERMIOS_P.  */
extern speed_t cfgetospeed (const struct termios *__termios_p);

/* Return the input baud rate stored in *TERMIOS_P.  */
extern speed_t cfgetispeed (const struct termios *__termios_p);

/* Set the output baud rate stored in *TERMIOS_P to SPEED.  */
extern int cfsetospeed (struct termios *__termios_p, speed_t __speed);

/* Set the input baud rate stored in *TERMIOS_P to SPEED.  */
extern int cfsetispeed (struct termios *__termios_p, speed_t __speed);

/* Set both the input and output baud rates in *TERMIOS_OP to SPEED.  */
extern int cfsetspeed (struct termios *__termios_p, speed_t __speed);

/* Put the state of FD into *TERMIOS_P.  */
extern int tcgetattr (int __fd, struct termios *__termios_p);

/* Set the state of FD to *TERMIOS_P.
   Values for OPTIONAL_ACTIONS (TCSA*) are in <bits/termios.h>.  */
extern int tcsetattr (int __fd, int __optional_actions,
              const struct termios *__termios_p);

/* Set *TERMIOS_P to indicate raw mode.  */
extern void cfmakeraw (struct termios *__termios_p);

/* Send zero bits on FD.  */
extern int tcsendbreak (int __fd, int __duration);

/* Wait for pending output to be written on FD.

   This function is a cancellation point and therefore not marked with
  .  */
extern int tcdrain (int __fd);

/* Flush pending data on FD.
   Values for QUEUE_SELECTOR (TC{I,O,IO}FLUSH) are in <bits/termios.h>.  */
extern int tcflush (int __fd, int __queue_selector);

/* Suspend or restart transmission on FD.
   Values for ACTION (TC[IO]{OFF,ON}) are in <bits/termios.h>.  */
extern int tcflow (int __fd, int __action);

#endif /* termios.h  */
