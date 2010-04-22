CFLAGS = -I `pg_config --includedir`
LIBS = -L `pg_config --libdir` -lpq
PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

UTILS = postpic_import postpic_export
postpic_import_OBJS = postpic_import.o postpic_utils.o
postpic_export_OBJS = postpic_export.o postpic_utils.o

all:	${UTILS}
install:	all
	cp ${UTILS} ${BINDIR}

.SECONDEXPANSION:
${UTILS}:	$$($$@_OBJS)
	gcc -o $@ ${$@_OBJS} ${LIBS}

postpic_import.o:	postpic_import.c postpic_utils.h
postpic_export.o:	postpic_export.c postpic_utils.h
postpic_utils.o:	postpic_utils.c postpic_utils.h

.c.o:
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f ${UTILS} *.o
