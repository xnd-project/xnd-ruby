
SRCDIR = ..

CC = gcc

LIBSTATIC = libndtypes.a
LIBSHARED = libndtypes.so.0.2.0dev3

CONFIGURE_CFLAGS = -I.. -Wall -Wextra -std=c11 -pedantic -O2 -g
NDT_CFLAGS = $(strip $(CONFIGURE_CFLAGS) $(CFLAGS))

CONFIGURE_COV_CFLAGS = -I.. -Wall -Wextra -std=c11 -pedantic -O0 -g -fno-inline -fprofile-arcs -ftest-coverage
CONFIGURE_COV_LDFLAGS = -shared -Wl,-soname,libndtypes.so.0 -fprofile-arcs
ifeq ($(MAKECMDGOALS), coverage)
  NDT_CFLAGS = $(strip $(CONFIGURE_COV_CFLAGS) $(CFLAGS))
  NDT_LDFLAGS = $(strip $(CONFIGURE_COV_LDFLAGS) $(LDFLAGS))
endif


default: runtest runtest_shared

coverage: runtest runtest_shared


runtest:\
Makefile runtest.c alloc_fail.c test_parse.c test_parse_error.c test_parse_roundtrip.c \
test_indent.c test_typecheck.c test_numba.c test_typedef.c test_match.c test_record.c \
test_buffer.c test.h alloc_fail.h \
$(SRCDIR)/ndtypes.h $(SRCDIR)/$(LIBSTATIC)
	$(CC) -DTEST_ALLOC $(NDT_CFLAGS) -o runtest runtest.c \
            alloc_fail.c test_parse.c test_parse_error.c test_parse_roundtrip.c \
            test_indent.c test_typedef.c test_match.c test_typecheck.c test_numba.c \
            test_record.c test_array.c test_buffer.c $(SRCDIR)/$(LIBSTATIC)

runtest_shared:\
Makefile runtest.c alloc_fail.c test_parse.c test_parse_error.c test_parse_roundtrip.c \
test_indent.c test_typecheck.c test_numba.c test_typedef.c test_match.c test_record.c \
test_buffer.c test.h alloc_fail.h \
$(SRCDIR)/ndtypes.h $(SRCDIR)/$(LIBSHARED)
	$(CC) -L$(SRCDIR) -DTEST_ALLOC $(NDT_CFLAGS) -o runtest_shared runtest.c \
            alloc_fail.c test_parse.c test_parse_error.c test_parse_roundtrip.c \
            test_indent.c test_typedef.c test_match.c test_typecheck.c test_numba.c \
            test_record.c test_array.c test_buffer.c -lndtypes


FORCE:

clean: FORCE
	rm -f *.o *.gch *.gcda *.gcno *.gcov *.dyn *.dpi *.lock
	rm -f runtest runtest_shared

distclean: clean
	rm -rf Makefile


