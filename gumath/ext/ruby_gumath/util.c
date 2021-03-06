/* BSD 3-Clause License
 *
 * Copyright (c) 2018, Quansight and Sameer Deshmukh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
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

/* 
   Utility functions for gumath. 
*/
#include "ruby_gumath_internal.h"

VALUE
array_new(int64_t size)
{
#if SIZE_MAX < INT64_MAX
    size_t n = safe_downcast(size);
    return n < 0 ? NULL : rb_ary_new2(n);
#else
    return rb_ary_new2(size);
#endif
}

int
xnd_exists(void)
{
  return RTEST(rb_const_get(rb_cObject, rb_intern("XND")));
}

int
ndt_exists(void)
{
  return RTEST(rb_const_get(rb_cObject, rb_intern("NDT")));
}

/* Raise an error stored in $!. Clears it before raising. */
void
raise_error(void)
{
  VALUE exeception = rb_errinfo();

  rb_set_errinfo(Qnil);
  rb_exc_raise(exeception);
}

/* Checks whether a given Ruby object is of a type using is_a? */
int
rb_is_a(VALUE obj, VALUE klass)
{
  return RTEST(rb_funcall(obj, rb_intern("is_a?"), 1, klass));
}

int
rb_klass_has_ancestor(VALUE klass, VALUE ancestor)
{
  return RTEST(
               rb_funcall(
                          rb_funcall(klass, rb_intern("ancestors"),0,NULL),
                          rb_intern("include?"),
                          1,
                          ancestor));
}

int
rb_ary_size(VALUE array)
{
  Check_Type(array, T_ARRAY);
  return FIX2INT(rb_funcall(array, rb_intern("size"), 0, NULL));
}

void
rb_puts(VALUE v) {
  ID sym_puts = rb_intern("puts");
  ID sym_inspect = rb_intern("inspect");
  rb_funcall(rb_mKernel, sym_puts, 1,
             rb_funcall(v, sym_inspect, 0));
}

