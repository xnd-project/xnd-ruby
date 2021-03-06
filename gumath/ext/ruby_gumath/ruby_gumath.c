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

static VALUE rb_eValueError;

VALUE
seterr(ndt_context_t *ctx)
{
  return rb_ndtypes_set_error(ctx);
}

/****************************************************************************/
/*                               Instance methods                           */
/****************************************************************************/

/* Parse optional arguments passed to GuFuncObject#call. 
 *
 * Populates the rbstack with all the input arguments. Then checks whether
 * the 'out' kwarg has been specified and populates the rest of rbstack
 * with contents of 'out'.
 */
void
parse_args(VALUE *rbstack, int *rb_nin, int *rb_nout, int *rb_nargs, int noptargs,
           VALUE *argv, VALUE out)
{
  size_t nin = noptargs, nout;

  if (noptargs == 0) {
    *rb_nin = 0;
  }
  
  for (int i = 0; i < nin; i++) {
    if (!rb_is_a(argv[i], cXND)) {
      rb_raise(rb_eArgError, "expected xnd arguments.");
    }
    rbstack[i] = argv[i];
  }

  if (out == Qnil) {
    nout = 0;
  }
  else {
    if (rb_xnd_check_type(out)) {
      nout = 1;
      if (nin + nout > NDT_MAX_ARGS) {
        rb_raise(rb_eTypeError, "max number of arguments is %d, got %ld.",
                 NDT_MAX_ARGS, nin+nout);
      }
      rbstack[nin] = out;
    }
    else if (RB_TYPE_P(out, T_ARRAY)) {
      nout = rb_ary_size(out);
      if (nout > NDT_MAX_ARGS || nin+nout > NDT_MAX_ARGS) {
        rb_raise(rb_eTypeError, "max number of arguments is %d, got %ld.",
                 NDT_MAX_ARGS, nin+nout);
      }

      for (int i = 0; i < nout; ++i) {
        VALUE v = rb_ary_entry(out, i);
        if (!rb_is_a(v, cXND)) {
          rb_raise(rb_eTypeError, "expected xnd argument in all elements of out array.");
        }
        rbstack[nin+i] = v;
      }
    }
    else {
      rb_raise(rb_eTypeError, "'out' argument must of type XND or Array of XND objects.");
    }
  }

  *rb_nin = (int)nin;
  *rb_nout = (int)nout;
  *rb_nargs = (int)nin + (int)nout;
}

/* Implement call method on the GufuncObject call. */
static VALUE
Gumath_GufuncObject_call(int argc, VALUE *argv, VALUE self)
{
  VALUE out = Qnil;
  VALUE dt = Qnil;
  VALUE cls = Qnil;
  
  NDT_STATIC_CONTEXT(ctx);
  VALUE rbstack[NDT_MAX_ARGS], opts = Qnil;
  xnd_t stack[NDT_MAX_ARGS];
  const ndt_t *types[NDT_MAX_ARGS];
  gm_kernel_t kernel;
  ndt_apply_spec_t spec = ndt_apply_spec_empty;
  int64_t li[NDT_MAX_ARGS];
  NdtObject *dt_p;
  int k;
  ndt_t *dtype = NULL;
  int nin = argc, nout, nargs;
  bool have_cpu_device = false;
  GufuncObject * self_p;
  bool check_broadcast = true, enable_threads = true;

  if (argc > NDT_MAX_ARGS) {
    rb_raise(rb_eArgError, "too many arguments.");
  }
  
  /* parse keyword arguments. */
  int noptargs = argc;
  for (int i = 0; i < argc; ++i) {
    if (RB_TYPE_P(argv[i], T_HASH)) {
      noptargs = i;
      opts = argv[i];
      break;
    }
  }
  
  if (NIL_P(opts)) { opts = rb_hash_new(); }
  
  out = rb_hash_aref(opts, ID2SYM(rb_intern("out")));
  dt = rb_hash_aref(opts, ID2SYM(rb_intern("dtype")));
  cls = rb_hash_aref(opts, ID2SYM(rb_intern("cls")));

  if (NIL_P(cls)) { cls = cXND; }
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

  if (!rb_klass_has_ancestor(cls, cXND)) {
    rb_raise(rb_eTypeError, "the 'cls' argument must be a subtype of 'xnd'.");
  }

  /* parse leading optional arguments */
  parse_args(rbstack, &nin, &nout, &nargs, noptargs, argv, out);

  for (k = 0; k < nargs; ++k) {
    if (!rb_xnd_is_cuda_managed(rbstack[k])) {
      have_cpu_device = true;
    }

    stack[k] = *rb_xnd_const_xnd(rbstack[k]);
    types[k] = stack[k].type;
    li[k] = stack[k].index;
  }
  GET_GUOBJ(self, self_p);
  
  if (have_cpu_device) {
    if (self_p->flags & GM_CUDA_MANAGED_FUNC) {
      rb_raise(rb_eValueError,
               "cannot run a cuda function on xnd objects with cpu memory.");
    }
  }

  kernel = gm_select(&spec, self_p->table, self_p->name, types, li, nin, nout,
                     nout && check_broadcast, stack, &ctx);

  if (kernel.set == NULL) {
    seterr(&ctx);
    raise_error();
  }

  if (dtype) {
    if (spec.nout != 1) {
      ndt_err_format(&ctx, NDT_TypeError,
                     "the 'dtype' argument is only supported for a single "
                     "return value.");
      ndt_apply_spec_clear(&spec);
      ndt_decref(dtype);
      seterr(&ctx);
      raise_error();
    }

    const ndt_t *u = spec.types[spec.nin];
    const ndt_t *v = ndt_copy_contiguous_dtype(u, dtype, 0, &ctx);

    ndt_apply_spec_clear(&spec);
    ndt_decref(dtype);

    if (v == NULL) {
      seterr(&ctx);
      raise_error();
    }

    types[nin] = v;
    kernel = gm_select(&spec, self_p->table, self_p->name, types, li, nin, 1,
                       1 && check_broadcast, stack, &ctx);
    if (kernel.set == NULL) {
      seterr(&ctx);
      raise_error();
    }
  }

  /*
   * Replace args/kwargs types with types after substitution and broadcasting.
   * This includes 'out' types, if explicitly passed as kwargs.
   */
  for (int i = 0; i < spec.nargs; ++i) {
    stack[i].type = spec.types[i];
  }

  if (nout == 0) {
    /* 'out' types have been inferred, create new XndObjects. */
    VALUE x;
    for (int i = 0; i < spec.nout; ++i) {
      if (ndt_is_concrete(spec.types[nin+i])) {
        uint32_t flags = self_p->flags == GM_CUDA_MANAGED_FUNC ? XND_CUDA_MANAGED : 0;
        x = rb_xnd_empty_from_type(cls, spec.types[nin+i], flags);
        rbstack[nin+i] = x;
        stack[nin+i] = *rb_xnd_const_xnd(x);
      }
      else {
        rb_raise(rb_eValueError,
                 "args with abstract types are temporarily disabled.");
      }
    }
  }

  if (self_p->flags == GM_CUDA_MANAGED_FUNC) {
#ifdef HAVE_CUDA
    /* populate with CUDA specific stuff */
#else
    ndt_err_format(&ctx, NDT_RuntimeError,
                   "internal error: GM_CUDA_MANAGED_FUNC set in a build without cuda support");
    ndt_apply_spec_clear(&spec);
    seterr(&ctx);
    raise_error();
#endif // HAVE_CUDA
  }
  else {
#ifdef HAVE_PTHREAD_H
    const int rounding = fegetround();
    fesetround(FE_TONEAREST);

    const int64_t N = enable_threads ? max_threads : 1;
    const int ret = gm_apply_thread(&kernel, stack, spec.outer_dims, N, &ctx);
    fesetround(rounding);

    if (ret < 0) {
      ndt_apply_spec_clear(&spec);
      seterr(&ctx);
      raise_error();
    }
#else
    const int rounding = fegetround();
    fesetround(FE_TONEAREST);

    const int ret = gm_apply(&kernel, stack, spec.outer_dims, &ctx);
    fesetround(rounding);

    if (ret < 0) {
      ndt_apply_spec_clear(&spec);
      seterr(&ctx);
      raise_error();
    }    
#endif // HAVE_PTHREAD_H
  }

  nin = spec.nin;
  nout = spec.nout;
  nargs = spec.nargs;
  ndt_apply_spec_clear(&spec);

  switch (nout) {
  case 0: {
    return Qnil;
  }
  case 1: {
    return rbstack[nin];
  }
  default: {
    VALUE arr = rb_ary_new2(nout);
    for (int i = 0; i < nout; ++i) {
      rb_ary_store(arr, i, rbstack[nin+i]);
    }
    return arr;
  }
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

  func = GufuncObject_alloc(a->table, f->name, GM_CPU_FUNC);
  if (func == NULL) {
    return -1;
  }

  func_hash = rb_ivar_get(a->module, GUMATH_FUNCTION_HASH);
  rb_hash_aset(func_hash, ID2SYM(rb_intern(f->name)), func);

  return 0;
}

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

  /* errors */
  rb_eValueError = rb_define_class("ValueError", rb_eRuntimeError);
  
  Init_gumath_functions();
  Init_gumath_examples();
}
