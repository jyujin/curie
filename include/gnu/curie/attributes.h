/**\file
 * \brief Compiler-specific attributes (GNU/GCC version)
 *
 * In this file we're trying to collect compiler-specific attributes that are
 * useful to compilers, but which are inherently compiler-specific, in order to
 * be able to use them with just about any compiler.
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

#ifndef LIBCURIE_ATTRIBUTES_H
#define LIBCURIE_ATTRIBUTES_H

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4))
#define UNREACHABLE __builtin_unreachable ();
#else
#define UNREACHABLE
#endif

#define USED __attribute__((used))

#define NORETURN __attribute__((noreturn))

#define EXTENSION __extension__

#define PURE __attribute__((pure))

#endif
