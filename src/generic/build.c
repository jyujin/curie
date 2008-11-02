/*
 *  build.c
 *  libcurie
 *
 *  Created by Magnus Deininger on 01/11/2008.
 *  Copyright 2008 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, Magnus Deininger All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. *
 * Neither the name of the project nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <curie/sexpr.h>
#include <curie/memory.h>
#include <curie/tree.h>

#include <stdlib.h>
#include <stdio.h>

#define BUFFERSIZE 4096

static sexpr sym_library   = sx_false;
static sexpr sym_programme = sx_false;
static sexpr sym_code      = sx_false;

static struct sexpr_io *stdio;

struct target {
    sexpr name;
    sexpr library;
    sexpr code;
};

enum fs_layout
{
    fs_fhs,
    fs_sheer
};

static void *rm_recover(unsigned long int s, void *c, unsigned long int l)
{
    exit(22);
    return (void *)0;
}

static void *gm_recover(unsigned long int s)
{
    exit(23);
    return (void *)0;
}

static sexpr sx_string_dir_prefix (sexpr file, sexpr p)
{
    char buffer[BUFFERSIZE];

    snprintf (buffer, BUFFERSIZE, "%s/%s", sx_string(p), sx_string(file));

    return make_string(buffer);
}

static sexpr find_code_in_permutations (sexpr file)
{
    sexpr r;

    r = sx_string_dir_prefix (file, make_string("src/generic"));

    sx_write (stdio, r);

    sx_destroy (r);
    sx_destroy (file);

    return sx_false;
}

static sexpr find_code_with_suffix (sexpr file, char *s)
{
    char buffer[BUFFERSIZE];

    snprintf (buffer, BUFFERSIZE, "%s%s", sx_string(file), s);

    return find_code_in_permutations (make_string(buffer));
}

static sexpr find_code_c (sexpr file)
{
    return find_code_with_suffix (file, ".c");
}

static void find_code (struct target *context, sexpr file)
{
    sexpr r;

    if (!stringp (file)) return;

    sx_xref (file);
    sx_xref (file);

    if (stringp (r = find_code_c (file)))
    {
        context->code = cons (cons(file, file), context->code);
    }
}

static struct target *get_context()
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER (sizeof(struct target));
    struct target *context = get_pool_mem (&pool);

    context->code    = sx_false;
    context->library = sx_false;
    context->code    = sx_end_of_list;

    return context;
}

static void process_definition (struct target *context, sexpr definition)
{
    while (consp(definition))
    {
        sexpr sxcar = car (definition);
        sexpr sxcaar = car (sxcar);

        if (truep(equalp(sxcaar, sym_code)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                find_code (context, car (sxc));

                sxc = cdr (sxc);
            }
        }

        definition = cdr (definition);
    }
}

static struct target *create_library (sexpr definition)
{
    struct target *context = get_context();

    context->name = car(definition);
    context->library = sx_true;
    sx_xref (context->name);

    process_definition (context, cdr(definition));

    return context;
}

static struct target *create_programme (sexpr definition)
{
    struct target *context = get_context();

    context->name = car(definition);
    sx_xref (context->name);

    process_definition (context, cdr(definition));

    return context;
}

static void do_build_target(struct target *t)
{
    fprintf (stdout, "building: %s\n", sx_string(t->name));
}

static void build_target (struct tree *targets, const char *target)
{
    struct tree_node *node = tree_get_node_string(targets, (char *)target);
    
    if (node != (struct tree_node *)0)
    {
        do_build_target (node_get_value(node));
    }
}

static void print_help(char *binaryname)
{
    fprintf (stdout,
        "Usage: %s [options] [targets]\n\n"
        "Options:\n"
        " -h          Print help and exit\n"
        " -t <chost>  Specify target CHOST\n\n"
        "The [targets] specify a list of things to build, according to the\n"
        "icemake.sx file located in the current working directory.\n\n",
        binaryname);
}

static void target_map_build (struct tree_node *node, void *u)
{
    do_build_target(node_get_value(node));
}

int main (int argc, char **argv)
{
    struct sexpr_io *io;
    sexpr r;
    struct tree targets = TREE_INITIALISER;
    int i = 1;
    char *target_architecture = (char *)0;
    sexpr buildtargets = sx_end_of_list;

    set_resize_mem_recovery_function(rm_recover);
    set_get_mem_recovery_function(gm_recover);

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            int y = 1;
            int xn = i + 1;
            while (argv[i][y] != 0)
            {
                switch (argv[i][y])
                {
                    case 't':
                        if (xn < argc)
                        {
                            target_architecture = argv[xn];
                            xn++;
                        }
                        break;
                    case 'h':
                    case '-':
                        print_help (argv[0]);
                        return 0;
                }
            }

            i = xn;
        }
        else
        {
            buildtargets = cons (make_string (argv[i]), buildtargets);
            i++;
        }
    }

    if (target_architecture != (char *)0)
    {
    }

    sym_library   = make_symbol ("library");
    sym_programme = make_symbol ("programme");
    sym_code      = make_symbol ("code");
    stdio = sx_open_stdio();

    io = sx_open_io (io_open_read("icemake.sx"), io_open(-1));

    while (!eofp(r = sx_read (io)))
    {
        struct target *t = (struct target *)0;

        if (nexp(r)) continue;
        if (consp(r)) {
            sexpr sxcar = car (r);

            if (truep(equalp(sxcar, sym_library)))
            {
                t = create_library (cdr (r));
            }
            else if (truep(equalp(sxcar, sym_programme)))
            {
                t = create_programme (cdr (r));
            }

        }

        if (t != (struct target *)0)
        {
            tree_add_node_string_value (&targets, (char *)sx_string(t->name), t);
        }

        sx_destroy (r);
    }

    if (eolp(buildtargets))
    {
        tree_map (&targets, target_map_build, (void *)0);
    }
    else while (consp(buildtargets))
    {
        build_target (&targets, sx_string(car(buildtargets)));
        buildtargets = cdr(buildtargets);
    }

    return 0;
}
