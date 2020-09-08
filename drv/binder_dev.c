
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nsproxy.h>
#include <linux/poll.h>
#include <linux/debugfs.h>
//#include <linux/rbtree.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <asm/uaccess.h>
//#include <linux/pid_namespace.h>
//#include <linux/security.h>
#include <linux/spinlock.h>
//#include <linux/ratelimit.h>
#include <linux/syscalls.h>
#include <linux/task_work.h>
#include <linux/sizes.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

//#include <uapi/linux/android/binderfs.h>
//#include <uapi/linux/android/binder.h>

//#include <asm/cacheflush.h>

//#include "binder_alloc.h"
//#include "binder_internal.h"
//#include "binder_trace.h"
#include "binder_ioctl.h"


//#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


struct binder_dev {
	struct miscdevice miscdev;
	refcount_t ref;
	int proc_count;
	struct proc_dir_entry *proc_entry;
	struct list_head client_list;
	struct list_head queue_list;
	struct list_head service_list;
	int queue_count;
};

static DEFINE_MUTEX(binder_lock);

static const char * process_type_str[] = {
	"default",
	"monitor",
	"ioman",
	"osd",
};

struct binder_client {
	int pid;
	process_type_e 	process_type;
	struct list_head client;
};

struct msgque{
	int qid;
	struct list_head queue;
	int count;
	char msgtxt[256];
};

struct binder_service{
	int pid;
	process_type_e process_type;
	process_type_e destination_type;
	int destination_pid;
	struct list_head service;
};

static struct binder_dev *gBinderDev;

static int binder_open(struct inode *nodp, struct file *filp){

	struct binder_client *mClient;

	mClient = kzalloc(sizeof(*mClient), GFP_KERNEL);
	mClient->pid = current->pid;
	mClient->process_type = PT_DEF;

	gBinderDev->proc_count++;

	list_add(&mClient->client,&gBinderDev->client_list);	
	return 0;
}


static int binder_release(struct inode *nodp, struct file *filp){
	struct list_head *pos, *q;
	struct binder_client *tmp;
	struct binder_service *stmp;

	list_for_each_safe(pos,q,&gBinderDev->client_list){
		tmp = list_entry(pos,struct binder_client, client );
		if(tmp->pid == current->pid){
			mutex_lock(&binder_lock);
			list_del(&tmp->client);
			printk("%s close\n",process_type_str[tmp->process_type]);
			kfree(tmp);
			mutex_unlock(&binder_lock);
		}
	}
	list_for_each_safe(pos,q,&gBinderDev->service_list){
		stmp = list_entry(pos,struct binder_service, service );
		if(stmp->pid == current->pid){
			mutex_lock(&binder_lock);
			list_del(&stmp->service);
			kfree(stmp);
			mutex_unlock(&binder_lock);
		}
		else if(stmp->destination_pid == current->pid){
			mutex_lock(&binder_lock);
			list_del(&stmp->service);
			kfree(stmp);
			mutex_unlock(&binder_lock);
		}
	}
	gBinderDev->proc_count--;
	return 0;
}

static int binder_register(unsigned long arg){
	struct binder_service *mservice;
	struct binder_client *tmp;
	int type = 0;
	int pid = 0;

	list_for_each_entry(tmp,&gBinderDev->client_list,client){
		if(tmp->process_type == arg) {
			type = arg;
			pid = tmp->pid;
		}
	}
	if(!type){
		return -1;
	}
	mservice = kzalloc(sizeof(*mservice), GFP_KERNEL);


	if (!mservice)
		return -ENOMEM;
	
	mservice->pid = current->pid;
	mservice->destination_pid = pid;
	mservice->destination_type = type;
	

	list_add(&mservice->service,&gBinderDev->service_list);	

	return 1;
}

static long binder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct binder_client *tmp;
	const char *pong ="PONG\0";
	int ret = 0;
	struct msgque * msgbuf;
	struct binder_service *q;

	switch(cmd){
		case IOSET_TYPE:
			list_for_each_entry(tmp,&gBinderDev->client_list,client){
				if(tmp->pid == current->pid) {
					tmp->process_type = arg;
				}
			}
			break; 
		case IO_PING:
			msgbuf = kzalloc(sizeof(*msgbuf),GFP_KERNEL);
			strcpy(msgbuf->msgtxt,pong);
			list_for_each_entry(q,&gBinderDev->service_list,service){
				if(q->pid == current->pid) {
				msgbuf->qid  = q->destination_pid;
				}
			}
			msgbuf->count = 4;
			
			list_add(&msgbuf->queue,&gBinderDev->queue_list);	
			gBinderDev->queue_count++;

			break;
		case IOG_MESSEGE:
			if(gBinderDev->queue_count > 0){
				printk("message \n");
				return gBinderDev->queue_count;	
			}
			break;
		case DM_DISPLAY_TEXT:
			if(!list_empty(&gBinderDev->queue_list)){
				return gBinderDev->queue_count;	
			}
			break;
		case IOC_REGISTER_SERVICE:
			if(binder_register(arg)<0)
				return -1;
			break;
		default:
			break; 
	}	
	return ret;
}

#if 0
static int binder_mmap(struct file *filp, struct vm_area_struct *vma){
	
	return 0;
}

static int binder_flush(struct file *filp, fl_owner_t id)
{
	return 0;
}

static __poll_t binder_poll(struct file *filp,
			struct poll_table_struct *wait)
{

	return 0;
}
#endif 
static long binder_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos){
	struct msgque *msgbuf;
	struct binder_service *tmp;

	msgbuf = kzalloc(sizeof(*msgbuf),GFP_KERNEL);
	if (!msgbuf)
		return -ENOMEM;
	list_for_each_entry(tmp,&gBinderDev->service_list,service){
		if(tmp->pid == current->pid) {
			msgbuf->qid  = tmp->destination_pid;
		}
	}

	if(!msgbuf->qid){      //no service 
		kfree(msgbuf);
		return -1;
	}

	list_for_each_entry(tmp,&gBinderDev->service_list,service){
		if(tmp->pid == current->pid) {
			msgbuf->qid  = tmp->destination_pid;
		}
	}
	
	copy_from_user(msgbuf->msgtxt,buffer,count);
	msgbuf->count = count;
	
	list_add(&msgbuf->queue,&gBinderDev->queue_list);

	gBinderDev->queue_count++;
	
	return 0;
}

static ssize_t binder_read (struct file *file, char __user *buffer, size_t count, loff_t *pos){
	int ret = 0;
	struct msgque *tmp;
	
	memset(buffer,0,count);

	tmp =list_entry(gBinderDev->queue_list.next, struct msgque, queue); 
	if(!tmp)
		return -ENOMEM;

	if(tmp->qid != current->pid)
		return ret;

	if((ret=copy_to_user(buffer,tmp->msgtxt , tmp->count)) < 0){
		printk("copy error\n");
		return 0;
	}

	list_del(gBinderDev->queue_list.next);
	gBinderDev->queue_count--;
	kfree(tmp);
	
	return ret;
}

static ssize_t binder_proc_read (struct file *file, char __user *buffer, size_t count, loff_t *pos){
	//int len;
	//static int finished = 0;
	struct binder_client *q;
	
#if 0
	if(finished){
		finished = 0;
		return 0;
	}
	finished = 1;
#endif

	printk("Connedted to application : %d\n", gBinderDev->proc_count);

	printk("process\n");

	list_for_each_entry(q, &gBinderDev->client_list,client){
		if (q->process_type < PT_MAX) {
			printk(" - pid: %d,  type = %s\n", q->pid, process_type_str[q->process_type]);
		}
	}

	return 0;
	//return len;
}

static const struct file_operations proc_fops = {
	.owner		=  THIS_MODULE,
	.read 		=  binder_proc_read,
	
};

const struct file_operations binder_fops = {
	.owner = THIS_MODULE,
	//.poll = binder_poll,
	.unlocked_ioctl = binder_ioctl,
	//.compat_ioctl = compat_ptr_ioctl,
	//.mmap = binder_mmap,
	.open = binder_open,
	//.flush = binder_flush,
	.read = binder_read,
	.write = binder_write,
	.release = binder_release,
};


static int __init binder_init(void)
{
	int ret = 0;
	struct binder_dev *dev = NULL;
	const char *name = "binder";
	
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return -ENOMEM;

#if 0
	refcount_set(&dev->ref, 1);
	dev->context.binder_context_mgr_uid = INVALID_UID	;
	dev->context.name = name;
	mutex_init(&dev->context.context_mgr_node_lock);
#endif

	dev->miscdev.fops = &binder_fops;
	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = name;
	ret = misc_register(&dev->miscdev);
	if (ret < 0) {
		kfree(dev);
		return ret;
	}
dev->proc_entry = proc_create("driver/binder",0644,NULL,&proc_fops);
	INIT_LIST_HEAD(&dev->client_list);
	INIT_LIST_HEAD(&dev->queue_list);
	INIT_LIST_HEAD(&dev->service_list);
#if 0  	
	ret = init_binderfs();
#endif
	
	gBinderDev = dev; 
	
	return ret;
}



static void __exit binder_exit(void)
{
	if (!gBinderDev) return;
	printk("exit device driver\n");
	remove_proc_entry("driver/binder",NULL);
	misc_deregister(&gBinderDev->miscdev);
	kfree(gBinderDev);
}

module_init(binder_init);
module_exit(binder_exit);

//#define CREATE_TRACE_POINTS
//#include "binder_trace.h"

MODULE_LICENSE("GPL v2");
