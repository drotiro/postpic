SUBDIRS = server utils

postpic:
	$(foreach i,$(SUBDIRS), make -C $i; )

jdbc:
	ant -f jdbc/build.xml
	
clean:
	$(foreach i,$(SUBDIRS), make -C $i clean ;)

jdbc-clean:
	ant -f jdbc/build.xml clean

install: postpic
	$(foreach i,$(SUBDIRS), make -C $i install)

all: postpic jdbc
clean-all: clean jdbc-clean

.PHONY:	jdbc jdbc-clean