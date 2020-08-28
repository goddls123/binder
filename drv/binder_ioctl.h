

#ifndef _BINDER_IOCTL
#define _BINDER_IOCTL

#ifdef __KERNEL__
//#include <linux/fs.h>
#endif

#define BINDER_MAGIC	'b'

#define IOSET_TYPE		_IOW(BINDER_MAGIC,0, int)
#define IO_PING			_IOR(BINDER_MAGIC,1, int)
#define IOG_MESSEGE		_IOR(BINDER_MAGIC,2, int)
#define DM_DISPLAY_TEXT		_IOR(BINDER_MAGIC,3, int)
#define IOC_REGISTER_SERVICE	_IOR(BINDER_MAGIC,4, int)
#define IOG_REGISTER		_IOR(BINDER_MAGIC,5, int)


typedef enum {
	PT_DEF		= 0,
	PT_MONITOR	= 1,
	PT_IOMAN	= 2,
	PT_OSD		= 3,
	PT_MAX
} process_type_e ;

struct service_list{
	int pid;
	process_type_e p_type;
};

#endif
