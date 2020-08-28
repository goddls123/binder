

CC =gcc
INCLUDE = -I../drv

SUBDIR = monitor ioman osd



build: $(addsuffix .build,$(SUBDIR))

$(addsuffix .build,$(SUBDIR)):
	echo $@
	$(MAKE) -C $(subst .build,,$@) all


clean: $(addsuffix .clean,$(SUBDIR))

$(addsuffix .clean,$(SUBDIR)):
	$(MAKE) -C $(subst .clean,,$@) clean
#	$(MAKE) -C $@ clean
