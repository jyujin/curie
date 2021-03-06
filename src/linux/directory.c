/**\file
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

#include <syscall/syscall.h>
#include <curie/directory.h>
#include <curie/directory-system.h>
#include <curie/internal-constants.h>
#include <curie/io-system.h>

/* okay, i give up, kernel header hell wins. i suppose this can't really change
   too often anyway, since it'd break binary compatibility. */

struct dirent64 {
    int_64          d_ino;
    int_64_s        d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

sexpr read_directory_rx (const char *base, sexpr rx)
{
    int fd = sys_open (base, 0x10800 /* O_RDONLY | O_NONBLOCK | O_DIRECTORY */,
                       0444);
    sexpr r = sx_end_of_list;

    if (fd >= 0)
    {
        char buffer[LIBCURIE_DIRENT_BUFFER_SIZE];
        int rc;

        while ((rc = sys_getdents64 (fd, &buffer, sizeof(buffer))) > 0)
        {
            unsigned int p = 0;

            for (struct dirent64 *e = (struct dirent64 *)buffer; p < rc;
                 e = (struct dirent64 *)(buffer + (p = p + e->d_reclen)))
            {
                const char *s = e->d_name;

                if (truep (rx_match (rx, s)))
                {
                    r = cons (make_string (s), r);
                }
            }
        }

        a_close (fd);
    }

    return r;
}
