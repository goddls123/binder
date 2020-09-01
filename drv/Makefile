
obj-m += binder.o

binder-y += binder_dev.o

KDIR := /lib/modules/$(shell uname -r)/build
#ccflags-y += -l/home/lee/work/binder/drv/

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo rmmod binder
	sudo insmod binder.ko
	sudo chmod 777 /dev/binder
