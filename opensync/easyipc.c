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

#include <sys/types.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#include "easyipc.h"

static eipc_err_func _err_func = NULL;
static eipc_exit_func _exit_func = exit;

int eipc_err_func_set(eipc_err_func func)
{
  _err_func = func;
  return 0;
}


eipc_err_func eipc_err_func_rem(void)
{
  eipc_err_func old = _err_func;
  _err_func = NULL;
  return old;
}


int eipc_exit_func_set(eipc_exit_func func)
{
  _exit_func = func;
  return 0;
}


eipc_exit_func eipc_exit_func_rem(void)
{
  eipc_exit_func old = _exit_func;
  _exit_func = exit;
  return old;
}


int eipc_make_connection(const EIpcSockType type, const char *identifier)
{
  /*
  int sock;
  const int on = 1;
  struct sockaddr_in name;
  struct hostent *hp;

  if ( (sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_CREATE, EIPC_NONE);
    return -1;
  }

  name.sin_family = AF_INET;
  name.sin_port = htons(port);

  switch (type) {

  case EIPC_SERVER:
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&on,
             sizeof(on)) < 0
        || bind(sock, (SA *)&name, sizeof(name)) < 0) {
      if (_err_func)
        (*_err_func) (strerror(errno), EIPC_CREATE,
                EIPC_NONE);
      return -1;
    }
    break;

  case EIPC_CLIENT:
    if ( (hp = gethostbyname(server)) == NULL) {
      if (_err_func)
        (*_err_func) ("Lookup failure", EIPC_CREATE,
                EIPC_NONE);
      return -1;
      break;
    }
    memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
    if (connect(sock, (SA *)&name, sizeof(name)) < 0) {
      if (_err_func)
        (*_err_func) (strerror(errno), EIPC_CREATE,
                EIPC_NONE);
      return -1;
    }
    break;
  }

  return sock;
  */

  return 0;
}


int eipc_make_connection_or_die(const EIpcSockType type, const char *identifier)
{
  int sock;

  if ( (sock = eipc_make_connection(type, identifier)) < 0)
    (*_exit_func) (-1);

  return sock;
}


int eipc_write_char(const int sock, const char message)
{
  if (eipc_writen(sock, &message, sizeof(char)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_WRITE, EIPC_CHAR);
    return -1;
  }

  return 0;
}


void eipc_write_char_or_die(const int sock, const char data)
{
  if (eipc_write_char(sock, data) < 0)
    (*_exit_func) (-1);
}


int eipc_read_char(const int sock, char *message)
{
  int status;

  if ( (status = eipc_readn(sock, message, sizeof(char))) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_READ, EIPC_CHAR);
    return -1;
  }

  if (status < sizeof(char)) {
    if (_err_func)
      (*_err_func) ("fd closed", EIPC_READ, EIPC_CHAR);
    return -1;
  }

  return 0;
}


void eipc_read_char_or_die(const int sock, char *data)
{
  if (eipc_read_char(sock, data) < 0)
    (*_exit_func) (-1);
}


/*
 * Take a host-byte order int and write on fd using
 * network-byte order.
 */
int eipc_write_int(const int sock, const int message)
{
  if (eipc_writen(sock, &message, sizeof(int)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_WRITE, EIPC_INT);
    return -1;
  }

  return 0;
}


void eipc_write_int_or_die(const int sock, const int data)
{
  if (eipc_write_int(sock, data) < 0)
    (*_exit_func) (-1);
}


/*
 * Read a network byte-order integer from the fd and
 * store it in host-byte order.
 */
int eipc_read_int(const int sock, int *message)
{
  int status;

  if ( (status = eipc_readn(sock, message, sizeof(int))) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_READ, EIPC_INT);
    return -1;
  }

  if (status < sizeof(int)) {
    if (_err_func)
      (*_err_func) ("fd closed", EIPC_READ, EIPC_INT);
    return -1;
  }

  return 0;
}


void eipc_read_int_or_die(const int sock, int *data)
{
  if (eipc_read_int(sock, data) < 0)
    (*_exit_func) (-1);
}

int eipc_write_long_long_int(const int sock, const long long int message)
{
  if (eipc_writen(sock, &message, sizeof(long long int)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_WRITE, EIPC_INT);
    return -1;
  }

  return 0;
}

int eipc_read_long_long_int(const int sock, long long int *message)
{
  long long int data, status;

  if ( (status = eipc_readn(sock, &data, sizeof(long long int))) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_READ, EIPC_INT);
    return -1;
  }

  if (status < sizeof(long long int)) {
    if (_err_func)
      (*_err_func) ("fd closed", EIPC_READ, EIPC_INT);
    return -1;
  }

  *message = data;
  return 0;
}

/*
 * Write a char string to the given fd preceeded by its size
 */
int eipc_write_string(const int sock, const char *message)
{
  unsigned int size = strlen(message) * sizeof(char) + 1;

  if (eipc_write_int(sock, size) < 0)
    return -1;

  if (eipc_writen(sock, message, size) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_WRITE, EIPC_STRING);
    return -1;
  }

  return 0;
}


int eipc_va_write_string(const int sock, const char *fmt, ...)
{
  char buf[4096];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);

  return eipc_write_string(sock, buf);
}


void eipc_write_string_or_die(const int sock, const char *data)
{
  if (eipc_write_string(sock, data) < 0)
    (*_exit_func) (-1);
}


void eipc_va_write_string_or_die(const int sock, const char *fmt, ...)
{
  char buf[4096];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);

  eipc_write_string_or_die(sock, buf);
}


/*
 * Read a char string from the given fd
 */
int eipc_read_string(const int sock, char *message, const unsigned int len)
{
  int size;
  int status;

  if (eipc_read_int(sock, &size) < 0)
    return -1;

  if (size > len) {
    if (_err_func)
      (*_err_func) ("String too long", EIPC_READ, EIPC_STRING);
    return -1;
  }

         if ( (status = eipc_readn(sock, message, size)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_READ, EIPC_STRING);
    return -1;
  }

  if (status < size) {
    if (_err_func)
      (*_err_func) ("fd closed", EIPC_READ, EIPC_STRING);
    return -1;
  }

  /* Guarantee NULL-termination */
  message[len-1] = '\0';

  return 0;
}


void eipc_read_string_or_die(const int sock, char *data, const unsigned int len)
{
  if (eipc_read_string(sock, data, len) < 0)
    (*_exit_func) (-1);
}


/*
 * Read a char string from the given fd and allocate space for it.
 */
int eipc_read_string_alloc(const int sock, char **message)
{
  int size;
  int status;

  if (eipc_read_int(sock, &size) < 0)
    return -1;

  if ( (*message = calloc((size+1), sizeof(char))) == NULL) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_ALLOCATE, EIPC_STRING);
    return -1;
  }

  if ( (status = eipc_readn(sock, *message, size)) < 0) {
    if (_err_func)
      (*_err_func) (strerror(errno), EIPC_READ, EIPC_STRING);
    return -1;
  }

  if (status < size) {
    if (_err_func)
      (*_err_func) ("fd closed", EIPC_READ, EIPC_STRING);
    return -1;
  }

  return 0;
}


void eipc_read_string_alloc_or_die(const int sock, char **data)
{
  if (eipc_read_string_alloc(sock, data) < 0)
    (*_exit_func) (-1);
}


/* Write "n" bytes to a descriptor. */
int eipc_writen(int fd, const void *vptr, size_t n)
{

  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (errno == EINTR)
        nwritten = 0;  /* and call write() again */
      else
        return (-1);  /* error */
    }

    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n);
}


/* Read "n" bytes from a descriptor. */
int eipc_readn(int fd, void *vptr, size_t n)
{

  size_t nleft;
  ssize_t nread;
  char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;  /* and call read() again */
      else
        return (-1);
    } else if (nread == 0)
      break;  /* EOF */

    nleft -= nread;
    ptr += nread;
  }
  return (n - nleft);  /* return >= 0 */
}
