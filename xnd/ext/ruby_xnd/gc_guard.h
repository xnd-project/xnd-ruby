/* Header file containing various functions for GC guard table. */

#ifndef GC_GUARD_H
#define GC_GUARD_H

#include "ruby_xnd_internal.h"

void rb_xnd_gc_guard_unregister(XndObject *);
void rb_xnd_gc_guard_register(XndObject *, VALUE, VALUE);
void rb_xnd_gc_guard_register_mblock(MemoryBlockObject *mblock, VALUE type);
void rb_xnd_gc_guard_unregsiter_mblock(MemoryBlockObject *mblock);
void rb_xnd_init_gc_guard(void);

#endif  /* GC_GUARD_H */
