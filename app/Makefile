TOP = $(shell pwd)
export TOP

CC = gcc
export CC

DRVPATH = $(shell cd ../drv; pwd)
export DRVPATH


INCLUDE = -I$(DRVPATH)


SUBDIR = monitor ioman osd


CLEANDIRS = $(SUBDIR:%=clean-%)

build: $(addsuffix .build,$(SUBDIR))

$(addsuffix .build,$(SUBDIR)):
	echo $@
	$(MAKE) -C $(subst .build,,$@) all


clean: $(CLEANDIRS)

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean
