# GraphicsMagick's configuration info
GMCONF = GraphicsMagick-config
GMFLAGS = `${GMCONF} --cppflags --cflags`
GMLIBS = `${GMCONF} --libs`

# info for pgxs
OBJS = postpic.o
MODULE_big = postpic
DATA = postpic.sql
PG_CPPFLAGS = ${GMFLAGS}
SHLIB_LINK = ${GMLIBS}

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
