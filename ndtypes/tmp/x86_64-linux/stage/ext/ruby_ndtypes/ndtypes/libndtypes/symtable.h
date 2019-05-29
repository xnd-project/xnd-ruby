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


#ifndef SYMTABLE_H
#define SYMTABLE_H


/*****************************************************************************/
/*                     Symbol tables used in type matching                   */
/*****************************************************************************/

enum symtable_entry {
  Unbound,
  Shape,
  Symbol,
  Type,
  BroadcastSeq,
  FixedSeq,
  VarSeq
};

typedef struct {
    int size;
    const ndt_t *dims[NDT_MAX_DIM];
} dim_list_t;

typedef struct {
    int size;
    int64_t dims[NDT_MAX_DIM];
}  broadcast_list_t;

typedef struct {
  enum symtable_entry tag;
  union {
    int64_t Shape;
    const char *Symbol;
    const ndt_t *Type;
    broadcast_list_t BroadcastSeq;
    dim_list_t FixedSeq;
    dim_list_t VarSeq;
  };
} symtable_entry_t;

typedef struct symtable {
    symtable_entry_t entry;
    struct symtable *next[];
} symtable_t;


/* LOCAL SCOPE */
NDT_PRAGMA(NDT_HIDE_SYMBOLS_START)


symtable_t *symtable_new(ndt_context_t *ctx);
void symtable_free_entry(symtable_entry_t entry);
void symtable_del(symtable_t *t);
int symtable_add(symtable_t *t, const char *key, const symtable_entry_t entry,
                 ndt_context_t *ctx);
symtable_entry_t symtable_find(const symtable_t *t, const char *key);
symtable_entry_t *symtable_find_ptr(symtable_t *t, const char *key);
int64_t symtable_find_shape(const symtable_t *tbl, const char *key, ndt_context_t *ctx);
const ndt_t *symtable_find_typevar(const symtable_t *tbl, const char *key, ndt_context_t *ctx);
const ndt_t *symtable_find_var_dim(const symtable_t *tbl, int ndim, ndt_context_t *ctx);


/* END LOCAL SCOPE */
NDT_PRAGMA(NDT_HIDE_SYMBOLS_END)


#endif /* SYMTABLE_H */
