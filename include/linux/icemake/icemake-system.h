/*
 * This file is part of the becquerel.org Curie project.
 * See the appropriate repository at http://git.becquerel.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008-2011, Kyuba Project Members
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

/*! \addgroup icemake
 *  \internal
 *
 * @{
 */

/*! \file
 *  \brief System Specific Definitions for the icemake Programme
 *
 *  The linux version is using libsyscall for most of the funky things.
 */

#ifndef ICEMAKE_ICEMAKE_SYSTEN_H
#define ICEMAKE_ICEMAKE_SYSTEM_H

#include <syscall/syscall.h>

#define symlink(a,b) sys_symlink(a,b)
#define mkdir(a,b)   sys_mkdir(a,b)
#define unlink(a)    sys_unlink(a)
#define getcwd(a,b)  sys_getcwd(a,b)
#define uname(a)     sys_uname(a)
#define chdir(a)     sys_chdir(a)

#endif

/*! @} */
