#GraphicsMagick's configuration info
GMCONF = GraphicsMagick-config
GMFLAGS = `${GMCONF} --cppflags --cflags`
GMLIBS = `${GMCONF} --libs`

# info for pgxs
PG_CPPFLAGS = ${GMFLAGS}
SHLIB_LINK  = ${GMLIBS}
PG_CONFIG   = pg_config

EXTENSION  = postpic
EXTVERSION = $(shell grep default_version $(EXTENSION).control | \\
	     sed -e "s/default_version[[:space:]]*=[[:space:]]*'\\([^']*\\)'/\\1/")

MODULES    = $(patsubst %.c,%,$(wildcard src/*.c))

DATA       = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))
TESTS      = $(wildcard test/sql/*.sql)
DOCS       = $(wildcard doc/*.md)

PG91         = $(shell $(PG_CONFIG) --version | grep -qE " 8\\.| 9\\.0" && echo no || echo yes)

ifeq ($(PG91),yes)
	all: sql/$(EXTENSION)--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	    cp $< $@

DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN = sql/$(EXTENSION)--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
