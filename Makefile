# Variables
SUBDIRS = server utils

# Targets
postpic: check
	@$(foreach i,$(SUBDIRS), make -C $i || exit 1; )

clean:
	@$(foreach i,$(SUBDIRS), make -C $i clean ;)

install: postpic
	@$(foreach i,$(SUBDIRS), make -C $i install; )

check:
	@echo " *** Checking for required programs"
	@(echo -n " GraphicsMagick-config: " && which GraphicsMagick-config) || (echo " missing!"  && exit 1)
	@(echo -n " pg_config            : " && which pg_config) || (echo " missing!"  && exit 1)

.PHONY:	install check
