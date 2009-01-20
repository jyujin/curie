/*
 *  icemake-crosslink.c
 *  libcurie/icemake
 *
 *  Split from icemake.c by Magnus Deininger on 20/01/2009.
 *  Copyright 2008, 2009 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, 2009, Magnus Deininger All rights reserved.
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

#include <curie/tree.h>

#include <stdlib.h>

#include <icemake/icemake.h>

static void do_cross_link (struct target *target, struct target *source)
{
    sexpr cur = source->code;

    while (consp (cur))
    {
        sexpr ccar = car (cur);
        sexpr cccdr = cdr (ccar);

        target->code = cons (cons (sym_link, cccdr), target->code);

        cur = cdr (cur);
    }
}

static void target_map_cross_link (struct tree_node *node, void *u)
{
    struct target *target = (struct target *) node_get_value (node);

    if (!eolp (target->use_objects))
    {
        sexpr cur = target->use_objects;

        while (consp (cur))
        {
            sexpr o = car (cur);
            struct tree_node *n1
                    = tree_get_node_string (&targets, (char *)sx_string (o));
            struct target *s;

            if (n1 == (struct tree_node *)0)
            {
                exit (68);
            }

            s = (struct target *) node_get_value (n1);

            do_cross_link (target, s);

            cur = cdr(cur);
        }
    }
}

void crosslink_objects ()
{
    tree_map (&targets, target_map_cross_link, (void *)0);
}
