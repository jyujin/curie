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

#if !defined(_WIN32)
#include <sys/utsname.h>
#endif

#include <icemake/icemake.h>

#include <curie/main.h>

#include <curie/memory.h>
#include <curie/multiplex.h>
#include <curie/exec.h>
#include <curie/signal.h>
#include <curie/filesystem.h>
#include <curie/stack.h>

#include <sievert/shell.h>
#include <sievert/tree.h>

#include <ctype.h>

#if !defined(NOVERSION)
#include <icemake/version.h>
#endif

char uname_os     [UNAMELENGTH]        = "generic";
char uname_arch   [UNAMELENGTH]        = "generic";
char uname_vendor [UNAMELENGTH]        = "unknown";

enum toolchain uname_toolchain;
enum fs_layout i_fsl                   = fs_afsl;
enum operating_system i_os             = os_unknown;
enum instruction_set i_is              = is_generic;

const char *archprefix                 = "generic-unknown-generic-generic";
char *tcversion                        = (char *)0;

static int alive_processes             = 0;
static unsigned int max_processes      = 1;

static int failures                    = 0;

sexpr co_freestanding                  = sx_false;

sexpr i_optimise_linking               = sx_false;
sexpr i_combine                        = sx_false;
sexpr i_static                         = sx_true;
sexpr i_destdir                        = sx_false;
sexpr i_pname                          = sx_false;
sexpr i_destlibdir                     = sx_false;
sexpr i_dynamic_libraries              = sx_true;
static sexpr in_dynamic_libraries      = sx_nil;

sexpr workstack                        = sx_end_of_list;

sexpr do_tests                         = sx_false;
sexpr do_install                       = sx_false;
sexpr do_build_documentation           = sx_false;

struct tree targets                    = TREE_INITIALISER;

sexpr p_c_compiler                     = sx_false;
sexpr p_cpp_compiler                   = sx_false;
sexpr p_resource_compiler              = sx_false;
sexpr p_assembler                      = sx_false;
sexpr p_linker                         = sx_false;
sexpr p_archiver                       = sx_false;
sexpr p_diff                           = sx_false;

sexpr p_latex                          = sx_false;
sexpr p_pdflatex                       = sx_false;
sexpr p_doxygen                        = sx_false;

sexpr architecture                     = sx_false;

static sexpr i_alternatives            = sx_end_of_list;

struct sexpr_io *stdio;

struct icemake_meta
{
    sexpr buildtargets;
};

void mkdir_pi (sexpr path)
{
    char buffer[BUFFERSIZE];
    const char *p = sx_string (path);
    int i;

    for (i = 0; (p[i] != 0) && (i < (BUFFERSIZE - 2)); i++)
    {
        if (((p[i] == '/') || (p[i] == '\\')) && (i > 0))
        {
            buffer[i] = 0;

            if (falsep (filep (make_string (buffer))))
            {
                mkdir (buffer, 0755);
            }
        }

        buffer[i] = p[i];
    }
}

void mkdir_p (sexpr path)
{
    mkdir_pi (path);
    mkdir (sx_string (path), 0755);
}

static const char *c_getenv (const char *var)
{
    const char **c = (const char **)curie_environment, *d;
    int i;

    while ((d = (*c)) != (const char *)0)
    {
        d = (*c);

        for (i = 0; d[i] == var[i]; i++);

        if ((d[i] == '=') && (var[i] == 0))
        {
            return d + i + 1;
        }

        c++;
    }

    return (const char *)0;
}

sexpr prepend_flags_from_environment (sexpr x, const char *var)
{
    const char *f = c_getenv (var);

    if (f != (char *)0)
    {
        char buffer[BUFFERSIZE];
        int j = 0, i;
        sexpr t = sx_end_of_list;

        for (i = 0; f[i] != 0; i++)
        {
            if (f[i] == ' ')
            {
                buffer[j] = 0;

                t = cons (make_string (buffer), t);

                j = 0;
            }
            else
            {
                buffer[j] = f[i];
                j++;
            }
        }

        if (j != 0)
        {
            buffer[j] = 0;

            t = cons (make_string (buffer), t);
        }

        while (consp (t)) { x = cons (car(t), x); t = cdr (t); }
    }

    return x;
}

char *mangle_path_borland (char *b)
{
    int i;

    for (i = 0; b[i]; i++)
    {
        if (b[i] == '+')
        {
            b[i] = 'x';
        }
    }

    return b;
}

sexpr mangle_path_borland_sx (sexpr b)
{
    char buffer[BUFFERSIZE];
    int i;
    const char *bx = sx_string (b);

    for (i = 0; (i < (BUFFERSIZE - 2)) && bx[i]; i++)
    {
        buffer[i] = bx[i];
    }

    buffer[i] = 0;

    return make_string (mangle_path_borland (buffer));
}

sexpr lowercase (sexpr s)
{
    char buffer[BUFFERSIZE], c;
    int i;
    const char *sx = sx_string (s);

    for (i = 0; (i < (BUFFERSIZE - 2)) && sx[i]; i++)
    {
        c = sx[i];

        buffer[i] = ((c >= 'A') && (c <= 'Z'))
                  ? ('a' + (c - 'A'))
                  : c;
    }

    buffer[i] = 0;

    return make_string (buffer);
}

sexpr sx_string_dir_prefix_c (const char *f, sexpr p)
{
    if (i_os == os_windows)
    {
        return sx_join (p, str_backslash, make_string (f));
    }

    return sx_join (p, str_slash, make_string (f));
}

static sexpr sx_string_dir_prefix (sexpr f, sexpr p)
{
    return sx_string_dir_prefix_c (sx_string(f), p);
}

static sexpr find_actual_file (sexpr p, sexpr file)
{
    sexpr r = sx_string_dir_prefix (file, p);

    return (truep(filep (r)) ? r : sx_false);
}

static sexpr find_in_permutations_vendor (sexpr p, sexpr file)
{
    sexpr r;

    if ((r = find_actual_file (sx_string_dir_prefix_c (uname_vendor, p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_actual_file (p, file)), stringp(r))
    {
        return r;
    }

    return sx_false;
}

static sexpr find_in_permutations_toolchain (sexpr p, sexpr file)
{
    sexpr r;

    switch (uname_toolchain)
    {
        case tc_gcc:
            if ((r = find_in_permutations_vendor (sx_string_dir_prefix_c ("gnu", p), file)), stringp(r))
            {
                return r;
            }
            break;
        case tc_borland:
            if ((r = find_in_permutations_vendor (sx_string_dir_prefix_c ("borland", p), file)), stringp(r))
            {
                return r;
            }
            break;
        case tc_msvc:
            if ((r = find_in_permutations_vendor (sx_string_dir_prefix_c ("msvc", p), file)), stringp(r))
            {
                return r;
            }
            break;
    }

    if ((r = find_in_permutations_vendor (p, file)), stringp(r))
    {
        return r;
    }

    return sx_false;
}

static sexpr find_in_permutations_arch (sexpr p, sexpr file)
{
    sexpr r;

    if ((r = find_in_permutations_toolchain (sx_string_dir_prefix_c (uname_arch, p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_toolchain (p, file)), stringp(r))
    {
        return r;
    }

    return sx_false;
}

static sexpr find_in_permutations_os (sexpr p, sexpr file)
{
    sexpr r;

    if ((r = find_in_permutations_arch (sx_string_dir_prefix_c (uname_os, p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_arch (sx_string_dir_prefix_c ("posix", p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_arch (sx_string_dir_prefix_c ("ansi", p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_arch (sx_string_dir_prefix_c ("generic", p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_arch (p, file)), stringp(r))
    {
        return r;
    }

    return sx_false;
}

static sexpr find_in_permutations (sexpr p, sexpr file)
{
    sexpr r;

    if ((r = find_in_permutations_os (sx_string_dir_prefix_c ("internal", p), file)), stringp(r))
    {
        return r;
    }
    else if ((r = find_in_permutations_os (p, file)), stringp(r))
    {
        return r;
    }

    return sx_false;
}

static sexpr find_code_with_suffix (sexpr file, char *s)
{
    return find_in_permutations
        (str_src,
         sx_join (file, make_string (s), sx_nil));
}

static sexpr find_test_case_with_suffix (sexpr file, char *s)
{
    return find_in_permutations
        (str_tests,
         sx_join (file, make_string (s), sx_nil));
}

static sexpr find_code_sx (sexpr file)
{
    return find_code_with_suffix (file, ".sx");
}

static sexpr find_code_c (sexpr file)
{
    return find_code_with_suffix (file, ".c");
}

static sexpr find_code_cpp (sexpr file)
{
    return find_code_with_suffix (file, ".c++");
}

static sexpr find_code_s (sexpr file)
{
    return find_code_with_suffix (file, ".s");
}

static sexpr find_code_pic_s (sexpr file)
{
    sexpr r = find_code_with_suffix (file, ".pic.s");

    return (falsep(r) ? find_code_s (file) : r);
}

static sexpr find_code_S (sexpr file)
{
    return find_code_with_suffix (file, ".S");
}

static sexpr find_code_pic_S (sexpr file)
{
    sexpr r = find_code_with_suffix (file, ".pic.S");

    return (falsep(r) ? find_code_S (file) : r);
}

static sexpr find_code_def (sexpr file)
{
    return find_code_with_suffix (file, ".def");
}

static sexpr find_code_rc (sexpr file)
{
    return find_code_with_suffix (file, ".rc");
}

static sexpr find_test_case_c (sexpr file)
{
    return find_test_case_with_suffix (file, ".c");
}

static sexpr find_test_case_cpp (sexpr file)
{
    return find_test_case_with_suffix (file, ".c++");
}

static sexpr find_header_with_suffix (sexpr name, sexpr file, char *s)
{
    return find_in_permutations
        (str_include,
         sx_join (name, str_slash, sx_join (file, make_string (s), sx_nil)));
}

static sexpr find_header_h (sexpr name, sexpr file)
{
    return find_header_with_suffix (name, file, ".h");
}

sexpr get_build_file (struct target *t, sexpr file)
{
    switch (uname_toolchain)
    {
        case tc_msvc:
            return sx_join (t->buildbase, architecture,
                   sx_join (str_backslash, t->name,
                     (nilp (file) ? sx_nil
                                  : sx_join (str_backslash, file, sx_nil))));
        case tc_borland:
            return mangle_path_borland_sx
                     (sx_join (t->buildbase, architecture,
                      sx_join (str_backslash, t->name,
                      (nilp (file) ? sx_nil
                                   : sx_join (str_backslash, file, sx_nil)))));
        case tc_gcc:
            return sx_join (t->buildbase, architecture,
                   sx_join (str_slash, t->name,
                   sx_join (str_slash, file, sx_nil)));
    }

    return sx_false;
}

sexpr get_install_file (struct target *t, sexpr file)
{
    switch (i_fsl)
    {
        case fs_fhs:
        case fs_fhs_binlib:
            switch (uname_toolchain)
            {
                case tc_msvc:
                case tc_borland:
                    return sx_join (i_destdir, str_backslash, file);
                case tc_gcc:
                    return sx_join (i_destdir, str_slash, file);
            }
        case fs_afsl:
            switch (uname_toolchain)
            {
                case tc_msvc:
                case tc_borland:
                    return sx_join (i_destdir, str_backslash,
                             sx_join (make_string (uname_os), str_backslash,
                               sx_join (make_string (uname_arch), str_backslash,
                                        file)));
                case tc_gcc:
                    return sx_join (i_destdir, str_slash,
                             sx_join (make_string (uname_os), str_slash,
                               sx_join (make_string (uname_arch), str_slash,
                                        file)));
            }
    }

    return sx_false;
}

static sexpr generate_file_name_with_suffix
    (sexpr file, struct target *t, sexpr suffix)
{
    return get_build_file (t, sx_join (file, suffix, sx_nil));
}

static sexpr generate_object_file_name (sexpr file, struct target *t)
{
    return generate_file_name_with_suffix
        (file, t, ((uname_toolchain == tc_gcc) ? str_dot_o : str_dot_obj));
}

static sexpr generate_resource_file_name (struct target *t)
{
    return generate_file_name_with_suffix (t->name, t, str_dot_res);
}

static sexpr generate_pic_object_file_name (sexpr file, struct target *t)
{
    return generate_file_name_with_suffix (file, t, str_dot_pic_o);
}

static sexpr generate_test_object_file_name (sexpr file, struct target *t)
{
    if (uname_toolchain == tc_gcc)
    {
        return generate_file_name_with_suffix
            (sx_join (str_test_dash, file, sx_nil), t, str_dot_o);
    }

    return generate_file_name_with_suffix
        (sx_join (str_test_dash, file, sx_nil), t, str_dot_obj);
}

static sexpr generate_test_executable_file_name (sexpr file, struct target *t)
{
    return generate_file_name_with_suffix
        (sx_join (str_test_dash, file, sx_nil), t,
                  ((i_os == os_windows ? str_dot_exe : sx_nil)));
}

static sexpr find_code_highlevel (struct target *context, sexpr file)
{
    sexpr r, subfile = sx_join (file, str_dhighlevel, sx_nil);

    if (((r = find_code_cpp (subfile)), stringp(r)))
    {
        context->options |= ICEMAKE_HAVE_CPP;

        return cons(sym_cpp, cons (r,
                 cons (generate_object_file_name (subfile, context),
                   sx_end_of_list)));

    }
    else if (((r = find_code_c (subfile)), stringp(r)))
    {
        return cons(sym_c, cons (r,
                 cons (generate_object_file_name (subfile, context),
                   sx_end_of_list)));
    }

    return sx_false;
}

static sexpr find_code_highlevel_pic (struct target *context, sexpr file)
{
    sexpr r, subfile = sx_join (file, str_dhighlevel, sx_nil);

    if (((r = find_code_cpp (subfile)), stringp(r)))
    {
        context->options |= ICEMAKE_HAVE_CPP;

        return cons(sym_cpp_pic, cons (r,
                 cons (generate_pic_object_file_name (subfile, context),
                   sx_end_of_list)));
    }
    else if (((r = find_code_c (subfile)), stringp(r)))
    {
        return cons(sym_c_pic, cons (r,
                 cons (generate_pic_object_file_name (subfile, context),
                   sx_end_of_list)));
    }

    return sx_false;
}

static void find_code (struct target *context, sexpr file)
{
    sexpr r;
    sexpr primus   = sx_false;
    sexpr secundus = sx_false;
    sexpr tertius  = sx_false;
    sexpr quartus  = sx_false;

    if (consp (file))
    {
        sexpr ch = car (file), d = i_alternatives;

        while (consp (d))
        {
            sexpr c = car (d);

            if (truep (equalp (car (c), ch)))
            {
                sexpr co = cdr (file), e = cdr (c);

                while (consp (co))
                {
                    if (truep (equalp (e, car (co))))
                    {
                        file = e;
                        break;
                    }

                    co = cdr (co);
                }

                if (consp (file))
                {
                    on_error (ie_invalid_choice, sx_string (file));
                }

                break;
            }

            d = cdr (d);
        }

        if (consp (file))
        {
            file = car (cdr (file));
        }
    }

    if (truep (co_freestanding))
    {
        if ((r = find_code_S (file)), stringp(r))
        {
            primus =
                cons(sym_preproc_assembly, cons (r,
                  cons (generate_object_file_name (file, context),
                    sx_end_of_list)));

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                secundus =
                    cons(sym_preproc_assembly_pic,
                      cons (find_code_pic_S (file),
                        cons (generate_pic_object_file_name
                                (file, context), sx_end_of_list)));
            }

            tertius = find_code_highlevel (context, file);

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                quartus = find_code_highlevel_pic (context, file);
            }
        }
        else if ((r = find_code_s (file)), stringp(r))
        {
            primus =
                cons(sym_assembly, cons (r,
                  cons (generate_object_file_name (file, context),
                    sx_end_of_list)));

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                secundus =
                    cons(sym_assembly_pic,
                      cons (find_code_pic_s (file),
                        cons (generate_pic_object_file_name
                                (file, context), sx_end_of_list)));
            }

            tertius = find_code_highlevel (context, file);

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                quartus = find_code_highlevel_pic (context, file);
            }
        }
    }

    if (falsep(primus))
    {
        if (((r = find_code_cpp (file)), stringp(r)))
        {
            context->options |= ICEMAKE_HAVE_CPP;

            primus = cons(sym_cpp, cons (r,
                       cons (generate_object_file_name (file, context),
                         sx_end_of_list)));

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                secundus = cons(sym_cpp_pic, cons (r,
                             cons (generate_pic_object_file_name
                                     (file, context), sx_end_of_list)));
            }
        }
        else if (((r = find_code_c (file)), stringp(r)))
        {
            primus = cons(sym_c, cons (r,
                       cons (generate_object_file_name (file, context),
                         sx_end_of_list)));

            if (truep(i_dynamic_libraries) && (uname_toolchain == tc_gcc) &&
                !(context->options & ICEMAKE_NO_SHARED_LIBRARY))
            {
                secundus = cons (sym_c_pic, cons (r,
                             cons (generate_pic_object_file_name
                                     (file, context), sx_end_of_list)));
            }
        }
        else
        {
            on_error (ie_missing_code_file, sx_string (file));
        }
    }

    context->code = cons (primus, context->code);
    if (!falsep(secundus))
    {
        context->code = cons (secundus, context->code);
    }
    if (!falsep(tertius))
    {
        context->code = cons (tertius, context->code);
    }
    if (!falsep(quartus))
    {
        context->code = cons (quartus, context->code);
    }
}

static void find_test_case (struct target *context, sexpr file)
{
    sexpr r;
    sexpr primus   = sx_false;

    if (((r = find_test_case_cpp (file)), stringp(r)))
    {
        primus = cons(sym_cpp, cons (r,
                   cons (generate_test_object_file_name (file, context),
                     cons (generate_test_executable_file_name (file, context),
                       sx_end_of_list))));
    }
    else if (((r = find_test_case_c (file)), stringp(r)))
    {
        primus = cons(sym_c, cons (r,
                   cons (generate_test_object_file_name (file, context),
                     cons (generate_test_executable_file_name (file, context),
                       sx_end_of_list))));
    }
    else
    {
        on_error (ie_missing_test_case, sx_string (file));
    }

    context->test_cases = cons (primus, context->test_cases);
}

static void find_header (struct target *context, sexpr file)
{
    sexpr r;

    if ((r = find_header_h (context->name, file)), stringp(r))
    {
        context->headers = cons (cons(file, cons (r, sx_end_of_list)), context->headers);
    }
    else
    {
        on_error (ie_missing_header, sx_string (file));
    }
}

static sexpr find_documentation_with_suffix (sexpr file, char *s)
{
    return find_in_permutations
        (str_documentation, sx_join (file, make_string (s), sx_nil));
}

static sexpr find_documentation_tex (sexpr file)
{
    return find_documentation_with_suffix (file, ".tex");
}

static void find_documentation (struct target *context, sexpr file)
{
    define_symbol (sym_man1, "man1");
    define_symbol (sym_man2, "man2");
    define_symbol (sym_man3, "man3");
    define_symbol (sym_man4, "man4");
    define_symbol (sym_man5, "man5");
    define_symbol (sym_man6, "man6");
    define_symbol (sym_man7, "man7");
    define_symbol (sym_man8, "man8");

    sexpr r;

    if ((r = find_documentation_tex (file)), stringp(r))
    {
        context->documentation = cons(cons(sym_tex, cons (file, r)), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".1")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man1, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".2")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man2, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".3")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man3, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".4")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man4, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".5")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man5, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".6")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man6, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".7")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man7, sx_end_of_list)))), context->documentation);
    }
    else if ((r = find_documentation_with_suffix (file, ".8")), stringp(r))
    {
        context->documentation = cons(cons(sym_man, cons (file, cons(r, cons (sym_man8, sx_end_of_list)))), context->documentation);
    }
    else
    {
        on_error (ie_missing_documentation, sx_string (file));
    }
}

static sexpr find_data (struct target *context, sexpr file)
{
    sexpr r;

    if ((r = find_in_permutations (str_data, file)), !stringp(r))
    {
        on_error (ie_missing_data, sx_string (file));
    }

    return r;
}

static struct target *get_context()
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER (sizeof(struct target));
    struct target *context = get_pool_mem (&pool);

    context->libraries        = sx_end_of_list;
    context->deffile          = sx_false;
    context->olibraries       = sx_end_of_list;
    context->code             = sx_end_of_list;
    context->test_cases       = sx_end_of_list;
    context->test_reference   = sx_end_of_list;
    context->headers          = sx_end_of_list;
    context->data             = sx_end_of_list;
    context->dname            = sx_false;
    context->description      = sx_false;
    context->dversion         = sx_false;
    context->durl             = sx_false;
    context->documentation    = sx_end_of_list;
    context->buildnumber      = make_integer (0);
    context->options          = 0;

    if (i_os == os_windows)
    {
        context->buildbase    = str_build_backslash;
    }
    else
    {
        context->buildbase    = str_build_slash;
    }

    return context;
}

static void process_definition (struct target *context, sexpr definition)
{
    sexpr sx;

    while (consp(definition))
    {
        sexpr sxcar = car (definition);
        sexpr sxcaar = car (sxcar);

        if (truep(equalp(sxcar, sym_no_shared_library)))
        {
            context->options |= ICEMAKE_NO_SHARED_LIBRARY;
        }
        else if (truep(equalp(sxcar, sym_hosted)))
        {
            context->options |= ICEMAKE_HOSTED;
        }
        else if (truep(equalp(sxcaar, sym_name)))
        {
            context->dname = car(cdr(sxcar));
        }
        else if (truep(equalp(sxcaar, sym_description)))
        {
            context->description = car(cdr(sxcar));
        }
        else if (truep(equalp(sxcaar, sym_version)))
        {
            context->dversion = car(cdr(sxcar));
        }
        else if (truep(equalp(sxcaar, sym_url)))
        {
            context->durl = car(cdr(sxcar));
        }
        else if (truep(equalp(sxcar, sym_libcurie)))
        {
            context->options |= ICEMAKE_USE_CURIE;
        }
        else if (truep(equalp(sxcaar, sym_code)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                find_code (context, car (sxc));
                
                sxc = cdr (sxc);
            }

            if ((i_os == os_windows) && (uname_toolchain == tc_msvc))
            {
                sexpr r = find_code_rc (context->name);

                if (stringp (r))
                {
                    context->code =
                        cons (cons(sym_resource, cons (r,
                                cons (generate_resource_file_name (context),
                                      sx_end_of_list))),
                              context->code);
                } 
            }
        }
        else if (truep(equalp(sxcaar, sym_libraries)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                context->libraries = cons (car (sxc), context->libraries);
                context->olibraries = cons (car (sxc), context->olibraries);

                sxc = cdr (sxc);
            }
        }
        else if (truep(equalp(sxcaar, sym_test_cases)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                find_test_case (context, car (sxc));

                sxc = cdr (sxc);
            }
        }
        else if (truep(equalp(sxcaar, sym_headers)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                find_header (context, car (sxc));

                sxc = cdr (sxc);
            }
        }
        else if (truep(equalp(sxcaar, sym_documentation)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                find_documentation (context, car (sxc));

                sxc = cdr (sxc);
            }
        }
        else if (truep(equalp(sxcaar, sym_data)))
        {
            sexpr name = cdr (sxcar);

            if (consp (name))
            {
                sexpr d = sx_end_of_list;
                sexpr sxc = cdr (name);
                name = car (name);

                while (consp (sxc))
                {
                    sexpr filename = car (sxc);
                    d = cons (cons (filename, find_data (context, filename)),d);

                    sxc = cdr (sxc);
                }

                d = cons (name, d);

                context->data = cons (d, context->data);
            }
        }
        else if (truep(equalp(sxcaar, sym_test_case_reference)))
        {
            sexpr sxc = cdr (sxcar);

            while (consp (sxc))
            {
                context->test_reference
                        = cons (car(sxc), context->test_reference);

                sxc = cdr (sxc);
            }
        }

        definition = cdr (definition);
    }

    if ((sx = find_code_sx(context->name)), stringp(sx))
    {
        const char *s = sx_string (sx);
        struct sexpr_io *in = sx_open_i (io_open_read(s));
        sexpr r;

        while (!eofp ((r = sx_read (in))))
        {
            if (truep (equalp (r, sym_freestanding)))
            {
                co_freestanding = sx_true;
            }
            else if (truep (equalp (r, sym_hosted)))
            {
                co_freestanding = sx_false;
                    
                if (i_os != os_windows)
                {
                    i_dynamic_libraries = sx_false;
                }
            }
        }

        sx_close_io (in);

        if (nilp (in_dynamic_libraries) && truep(co_freestanding))
        {
            i_dynamic_libraries = sx_true;
        }
    }

    mkdir_p (get_build_file (context, sx_nil));

    mkdir_p (sx_join (context->buildbase, architecture, str_sinclude));

    mkdir_p (sx_join (context->buildbase, architecture,
                      sx_join (str_sincludes, context->name, sx_nil)));

    context->headers =
        cons (cons (str_version,
                    cons (sx_join (context->buildbase, architecture,
                                   sx_join (str_sincludes, context->name,
                                            str_sversiondh)),
                          sx_end_of_list)),
              context->headers);

    if (uname_toolchain == tc_gcc)
    {
        if ((context->options & ICEMAKE_HAVE_CPP) &&
             (context->options & ICEMAKE_HOSTED))
        {
            context->libraries = cons (str_supcpp, context->libraries);
            if (i_os == os_darwin)
            {
                context->libraries = cons (str_gcc_eh, context->libraries);
            }
        }
        else
        {
            context->libraries = cons (str_gcc, context->libraries);
        }

        if ((context->options & ICEMAKE_HOSTED) &&
            !(context->options & ICEMAKE_HAVE_CPP))
        {
            switch (i_os)
            {
                case os_windows:
                    context->libraries = cons (str_kernel32, context->libraries);
                    context->libraries = cons (str_mingw32, context->libraries);
                    context->libraries = cons (str_coldname, context->libraries);
                    context->libraries = cons (str_mingwex, context->libraries);
                    context->libraries = cons (str_msvcrt, context->libraries);
                    break;
                default:
                    context->libraries = cons (str_lc, context->libraries);
            }
        }
    }

    if (context->options & ICEMAKE_USE_CURIE)
    {
        if (!(context->options & ICEMAKE_HOSTED))
        {
            context->libraries = cons (str_curie_bootstrap, context->libraries);
        }

        context->libraries = cons (str_curie, context->libraries);

        if (falsep(equalp(str_curie, context->name)))
        {
            context->olibraries = cons (str_curie, context->olibraries);
        }
    }
}

static struct target *create_library (sexpr definition)
{
    struct target *context = get_context();

    context->name = car(definition);

    context->options |= ICEMAKE_LIBRARY;

    process_definition (context, cdr(definition));

    if (falsep(equalp(str_curie, context->name)))
    {
        context->libraries = cons (context->name, context->libraries);
    }
    context->deffile = find_code_def (context->name);

    return context;
}

static struct target *create_programme (sexpr definition)
{
    struct target *context = get_context();

    context->name = car(definition);

    context->options |= ICEMAKE_PROGRAMME;

    process_definition (context, cdr(definition));

    return context;
}

static struct target *create_documentation (sexpr definition)
{
    struct target *context = get_context();

    context->name = car(definition);

    process_definition (context, cdr(definition));

    return context;
}

static void combine_code (struct target *context, sexpr type, sexpr object_file)
{
    sexpr ccur = sx_end_of_list;
    sexpr cur = context->code;

    sexpr clist = sx_end_of_list;

    do {
        sexpr sxcar = car (cur);
        sexpr sxcaar = car (sxcar);
        sexpr sxcadar = car (cdr (sxcar));

        if (truep(equalp(sxcaar, type)))
        {
            clist = cons (sxcadar, clist);
        }
        else
        {
            ccur = cons (sxcar, ccur);
        }

        cur = cdr (cur);
    } while (consp(cur));

    if (consp (clist))
    {
        ccur = cons (cons (type, cons (clist, cons (object_file,
                     sx_end_of_list))), ccur);
    }

    context->code = ccur;
}

static void target_map_merge (struct tree_node *node, void *u)
{
    struct target *context = (struct target *)node_get_value(node), *tcontext;
    sexpr cur, ca, nc, ttype;
    const char *target;

    if (context->options & ICEMAKE_PROGRAMME)
        /* dont merge objects for libraries */
    {
        for (cur = context->libraries; consp (cur); cur = cdr (cur))
        {
            ca = car (cur);
            target = sx_string (ca);

            if ((node = tree_get_node_string(&targets, (char *)target))
                != (struct tree_node *)0)
            {
                tcontext = (struct target *)node_get_value (node);
                context->code = sx_set_merge (context->code, tcontext->code);
                context->libraries = sx_set_remove (context->libraries, ca);
            }
        }
    }

    if (context->options & ICEMAKE_PROGRAMME)
        /* dont need PIC objects for programmes */
    {
        nc = sx_end_of_list;
        for (cur = context->code; consp(cur); cur = cdr (cur))
        {
            ca = car (cur);
            ttype = car (ca);

            if (falsep(equalp(ttype, sym_assembly_pic)) &&
                falsep(equalp(ttype, sym_preproc_assembly_pic)) &&
                falsep(equalp(ttype, sym_c_pic)) &&
                falsep(equalp(ttype, sym_cpp_pic)))
            {
                nc = sx_set_add (nc, ca);
            }
        }

        context->code = nc;
    }
}

static void target_map_combine (struct tree_node *node, void *u)
{
    struct target *context = (struct target *)node_get_value(node);

    if ((uname_toolchain == tc_gcc) && truep(i_combine) && consp(context->code))
    {
        combine_code (context, sym_c,
                      generate_object_file_name
                        (str_combined_c_source, context));

        combine_code (context, sym_c_pic,
                      generate_pic_object_file_name
                        (str_combined_c_source, context));
    }
}

static void merge_contexts ( void )
{
    tree_map (&targets, target_map_merge, (void *)0);
    tree_map (&targets, target_map_combine, (void *)0);
}

static void print_help ()
{
    sx_write (stdio, make_symbol (
#if !defined(NOVERSION)
        icemake_version_long "\n\n"
#endif
        "Usage: icemake [options] [targets]\n"
        "\n"
        "Options:\n"
        " -h           Print help (this text) and exit.\n"
        " -v           Print version and exit.\n\n"
        " -t <chost>   Specify target CHOST\n"
        " -z <version> Specify target toolchain version\n"
        " -d <destdir> Specify the directory to install to.\n"
        " -i           Install resulting binaries\n"
        " -r           Execute runtime tests\n"
        " -f           Use the FHS layout for installation\n"
        " -b <pname>   Use a modified FHS layout (lib/<pname>/bin/ vs. bin/)\n"
        " -l <libdir>  Use <libdir> instead of 'lib' when installing\n"
        " -s           Use the default FS layout for installation\n"
        " -L           Optimise linking.\n"
        " -c           Use gcc's -combine option for C source files.\n"
        " -o           Don't link dynamic libraries.\n"
        " -O           Do link dynamic libraries.\n"
        " -S           Enforce a static link (default).\n"
        " -R           Enforce a dynamic link.\n"
        " -j <num>     Spawn <num> processes simultaneously.\n"
        " -a <1> <2>   Use implementation <2> for code part <1>.\n"
        " -x           Build documentation (if possible).\n"
        "\n"
        "The [targets] specify a list of things to build, according to the\n"
        "icemake.sx file located in the current working directory.\n"
        "\n"));
}

static void print_version ()
{
    sx_write (stdio, make_symbol (
#if defined(NOVERSION)
        "icemake (version info not available)"
#else
        icemake_version_long "\n"
#endif
            ));
}
 
static void write_uname_element (char *source, char *target, int tlen)
{
    int  i = 0;
    char c = 0;

    while (source[i] != 0)
    {
        c = source[i];

        if ((c >= '0') && (c <= '9'))
        {
            target[i] = c;
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            target[i] = c;
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            target[i] = (c - 'A') + 'a';
        }
        else
        {
            target[i] = '-';
        }

        i++;

        if ((i >= (UNAMELENGTH - 1)) || (i >= tlen))
        {
            target[i] = 0;
            return;
        }
    }

    target[i] = 0;
}

static sexpr xwhich (char *programme)
{
    sexpr w, p = make_string (programme);

    if ((tcversion != (char *)0) &&
          ((w = which (sx_join (architecture, str_dash,
                         sx_join (p, str_dash, make_string (tcversion))))),
           stringp(w)))
    {
        return w;
    }
    else if ((w = which (sx_join (architecture, str_dash, p))), stringp(w))
    {
        return w;
    }
    else if ((w = which (make_string (programme))), stringp(w))
    {
        return w;
    }

    return sx_false;
}

static void initialise_toolchain_gcc()
{
    p_c_compiler = xwhich ("gcc");
    if (falsep(p_c_compiler)) { p_c_compiler = xwhich ("cc"); }
    if (falsep(p_c_compiler))
    {
        on_error (ie_missing_tool, "gcc");
    }

    p_linker = p_c_compiler;
/*    p_assembler = p_c_compiler;*/

    p_cpp_compiler = xwhich ("g++");
    if (falsep(p_cpp_compiler))
    {
        on_error (ie_missing_tool, "g++");
    }

    p_assembler = xwhich ("as");
    if (falsep(p_assembler))
    {
        on_error (ie_missing_tool, "as");
    }

    p_archiver = xwhich ("ar");
    if (falsep(p_archiver))
    {
        on_error (ie_missing_tool, "ar");
    }

    p_diff = xwhich ("diff");
    if (falsep(p_diff))
    {
        on_warning (ie_missing_tool, "diff");
    }
}

static void initialise_toolchain_borland()
{
    p_c_compiler = xwhich ("bcc32");
    if (falsep(p_c_compiler))
    {
        on_error (ie_missing_tool, "bcc32");
    }

    p_linker = p_c_compiler;
    p_assembler = p_c_compiler;
    p_cpp_compiler = p_c_compiler;

    p_archiver = xwhich ("tlib");
    if (falsep(p_archiver))
    {
        on_error (ie_missing_tool, "tlib");
    }

    p_diff = xwhich ("diff");
    if (falsep(p_diff))
    {
        on_warning (ie_missing_tool, "diff");
    }
}

static void initialise_toolchain_msvc()
{
    p_c_compiler = xwhich ("cl");
    if (falsep(p_c_compiler))
    {
        on_error (ie_missing_tool, "cl");
    }

    p_resource_compiler = xwhich ("rc");
    if (falsep(p_resource_compiler))
    {
        on_error (ie_missing_tool, "rc");
    }

    p_linker = xwhich ("link");
    if (falsep(p_linker))
    {
        on_error (ie_missing_tool, "link");
    }

    p_assembler = p_c_compiler;
    p_cpp_compiler = p_c_compiler;

    p_archiver = xwhich ("lib");
    if (falsep(p_archiver))
    {
        on_error (ie_missing_tool, "lib");
    }

    p_diff = xwhich ("diff");
    if (falsep(p_diff))
    {
        on_warning (ie_missing_tool, "diff");
    }
}

static void initialise_toolchain_tex()
{
    p_latex = xwhich ("latex");
    if (falsep(p_latex))
    {
        on_warning (ie_missing_tool, "latex");
    }

    p_pdflatex = xwhich ("pdflatex");
    if (falsep(p_pdflatex))
    {
        on_warning (ie_missing_tool, "pdflatex");
    }
}

static void initialise_toolchain_doxygen()
{
    p_doxygen = xwhich ("doxygen");
    if (falsep(p_doxygen))
    {
        on_warning (ie_missing_tool, "doxygen");
    }
}

static void spawn_stack_items(void (*)(struct exec_context *, void *));

static void process_on_death(struct exec_context *context, void *p)
{
    if (context->status == ps_terminated)
    {
        sexpr sx = (sexpr)p;
        alive_processes--;

        if (context->exitstatus != 0)
        {
            sx_write (stdio, cons (sym_failed,
                                   cons (make_integer (context->exitstatus),
                                         cons (sx,
                                             sx_end_of_list))));

            on_error (ie_programme_failed, "");
        }
        else
        {
            sx_write (stdio, cons (sym_completed, cons (sx, sx_end_of_list)));
        }

        free_exec_context (context);
    }
}

static void process_on_death_nokill(struct exec_context *context, void *p)
{
    if (context->status == ps_terminated)
    {
        sexpr sx = (sexpr)p;
        alive_processes--;

        if (context->exitstatus != 0)
        {
            sx_write (stdio, cons (sym_failed,
                      cons (make_integer (context->exitstatus),
                            cons (sx,
                                  sx_end_of_list))));

            failures++;
        }
        else
        {
            sx_write (stdio, cons (sym_completed, cons (sx, sx_end_of_list)));
        }

        free_exec_context (context);
    }
}

static void map_environ (struct tree_node *node, void *psx)
{
    sexpr *sx = (sexpr *)psx;
    struct target *t = node_get_value (node);

    if (falsep (*sx))
    {
        switch (i_os)
        {
            case os_darwin:
                *sx = sx_join (str_DYLIB_LIBRARY_PATHe,
                               get_build_file (t, sx_nil), sx_nil);
                break;
            default:
                *sx = sx_join (str_LD_LIBRARY_PATHe,
                               get_build_file (t, sx_nil), sx_nil);
        }
    }
    else
    {
        *sx = sx_join (*sx, str_colon, get_build_file (t, sx_nil));
    }
}

static void spawn_item (sexpr sx, void (*f)(struct exec_context *, void *))
{
    sexpr cur = sx, cf = car (sx);
    struct exec_context *context;
    const char *wdir = (const char *)0;
    char odir[BUFFERSIZE];
    int c = 0, exsize;
    char **ex;
    sexpr rsx = sx_false;

    tree_map (&targets, map_environ, (void *)&rsx);

    sx_write (stdio, cons (sym_execute, sx));

    if (truep(equalp(cf, sym_chdir)))
    {
        sexpr cfcdr = cdr (cur);
        sexpr cfcdar = car (cfcdr);

        wdir = sx_string (cfcdar);
        if ((char *)getcwd (odir, BUFFERSIZE) == (char *)0)
        {
            return;
        }

        sx = cdr (cfcdr);
    }

    for (c = 0; consp (cur); cur = cdr (cur))
    {
        c++;
    }
    c++;

    exsize = sizeof(char*)*c;
    ex = aalloc (exsize);

    for (c = 0, cur = sx; consp (cur); cur = cdr (cur))
    {
        sexpr sxcar = car (cur);
        ex[c] = (char *)sx_string (sxcar);
        c++;
    }
    ex[c] = (char *)0;

    if (wdir != (const char *)0)
    {
        chdir (wdir);
    }

    if ((i_os == os_windows) || falsep (rsx))
        /* dynamic linker overrides are useless on win32*/
    {
        context = execute (EXEC_CALL_NO_IO | EXEC_CALL_PURGE, ex,
                           curie_environment);
    }
    else
    {
        char *tenv[3];
        sexpr s = sx_join (str_PATHe, make_string (c_getenv ("PATH")), sx_nil);
            
        tenv[0] = (char *)sx_string (rsx);
        tenv[1] = (char *)sx_string (s);;
        tenv[2] = (char *)0;

        context = execute (EXEC_CALL_NO_IO | EXEC_CALL_PURGE, ex, tenv);
    }
    if (wdir != (const char *)0)
    {
        chdir (odir);
    }

    sx = car (sx);

    switch (context->pid)
    {
        case -1: on_error (ie_failed_to_spawn_subprocess, "");
                 break;
        case 0:  on_error (ie_failed_to_execute_binary_image, "");
                 break;
        default: alive_processes++;
                 multiplex_add_process (context, f, (void *)sx);
                 afree (exsize, ex);
    }
}

static void spawn_stack_items (void (*f)(struct exec_context *, void *))
{
    while (consp (workstack) && (alive_processes < max_processes))
    {
        sexpr wcdr = cdr (workstack);

        spawn_item (car (workstack), f);

        workstack = wcdr;
    }
}

enum signal_callback_result cb_on_bad_signal(enum signal s, void *p)
{
    on_error (ie_problematic_signal, (const char *)0);

    return scr_keep;
}

static sexpr initialise_libcurie_filename (sexpr f)
{
    struct sexpr_io *io;
    sexpr r;
    const char *filename = sx_string (f);

    if (falsep (filep (f))) return sx_false;

    io = sx_open_i (io_open_read(filename));

    while (!eofp(r = sx_read (io)))
    {
        if (truep(equalp(r, sym_freestanding)))
        {
            co_freestanding = sx_true;
        }
        else if (truep(equalp(r, sym_hosted)))
        {
            co_freestanding = sx_false;

            if (i_os != os_windows)
            {
                i_dynamic_libraries = sx_false;
            }
        }
    }

    sx_close_io (io);

    return sx_true;
}

static void initialise_libcurie()
{
    if (!falsep(i_destdir))
    {
        if (truep (initialise_libcurie_filename
                       (sx_join (i_destdir, str_slash,
                          sx_join (i_destlibdir, str_slibcuriedsx, sx_nil)))))
        {
            return;
        }

        switch (i_fsl)
        {
            case fs_fhs:
            case fs_fhs_binlib:
                if (truep (initialise_libcurie_filename
                               (sx_join (i_destdir, str_susrs,
                                  sx_join (i_destlibdir, str_slibcuriedsx,
                                           sx_nil)))))
                {
                    return;
                }
                break;
            case fs_afsl:
                if (truep (initialise_libcurie_filename
                     (sx_join (i_destdir, str_slash,
                        sx_join (make_string (uname_os), str_slash,
                          sx_join (make_string (uname_arch),
                            str_slibslibcuriedsx, sx_nil))))))
                {
                    return;
                }
        }
    }

    if (truep (initialise_libcurie_filename
                   (sx_join (str_slash, i_destlibdir, str_slibcuriedsx))))
    {
        return;
    }

    switch (i_fsl)
    {
        case fs_fhs:
        case fs_fhs_binlib:
            if (truep (initialise_libcurie_filename
                 (sx_join (str_susrs, i_destlibdir, str_slibcuriedsx))))
            {
                return;
            }
            if (truep (initialise_libcurie_filename
                 (sx_join (str_slash, i_destlibdir, str_slibcuriedsx))))
            {
                return;
            }
            break;
        case fs_afsl:
            if (truep (initialise_libcurie_filename
                 (sx_join (str_slash, make_string (uname_os),
                    sx_join (str_slash, make_string (uname_arch),
                        str_slibslibcuriedsx)))))
            {
                return;
            }
    }
}

void count_print_items()
{
    int count = 0;
    sexpr cur;

    for (cur = workstack; consp (cur); count++, cur = cdr (cur));

    if (count > 0)
    {
        sx_write (stdio, cons (sym_items_total,
                               cons (make_integer (count), sx_end_of_list)));
    }
}

void loop_processes()
{
    count_print_items();

    spawn_stack_items (process_on_death);

    while (alive_processes > 0)
    {
        multiplex();
        spawn_stack_items (process_on_death);
    }

    sx_write (stdio, cons (sym_phase, cons (sym_completed, sx_end_of_list)));
}

void loop_processes_nokill()
{
    count_print_items();

    spawn_stack_items (process_on_death_nokill);

    while (alive_processes > 0)
    {
        multiplex();
        spawn_stack_items (process_on_death_nokill);
    }

    sx_write (stdio, cons (sym_phase, cons (sym_completed, sx_end_of_list)));
}

static void read_metadata ( void )
{
    struct sexpr_io *io = sx_open_i (io_open_read("metadata.sx"));
    sexpr r;

    while (!eofp(r = sx_read (io)) && !nexp (r))
    {
        struct target *t = (struct target *)0;

        sx_write (stdio, r);

        if (consp(r)) {
            sexpr sxcar = car (r);
            struct tree_node *node =
                tree_get_node_string(&targets, (char *)sx_string(sxcar));

            if (node != (struct tree_node *)0)
            {
                t = (struct target *)node_get_value (node);
                
                t->buildnumber = car (cdr (r));
            }
        }
    }

    sx_close_io (io);
}

static void target_map_save_metadata (struct tree_node *node, void *i)
{
    struct sexpr_io *io = (struct sexpr_io *)i;
    struct target *t = (struct target *)node_get_value (node);

    sx_write (io, cons (t->name, cons (t->buildnumber, sx_end_of_list)));
}

static void save_metadata ( void )
{
    struct sexpr_io *io = sx_open_o (io_open_write("metadata.sx"));

    tree_map (&targets, target_map_save_metadata, (void *)io);

    sx_close_io (io);
}


void on_error (enum icemake_error error, const char *text)
{
    sx_write (stdio,
              cons (sym_error, cons (make_string (text), sx_end_of_list)));

    cexit (error);
}

void on_warning (enum icemake_error error, const char *text)
{
    sx_write (stdio,
              cons (sym_warning, cons (make_string (text), sx_end_of_list)));
}

int icemake_prepare_toolchain
    (const char *name,
     int (*with_data)(struct toolchain_descriptor *, void *), void *aux)
{
    struct toolchain_descriptor td;
    int q;

    if (name != (char *)0)
    {
        int j = 0;

        archprefix = name;

        for (j = 0; name[j] != 0; j++)
        {
            if (name[j] == '-')
            {
                write_uname_element(name, uname_arch, j);
                j++;

                break;
            }
        }

        name += j;
        for (j = 0; name[j] != 0; j++)
        {
            if (name[j] == '-')
            {
                write_uname_element(name, uname_vendor, j);
                j++;

                break;
            }
        }

        name += j;
        for (j = 0; name[j] != 0; j++)
        {
            if (name[j] == '-')
            {
                write_uname_element(name, uname_os, j);
                j++;

                break;
            }
        }

        name += j;
/*        if (name[0] == 'g')
        {
            uname_toolchain = tc_gcc;
        }
        else*/ if (name[0] == 'b')
        {
            uname_toolchain = tc_borland;
        }
        else if (name[0] == 'm')
        {
            uname_toolchain = tc_msvc;
        }
        else
        {
            uname_toolchain = tc_gcc;
        }
    
        architecture = make_string (archprefix);
    }
    else
    {
        char *toolchain = "unknown";
#if !defined(_WIN32)
        struct utsname un;
#endif

        if (!falsep(which(str_cl)))
        {
            uname_toolchain = tc_msvc;
        }
        else if (!falsep(which(str_bcc32)))
        {
            uname_toolchain = tc_borland;
        }
        else /* if nothing specific is found, guess it's gcc*/
        {
            uname_toolchain = tc_gcc;
        }

#if !defined(_WIN32)
        if (uname (&un) >= 0)
        {
            write_uname_element(un.sysname, uname_os,   UNAMELENGTH - 1);
            if ((un.machine[0] == 'i') &&
                ((un.machine[1] == '3') || (un.machine[1] == '4') ||
                 (un.machine[1] == '5') || (un.machine[1] == '6')) &&
                (un.machine[2] == '8') && (un.machine[3] == '6'))
            {
                write_uname_element("x86-32", uname_arch, UNAMELENGTH - 1);
            }
            else
            {
                write_uname_element(un.machine, uname_arch, UNAMELENGTH - 1);
            }
        }
#else

        write_uname_element ("windows", uname_os, UNAMELENGTH-1);
        write_uname_element ("microsoft", uname_vendor, UNAMELENGTH-1);

#if defined(__ia64__) || defined(__ia64) || defined(_M_IA64) || defined(_M_X64)
        write_uname_element ("x86-64", uname_arch, UNAMELENGTH-1);
#elif defined(i386) || defined(__i386__) || defined(_X86_) || defined(_M_IX86) || defined(__INTEL__)
        write_uname_element ("x86-32", uname_arch, UNAMELENGTH-1);
#endif
#endif

        switch (uname_toolchain)
        {
            case tc_gcc:     toolchain = "gnu";     break;
            case tc_borland: toolchain = "borland"; break;
            case tc_msvc:    toolchain = "msvc";    break;
        }

        architecture = lowercase (sx_join (make_string (uname_arch), str_dash,
                                  sx_join (make_string (uname_vendor), str_dash,
                                  sx_join (make_string (uname_os), str_dash,
                                           make_string (toolchain)))));

        archprefix = (char *)sx_string (architecture);
    }

    switch (uname_toolchain)
    {
        case tc_gcc:     initialise_toolchain_gcc();     break;
        case tc_borland: initialise_toolchain_borland(); break;
        case tc_msvc:    initialise_toolchain_msvc();    break;
    }

    initialise_toolchain_tex ();
    initialise_toolchain_doxygen ();

    for (q = 0; uname_os[q] && (uname_os[q] == "darwin"[q]); q++);
    if ((q == 6) && (uname_os[q] == 0)) i_os = os_darwin;
    for (q = 0; uname_os[q] && (uname_os[q] == "linux"[q]); q++);
    if ((q == 5) && (uname_os[q] == 0)) i_os = os_linux;
    for (q = 0; uname_os[q] && (uname_os[q] == "windows"[q]); q++);
    if (((q == 7) && (uname_os[q] == 0)) || (q == 3)) i_os = os_windows;

    for (q = 0; uname_arch[q] && (uname_arch[q] == "arm"[q]); q++);
    if ((q == 3) || ((q == 4) && (uname_arch[q] == 0))) i_is = is_arm;

    if (i_os == os_darwin)
    {
        in_dynamic_libraries = sx_false;
    }

    initialise_libcurie();

    return with_data (&td, aux);
}

int icemake_prepare
    (struct icemake *im, const char *path, struct toolchain_descriptor *td,
     int (*with_data)(struct icemake *, void *), void *aux)
{
    struct icemake iml = { on_error, on_warning, sx_end_of_list, stdio };
    sexpr icemake_sx_path = make_string (path);
    struct sexpr_io *io;
    sexpr r;

    define_string (str_sicemakedsx, "/icemake.sx");

    if (im == (struct icemake *)0)
    {
        im = &iml;
    }

    icemake_sx_path = sx_join (icemake_sx_path, str_sicemakedsx, sx_nil);

    io = sx_open_i (io_open_read (sx_string (icemake_sx_path)));

    while (!eofp(r = sx_read (io)))
    {
        struct target *t = (struct target *)0;

        if (nexp(r)) continue;
        if (consp(r))
        {
            sexpr sxcar = car (r);

            if (truep(equalp(sxcar, sym_library)))
            {
                t = create_library (cdr (r));
            }
            else if (truep(equalp(sxcar, sym_programme)))
            {
                t = create_programme (cdr (r));
            }
            else if (truep(equalp(sxcar, sym_documentation)))
            {
                t = create_documentation (cdr (r));
            }
        }

        if (t != (struct target *)0)
        {
            tree_add_node_string_value (&targets, (char *)sx_string(t->name),t);
        }
    }

    sx_close_io (io);

    td->global = im;
    return with_data (im, aux);
}

static void collect_targets (struct tree_node *n, void *aux)
{
    sexpr *targets = (sexpr *)aux;
    const char *key = (const char *)n->key;

    *targets = sx_set_add (*targets, make_string (key));
}

int icemake
    (struct icemake *im)
{
    read_metadata ();
    merge_contexts ();

    if (!consp(im->buildtargets))
    {
        tree_map (&targets, collect_targets, &(im->buildtargets));
    }

    sx_write (stdio, cons (sym_targets, im->buildtargets));

    icemake_build (im);
    icemake_link (im);

    if (truep (do_build_documentation))
    {
        icemake_build_documentation (im);
    }

    if (truep (do_tests))
    {
        icemake_run_tests (im);
    }

    if (truep (do_install))
    {
        icemake_install (im);
    }

    if (!eolp (im->buildtargets))
    {
        sx_write
            (stdio, cons (sym_completed, cons (sym_targets, im->buildtargets)));
    }

    save_metadata ();

    return failures;
}

static int with_icemake (struct icemake *im, void *aux)
{
    struct icemake_meta *imc = (struct icemake_meta *)aux;

    im->buildtargets = imc->buildtargets;

    return icemake (im);
}

static int with_toolchain (struct toolchain_descriptor *td, void *aux)
{
    struct icemake_meta *im = (struct icemake_meta *)aux;

    if (nilp(in_dynamic_libraries))
    {
        if (i_os == os_windows)
        {
            i_dynamic_libraries = sx_true;
        }
        else if (falsep(co_freestanding))
        {
            i_dynamic_libraries = sx_false;
        }
    }
    else
    {
        i_dynamic_libraries = in_dynamic_libraries;
    }

    return icemake_prepare
        ((struct icemake *)0, ".", td, with_icemake, (void *)im);
}

int cmain ()
{
    int i = 1;
    char *target_architecture = (char *)c_getenv("CHOST");
    struct icemake_meta im = { sx_end_of_list };

    stdio = sx_open_stdout ();

    if (falsep (filep (make_string ("icemake.sx"))))
    {
        on_error (ie_missing_description_file, "");
    }

    mkdir ("build", 0755);

    i_destlibdir = str_lib;

    while (curie_argv[i])
    {
        if (curie_argv[i][0] == '-')
        {
            int y = 1;
            int xn = i + 1;
            while (curie_argv[i][y] != 0)
            {
                switch (curie_argv[i][y])
                {
                    case 't':
                        if (curie_argv[xn])
                        {
                            target_architecture = curie_argv[xn];
                            xn++;
                        }
                        break;
                    case 'z':
                        if (curie_argv[xn])
                        {
                            tcversion = curie_argv[xn];
                            xn++;
                        }
                        break;
                    case 'd':
                        if (curie_argv[xn])
                        {
                            i_destdir = make_string(curie_argv[xn]);
                            xn++;
                        }
                        break;
                    case 'i':
                        do_install = sx_true;
                        break;
                    case 'L':
                        i_optimise_linking = sx_true;
                        break;
                    case 'c':
                        i_combine = sx_true;
                        break;
                    case 'S':
                        i_static = sx_true;
                        break;
                    case 'R':
                        i_static = sx_false;
                        break;
                    case 'x':
                        do_build_documentation = sx_true;
                        break;
                    case 'f':
                        i_fsl = fs_fhs;
                        break;
                    case 'o':
                        in_dynamic_libraries = sx_false;
                        break;
                    case 'O':
                        in_dynamic_libraries = sx_true;
                        break;
                    case 's':
                        i_fsl = fs_afsl;
                        break;
                    case 'b':
                        i_fsl = fs_fhs_binlib;
                        if (curie_argv[xn])
                        {
                            i_pname = make_string(curie_argv[xn]);
                            xn++;
                        }
                    case 'l':
                        if (curie_argv[xn])
                        {
                            i_destlibdir = make_string(curie_argv[xn]);
                            xn++;
                        }
                        break;
                    case 'a':
                        if (curie_argv[(xn + 1)])
                        {
                            i_alternatives
                                    = cons (cons
                                        (make_symbol(curie_argv[xn]),
                                         make_string(curie_argv[(xn + 1)])),
                                      i_alternatives);

                            xn += 2;
                        }
                        break;
                    case 'j':
                        if (curie_argv[xn])
                        {
                            char *s = curie_argv[xn];
                            unsigned int j;

                            max_processes = 0;

                            for (j = 0; s[j]; j++)
                            {
                                max_processes = 10 * max_processes + (s[j]-'0');
                            }

                            xn++;
                        }

                        if (max_processes == 0)
                        {
                            max_processes = 1;
                        }
                        break;
                    case 'r':
                        do_tests = sx_true;
                        break;
                    case 'v':
                        print_version ();
                        return 0;
                    case 'h':
                    case '-':
                        print_help (curie_argv[0]);
                        return 0;
                }

                y++;
            }

            i = xn;
        }
        else
        {
            im.buildtargets =
                sx_set_add (im.buildtargets, make_string (curie_argv[i]));
            i++;
        }
    }

    multiplex_io();
/*    multiplex_all_processes();*/
    multiplex_signal_primary();
    multiplex_process();
    multiplex_sexpr();

    multiplex_add_signal (sig_segv, cb_on_bad_signal, (void *)0);
    multiplex_add_signal (sig_int,  cb_on_bad_signal, (void *)0);
    multiplex_add_signal (sig_term, cb_on_bad_signal, (void *)0);
    multiplex_add_signal (sig_bus,  cb_on_bad_signal, (void *)0);

    multiplex_add_sexpr (stdio, (void *)0, (void *)0);

    return icemake_prepare_toolchain
        (target_architecture, with_toolchain, (void *)&im);
}
