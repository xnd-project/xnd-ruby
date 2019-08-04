/* Header file containing various functions for GC guard table. */

#ifndef GC_GUARD_H
#define GC_GUARD_H

#include "ruby_xnd_internal.h"

void rb_xnd_gc_guard_register_xnd_mblock(XndObject *xnd, VALUE mblock);
void rb_xnd_gc_guard_register_xnd_type(XndObject *xnd, VALUE type);
void rb_xnd_gc_guard_register_mblock_type(MemoryBlockObject *mblock, VALUE type);

void rb_xnd_gc_guard_unregister_xnd_mblock(XndObject *mblock);
void rb_xnd_gc_guard_unregister_xnd_type(XndObject *mblock);
void rb_xnd_gc_guard_unregister_mblock_type(MemoryBlockObject *mblock);

void rb_xnd_init_gc_guard(void);

#endif  /* GC_GUARD_H */
