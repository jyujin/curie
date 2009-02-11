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

#include "curie/exec.h"
#include "curie/sexpr.h"

#include <curie/sexpr-internal.h>

#define TEST_STRING_1 "hello-world!"
#define TEST_STRING_2 "hello \"world\"!"

define_symbol(test_symbol_1, "hello-world!");
define_string(test_string_1, "hello \"world\"!");
define_symbol(sym_success, "success");
define_symbol(sym_failure, "failure");

int cmain(void) {
    struct exec_context *context;
    int rv = 0;
    sexpr i,
          t1 = test_symbol_1,
          t2 = test_string_1,
          t3 = sym_success;
    struct sexpr_io *io, *stdio;

    context = execute(EXEC_CALL_PURGE, (char **)0, (char **)0);

    stdio = sx_open_stdio();

    switch (context->pid) {
        case -1: /* only way this would fail ... */
            sx_write (stdio, sym_failure);
            return 1;
        case 0:
            {
                do {
                    i = sx_read(stdio);
                } while (i == sx_nonexistent);

                rv |= (equalp(i, t1) ? 0 : 1) << 1;
                sx_destroy(i);

                do {
                    i = sx_read(stdio);
                } while (i == sx_nonexistent);

                rv |= (equalp(i, t2) ? 0 : 1) << 2;
                sx_destroy(i);

                i = sx_read(stdio);
                rv |= (equalp(i, sx_nonexistent) ? 0 : 1) << 3;
                sx_destroy(i);
            }

            break;
        default:
            {
                io = sx_open_io (context->in, context->out);

                sx_write (io, t1);
                sx_write (io, t2);

                while (io_commit (io->out) == io_incomplete);

                do {
                    i = sx_read(io);
                } while (i == sx_nonexistent);

                rv |= (equalp(i, t3) ? 0 : 1) << 4;
                sx_write (stdio, i);
                sx_destroy(i);

                sx_close_io (io);
            }

            break;
    }

    if (rv != 0) {
        sx_write (stdio, sym_failure);
        sx_write (stdio, make_integer (rv));
    } else {
        sx_write (stdio, t3);
    }

    sx_destroy (t1);
    sx_destroy (t2);
    sx_destroy (t3);

    sx_close_io (stdio);

    if (context->pid > 0) {
        while (context->status == ps_running)
            check_exec_context (context);
    }
    free_exec_context (context);

    return rv;
}
