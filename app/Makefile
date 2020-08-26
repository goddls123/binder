

CC =gcc
INCLUDE = -I../drv

SUBDIR = monitor ioman

all: $(SUBDIR)

$(SUBDIR)::
	$(MAKE) -C $@ all


clean:
	 $(SUBDIR) rm -f *_app *.o
