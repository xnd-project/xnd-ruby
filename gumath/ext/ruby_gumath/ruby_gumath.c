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
#include "ruby_gumath_internal.h"

/* libxnd.so is not linked without at least one xnd symbol. */
const void *dummy = NULL;

/****************************************************************************/
/*                              Class globals                               */
/****************************************************************************/

/* Function table */
static gm_tbl_t *table = NULL;

/* Maximum number of threads */
static int64_t max_threads = 1;
static int initialized = 0;
VALUE cGumath;

/****************************************************************************/
/*                               Error handling                             */
/****************************************************************************/

VALUE
seterr(ndt_context_t *ctx)
{
  return rb_ndtypes_set_error(ctx);
}

/****************************************************************************/
/*                               Instance methods                           */
/****************************************************************************/

/* Implement call method on the GufuncObject call. */
static VALUE
Gumath_GufuncObject_call(int argc, VALUE *argv, VALUE self)
{
  VALUE out = Qnil;
  VALUE dt = Qnil;
  VALUE cls = Qnil;
  
  NDT_STATIC_CONTEXT(ctx);
  VALUE result[NDT_MAX_ARGS], opts;
  xnd_t stack[NDT_MAX_ARGS];
  const ndt_t *types[NDT_MAX_ARGS];
  gm_kernel_t kernel;
  ndt_apply_spec_t spec = ndt_apply_spec_empty;
  int64_t li[NDT_MAX_ARGS];
  GufuncObject *self_p;
  NdtObject *dt_p;
  int k;
  ndt_t *dtype = NULL;
  size_t nin = argc, nout, nargs;
  bool have_cpu_device = false;

  if (argc > NDT_MAX_ARGS) {
    rb_raise(rb_eArgError, "too many arguments.");
  }
  
  /* parse keyword arguments. */
  for (int i = 0; i < argc; ++i) {
    if (RB_TYPE_P(argv[i], T_HASH)) {
      opts = argv[i];
      break;
    }
  }
  out = rb_hash_aref(opts, rb_intern("out"));
  dt = rb_hash_aref(opts, rb_intern("dtype"));
  cls = rb_hash_aref(opts, rb_intern("cls"));
  if (NIL_P(cls)) {
    cls = cXND;
  }
  if (!NIL_P(dt)) {
    if (!NIL_P(out)) {
      rb_raise(rb_eArgError, "the 'out' and 'dtype' arguments are mutually exclusive.");
    }

    if (!rb_ndtypes_check_type(dt)) {
      rb_raise(rb_eArgError, "'dtype' argument must be an NDT object.");
    }
    dtype = (ndt_t *)rb_ndtypes_const_ndt(dt);
    ndt_incref(dtype);
  }
}

/****************************************************************************/
/*                               Singleton methods                          */
/****************************************************************************/

static VALUE
Gumath_s_unsafe_add_kernel(int argc, VALUE *argv, VALUE klass)
{
  /* TODO: implement this. */
}

static VALUE
Gumath_s_get_max_threads(VALUE klass)
{
  return INT2NUM(max_threads);
}

static VALUE
Gumath_s_set_max_threads(VALUE klass, VALUE threads)
{
  Check_Type(threads, T_FIXNUM);
  
  max_threads = NUM2INT(threads);
}

/****************************************************************************/
/*                                   Other functions                        */
/****************************************************************************/

static void
init_max_threads(void)
{
  VALUE rb_max_threads = rb_funcall(rb_const_get(rb_cObject, rb_intern("Etc")),
                                    rb_intern("nprocessors"), 0, NULL);
  max_threads = NUM2INT(rb_max_threads);
}

/****************************************************************************/
/*                                   C-API                                  */
/****************************************************************************/

struct map_args {
  VALUE module;
  const gm_tbl_t *table;
};

/* Function called by libgumath that will load function kernels from function
   table of type gm_tbl_t into a Ruby module. Don't call this directly use
   rb_gumath_add_functions.
 */
int
add_function(const gm_func_t *f, void *args)
{
  struct map_args *a = (struct map_args *)args;
  VALUE func, func_hash;

  func = GufuncObject_alloc(a->table, f->name);
  if (func == NULL) {
    return -1;
  }

  func_hash = rb_ivar_get(a->module, GUMATH_FUNCTION_HASH);
  rb_hash_aset(func_hash, ID2SYM(rb_intern(f->name)), func);

  return 0;
}

/* C API call for adding functions from a gumath kernel table to  */
int
rb_gumath_add_functions(VALUE module, const gm_tbl_t *tbl)
{
  struct map_args args = {module, tbl};

  if (gm_tbl_map(tbl, add_function, &args) < 0) {
    return -1;
  }
}

void Init_ruby_gumath(void)
{
  NDT_STATIC_CONTEXT(ctx);

  if (!initialized) {
    dummy = &xnd_error;

    gm_init();

    if (!xnd_exists()) {
      rb_raise(rb_eLoadError, "Need XND for gumath.");
    }

    if (!ndt_exists()) {
      rb_raise(rb_eLoadError, "Need NDT for gumath.");
    }

    table = gm_tbl_new(&ctx);
    if (table == NULL) {
      seterr(&ctx);
      raise_error();
    }

    init_max_threads();

    initialized = 1;
  }

  cGumath = rb_define_class("Gumath", rb_cObject);
  cGumath_GufuncObject = rb_define_class_under(cGumath, "GufuncObject", rb_cObject);
    
  /* Class: Gumath */
  
  /* Singleton methods */
  rb_define_singleton_method(cGumath, "unsafe_add_kernel", Gumath_s_unsafe_add_kernel, -1);
  rb_define_singleton_method(cGumath, "get_max_threads", Gumath_s_get_max_threads, 0);
  rb_define_singleton_method(cGumath, "set_max_threads", Gumath_s_set_max_threads, 1);

  /* Class: Gumath::GufuncObject */

  /* Instance methods */
  rb_define_method(cGumath_GufuncObject, "call", Gumath_GufuncObject_call,-1);
  
  Init_gumath_functions();
  Init_gumath_examples();
}
