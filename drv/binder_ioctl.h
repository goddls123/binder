
#ifndef _BINDER_IOCTL
#define _BINDER_IOCTL

#ifdef __KERNEL__
//#include <linux/fs.h>
#endif

#define BINDER_MAGIC	'b'

#define IOSET_TYPE		_IOW(BINDER_MAGIC,0, int)
#define IO_PING			_IOR(BINDER_MAGIC,1, int)

typedef enum {
	PT_DEF		= 0,
	PT_MONITOR	= 1,
	PT_IOMAN	= 2,
	PT_MAX
} process_type_e ;

struct io{
	int size;
	char b[10];
};

#endif
