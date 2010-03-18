# Variables
SUBDIRS = server utils

# Targets
postpic: check
	@$(foreach i,$(SUBDIRS), make -C $i || exit 1; )

jdbc: jcheck
	@ant -f jdbc/build.xml

examples: jdbc
	@ant -f examples/PostPicSQL/build.xml

clean:
	@$(foreach i,$(SUBDIRS), make -C $i clean ;)

jdbc-clean:
	@ant -f jdbc/build.xml clean

install: postpic
	@$(foreach i,$(SUBDIRS), make -C $i install; )

all: postpic jdbc
clean-all: clean jdbc-clean

jcheck:
	@echo " *** Checking for required programs"
	@(echo -n " ant                  :" && which ant) || (echo " missing!"  && exit 1)

check:
	@echo " *** Checking for required programs"
	@(echo -n " GraphicsMagick-config: " && which GraphicsMagick-config) || (echo " missing!"  && exit 1)
	@(echo -n " pg_config            : " && which pg_config) || (echo " missing!"  && exit 1)

.PHONY:	jdbc jdbc-clean check