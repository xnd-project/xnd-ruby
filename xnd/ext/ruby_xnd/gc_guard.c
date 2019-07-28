/* Functions useful for interfacing shared rbuf objects with the Ruby GC. */
/* Author: Sameer Deshmukh (@v0dro) */
#include "ruby_xnd_internal.h"

#define GC_GUARD_TABLE_NAME "@__gc_guard_table"
#define GC_GUARD_MBLOCK "@__gc_guard_mblock"
#define GC_GUARD_TYPE "@__gc_guard_type"

static ID id_gc_guard_table;
static ID id_gc_guard_mblock;
static ID id_gc_guard_type;

/* Unregister an NDT object-rbuf pair from the GC guard. */
void
rb_xnd_gc_guard_unregister(XndObject *xnd)
{
  VALUE table = rb_ivar_get(mRubyXND_GCGuard, id_gc_guard_table);
  rb_hash_delete(table, PTR2NUM(xnd));
}

void rb_xnd_gc_guard_register_type(XndObject *xnd, VALUE type)
{
  VALUE table = rb_ivar_get(mRubyXND_GCGuard, id_gc_guard_type);
  if (table == Qnil) {
    rb_raise(rb_eLoadError, "GC guard not initialized.");
  }
  
  rb_hash_aset(table, PTR2NUM(xnd), type);
}

/* Register a XND-mblock pair in the GC guard.  */
void
rb_xnd_gc_guard_register(XndObject *xnd, VALUE mblock)
{
  VALUE table = rb_ivar_get(mRubyXND_GCGuard, id_gc_guard_table);
  if (table == Qnil) {
    rb_raise(rb_eLoadError, "GC guard not initialized.");
  }
  
  rb_hash_aset(table, PTR2NUM(xnd), mblock);
}

void
rb_xnd_gc_guard_unregsiter_mblock(MemoryBlockObject *mblock)
{
  VALUE table = rb_ivar_get(mRubyXND_GCGuard, id_gc_guard_mblock);
  rb_hash_delete(table, PTR2NUM(mblock));
}

void
rb_xnd_gc_guard_register_mblock(MemoryBlockObject *mblock, VALUE type)
{
  VALUE table = rb_ivar_get(mRubyXND_GCGuard, id_gc_guard_mblock);
  if (table == Qnil) {
    rb_raise(rb_eLoadError, "Mblock guard not initialized.");    
  }
  rb_hash_aset(table, PTR2NUM(mblock), type);
}

/* Initialize the global GC guard table. klass is a VALUE reprensenting NDTypes class. */
void
rb_xnd_init_gc_guard(void)
{
  id_gc_guard_table = rb_intern(GC_GUARD_TABLE_NAME);
  rb_ivar_set(mRubyXND_GCGuard, id_gc_guard_table, rb_hash_new());

  id_gc_guard_mblock = rb_intern(GC_GUARD_MBLOCK);
  rb_ivar_set(mRubyXND_GCGuard, id_gc_guard_mblock, rb_hash_new());

  id_gc_guard_type = rb_intern(GC_GUARD_TYPE);
  rb_ivar_set(mRubyXND_GCGuard, id_gc_guard_type, rb_hash_new());  
}

