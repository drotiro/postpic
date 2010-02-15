CFLAGS = -I `pg_config --includedir`
LIBS = -L `pg_config --libdir` -lpq

UTILS = postpic_import


all:	${UTILS}


postpic_import:	postpic_import.o
	gcc -o $@ $< ${LIBS}

.c.o:
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f ${UTILS} *.o
