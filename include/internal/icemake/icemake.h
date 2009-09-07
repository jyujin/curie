/*
 * This file is part of the kyuba.org Curie project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008, 2009, Kyuba Project Members
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

/*! \defgroup icemake icemake Internals
 *  \internal
 *
 * @{
 */

/*! \file
 *  \brief Global Definitions for the icemake Programme
 *
 */

#ifndef ICEMAKE_ICEMAKE_H
#define ICEMAKE_ICEMAKE_H

#include <curie/sexpr.h>

struct toolchain
{
};

struct os
{
    sexpr (*get_os_search_paths)();
    sexpr (*mkdir)(sexpr);
};

struct arch
{
    sexpr generic_name;
    sexpr compatible_with;
};

void initialise_targets ();

sexpr get_host_chost   ();
sexpr get_target_chost (const char *chost);

sexpr set_install_destination (sexpr destination);
sexpr write_output (sexpr output);

sexpr build_file (sexpr type, sexpr shortname, sexpr file, sexpr target);
sexpr link_file (sexpr type, sexpr file, sexpr target);
sexpr install_file (sexpr type, sexpr file);

#endif

/*! @} */
