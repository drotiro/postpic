#GraphicsMagick's configuration info
GMCONF = GraphicsMagick-config
GMFLAGS = `${GMCONF} --cppflags --cflags`
GMLIBS = `${GMCONF} --libs`

# info for pgxs
PG_CPPFLAGS = ${GMFLAGS}
SHLIB_LINK  = ${GMLIBS}
PG_CONFIG   = pg_config

MODULES    = $(patsubst %.c,%,$(wildcard src/*.c))

DATA       = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))
TESTS      = $(wildcard test/sql/*.sql)
DOCS       = $(wildcard doc/*.md)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
