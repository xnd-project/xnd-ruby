
CC = gcc
LD = gcc

CONFIGURE_CFLAGS = -I.. -Wall -Wextra -std=c11 -pedantic -O2 -g
NDT_CFLAGS = $(strip $(CONFIGURE_CFLAGS) $(CFLAGS))
NDT_CFLAGS_SHARED = $(NDT_CFLAGS) -fPIC

CONFIGURE_COV_CFLAGS = -I.. -Wall -Wextra -std=c11 -pedantic -O0 -g -fno-inline -fprofile-arcs -ftest-coverage
ifeq ($(MAKECMDGOALS), coverage)
  NDT_CFLAGS = $(strip $(CONFIGURE_COV_CFLAGS) $(CFLAGS))
endif


OBJS = serialize.o deserialize.o
SHARED_OBJS = .objs/serialize.o .objs/deserialize.o


default: $(OBJS) $(SHARED_OBJS)


serialize.o:\
Makefile serialize.c ../ndtypes.h
	$(CC) $(NDT_CFLAGS) -c serialize.c

.objs/serialize.o:\
Makefile serialize.c ../ndtypes.h
	$(CC) $(NDT_CFLAGS_SHARED) -c serialize.c -o .objs/serialize.o

deserialize.o:\
Makefile deserialize.c ../ndtypes.h
	$(CC) $(NDT_CFLAGS) -c deserialize.c

.objs/deserialize.o:\
Makefile deserialize.c ../ndtypes.h
	$(CC) $(NDT_CFLAGS_SHARED) -c deserialize.c -o .objs/deserialize.o


FORCE:

clean: FORCE
	rm -f *.o *.so *.gch *.gcda *.gcno *.gcov *.dyn *.dpi *.lock
	cd .objs && rm -f *.o *.so *.gch *.gcda *.gcno *.gcov *.dyn *.dpi *.lock

distclean: clean
	rm -f Makefile


