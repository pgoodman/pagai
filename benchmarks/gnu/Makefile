# List of subdirectories:
BENCHDIR=.

-include config.mk

SUBDIRS=$(shell echo $(BENCHDIR)/*/)
TIMESTAMPS=$(SUBDIRS:%=%/timestamp.txt)
CTIMESTAMPS=$(SUBDIRS:%=%/ctimestamp.txt)
NTIMESTAMPS=$(SUBDIRS:%=%/ntimestamp.txt)

all: $(TIMESTAMPS)

compare: $(CTIMESTAMPS)

ncompare: $(NTIMESTAMPS)

%/timestamp.txt:
		cd $* && $(MAKE) -f $(PWD)/Makefile.model test

%/ctimestamp.txt:
		cd $* && $(MAKE) -f $(PWD)/Makefile.model compare

%/ntimestamp.txt:
		cd $* && $(MAKE) -f $(PWD)/Makefile.model ncompare

clean:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model clean && cd ..);\
	done ;

nclean:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model nclean && cd ..);\
	done ;

cclean:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model cclean && cd ..);\
	done ;

cdiff:
	./diff_analyzer.sh $(BENCHDIR)

ndiff:
	./diff_narrowing.sh $(BENCHDIR)

resdir:
	for p in  $(SUBDIRS); do\
	( cd $$p && $(MAKE) -f $(PWD)/Makefile.model resdir && cd ..);\
	done ;

debug:
	@echo BENCHDIR=$(BENCHDIR)
	@echo SUBDIRS=$(SUBDIRS)
