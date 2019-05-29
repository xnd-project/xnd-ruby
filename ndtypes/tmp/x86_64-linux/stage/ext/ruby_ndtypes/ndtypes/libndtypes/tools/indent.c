/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2017-2018, plures
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include "ndtypes.h"


#ifdef YYDEBUG
int yydebug = 1;
#endif

int
main(int argc, char **argv)
{
    ndt_context_t *ctx;
    ndt_t *t;
    char *s;

    if (argc != 2) {
        fprintf(stderr, "usage: ./parser file\n");
        return 1;
    }

    ctx = ndt_context_new();
    if (ctx == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (ndt_init(ctx) < 0) {
        ndt_err_fprint(stderr, ctx);
        ndt_context_del(ctx);
        return 1;
    }

    t = ndt_from_file(argv[1], ctx);
    if (t == NULL) {
        ndt_err_fprint(stderr, ctx);
        assert(ndt_err_occurred(ctx));
        ndt_context_del(ctx);
        return 1;
    }
    assert(!ndt_err_occurred(ctx));

    s = ndt_indent(t, ctx);
    ndt_del(t);
    if (s == NULL) {
        ndt_err_fprint(stderr, ctx);
        assert(ndt_err_occurred(ctx));
        ndt_context_del(ctx);
        return 1;
    }
    assert(!ndt_err_occurred(ctx));

    printf("%s\n", s);
    ndt_free(s);
    ndt_context_del(ctx);
    ndt_finalize();
 
    return 0; 
}
