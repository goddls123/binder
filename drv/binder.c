
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

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
#include <linux/rbtree.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/pid_namespace.h>
#include <linux/security.h>
#include <linux/spinlock.h>
#include <linux/ratelimit.h>
#include <linux/syscalls.h>
#include <linux/task_work.h>
#include <linux/sizes.h>
#include <linux/proc_fs.h>

#include <uapi/linux/android/binderfs.h>
#include <uapi/linux/android/binder.h>

#if 0
#include <asm/cacheflush.h>
#endif

#include "binder_alloc.h"
#include "binder_internal.h"
#include "binder_trace.h"

enum binder_stat_types {
	BINDER_STAT_PROC,
	BINDER_STAT_THREAD,
	BINDER_STAT_NODE,
	BINDER_STAT_REF,
	BINDER_STAT_DEATH,
	BINDER_STAT_TRANSACTION,
	BINDER_STAT_TRANSACTION_COMPLETE,
	BINDER_STAT_COUNT
};

struct binder_stats {
	atomic_t br[_IOC_NR(BR_FAILED_REPLY) + 1];
	atomic_t bc[_IOC_NR(BC_REPLY_SG) + 1];
	atomic_t obj_created[BINDER_STAT_COUNT];
	atomic_t obj_deleted[BINDER_STAT_COUNT];
};

struct binder_proc {
	struct hlist_node proc_node;
	struct rb_root threads;
	struct rb_root nodes;
	struct rb_root refs_by_desc;
	struct rb_root refs_by_node;
	struct list_head waiting_threads;
	int pid;
	struct task_struct *tsk;
	struct hlist_node deferred_work_node;
	int deferred_work;
	bool is_dead;

	struct list_head todo;
	struct binder_stats stats;
	struct list_head delivered_death;
	int max_threads;
	int requested_threads;
	int requested_threads_started;
	int tmp_ref;
	long default_priority;
	struct dentry *debugfs_entry;
	struct binder_alloc alloc;
	struct binder_context *context;
	spinlock_t inner_lock;
	spinlock_t outer_lock;
	struct dentry *binderfs_entry;
};

static int proc_count = 0;
struct proc_dir_entry *Our_Proc_File;

static struct dentry *binder_debugfs_dir_entry_root;
static struct dentry *binder_debugfs_dir_entry_proc;
static HLIST_HEAD(binder_devices);
static HLIST_HEAD(binder_procs);

static int binder_open(struct inode *nodp, struct file *filp){

	struct binder_proc *proc, *itr;
	struct binder_device *binder_dev;
	struct binderfs_info *info;
	struct dentry *binder_binderfs_dir_entry_proc = NULL;
	bool existing_pid = false;

	proc_count++;

	printk("binder open\n");
	

#if 0
	proc = kzalloc(sizeof(struct binder_proc), GFP_KERNEL);
	if (proc == NULL)
		return -ENOMEM;
	spin_lock_init(&proc->inner_lock);
	spin_lock_init(&proc->outer_lock);
	get_task_struct(current->group_leader);
	proc->tsk = current->group_leader;
	INIT_LIST_HEAD(&proc->todo);
	proc->default_priority = task_nice(current);
	/* binderfs stashes devices in i_private */
	if (is_binderfs_device(nodp)) {
		binder_dev = nodp->i_private;
		info = nodp->i_sb->s_fs_info;
		binder_binderfs_dir_entry_proc = info->proc_log_dir;
	} else {
		binder_dev = container_of(filp->private_data,
					  struct binder_device, miscdev);
	}
	refcount_inc(&binder_dev->ref);
	proc->context = &binder_dev->context;
	binder_alloc_init(&proc->alloc);

	binder_stats_created(BINDER_STAT_PROC);
	proc->pid = current->group_leader->pid;
	INIT_LIST_HEAD(&proc->delivered_death);
	INIT_LIST_HEAD(&proc->waiting_threads);
	filp->private_data = proc;
	
	mutex_lock(&binder_procs_lock);
	hlist_for_each_entry(itr, &binder_procs, proc_node) {
		if (itr->pid == proc->pid) {
			existing_pid = true;
			break;
		}
	}
	hlist_add_head(&proc->proc_node, &binder_procs);
	mutex_unlock(&binder_procs_lock);
/////////////////////////////////////////////////////////////////
	if (binder_debugfs_dir_entry_proc && !existing_pid) {
		char strbuf[11];

		snprintf(strbuf, sizeof(strbuf), "%u", proc->pid);
		/*
		 * proc debug entries are shared between contexts.
		 * Only create for the first PID to avoid debugfs log spamming
		 * The printing code will anyway print all contexts for a given
		 * PID so this is not a problem.
		 */
		proc->debugfs_entry = debugfs_create_file(strbuf, 0444,
			binder_debugfs_dir_entry_proc,
			(void *)(unsigned long)proc->pid,
			&proc_fops);
	}

	if (binder_binderfs_dir_entry_proc && !existing_pid) {
		char strbuf[11];
		struct dentry *binderfs_entry;

		snprintf(strbuf, sizeof(strbuf), "%u", proc->pid);
		/*
		 * Similar to debugfs, the process specific log file is shared
		 * between contexts. Only create for the first PID.
		 * This is ok since same as debugfs, the log file will contain
		 * information on all contexts of a given PID.
		 */
		binderfs_entry = binderfs_create_file(binder_binderfs_dir_entry_proc,
			strbuf, &proc_fops, (void *)(unsigned long)proc->pid);
		if (!IS_ERR(binderfs_entry)) {
			proc->binderfs_entry = binderfs_entry;
		} else {
			int error;

			error = PTR_ERR(binderfs_entry);
			pr_warn("Unable to create file %s in binderfs (error %d)\n",
				strbuf, error);
		}
	}

#endif
	return 0;
}


static int binder_release(struct inode *nodp, struct file *filp){
	proc_count--;
	printk("binder close\n");
	return 0;
}

static long binder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("ioctl\n");
	return 0;
}

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

static ssize_t binder_proc_read (struct file *file, char __user *buffer, size_t count, loff_t *pos){
	int len;
	
	len = sprintf(buffer,"conneted application : %d\n", proc_count);

	return len;
}

static const struct file_operations proc_fops = {
	.owner		=  THIS_MODULE,
	.read 		=  binder_proc_read,
	
};

const struct file_operations binder_fops = {
	.owner = THIS_MODULE,
	.poll = binder_poll,
	.unlocked_ioctl = binder_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.mmap = binder_mmap,
	.open = binder_open,
	.flush = binder_flush,
	.release = binder_release,
};

static struct binder_device *binder_device1;

static int __init binder_init(void)
{
	int ret = 0;
	struct binder_device *   binder_dev = NULL;
	const char *name = "binder";
	
	binder_dev = kzalloc(sizeof(* binder_dev), GFP_KERNEL);

	binder_device1 = binder_dev; 
	if (!binder_dev)
		return -ENOMEM;

	printk(" assigned  %p size=%ld \n", binder_dev, sizeof(struct binder_device));

	binder_dev->miscdev.fops = &binder_fops;
	binder_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	binder_dev->miscdev.name = name;

	refcount_set(&binder_dev->ref, 1);
	binder_dev->context.binder_context_mgr_uid = INVALID_UID;
	binder_dev->context.name = name;
	mutex_init(&binder_dev->context.context_mgr_node_lock);

	ret = misc_register(&binder_dev->miscdev);
	if (ret < 0) {
		kfree(binder_dev);
		return ret;
	}
	
	Our_Proc_File = proc_create("driver/binder",0644,NULL,&proc_fops);
	
#if 0  	
	hlist_add_head(&binder_dev->hlist, &binder_devices);

	ret = init_binderfs();
#endif
	
	return ret;
}



static void __exit binder_exit(void)
{
	printk("exit device driver\n");
	misc_deregister(&binder_device1->miscdev);
	kfree(binder_device1);
	remove_proc_entry("driver/binder",NULL);
}

module_init(binder_init);
module_exit(binder_exit);

//#define CREATE_TRACE_POINTS
//#include "binder_trace.h"

MODULE_LICENSE("GPL v2");
