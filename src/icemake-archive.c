/*
 * This file is part of the kyuba.org Curie project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008-2010, Kyuba Project Members
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

#include <icemake/icemake.h>

#include <curie/multiplex.h>
#include <curie/directory.h>
#include <curie/network.h>
#include <curie/memory.h>

#include <sievert/cpio.h>
#include <sievert/filesystem.h>

struct archive_metadata
{
    struct io *out;
    sexpr name;
    sexpr source;
    int length;
    struct cpio *cpio;
};

static int open_archive_files = 0;

static void on_read_cpio_archive_source_file (struct io *io, void *aux)
{
}

static void on_close_cpio_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    if (consp (ad->source))
    {
        sexpr ca = car (ad->source);
        struct io *i = io_open_read (sx_string (ca));

        cpio_next_file (ad->cpio, sx_string (ca), (struct metadata *)0, i);

        multiplex_add_io
            (i, on_read_cpio_archive_source_file,
             on_close_cpio_archive_source_file, ad);

        ad->source = cdr (ad->source);
    }
    else
    {
        cpio_close (ad->cpio);
    }
}

static void on_read_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    char s[] = "\\xxx";
    int i = 0;
    const char *b = io->buffer;

    for (i = io->position; i < io->length; i++)
    {
        unsigned char t = (unsigned char)b[i];

        switch (t)
        {
            case '"':  io_collect (ad->out, (char *)"\\\"", 2); break;
            case '\a': io_collect (ad->out, (char *)"\\a",  2); break;
            case '\b': io_collect (ad->out, (char *)"\\b",  2); break;
            case '\f': io_collect (ad->out, (char *)"\\f",  2); break;
            case '\\': io_collect (ad->out, (char *)"\\\\", 2); break;
            case '\n': io_collect (ad->out, (char *)"\\n",  2); break;
            case '\r': io_collect (ad->out, (char *)"\\r",  2); break;
            case '\t': io_collect (ad->out, (char *)"\\t",  2); break;
            case '\v': io_collect (ad->out, (char *)"\\v",  2); break;

            default:
                if ((t >= 0x20) && (t <= 0x7f))
                {
                    io_collect (ad->out, (char *)&t, 1);
                }
                else
                {
                    s[3] =  (t % 16);
                    s[2] = ((t - s[3]) / 16);

                    if (s[2] < 10) { s[2] = '0' + s[2]; }
                    else           { s[2] = 'a' + s[2] - 10; }
                    if (s[3] < 10) { s[3] = '0' + s[3]; }
                    else           { s[3] = 'a' + s[3] - 10; }

                    io_collect (ad->out, s, 4);
                }
                break;
        }
    }

    io_commit (ad->out);

    ad->length   += io->length - io->position;

    io->position  = io->length;
}

static void on_close_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    const char *s = sx_string (ad->name);
    char x[8] = "00000000";
    int j = 0;
    signed int i;
    char c;

    io_write (ad->out, "\";\nconst unsigned long ", 23);
    for (j = 0; s[j] != (char)0; j++);
    io_write (ad->out, s, j);
    io_write (ad->out, "_length=0x", 10);

    for (i = 8; (i >= 0) && (ad->length >= 0); i--)
    {
        c = ad->length % 16;
        if (c < 10)
        {
            x[i] = '0' + c;
        }
        else
        {
            x[i] = 'a' + (c - 10);
        }

        ad->length = (ad->length - c) / 16;
    }

    io_write (ad->out, x, 8);
    io_write (ad->out, ";\n", 2);
    io_close (ad->out);

    open_archive_files--;
    free_pool_mem (aux);
}

static void target_map_prepare_archives (struct tree_node *node, void *u)
{
    struct target *context = (struct target *)node_get_value(node);
    sexpr c = context->code, d, da, dd, name, source, out, df, ca;
    struct memory_pool pool =
        MEMORY_POOL_INITIALISER(sizeof(struct archive_metadata));
    struct archive_metadata *ad;
    struct io *o, *i, *co, *ci;
    const char *s;
    int j = 0;
    struct io *header;

    out = icemake_decorate_file (context, ft_header, fet_build_file, str_data);
    header = io_open_write (sx_string(out));
    io_collect (header, "/* this file was generated by icemake */\n\n", 42);

    context->headers = cons (cons(str_data, cons (out, sx_end_of_list)),
                             context->headers);

    while (consp (c))
    {
        d  = car (c);
        da = car (d);

        if (truep (equalp (da, sym_raw_c)) ||
            truep (equalp (da, sym_cpio_c)))
        {
            dd     = cdr (d);
            name   = car (dd);
            df     = sx_join (str_datas, name, sx_nil);

            s = sx_string (name);
            for (j = 0; s[j] != (char)0; j++);

            io_collect (header, "extern const char *", 19);
            io_collect (header, s, j);
            io_collect (header, ";\n", 2);

            io_collect (header, "extern const unsigned long int ", 31);
            io_collect (header, s, j);
            io_collect (header, "_length;\n", 9);

            if (truep (equalp (da, sym_raw_c)))
            {
                source = car(cdr (dd));

                out    = icemake_decorate_file
                             (context, ft_code_c, fet_build_file, df);
                open_archive_files++;

                i = io_open_read  (sx_string (source));
                o = io_open_write (sx_string (out));

                io_collect (o,      "const char *",        12);

                io_collect (o,      s, j);
                io_collect (o,      "=\"", 2);

                ad = (struct archive_metadata *)get_pool_mem (&pool);
                ad->out    = o;
                ad->name   = name;
                ad->source = sx_end_of_list;
                ad->length = 0;
                ad->cpio   = (struct cpio *)0;

                multiplex_add_io
                    (i, on_read_archive_source_file,
                     on_close_archive_source_file, ad);

                context->code = sx_set_remove (context->code, d);

                context->code = sx_set_add
                    (context->code,
                     cons (sym_c, cons (out, cons (icemake_decorate_file
                             (context, ft_object, fet_build_file, df),
                              sx_end_of_list))));
            }
            else
            {
                dd     = cdr (dd);
                source = sx_end_of_list;

                while (consp (dd))
                {
                    source = sx_set_merge
                        (source, read_directory_sx (car (dd)));

                    dd = cdr (dd);
                }

                source = path_normalise (source);

                context->code = sx_set_remove (context->code, d);

                if (consp (source))
                {
                    out    = icemake_decorate_file
                                 (context, ft_code_c, fet_build_file, df);
                    open_archive_files++;

                    o  = io_open_write (sx_string (out));

                    io_collect (o,      "const char *",        12);

                    io_collect (o,      s, j);
                    io_collect (o,      "=\"", 2);

                    net_open_loop (&ci, &co);

                    ad = (struct archive_metadata *)get_pool_mem (&pool);
                    ad->out    = o;
                    ad->name   = name;
                    ad->source = cdr(source);
                    ad->length = 0;
                    ad->cpio   = cpio_create_archive (co);

                    ca = car (source);

                    i = io_open_read (sx_string (ca));

                    cpio_next_file
                        (ad->cpio, sx_string (ca), (struct metadata *)0, i);

                    multiplex_add_io
                        (i, on_read_cpio_archive_source_file,
                         on_close_cpio_archive_source_file, ad);

                    multiplex_add_io
                        (ci, on_read_archive_source_file,
                         on_close_archive_source_file, ad);

                    context->code = sx_set_add
                        (context->code,
                         cons (sym_c, cons (out, cons (icemake_decorate_file
                                 (context, ft_object, fet_build_file, df),
                                  sx_end_of_list))));

                }

                context->code = sx_set_remove (context->code, d);
            }
        }

        c  = cdr (c);
    }

    io_close (header);
}

void icemake_prepare_archives (struct icemake *im)
{
    multiplex_cpio();

    tree_map (&(im->targets), target_map_prepare_archives, (void *)im);

    while (open_archive_files > 0)
    {
        multiplex();
    }
}

