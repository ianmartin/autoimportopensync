/*
    This file is part of OpenSync
    Copyright (C) 2005 Tobias Koenig <tokoe@kde.org>
                       based on libeasysock by Brent Hendricks

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef EASYIPC_H
#define EASYIPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef enum {
  EIPC_CREATE,
  EIPC_READ,
  EIPC_WRITE,
  EIPC_ALLOCATE
} EIpcOpType;

typedef enum {
  EIPC_NONE,
  EIPC_CHAR,
  EIPC_INT,
  EIPC_STRING,
  EIPC_FD
} EIpcDataType;


/****************************************************************************
 * Error function
 *
 * Any easyipc function that generates an error will call this
 * function, if defined.
 *
 * Upon removal, the old error function is returned.
 *
 ***************************************************************************/
typedef void (*eipc_err_func) (const char *, const EIpcOpType, const EIpcDataType);

int eipc_err_func_set(eipc_err_func func);
eipc_err_func eipc_err_func_rem(void);


/****************************************************************************
 * Exit function
 *
 * Any of the *_or_die() functions will call the set exit function
 * if there is an error.  If there is no set function, exit() will be
 * called.
 *
 * Upon removal, the old exit function is returned.
 *
 ***************************************************************************/
typedef void (*eipc_exit_func) (int);

int eipc_exit_func_set(eipc_exit_func func);
eipc_exit_func eipc_exit_func_rem(void);


/****************************************************************************
 * Creating a new communication channel.
 *
 * type       : one of EIPC_SERVER or EIPC_CLIENT
 * identifier : the unique identifier for this connection
 * Returns socket fd or -1 on error
 ***************************************************************************/
typedef enum {
  EIPC_SERVER,
  EIPC_CLIENT
} EIpcSockType;

int eipc_make_connection(const EIpcSockType type, const char *identifier);
int eipc_make_connection_or_die(const EIpcSockType type, const char *identifier);

/****************************************************************************
 * Reading/Writing a single char.
 *
 * sock  :  socket fd
 * data  :  single char for write.  pointer to char for read
 *
 * Returns 0 if successful, -1 on error.
 ***************************************************************************/
int eipc_write_char(const int sock, const char data);
void eipc_write_char_or_die(const int sock, const char data);
int eipc_read_char(const int sock, char *data);
void eipc_read_char_or_die(const int sock, char *data);


/****************************************************************************
 * Reading/Writing an integer in network byte order
 *
 * sock  :  socket fd
 * data  :  int for write.  pointer to int for read
 *
 * Returns 0 if successful, -1 on error.
 ***************************************************************************/
int eipc_write_int(const int sock, const int data);
void eipc_write_int_or_die(const int sock, const int data);
int eipc_read_int(const int sock, int *data);
void eipc_read_int_or_die(const int sock, int *data);

int eipc_write_long_long_int(const int sock, const long long int data);
int eipc_read_long_long_int(const int sock, long long int *data);


/****************************************************************************
 * Reading/Writing a char string
 *
 * sock  : socket fd
 * data  : char string or address of char string for alloc func
 * fmt   : format string for sprintf-like behavior
 * len   : length of user-provided data buffer in bytes
 *
 * Returns 0 if successful, -1 on error.
 *
 ***************************************************************************/
int eipc_write_string(const int sock, const char *data);
void eipc_write_string_or_die(const int sock, const char *data);
int eipc_va_write_string(const int sock, const char *fmt, ...);
void eipc_va_write_string_or_die(const int sock, const char *fmt, ...);
int eipc_read_string(const int sock, char *data, const unsigned int len);
void eipc_read_string_or_die(const int sock, char *data, const unsigned int len);
int eipc_read_string_alloc(const int sock, char **data);
void eipc_read_string_alloc_or_die(const int sock, char **data);


/****************************************************************************
 * Reading/Writing a byte sequence
 *
 * sock  : socket fd
 * data  : address of data to be read/written
 * n     : size of data (in bytes) to be read/written
 *
 * Returns number of bytes processed, or -1 on error
 *
 * Many thanks to Richard Stevens and his wonderful books, from which
 * these functions come.
 *
 ***************************************************************************/
int eipc_readn(const int sock, void *data, size_t n);
int eipc_writen(const int sock, const void *vdata, size_t n);

#ifdef __cplusplus
}
#endif

#endif
