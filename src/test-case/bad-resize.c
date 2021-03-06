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

#include <curie/main.h>
#include <curie/memory.h>

static void *rm_recover(unsigned long int s, void *c, unsigned long int l)
{
    static char run = (char)0;

    if (run == (char)0) {
        run = (char)1;
        return (void *)1;
    }

    cexit(0);
    return (void *)0;
}

int cmain(void) {
    void *y = get_mem (1), *z;

    if (y == (void *)0) return 1;

    resize_mem_recovery = rm_recover;
    z = resize_mem (1, y, ~0); /* rm_recover (hopefully) */

    if (z != (void *)1) return 2; /* value from rm_recover */
    resize_mem (1, y, ~0); /* now it should call cexit() */

    return 3;
}
