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

/* File containing the majority implementation of the Ruby XND wrapper.
 *
 * Author: Sameer Deshmukh (@v0dro)
 */

#include "ruby_xnd_internal.h"
#include "xnd.h"

VALUE cRubyXND;
VALUE cXND;
static VALUE cRubyXND_MBlock;
static const rb_data_type_t MemoryBlockObject_type;
static const rb_data_type_t XndObject_type;

static VALUE rb_eValueError;

VALUE mRubyXND_GCGuard;

static VALUE seterr(ndt_context_t *ctx);

/****************************************************************************/
/*                               Error handling                             */
/****************************************************************************/
static VALUE
seterr(ndt_context_t *ctx)
{
  return rb_ndtypes_set_error(ctx);
}

#ifdef XND_DEBUG
void
obj_inspect(const char* msg, VALUE obj)
{
  VALUE insp = rb_funcall(obj, rb_intern("inspect"), 0, NULL);
  printf("%s %s\n.", msg, StringValuePtr(insp));
}
#endif

/****************************************************************************/
/*                           Singletons                                     */
/****************************************************************************/
static VALUE
xnd_ellipsis(void)
{
  return rb_funcall(rb_const_get_at(cXND, rb_intern("Ellipsis")),
                    rb_intern("new"), 0, NULL);
}

/****************************************************************************/
/*                           MemoryBlock Object                             */
/****************************************************************************/

/* The MemoryBlockObject is shared among several XND views/objects. */
typedef struct MemoryBlockObject {
  VALUE type;        /* type owner (ndtype) */  
  xnd_master_t *xnd; /* memblock owner */
} MemoryBlockObject;

#define GET_MBLOCK(obj, mblock_p) do {                              \
    TypedData_Get_Struct((obj), MemoryBlockObject,                  \
                         &MemoryBlockObject_type, (mblock_p));      \
  } while (0)
#define MAKE_MBLOCK(self, mblock_p) TypedData_Make_Struct(self, MemoryBlockObject, \
                                                          &MemoryBlockObject_type, mblock_p)
#define WRAP_MBLOCK(self, mblock_p) TypedData_Wrap_Struct(self,         \
                                                          &MemoryBlockObject_type, mblock_p)

/* Mark Ruby objects within MemoryBlockObject. */
static void
MemoryBlockObject_dmark(void *self)
{
  MemoryBlockObject *mblock = (MemoryBlockObject*)self;

  rb_gc_mark(mblock->type);
}

static void
MemoryBlockObject_dfree(void *self)
{
  MemoryBlockObject *mblock = (MemoryBlockObject*)self;

  rb_xnd_gc_guard_unregister_mblock_type(mblock);
  xnd_del(mblock->xnd);
  mblock->xnd = NULL;
  xfree(mblock);
}

static size_t
MemoryBlockObject_dsize(const void *self)
{
  return sizeof(MemoryBlockObject);
}

static const rb_data_type_t MemoryBlockObject_type = {
  .wrap_struct_name = "MemoryBlockObject",
  .function = {
    .dmark = MemoryBlockObject_dmark,
    .dfree = MemoryBlockObject_dfree,
    .dsize = MemoryBlockObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

/* Allocate a MemoryBlockObject and return a pointer to allocated memory. */
static MemoryBlockObject *
mblock_alloc(void)
{
  MemoryBlockObject *self;

  self = ALLOC(MemoryBlockObject);
  if (self == NULL) {
    
  }

  self->type = NULL;
  self->xnd = NULL;

  return self;
}

/* Allocate a MemoryBlockObject and wrap it in a Ruby object. */
static VALUE
mblock_allocate(void)
{
  MemoryBlockObject *self = mblock_alloc();

  return WRAP_MBLOCK(cRubyXND_MBlock, self);
}

/* Create empty mblock with no data. */
static VALUE
mblock_empty(VALUE type, uint32_t flags)
{
  NDT_STATIC_CONTEXT(ctx);
  MemoryBlockObject *mblock_p;
  const ndt_t * ndt_p;
  
  if (!rb_ndtypes_check_type(type)) {
    rb_raise(rb_eArgError, "require NDT object to create mblock in mblock_empty.");
  }

  mblock_p = mblock_alloc();
  ndt_p = rb_ndtypes_const_ndt(type);
  //  ndt_incref(ndt_p);
  
  mblock_p->xnd = xnd_empty_from_type(ndt_p, XND_OWN_EMBEDDED|flags, &ctx);
  if (mblock_p->xnd == NULL) {
    rb_raise(rb_eValueError, "cannot create mblock object from given type.");
  }
  mblock_p->type = type;

  return WRAP_MBLOCK(cRubyXND_MBlock, mblock_p);
}

static VALUE
mblock_from_xnd(xnd_t *src)
{
  NDT_STATIC_CONTEXT(ctx);
  MemoryBlockObject *mblock_p;
  VALUE type, mblock;
  xnd_master_t *x;

  x = xnd_from_xnd(src, XND_OWN_EMBEDDED, &ctx);
  if (x == NULL) {
    seterr(&ctx);
    raise_error();
  }

  type = rb_ndtypes_from_type((ndt_t *)x->master.type);
  mblock = mblock_allocate();

  GET_MBLOCK(mblock, mblock_p);

  mblock_p->type = type;
  mblock_p->xnd = x;

  return mblock;
}

static void
_strncpy(char *dest, const void *src, size_t len, size_t size)
{
    assert (len <= size);
    memcpy(dest, src, len);
    memset(dest+len, '\0', size-len);
}

static int
string_is_ascii(VALUE str)
{
  return RB_ENCODING_IS_ASCII8BIT(str);
}

/* FIXME: Below functions make Ruby function calls. Find more efficient way. */

/* Return true if String is UTF-8 encoding. */
static int
string_is_u8(VALUE str)
{
  return RTEST(
               rb_funcall(
                    rb_funcall(str, rb_intern("encoding"), 0, NULL),
                    rb_intern("=="), 1,
                    rb_const_get(rb_cEncoding, rb_intern("UTF_8"))
                          )
               );
}

static int64_t
u8_skip_trailing_zero(const uint8_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
}

static int64_t
u16_skip_trailing_zero(const uint16_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
}

static int64_t
u32_skip_trailing_zero(const uint32_t *ptr, int64_t codepoints)
{
    int64_t i;

    for (i=codepoints-1; i >= 0; i--)
        if (ptr[i] != 0)
            return i+1;

    return 0;
}

static int64_t
get_int(VALUE data, int64_t min, int64_t max)
{
  int64_t x;

  x = NUM2LL(data);
  if (x < min || x > max) {
    rb_raise(rb_eRangeError, "Number out of range of int64 range.");
  }

  return x;
}

static uint64_t
get_uint(VALUE data, uint64_t max)
{ 
  unsigned long long x = NUM2ULL(data);

  if (x == (unsigned long long)-1) {
    rb_raise(rb_eRangeError, "cannot assigned negative number to unsigned type.");
  }
  
  if (x > max) {
    rb_raise(rb_eRangeError, "number out of range: %ulld", x);
  }

  return x;
}

/* Initialize an mblock object with data. */
static int
mblock_init(xnd_t * const x, VALUE data)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t * const t = x->type;

  if (!check_invariants(t)) {
    rb_raise(rb_eArgError, "invariants in type.");
  }

  if (ndt_is_abstract(t)) {
    rb_raise(rb_eTypeError, "specified NDT has abstract type.");
  }

  /* set missing value. */
  if (ndt_is_optional(t)) {
    if (t->ndim > 0) {
      rb_raise(rb_eNotImpError,
               "optional dimensions are not implemented.");
    }

    if (data == Qnil) {
      xnd_set_na(x);
      return 0;
    }

    xnd_set_valid(x);
  }

  switch (t->tag) {
  case FixedDim: {
    const int64_t shape = t->FixedDim.shape;
    int64_t i;

    Check_Type(data, T_ARRAY);

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eArgError,
               "Input length (%ld) and type length (%ld) mismatch.",
               RARRAY_LEN(data), shape);
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_fixed_dim_next(x, i);
      VALUE rb_index[1] = { LL2NUM(i) };

      mblock_init(&next, rb_ary_aref(1, rb_index, data));
    }
    return 0;
  }

  case VarDim: {
    int64_t start, step, shape;
    int64_t i;

    Check_Type(data, T_ARRAY);

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      seterr(&ctx);
      raise_error();
    }

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eValueError, "expected Array with size %ld not %ld.",
               RARRAY_LEN(data), shape);
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_var_dim_next(x, start, step, i);
      VALUE rb_index[1] = { LL2NUM(i) };
      
      mblock_init(&next, rb_ary_aref(1, rb_index, data));
    }

    return 0;
  }
    
  case Tuple: {
    const int64_t shape = t->Tuple.shape;
    int64_t i;

    /* since ruby does not have immutable tuple-type, we use Array instead. */
    Check_Type(data, T_ARRAY);

    if (RARRAY_LEN(data) != shape) {
      rb_raise(rb_eArgError,
               "expected Array with size %ld, not %ld.",
               shape, RARRAY_LEN(data));
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_tuple_next(x, i, &ctx);
      if (next.ptr == NULL) {
        seterr(&ctx);
        raise_error();
      }
      VALUE rb_index[1] = { LL2NUM(i) };
      
      mblock_init(&next, rb_ary_aref(1, rb_index, data));
    }

    return 0;
  }

  case Record: {
    const int64_t shape = t->Record.shape;
    VALUE temp;
    int64_t i;

    Check_Type(data, T_HASH);

    if (rb_xnd_hash_size(data) != shape) {
      rb_raise(rb_eArgError, "expected Hash size does not match with shape size.");
    }

    for (i = 0; i < shape; i++) {
      xnd_t next = xnd_record_next(x, i, &ctx);
      if (next.ptr == NULL) {
        seterr(&ctx);
        raise_error();
      }

      temp = rb_hash_aref(data, rb_str_new2(t->Record.names[i]));
      mblock_init(&next, temp);
    }

    return 0;
  }

  case Ref: {
    xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return mblock_init(&next, data);
  }

  case Constr: {
    xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();      
    }

    return mblock_init(&next, data);
  }

  case Nominal: {
    xnd_t next = xnd_nominal_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    if (t->Nominal.meth->init != NULL) {
      if (!t->Nominal.meth->init(&next, x, &ctx)) {
        rb_raise(rb_eTypeError, "could not init Nominal type in mblock_init.");
      }
      return 0;
    }

    mblock_init(&next, data);

    if (t->Nominal.meth->constraint != NULL &&
        !t->Nominal.meth->constraint(&next, &ctx)) {
      seterr(&ctx);
      raise_error();
    }

    return 0;
  }

  case Bool: {
    bool b;

    if (data == Qnil) {
      rb_raise(rb_eTypeError,
               "assigning nil to memory block with non-optional type.");
    }

    if (RTEST(data)) {
      if (FIXNUM_P(data) || RB_FLOAT_TYPE_P(data)) {
        if (NUM2INT(data) == 0) {
          b = 0;
        }
      }
      else {
        b = 1;
      }
    }
    else {
      b = 0;
    }
    
    PACK_SINGLE(x->ptr, b, bool, t->flags);
    return 0;
  }
    
  case Int8: {
    int8_t temp = (int8_t)get_int(data, INT8_MIN, INT8_MAX);

    PACK_SINGLE(x->ptr, temp, int8_t, t->flags);
    return 0;
  }

  case Int16: {
    int16_t temp = (int16_t)get_int(data, INT16_MIN, INT16_MAX);

    PACK_SINGLE(x->ptr, temp, int16_t, t->flags);
    return 0;
  }

  case Int32: {
    int32_t temp = (int32_t)get_int(data, INT32_MIN, INT32_MAX);
    
    PACK_SINGLE(x->ptr, temp, int32_t, t->flags);
    return 0;
  }

  case Int64: {
    int64_t temp = get_int(data, INT64_MIN, INT64_MAX);

    PACK_SINGLE(x->ptr, temp, int64_t, t->flags);
    return 0;
  }
    
  case Uint8: {
    uint8_t temp = (uint8_t)get_uint(data, UINT8_MAX);
    PACK_SINGLE(x->ptr, temp, uint8_t, t->flags);
    return 0;
  }

  case Uint16: {
    uint16_t temp = (uint16_t)get_uint(data, UINT16_MAX);

    PACK_SINGLE(x->ptr, temp, uint16_t, t->flags);
    return 0;
  }

  case Uint32: {
    uint32_t temp = (uint32_t)get_uint(data, UINT32_MAX);
    
    PACK_SINGLE(x->ptr, temp, uint32_t, t->flags);
    return 0;
  }

  case Uint64: {
    uint64_t temp = get_uint(data, UINT64_MAX);

    PACK_SINGLE(x->ptr, temp, uint64_t, t->flags);
    return 0;
  }

  case Float16: {
    rb_raise(rb_eNotImpError, "float16 not implemented.");
  }

  case Float32: {
    double temp = NUM2DBL(data);
    return rb_xnd_pack_float32(temp, (unsigned char*)x->ptr, le(t->flags));
  }

  case Float64: {
    double temp = NUM2DBL(data);
    return rb_xnd_pack_float64(temp, (unsigned char*)x->ptr, le(t->flags));
  }

  case Complex32: {
    rb_raise(rb_eNotImpError, "complex32 not implemented.");
  }

  case Complex64: {
    double real, imag;

    rb_xnd_get_complex_values(data, &real, &imag);
    
    rb_xnd_pack_float32(real, (unsigned char*)x->ptr, le(t->flags));
    rb_xnd_pack_float32(imag, (unsigned char*)x->ptr + 4, le(t->flags));

    return 0;
  }

  case Complex128: {
    double real, imag;

    rb_xnd_get_complex_values(data, &real, &imag);
    
    rb_xnd_pack_float64(real, (unsigned char*)x->ptr, le(t->flags));
    rb_xnd_pack_float64(imag, (unsigned char*)x->ptr + 8, le(t->flags));

    return 0;
  }

  case FixedString: {
    int64_t codepoints = t->FixedString.size;
    int64_t len;

    Check_Type(data, T_STRING);

    /* FIXME: check for unicode string. */
    
    switch (t->FixedString.encoding) {
    case Ascii: {
      if (!string_is_ascii(data)) {
        rb_raise(rb_eValueError, "string must be ascii");
      }

      len = RSTRING_LEN(data);
      if (len > t->datasize) {
        rb_raise(rb_eValueError,
                 "maxmimum string size in bytes is %" PRIi64, codepoints);
      }

      _strncpy(x->ptr, StringValuePtr(data), (size_t)len, (size_t)t->datasize);
      return 0;
    }
      
    case Utf8: {
      if (!string_is_u8(data)) {
        rb_raise(rb_eValueError, "string must be utf-8");
      }

      len = RSTRING_LEN(data);
      if (len > t->datasize) {
        rb_raise(rb_eValueError,
                 "maximum string size (in UTF-8 code points) is %" PRIi64, codepoints);
      }

      _strncpy(x->ptr, StringValuePtr(data), (size_t)len, (size_t)t->datasize);
      return 0;
    }

    case Utf16: {
      rb_encoding *utf16 = rb_enc_find("UTF-16");
      VALUE b = rb_str_export_to_enc(data, utf16);

      len = RSTRING_LEN(b);
      if (len-2 > t->datasize) {
        rb_raise(rb_eValueError,
                 "maximum string size (in UTF-16 code points) is %" PRIi64, codepoints);
      }
      
#ifdef XND_DEBUG
      /* skip byte order mark. */
      assert(len >= 2);
#endif
      _strncpy(x->ptr, StringValuePtr(b) + 2, (size_t)(len-2), (size_t)t->datasize);
      return 0;
    }

    case Utf32: {
      VALUE b = rb_str_export_to_enc(data, rb_enc_find("UTF-32"));

      len = RSTRING_LEN(b);
      if (len-4 > t->datasize) {
        rb_raise(rb_eValueError,
                 "maximum string size (in UTF-32 code points) is %" PRIi64, codepoints);
      }

#ifdef XND_DEBUG
      /* skip byte order mark. */
      assert(len >= 4);
#endif
      _strncpy(x->ptr, StringValuePtr(b)+4, (size_t)(len-4), (size_t)t->datasize);
      return 0;
    }

    case Ucs2: {
      rb_raise(rb_eNotImpError, "UCS2 encoding not implemented.");
    }

    default: {
      rb_raise(rb_eRuntimeError, "invaling string encoding.");
    }
    }
  }

  case FixedBytes: {
    int64_t size = t->FixedBytes.size;
    int64_t len;

    Check_Type(data, T_STRING);

    if (!string_is_ascii(data)) {
      rb_raise(rb_eTypeError, "String must be ASCII encoded for FixedBytes.");
    }

    len = RSTRING_LEN(data);

    if (len > size) {
      rb_raise(rb_eValueError, "maximum number of bytes in string is %", PRIi64, size);
    }

    _strncpy(x->ptr, StringValuePtr(data), (size_t)len, (size_t)size);
    
    return 0;
  }

  case String: {
    size_t size;
    const char *cp;
    char *s;

    Check_Type(data, T_STRING);
    
    cp = StringValuePtr(data);
    size = RSTRING_LEN(data);
    s = ndt_strdup(cp, &ctx);
    if (s == NULL) {
      seterr(&ctx);
      raise_error();
    }

    if (XND_POINTER_DATA(x->ptr)) {
      ndt_free(XND_POINTER_DATA(x->ptr));
    }

    XND_POINTER_DATA(x->ptr) = s;
    return 0;
  }

  case Bytes: {
    size_t size;
    char *cp, *s;

    Check_Type(data, T_STRING);

    size = RSTRING_LEN(data);
    cp = StringValuePtr(data);

    s = ndt_aligned_calloc(t->Bytes.target_align, size);
    if (s == NULL) {
      rb_raise(rb_eNoMemError, "no memory for allocating bytes.");
    }

    memcpy(s, cp, size);

    if (XND_BYTES_DATA(x->ptr)) {
      ndt_aligned_free(XND_BYTES_DATA(x->ptr));
    }

    XND_BYTES_SIZE(x->ptr) = size;
    XND_BYTES_DATA(x->ptr) = (uint8_t *)s;

    return 0;
  }

  case Categorical: {
    int64_t k;
    
    if (RB_TYPE_P(data, T_TRUE) || RB_TYPE_P(data, T_FALSE)) {
      int temp = RTEST(data);

      for (k = 0; k < t->Categorical.ntypes; k++) {
        if (t->Categorical.types[k].tag == ValBool &&
            temp == t->Categorical.types[k].ValBool) {
          PACK_SINGLE(x->ptr, k, int64_t, t->flags);
          return 0;
        }
      }
    }
    else if (RB_TYPE_P(data, T_FIXNUM)) {
      int64_t temp = get_int(data, INT64_MIN, INT64_MAX);

      for (k = 0; k < t->Categorical.ntypes; k++) {
        if (t->Categorical.types[k].tag == ValInt64 &&
            temp == t->Categorical.types[k].ValInt64) {
          PACK_SINGLE(x->ptr, k, int64_t, t->flags);
          return 0;
        }
      }
    }
    else if (RB_TYPE_P(data, T_FLOAT)) {
      double temp = NUM2DBL(data);

      for (k = 0; k < t->Categorical.ntypes; k++) {
        if (t->Categorical.types[k].tag == ValFloat64 &&
            temp == t->Categorical.types[k].ValFloat64) {
          PACK_SINGLE(x->ptr, k, int64_t, t->flags);
          return 0;
        }
      }
    }
    else if (RB_TYPE_P(data, T_STRING)) {
      const char *temp = StringValuePtr(data);

      for (k = 0; k < t->Categorical.ntypes; k++) {
        if (t->Categorical.types[k].tag == ValString &&
            strcmp(temp, t->Categorical.types[k].ValString) == 0) {
          PACK_SINGLE(x->ptr, k, int64_t, t->flags);
          return 0;
        }
      }
    }
    
    for (k = 0; k < t->Categorical.ntypes; k++) {
      if (t->Categorical.types[k].tag == ValNA) {
        PACK_SINGLE(x->ptr, k, int64_t, t->flags);
        return 0;
      }
    }

    rb_raise(rb_eValueError, "category not found.");
  }

  case Char: {
    rb_raise(rb_eNotImpError, "'Char' type semantics need to be defined.");
  }

  case Module: {
    rb_raise(rb_eNotImpError, "'Module' type not implemented.");
  }

    /* NOT REACHED: intercepted by ndt_is_abstract(). */
  case AnyKind: case SymbolicDim: case EllipsisDim: case Typevar:
  case ScalarKind: case SignedKind: case UnsignedKind: case FloatKind:
  case ComplexKind: case FixedStringKind: case FixedBytesKind:
  case Function:
    rb_raise(rb_eArgError, "unexpected abstract type.");
  }
}

/* Create mblock from NDT type. 
 *
 * @param type - NDT Ruby object.
 * @param data - Data as a Ruby object.
 */
static VALUE
mblock_from_typed_value(VALUE type, VALUE data, int32_t flags)
{
  VALUE mblock;
  MemoryBlockObject *mblock_p;

  mblock = mblock_empty(type, flags);
  GET_MBLOCK(mblock, mblock_p);
  mblock_init(&mblock_p->xnd->master, data);

  return mblock;
}

/****************************************************************************/
/*                                 xnd object                               */
/****************************************************************************/

typedef struct XndObject {
  VALUE mblock;              /* owner of the primary type and memory block */
  VALUE type;                /* owner of the current type. lives and dies with this obj. */
  xnd_t xnd;                 /* typed view, does not own anything */
} XndObject;

#define XND(xnd_p) (&(((XndObject *)xnd_p)->xnd))
#define TYPE_OWNER(xnd_p) ((((XndObject *)xnd_p)->type))
#define XND_TYPE(xnd_p) (((XndObject *)xnd_p)->xnd.type)
#define XND_INDEX(xnd_p) (((XndObject *)xnd_p)->xnd.index)
#define XND_PTR(xnd_p) (((XndObject *)xnd_p)->xnd.ptr)

#define XND_CHECK_TYPE(xnd) (CLASS_OF(xnd) == cXND)
#define GET_XND(obj, xnd_p) do {                        \
    TypedData_Get_Struct((obj), XndObject,              \
                         &XndObject_type, (xnd_p));     \
  } while (0)
#define MAKE_XND(klass, xnd_p) TypedData_Make_Struct(klass, XndObject, \
                                                    &XndObject_type, xnd_p)
#define WRAP_XND(klass, xnd_p) TypedData_Wrap_Struct(klass, &XndObject_type, xnd_p)

static VALUE XND_size(VALUE self);

/* Allocate an XndObject and return wrapped in a Ruby object. */
static VALUE
XndObject_alloc(void)
{
  XndObject *xnd;

  xnd = ZALLOC(XndObject);

  xnd->mblock = 0;
  xnd->type = 0;
  xnd->xnd.bitmap.data = NULL;
  xnd->xnd.bitmap.size = 0;
  xnd->xnd.bitmap.next = NULL;
  xnd->xnd.index = 0;
  xnd->xnd.type  = NULL;
  xnd->xnd.ptr = NULL;

  return WRAP_XND(cXND, xnd);
}

/* Mark Ruby objects within XndObject. */
static void
XndObject_dmark(void *self)
{
  XndObject *xnd = (XndObject*)self;

  rb_gc_mark(xnd->type);
  rb_gc_mark(xnd->mblock);
}

static void
XndObject_dfree(void *self)
{
  XndObject *xnd = (XndObject*)self;

  rb_xnd_gc_guard_unregister_xnd_mblock(xnd);
  rb_xnd_gc_guard_unregister_xnd_type(xnd);
  xfree(xnd);
}

static size_t
XndObject_dsize(const void *self)
{
  return sizeof(XndObject);
}

static const rb_data_type_t XndObject_type = {
  .wrap_struct_name = "XndObject",
  .function = {
    .dmark = XndObject_dmark,
    .dfree = XndObject_dfree,
    .dsize = XndObject_dsize,
    .reserved = {0,0},
  },
  .parent = 0,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void
XND_from_mblock(XndObject *xnd_p, VALUE mblock)
{
  MemoryBlockObject *mblock_p;

  GET_MBLOCK(mblock, mblock_p);
  
  xnd_p->mblock = mblock;
  xnd_p->type = mblock_p->type;
  xnd_p->xnd = mblock_p->xnd->master;
}

/* Allocator for RubyXND object. Called by Ruby before initialize. */
static VALUE
RubyXND_allocate(VALUE klass)
{
  XndObject *xnd;

  xnd = ZALLOC(XndObject);

  xnd->mblock = 0;
  xnd->type = 0;
  xnd->xnd.bitmap.data = NULL;
  xnd->xnd.bitmap.size = 0;
  xnd->xnd.bitmap.next = NULL;
  xnd->xnd.index = 0;
  xnd->xnd.type  = NULL;
  xnd->xnd.ptr = NULL;

  return WRAP_XND(klass, xnd);
}

static uint32_t
device_flags(VALUE array)
{
  VALUE device, no;

  Check_Type(array, T_ARRAY);
  if (RARRAY_LEN(array) != 2) {
    rb_raise(rb_eTypeError, "device argument must be of the form (device_name, device_no).");
  }

  device = rb_ary_entry(array, 0);
  Check_Type(device, T_STRING);
  if (RTEST(rb_funcall(device, rb_intern("=="), 1, rb_str_new_literal("cuda")))) {
    rb_raise(rb_eValueError, "currently only 'cuda' is supported as a device name.");
  }
    
  no = rb_ary_entry(array, 1);
  Check_Type(no, T_FIXNUM);
  if (FIX2NUM(no) != -1) {
    rb_raise(rb_eValueError, "currently only 'cuda:managed' is supported as a device.");
  }

  return XND_CUDA_MANAGED;
}

/* Initialize a RubyXND object. */
static VALUE
RubyXND_initialize(VALUE self, VALUE type, VALUE data, VALUE device)
{
  VALUE mblock;
  MemoryBlockObject *mblock_p;
  XndObject *xnd_p;
  uint32_t flags = 0;

  if (device != Qnil) {
    flags = device_flags(device);
    if (flags == UINT32_MAX) {
      rb_raise(rb_eValueError, "device ID cannot be handled.");
    }
  }

  mblock = mblock_from_typed_value(type, data, flags);
  GET_MBLOCK(mblock, mblock_p);
  
  rb_xnd_gc_guard_register_mblock_type(mblock_p, type);
    
  GET_XND(self, xnd_p);
  XND_from_mblock(xnd_p, mblock);
  
  rb_xnd_gc_guard_register_xnd_mblock(xnd_p, mblock);
  rb_xnd_gc_guard_register_xnd_type(xnd_p, type);

#ifdef XND_DEBUG
  assert(XND(xnd_p)->type);
  assert(XND(xnd_p)->ptr);
#endif

  return self;
}

static size_t
XND_get_size(VALUE xnd)
{
  size_t size;
  int state;
  VALUE rb_size;

  rb_size = rb_protect(XND_size, xnd, &state);
  size = state ? 0 : NUM2LL(rb_size);

  return size;
}

/*************************** object properties ********************************/

/* Return the ndtypes object of this xnd object. */
static VALUE
XND_type(VALUE self)
{
  XndObject *xnd_p;

  GET_XND(self, xnd_p);

  return xnd_p->type;
}

static VALUE
_XND_value(const xnd_t * const x, const int64_t maxshape)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t * const t = x->type;

#ifdef XND_DEBUG
  assert(t);
  assert(x);
#endif

  if (!ndt_is_concrete(t)) {
    rb_raise(rb_eTypeError, "type must be concrete for returning value.");
  }

  /* bitmap access needs linear index. */
  if (xnd_is_na(x)) {
    return Qnil;
  }

  switch (t->tag) {
  case FixedDim: {
    VALUE array, v;
    int64_t shape, i;

    shape = t->FixedDim.shape;
    if (shape > maxshape) {
      shape = maxshape;
    }

    array = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape-1) {
        rb_ary_store(array, i, xnd_ellipsis());
        break;
      }

      const xnd_t next = xnd_fixed_dim_next(x, i);
      v = _XND_value(&next, maxshape);
      rb_ary_store(array, i, v);
    }

    return array;
  }

  case VarDim: {
    VALUE array, v;
    int64_t start, step, shape;
    int64_t i;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      seterr(&ctx);
      raise_error();
    }
    if (shape > maxshape) {
      shape = maxshape;
    }

    array = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape-1) {
        rb_ary_store(array, i, xnd_ellipsis());
        break;
      }

      xnd_t next = xnd_var_dim_next(x, start, step, i);
      v = _XND_value(&next, maxshape);
      rb_ary_store(array, i, v);
    }

    return array;
  }

  case VarDimElem: {
    int64_t start, step, shape;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      seterr(&ctx);
      raise_error();      
    }

    const int64_t i = adjust_index(t->VarDimElem.index, shape, &ctx);
    if (i < 0) {
      seterr(&ctx);
      raise_error();      
    }

    const xnd_t next = xnd_var_dim_next(x, start, step, i);
    return _XND_value(&next, maxshape);
  }
    
  case Tuple: {
    VALUE tuple, v;
    int64_t shape, i;

    shape = t->Tuple.shape;
    if (shape > maxshape) {
      shape = maxshape;
    }

    tuple = array_new(shape);

    for (i = 0; i < shape; i++) {
      if (i == maxshape) {
        rb_ary_store(tuple, i, xnd_ellipsis());
        break;
      }

      const xnd_t next = xnd_tuple_next(x, i, &ctx);
      if (next.ptr == NULL) {
        seterr(&ctx);
        raise_error();
      }

      v = _XND_value(&next, maxshape);
      rb_ary_store(tuple, i, v);
    }

    return tuple;
  }
    
  case Record: {
    VALUE hash, v;
    int64_t shape, i;

    shape = t->Record.shape;
    if (shape > maxshape) {
      shape = maxshape;
    }

    hash = rb_hash_new();

    for (i = 0; i < shape; ++i) {
      if (i == maxshape - 1) {
        rb_hash_aset(hash, xnd_ellipsis(), xnd_ellipsis());
        break;
      }

      xnd_t next = xnd_record_next(x, i, &ctx);
      if (next.ptr == NULL) {
        seterr(&ctx);
        raise_error();
      }

      v = _XND_value(&next, maxshape);
      rb_hash_aset(hash, rb_str_new2(t->Record.names[i]), v);
    }

    return hash;
  }

  case Union: {
    VALUE array, tag, v;

    array = rb_ary_new2(2);
    const uint8_t i = XND_UNION_TAG(x->ptr);
    tag = rb_str_new2(t->Union.tags[i]);

    rb_ary_store(array, 0, tag);

    const xnd_t next = xnd_union_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }
    v = _XND_value(&next, maxshape);

    rb_ary_store(array, 1, v);

    return array;
  }

  case Ref: {
    xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_value(&next, maxshape);
  }

  case Constr: {
    xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_value(&next, maxshape);
  }

  case Nominal: {
    xnd_t next = xnd_nominal_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();      
    }

    if (t->Nominal.meth->repr != NULL) {
      return t->Nominal.meth->repr(&next, &ctx);
    }

    return _XND_value(&next, maxshape);
  }

  case Bool: {
    bool temp;
    UNPACK_SINGLE(temp, x->ptr, bool, t->flags);
    return INT2BOOL(temp);
  }

  case Int8: {
    int8_t temp;
    UNPACK_SINGLE(temp, x->ptr, int8_t, t->flags);
    return INT2NUM(temp);
  }

  case Int16: {
    int16_t temp;
    UNPACK_SINGLE(temp, x->ptr, int16_t, t->flags);
    return INT2NUM(temp);
  }
    
  case Int32: {
    int32_t temp;
    UNPACK_SINGLE(temp, x->ptr, int32_t, t->flags);
    return INT2NUM(temp);
  }
    
  case Int64: {
    int64_t temp;
    UNPACK_SINGLE(temp, x->ptr, int64_t, t->flags);
    return LL2NUM(temp);
  }

  case Uint8: {
    uint8_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint8_t, t->flags);
    return UINT2NUM(temp);
  }

  case Uint16: {
    uint16_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint16_t, t->flags);
    return UINT2NUM(temp);
  }

  case Uint32: {
    uint32_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint32_t, t->flags);
    return ULL2NUM(temp);
  }

  case Uint64: {
    uint64_t temp;
    UNPACK_SINGLE(temp, x->ptr, uint64_t, t->flags);
    return ULL2NUM(temp);
  }

  case Float16: {
    rb_raise(rb_eNotImpError, "float16 is not implemented.");
  }

  case Float32: {
    float temp = 0.0;

    rb_xnd_unpack_float32(&temp, (unsigned char*)x->ptr, le(t->flags));
    return DBL2NUM(temp);
  }

  case Float64: {
    double temp = 0.0;
    
    rb_xnd_unpack_float64(&temp, (unsigned char*)x->ptr, le(t->flags));
    return DBL2NUM(temp);
  }

  case Complex32: {
    rb_raise(rb_eNotImpError, "complex32 not implemented.");
  }

  case Complex64: {
    float real = 0.0, imag = 0.0;

    rb_xnd_unpack_float32(&real, (unsigned char*)x->ptr, le(t->flags));
    rb_xnd_unpack_float32(&imag, (unsigned char*)x->ptr+4, le(t->flags));

    return rb_complex_new(DBL2NUM(real), DBL2NUM(imag));
  }

  case Complex128: {
    double real = 0.0, imag = 0.0;

    rb_xnd_unpack_float64(&real, (unsigned char*)x->ptr, le(t->flags));
    rb_xnd_unpack_float64(&imag, (unsigned char*)x->ptr+8, le(t->flags));

    return rb_complex_new(DBL2NUM(real), DBL2NUM(imag));
  }

  case FixedString: {
    int64_t codepoints = t->FixedString.size;
    
    switch (t->FixedString.encoding) {
    case Ascii: {
      codepoints = u8_skip_trailing_zero((uint8_t *)x->ptr, codepoints);
      return rb_usascii_str_new(x->ptr, codepoints);
    }
      
    case Utf8: {
      codepoints = u8_skip_trailing_zero((uint8_t *)x->ptr, codepoints);
      return rb_utf8_str_new(x->ptr, codepoints);
    }

    case Utf16: {
      rb_encoding *utf16 = rb_enc_find("UTF-16");      
      codepoints = u16_skip_trailing_zero((uint16_t *)x->ptr, codepoints);

      return rb_enc_str_new(x->ptr, codepoints*2, utf16);
    }

    case Utf32: {
      rb_encoding *utf32 = rb_enc_find("UTF-32");
      codepoints = u32_skip_trailing_zero((uint32_t *)x->ptr, codepoints);
      
      return rb_enc_str_new(x->ptr, codepoints*4, utf32);
    }

    case Ucs2: {
      rb_raise(rb_eNotImpError, "UCS2 encoding not implemented.");
    }

    default: {
      rb_raise(rb_eRuntimeError, "invalid string encoding.");
    } 
    }
  }

  case FixedBytes: {
    return bytes_from_string_and_size(x->ptr, t->FixedBytes.size);
  }

  case String: {
    const char *s = XND_STRING_DATA(x->ptr);
    size_t size = strlen(s);

    return rb_utf8_str_new(s, size);
  }

  case Bytes: {
    char *s = (char *)XND_BYTES_DATA(x->ptr);
    size_t size = s ? strlen(s) : 0;

    return bytes_from_string_and_size(s, size);
  }

  case Categorical: {
    int64_t k;

    UNPACK_SINGLE(k, x->ptr, int64_t, t->flags);

    switch(t->Categorical.types[k].tag) {
    case ValBool: {
      bool temp = t->Categorical.types[k].ValBool;
      return INT2BOOL(temp);
    }

    case ValInt64: {
      int64_t temp = t->Categorical.types[k].ValInt64;
      return LL2NUM(temp);
    }

    case ValFloat64: {
      double temp = t->Categorical.types[k].ValFloat64;
      return DBL2NUM(temp);
    }

    case ValString: {
      const char *temp = t->Categorical.types[k].ValString;
      return rb_str_new2(temp);
    }

    case ValNA: {
      return Qnil;
    }

    default: {
      rb_raise(rb_eRuntimeError, "unexpected category tag.");
    }
    }
  }

  case Char: {
    rb_raise(rb_eNotImpError, "char semantics need to be defined.");
  }

  case Module: {
    rb_raise(rb_eNotImpError, "'Module' type not implemented yet.");
  }

    /* NOT REACHED: intercepted by ndt_is_abstract(). */
  case AnyKind: case SymbolicDim: case EllipsisDim: case Typevar:
  case ScalarKind: case SignedKind: case UnsignedKind: case FloatKind:
  case ComplexKind: case FixedStringKind: case FixedBytesKind:
  case Function:
    rb_raise(rb_eArgError, "unexpected abstract type.");
  }

  rb_raise(rb_eRuntimeError, "invalid type tag %d.", t->tag);
}

/* Return the value of this xnd object. Aliased to to_a. */
static VALUE
XND_value(VALUE self)
{
  XndObject *xnd_p;

  GET_XND(self, xnd_p);

  return _XND_value(XND(xnd_p), INT64_MAX);
}

/*************************** slicing functions ********************************/

#define KEY_INDEX 1
#define KEY_FIELD 2
#define KEY_SLICE 4
#define KEY_ERROR 128

/* 
   @param src_p Pointer to the source XND object from which view is being created.
   @param x Metadata for creating the view.
 */
static VALUE
RubyXND_view_move_type(XndObject *src_p, xnd_t *x)
{
  XndObject *view_p;
  VALUE type, view;

  type = rb_ndtypes_from_type(x->type);
  ndt_decref(x->type);
  
  view = XndObject_alloc();
  GET_XND(view, view_p);

  view_p->mblock = src_p->mblock;
  view_p->type = type;
  view_p->xnd = *x;

  //  rb_xnd_gc_guard_register(view_p, view_p->mblock, type);

  return view;
}

/* Convert a single Ruby object index into a form that XND can understand. 
   
   @param *key
   @param obj
   @param size Size of object. 1 for all scalars and the actual size otherwise.
*/
static uint8_t
convert_single(xnd_index_t *key, VALUE obj, size_t size)
{
  if (RB_TYPE_P(obj, T_FIXNUM)) {
    int64_t i = NUM2LL(obj);

    key->tag = Index;
    key->Index = i;

    return KEY_INDEX;
  }
  else if (RB_TYPE_P(obj, T_STRING)) {
    const char *s = StringValuePtr(obj);

    key->tag = FieldName;
    key->FieldName = s;

    return KEY_FIELD;
  }
  else if (CLASS_OF(obj) == rb_cRange) {
    if (size == 0) {
      rb_raise(rb_eIndexError, "Cannot use Range on this type.");
    };
    
    long long begin, end, step;

    rb_range_unpack(obj, &begin, &end, &step, size);
    key->tag = Slice;
    key->Slice.start = begin;
    key->Slice.stop = end;
    key->Slice.step = step;

    return KEY_SLICE;
  }
  // case of INF (infinite range syntax sugar)
  else if (RB_TYPE_P(obj, T_FLOAT)) {
    double value = RFLOAT_VALUE(obj);

    if (isinf(value)) {
      key->tag = Slice;
      key->Slice.start = 0;
      key->Slice.stop = INT64_MAX;
      key->Slice.step = 1;

      return KEY_SLICE;
    }
    else {
      rb_raise(rb_eArgError, "wrong object specified in index.");
    }
  }
  else {
    rb_raise(rb_eArgError, "wrong object specified in index.");
  }
}

static uint8_t
convert_key(xnd_index_t *indices, int *len, int argc, VALUE *argv, size_t size)
{
  uint8_t flags = 0;
  VALUE x;

  if (argc > 1) {
    if (argc > NDT_MAX_DIM) {
      rb_raise(rb_eArgError, "too many indices %d.", argc);
    }

    for (unsigned int i = 0; i < argc; i++) {
      x = argv[i];
      flags |= convert_single(indices+i, x, size);
      if (flags & KEY_ERROR) {
        return KEY_ERROR;
      }
    }

    *len = argc;
    return flags;
  }
  else if (argc == 1 && RB_TYPE_P(argv[0], T_ARRAY)) { // args as an array
    *len = RARRAY_LEN(argv[0]);

    for (int i = 0; i < *len; i++) {
      VALUE args[1] = { INT2NUM(i) };
      flags |= convert_single(indices+i, rb_ary_aref(1, args, argv[0]), size);
      if (flags & KEY_ERROR) {
        return KEY_ERROR;
      }
    }
    return flags;
  }

  *len = 1;
  return convert_single(indices, argv[0], size);
}

/* Implement the #[] Ruby method. */
static VALUE
XND_array_aref(int argc, VALUE *argv, VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  xnd_index_t indices[NDT_MAX_DIM];
  xnd_t x;
  int len;
  uint8_t flags;
  XndObject *xnd_p;
  size_t size;

  if (argc == 0) {
    rb_raise(rb_eArgError, "expected atleast one argument for #[].");
  }

  GET_XND(self, xnd_p);
  size = XND_get_size(self);
  
  flags = convert_key(indices, &len, argc, argv, size);
  if (flags & KEY_ERROR) {
    rb_raise(rb_eArgError, "something is wrong with the array key.");
  }
  
  x = xnd_subscript(&xnd_p->xnd, indices, len, &ctx);
  if (x.ptr == NULL) {
    seterr(&ctx);
    raise_error();
  }

  return RubyXND_view_move_type(xnd_p, &x);
}

/* Compare between data of an XND object and Ruby object. */
static VALUE
convert_compare(VALUE self, VALUE other)
{
  VALUE vcmp = XND_value(self);

  return rb_funcall(vcmp, rb_intern("=="), 1, other);
}

/* Implementation for #== method. 

   @param other Other Ruby object to compare with.
   @return VALUE [TrueClass|FalseClass]
*/
static VALUE
XND_eqeq(VALUE self, VALUE other)
{
  NDT_STATIC_CONTEXT(ctx);
  XndObject *left_p, *right_p;
  int r;

  if (!XND_CHECK_TYPE(other)) {
    return convert_compare(self, other);
  }

  GET_XND(self, left_p);
  GET_XND(other, right_p);

  r = xnd_equal(XND(left_p), XND(right_p), &ctx);

  if (r == 1) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

/* Implement Ruby spaceship operator. */
static VALUE
XND_spaceship(VALUE self, VALUE other)
{
  rb_raise(rb_eNotImpError, "spaceship not implemented yet.");

  return Qnil;
}

/* XND#strict_equal */
static VALUE
XND_strict_equal(VALUE self, VALUE other)
{
  NDT_STATIC_CONTEXT(ctx);
  XndObject *left_p, *right_p;
  int r;

  if (!XND_CHECK_TYPE(other)) {
    rb_raise(rb_eArgError, "argument type has to be XND.");
  }
  
  GET_XND(self, left_p);
  GET_XND(other, right_p);

  r = xnd_strict_equal(XND(left_p), XND(right_p), &ctx);
  if (r < 0) {
    seterr(&ctx);
    raise_error();
  }

  if (r) {
    return Qtrue;
  }
  else {
    return Qfalse;
  }
}

static size_t
_XND_size(const xnd_t *x)
{
  NDT_STATIC_CONTEXT(ctx);
  const ndt_t *t = x->type;

  if (!ndt_is_concrete(t)) {
    rb_raise(rb_eTypeError, "NDT must be concrete to get size.");
  }

  if (t->ndim > 0 && ndt_is_optional(t)) {
    rb_raise(rb_eNotImpError, "optional dimensions are not supported.");
  }

  if (xnd_is_na(x)) {
    return 0;
  }

  switch (t->tag) {
  case FixedDim: {
    return safe_downcast(t->FixedDim.shape);
  }

  case VarDim: {
    int64_t start, step, shape;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      seterr(&ctx);
      raise_error();
    }

    return safe_downcast(shape);
  }

  case VarDimElem: {
    NDT_STATIC_CONTEXT(ctx);
    int64_t start, step, shape;

    shape = ndt_var_indices(&start, &step, t, x->index, &ctx);
    if (shape < 0) {
      seterr(&ctx);
      raise_error();
    }

    const int64_t i = adjust_index(t->VarDimElem.index, shape, &ctx);
    if (i < 0) {
      seterr(&ctx);
      raise_error();
    }

    const xnd_t next = xnd_var_dim_next(x, start, step, i);
    return _XND_size(&next);
  }

  case Tuple: {
    return safe_downcast(t->Tuple.shape);
  }

  case Record: {
    return safe_downcast(t->Record.shape);
  }

  case Union: {
    const xnd_t next = xnd_union_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_size(&next);
  }

  case Ref: {
    const xnd_t next = xnd_ref_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_size(&next);
  }

  case Constr: {
    const xnd_t next = xnd_constr_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_size(&next);
  }

  case Nominal: {
    const xnd_t next = xnd_nominal_next(x, &ctx);
    if (next.ptr == NULL) {
      seterr(&ctx);
      raise_error();
    }

    return _XND_size(&next); 
  }

  default: {
    rb_raise(rb_eNoMethodError, "This type has no size method.");
  }
  }  
}

/* Implement XND#size. */
static VALUE
XND_size(VALUE self)
{
  XndObject *self_p;
    
  GET_XND(self, self_p);
  return ULL2NUM(_XND_size(XND(self_p)));
}

/* Implement #[]= */
static VALUE
XND_array_store(int argc, VALUE *argv, VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  xnd_index_t indices[NDT_MAX_DIM];
  xnd_t x;
  int free_type = 0, ret, len;
  uint8_t flags;
  XndObject *self_p, *value_p;
  MemoryBlockObject *self_mblock_p;
  size_t size;
  VALUE value;

  if (argc < 2) {
    rb_raise(rb_eArgError, "wrong number of arguments (given %d expected atleast 2)."
             , argc);
  }

  GET_XND(self, self_p);
  
  size = XND_get_size(self);
  flags = convert_key(indices, &len, argc-1, argv, size);
  if (flags & KEY_ERROR) {
    rb_raise(rb_eIndexError, "wrong kind of key in []=");
  }

  x = xnd_subscript(&self_p->xnd, indices, len, &ctx);
  if (x.ptr == NULL) {
    seterr(&ctx);
    raise_error();
  }

  value = argv[argc-1];

  if (XND_CHECK_TYPE(value)) {
    GET_XND(value, value_p);
    GET_MBLOCK(self_p->mblock, self_mblock_p);
    
    ret = xnd_copy(&x, XND(value_p), self_mblock_p->xnd->flags, &ctx);
    if (ret < 0) {
      seterr(&ctx);
      raise_error();
    }
  }
  else {
    ret = mblock_init(&x, value);
  }

  ndt_decref(x.type);

  return value;
}

/* Implement XND#each */
static VALUE
XND_each(VALUE self)
{
  
}

/* Implement XND#short_value */
static VALUE
XND_short_value(VALUE self, VALUE maxshape)
{
  if (maxshape == Qnil) {
    return XND_value(self);
  }
  else {
    long max = NUM2LONG(maxshape);
    XndObject *self_p;

    GET_XND(self, self_p);

    if (max < 0) {
      rb_raise(rb_eArgError, "maxshape must be positive.");
    }

    return _XND_value(XND(self_p), max);
  }
}

static VALUE
XND_copy_contiguous(int argc, VALUE *argv, VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  XndObject *self_p;
  NdtObject *dtype_p;
  VALUE dtype;
  VALUE dest;
  const ndt_t *t;

  if (argc == 1) {
    dtype = argv[0];
  }
  else if (argc == 0) {
    dtype = Qnil;
  }
  else {
    rb_raise(rb_eArgError, "copy_contiguous can accept only one arg for dtype.");
  }

  GET_XND(self, self_p);

  if (dtype != Qnil) {
    if (!rb_ndtypes_check_type(dtype)) {
      rb_raise(rb_eTypeError, "dtype must be of type ndtypes.");
    }
    /* t = ndt_copy_contiguous_dtype(XND_TYPE(self_p), */
    /*                               rb_ndtypes_get_ndt_object(dtype), */
    /*                               XND_INDEX(self_p), &ctx); */
  }
  else {
    t = ndt_copy_contiguous(XND_TYPE(self_p), XND_INDEX(self_p), &ctx);
  }

  if (t == NULL) {
    seterr(&ctx);
  }

}

static VALUE
XND_serialize(VALUE self)
{
  NDT_STATIC_CONTEXT(ctx);
  bool overflow = false;
  const xnd_t *x;
  const ndt_t* t;
  VALUE result;
  char *cp, *s;
  int64_t tlen, size;
  XndObject *self_p;

  GET_XND(self, self_p);
  x = XND(self_p);
  t = XND_TYPE(self_p);

  if (!ndt_is_pointer_free(t)) {
    rb_raise(rb_eNotImpError, "serializing memory blocks with pointers is not implemented.");
  }

  if (ndt_is_optional(t) || ndt_subtree_is_optional(t)) {
    rb_raise(rb_eNotImpError, "serializing bitmaps is not implemented.");
  }

  if (!ndt_is_c_contiguous(t) && !ndt_is_f_contiguous(t) &&
      !ndt_is_var_contiguous(t)) {
    rb_raise(rb_eNotImpError, "serializing non-contiguos memory blocks is not implemented.");
  }

  tlen = ndt_serialize(&s, t, &ctx);
  if (tlen < 0) {
    seterr(&ctx);
    raise_error();
  }

  size = ADDi64(t->datasize, tlen, &overflow);
  size = ADDi64(size, 8, &overflow);
  if (overflow) {
    ndt_free(s);
    /* FIXME: maybe create a new OverflowError for this. */
    rb_raise(rb_eTypeError, "too large to serialize.");
  }

  result = rb_str_new(NULL, size);
  cp = RSTRING_PTR(result);

  char *ptr = x->ptr;
  if (t->ndim != 0) {
    ptr = x->ptr + x->index * t->Concrete.FixedDim.itemsize;
  }

  memcpy(cp, ptr, t->datasize); cp += t->datasize;
  memcpy(cp, s, tlen); cp += tlen;
  memcpy(cp, &t->datasize, 8);
  ndt_free(s);

  return result;
}
  

/*************************** Singleton methods ********************************/


/* Implement XND.deserialize */
static VALUE
XND_s_deserialize(VALUE klass, VALUE v)
{
  NDT_STATIC_CONTEXT(ctx);
  VALUE mblock, self;
  bool overflow = false;
  int64_t mblock_size;
  MemoryBlockObject *mblock_p;
  XndObject *self_p;

  Check_Type(v, T_STRING);

  const int64_t size = RSTRING_LEN(v);
  if (size < 8) {
    goto invalid_format;
  }

  const char *s = RSTRING_PTR(v);
  memcpy(&mblock_size, s+size-8, 8);
  if (mblock_size < 0) {
    goto invalid_format;
  }

  const int64_t tmp = ADDi64(mblock_size, 8, &overflow);
  const int64_t tlen = size - tmp;
  if (overflow || tlen < 0) {
    goto invalid_format;
  }

  const ndt_t *t = ndt_deserialize(s+mblock_size, tlen, &ctx);
  if (t == NULL) {
    seterr(&ctx);
  }

  if (t->datasize != mblock_size) {
    goto invalid_format;
  }

  VALUE type = rb_ndtypes_from_type(t);
  ndt_decref(t);

  mblock = mblock_empty(type, XND_OWN_EMBEDDED);
  GET_MBLOCK(mblock, mblock_p);
  rb_xnd_gc_guard_register_mblock_type(mblock_p, type);
  
  memcpy(mblock_p->xnd->master.ptr, s, mblock_size);

  self = XndObject_alloc();
  GET_XND(self, self_p);
  XND_from_mblock(self_p, mblock);
    
  rb_xnd_gc_guard_register_xnd_mblock(self_p, mblock);
  rb_xnd_gc_guard_register_xnd_type(self_p, type);

  return self;
  
 invalid_format:
  rb_raise(rb_eValueError, "invalid format for xnd deserialization.");
}

static VALUE
RubyXND_s_empty(VALUE klass, VALUE origin_type, VALUE device)
{
  XndObject *self_p;
  MemoryBlockObject *mblock_p;
  VALUE self, mblock;
  VALUE type;
  VALUE mblock_type, xnd_type;
  uint32_t flags = 0;

  self = XndObject_alloc();
  GET_XND(self, self_p);

  if (device != Qnil) {
    flags = device_flags(device);
  }
    
  type = rb_ndtypes_from_object(origin_type);
  mblock = mblock_empty(type, flags);
  
  XND_from_mblock(self_p, mblock);
  GET_MBLOCK(mblock, mblock_p);
  
  rb_xnd_gc_guard_register_xnd_mblock(self_p, mblock);
  rb_xnd_gc_guard_register_xnd_type(self_p, type);
  rb_xnd_gc_guard_register_mblock_type(mblock_p, type);

  return self;
}

/*************************** C-API ********************************/

size_t
rb_xnd_hash_size(VALUE hash)
{
  Check_Type(hash, T_HASH);

  return NUM2ULL(rb_funcall(hash, rb_intern("size"), 0, NULL));
}

/* FIXME: Find a better way to access real/imag parts of complex number.
   This is too slow.
*/
int
rb_xnd_get_complex_values(VALUE comp, double *real, double *imag)
{
  Check_Type(comp, T_COMPLEX);
  
  *real = NUM2DBL(rb_funcall(comp, rb_intern("real"), 0, NULL));
  *imag = NUM2DBL(rb_funcall(comp, rb_intern("imag"), 0, NULL));

  return 0;
}

/* Return true if obj is of type XND. */
int
rb_xnd_check_type(VALUE obj)
{
  return XND_CHECK_TYPE(obj);
}

/* Return the xnd_t internal object within this Ruby object. */
const xnd_t *
rb_xnd_const_xnd(VALUE xnd)
{
  XndObject *xnd_p;

  GET_XND(xnd, xnd_p);

  return &((XndObject *)xnd_p)->xnd;
}

/* Creae a new XND object from xnd_t type.  */
VALUE
rb_xnd_from_xnd(xnd_t *x)
{
  VALUE mblock, xnd, type;
  XndObject *xnd_p;
  MemoryBlockObject *mblock_p;
  
  mblock = mblock_from_xnd(x);
  xnd = XndObject_alloc();
  GET_XND(xnd, xnd_p);
  GET_MBLOCK(mblock, mblock_p);
  type = mblock_p->type;
  
  XND_from_mblock(xnd_p, mblock);
  
  rb_xnd_gc_guard_register_xnd_mblock(xnd_p, mblock);
  rb_xnd_gc_guard_register_xnd_type(xnd_p, type);
  rb_xnd_gc_guard_register_mblock_type(mblock_p, type);

  return xnd;
}

/* Create an XND object of type ndt_t */
VALUE
rb_xnd_empty_from_type(const ndt_t *t, uint32_t flags)
{
  MemoryBlockObject *mblock_p;
  XndObject *xnd_p;
  VALUE type, mblock, xnd;

  type = rb_ndtypes_from_type(t);
  mblock = mblock_empty(type, flags);
  xnd = XndObject_alloc();

  GET_XND(xnd, xnd_p);

  XND_from_mblock(xnd_p, mblock);

  rb_xnd_gc_guard_register_xnd_mblock(xnd_p, mblock);
  rb_xnd_gc_guard_register_xnd_type(xnd_p, type);
  rb_xnd_gc_guard_register_mblock_type(mblock_p, type);

  return xnd;
}

VALUE
rb_xnd_get_type(void)
{
  
}

/*
 * This function handles two common view cases:
 *
 *   a) A pristine view that owns everything, including new memory.
 *   b) A view that owns its type after xnd_subscript().
 */
/* VALUE */
/* rb_xnd_from_xnd_view(xnd_view_t *x) */
/* { */
/*   if (x->obj == NULL && (x->flags & XND_OWN_ALL) == XND_OWN_ALL) { */
/*     VALUE type = rb_xnd_get_type(); */
/*   } */
/* } */

void Init_ruby_xnd(void)
{
  NDT_STATIC_CONTEXT(ctx);
  static int initialized = 0;

  if (!initialized) {
    if (xnd_init_float(&ctx) < 0) {
      seterr(&ctx);
      raise_error();
    }

    if (!ndt_exists()) {
      rb_raise(rb_eLoadError, "NDT does not exist.");
    }

    initialized = 1;
  }
  
  /* init classes */
  cRubyXND = rb_define_class("RubyXND", rb_cObject);
  cXND = rb_define_class("XND", cRubyXND);
  cRubyXND_MBlock = rb_define_class_under(cRubyXND, "MBlock", rb_cObject);
  mRubyXND_GCGuard = rb_define_module_under(cRubyXND, "GCGuard");

  /* errors */
  rb_eValueError = rb_define_class("ValueError", rb_eRuntimeError);
  
  /* initializers */
  rb_define_alloc_func(cRubyXND, RubyXND_allocate);
  rb_define_method(cRubyXND, "initialize", RubyXND_initialize, 3);

  /* singleton methods */
  rb_define_singleton_method(cRubyXND, "empty", RubyXND_s_empty, 2);
  rb_define_singleton_method(cXND, "deserialize", XND_s_deserialize, 1);
  
  /* instance methods */
  rb_define_method(cXND, "type", XND_type, 0);
  rb_define_method(cXND, "value", XND_value, 0);
  rb_define_method(cXND, "[]", XND_array_aref, -1);
  rb_define_method(cXND, "[]=", XND_array_store, -1);
  rb_define_method(cXND, "==", XND_eqeq, 1);
  rb_define_method(cXND, "serialize", XND_serialize, 0);
  rb_define_method(cXND, "copy_contiguous", XND_copy_contiguous, -1);
  
  //  rb_define_method(cXND, "!=", XND_neq, 1);
  rb_define_method(cXND, "<=>", XND_spaceship, 1);
  rb_define_method(cXND, "strict_equal", XND_strict_equal, 1);
  rb_define_method(cXND, "size", XND_size, 0);
  rb_define_method(cXND, "short_value", XND_short_value, 1);

  /* iterators */
  rb_define_method(cXND, "each", XND_each, 0);

  /* GC guard */
  rb_xnd_init_gc_guard();

#ifdef XND_DEBUG
  run_float_pack_unpack_tests();
  rb_define_const(cRubyXND, "XND_DEBUG", Qtrue);
#else
  rb_define_const(cRubyXND, "XND_DEBUG", Qnil);
#endif
}
