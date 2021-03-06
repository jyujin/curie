/**\internal
 *
 * @{
 */

/**\file
 * \brief Glue Code Header for io.h
 *
 * \copyright
 * Copyright (c) 2008-2014, Kyuba Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/curie
 * \see Project Source Code: http://git.becquerel.org/kyuba/curie.git
*/

#ifndef LIBCURIE_IO_SYSTEM_H
#define LIBCURIE_IO_SYSTEM_H

#include <curie/io.h>

#ifdef __cplusplus
extern "C" {
#endif
  int a_read  (int fd, void *buf, unsigned int count);
  int a_write (int fd, const void *buf, unsigned int count);

  int a_open_read (const char *path);
  int a_open_write (const char *path);
  int a_create (const char *path, int mode);
  int a_close (int fd);

  int a_dup (int ofd, int nfd);
  int a_dup_n (int fd);

  int a_make_nonblocking (int fd);

  int a_unlink (const char *path);

  int a_stat(const char *path, void *buffer);
  int a_lstat(const char *path, void *buffer);

  extern char last_error_recoverable_p;

  struct io *io_create ();
  void io_destroy (struct io *io);
#ifdef __cplusplus
}
#endif

#endif /* LIBCURIE_IO_SYSTEM_H */

/*! @} */
