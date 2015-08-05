#include <linux/module.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h> 
#include <linux/cdrom.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/blkpg.h>
#include <linux/init.h>
#include <linux/fcntl.h>
#include <linux/blkdev.h>
#include <linux/times.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/rtc.h>
#include <linux/freezer.h>
#include <linux/mmu_context.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
//#include <asm/cp15.h>
//#include <asm/cputype.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
//#include <asm/sizes.h>
//#include <asm/system_info.h>
#include <linux/delay.h>
//#include <asm/mach/map.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/kprobes.h>

#define DEBUG_SOUND_REGISTER
#define debug_printk(format, arg...) do{if(debug&1){printk(format, ##arg);}}while(0)

static struct dentry *debug_wgz;

/***********************************************************************/
unsigned long debug = 0x00000000;
static unsigned long vir_address = (unsigned long)&debug;
static unsigned long vir_value = 0x00000000;
static unsigned long phy_address = 0x20000;
static unsigned long phy_value = 0;
static long taskpid = -1;

int rw_task(int t_id,unsigned long addr,void * buf,int len,int write)
{
	int ret = 0;
	struct task_struct *t = find_task_by_vpid(t_id);
	if(t == NULL)
	{
		printk("error can not find task %d",t_id);
		return 0;
	}
	atomic_inc(&t->mm->mm_users);
	ret = access_remote_vm(t->mm,addr,buf,len,write);
	mmput(t->mm);
	return ret;
}
static int debugfs_set_reg(void *data, u64 val)
{
	int taskpid_temp = taskpid;
	unsigned long vir_address_temp = vir_address;
	
	if(taskpid_temp == -1)
	{
		if(kern_addr_valid(vir_address_temp))
		{
			*(int*)vir_address_temp = (int)val;
		}
		else
		{
			printk("addr error \n");
		}
	}
	else
	{
		rw_task(taskpid_temp,vir_address_temp,&val,sizeof(int),1);
	}
	return 0;
}
static int debugfs_get_reg(void *data, u64 *val)
{
	int taskpid_temp = taskpid;
	unsigned long vir_address_temp = vir_address;
	
	if(taskpid_temp <= 0)
	{
		if(kern_addr_valid(vir_address_temp))
		{
			*(int*)val = *(int*)vir_address_temp ;
		}
		else
		{
			printk("addr error \n");
			*(int*)val = 0x57575777;
		}
	}
	else
	{
		rw_task(taskpid_temp,vir_address_temp,val,sizeof(int),0);
	}
 	return 0;
}

void __iomem * remap_io_or_mem(phys_addr_t phy_addr_temp, unsigned long size)
{
	void __iomem *vir_address = 0;
	struct page *first_page;
	if(pfn_valid(__phys_to_pfn(phy_addr_temp)))
	{
		debug_printk("map memory area, phy_addr_temp=0x%lx,size=0x%lx\n" , (unsigned long)phy_addr_temp, size);
		first_page = phys_to_page(phy_addr_temp);
		debug_printk("first_page = 0x%p\n" , first_page);

		vir_address = vmap(&first_page, (size+PAGE_SIZE)>>PAGE_SHIFT, VM_MAP, PAGE_KERNEL);
		debug_printk("vir_address = 0x%p\n" , vir_address);
		vir_address = (void*)(((unsigned long)vir_address&PAGE_MASK)| (phy_addr_temp&~PAGE_MASK));
		debug_printk("last vir_address = 0x%p\n" , vir_address);
	}
	else
	{
		debug_printk("map io area\n");
#ifndef CONFIG_UML
		vir_address = ioremap((phys_addr_t)phy_addr_temp , size); 
#endif
	}
	if(vir_address == 0)
	{
		debug_printk("error vir_base is null");
	}
	debug_printk("phy_addr = 0x%lx ,  vir_address = 0x%p" , (unsigned long)phy_addr_temp  , vir_address);
	return vir_address;
}

void unmap_io_or_mem(phys_addr_t phy_addr_temp , void* vir_address)
{
	if(pfn_valid(__phys_to_pfn(phy_addr_temp)))
	{
		vunmap((void*)((unsigned long)vir_address&PAGE_MASK));
	}
	else
	{
#ifndef CONFIG_UML
		iounmap(vir_address);
#endif
	}
}

static int set_phy_value(unsigned long phy_addr, u32 val)
{
	void __iomem *vir_address;
	vir_address = remap_io_or_mem(phy_addr,4);
	if(vir_address != NULL)
	{
		*(u32 *)vir_address = val;
		unmap_io_or_mem(phy_addr , vir_address);
		return 0;
	}
	return -1;
}

u32 get_phy_value(unsigned long phy_addr)
{
	void __iomem *vir_address;
	u32 value = 0;
	vir_address = remap_io_or_mem(phy_addr,4);
	if(vir_address != NULL)
	{
		value = *(u32 *)vir_address ;
		unmap_io_or_mem(phy_addr , vir_address);
	}
	return value;
}

static int phy_value_set_func(void *data, u64 val)
{
	int unsigned long phy_address_temp = phy_address;
	set_phy_value(phy_address_temp , (u32)val);
	return 0;
}
static int phy_value_get_func(void *data, u64 *val)
{
	int unsigned long phy_address_temp = phy_address;
	*(int*)val = get_phy_value(phy_address_temp);
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(fops_vir_value, debugfs_get_reg, debugfs_set_reg, "0x%016llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_value, phy_value_get_func, phy_value_set_func, "0x%016llx\n");

static void init_debug_memfs(void)
{
	struct dentry *memory = debugfs_create_dir("memory", debug_wgz);
	debugfs_create_x64("debug", S_IRWXUGO, memory,(u64 *)&debug);
	debugfs_create_x64("vir_address", S_IRWXUGO, memory,(u64 *)&vir_address);
	debugfs_create_x32("taskpid", S_IRWXUGO, memory,(u32 *)&taskpid);
	debugfs_create_file("vir_value", 0777,memory, &vir_value,&fops_vir_value);
	debugfs_create_x32("phy_address", S_IRWXUGO, memory,(u32 *)&phy_address);
	debugfs_create_file("phy_value", 0777,memory, &phy_value,&fops_phy_value);
}

/**********************************Thread**********************************************/

#ifdef WGZ_DEBUG_SCHEDULE
/**************
must modify :
include\linux\sched.h
kernel/panic.c
kernel/sched/core.c

*****************/

static unsigned int dump_pid = -1;

static int dump_pid_set(void *data, u64 val)
{
	struct task_struct *tsk;
	*(u32*)data = (u32)val;
	if(val < 0)
	{
		return 0;
	}
	tsk = find_task_by_vpid(val);
	if(!tsk)
	{
		return 0;
	}
	printk("\n-----------------------------------------------------\n");
	printk(KERN_EMERG"pid = %d,comm = %s,last_schedule=%lu\n",tsk->pid,tsk->comm,tsk->last_schedule_jiffies);
	show_stack(tsk , NULL);
	printk("-----------------------------------------------------\n\n");
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_dump_pid, 0 , dump_pid_set, "0x%08llx\n");


static unsigned int dump_dead_kernel_thread = -1;

static int dump_dead_kernel_thread_get(void *data, u64 *val)
{
	struct task_struct *g, *p;
	
	//read_lock(&tasklist_lock);
	do_each_thread(g, p) 
	{
		if ((p->debug_state == __TASK_STOPPED)&&(p->state == __TASK_STOPPED) && (p->flags&PF_KTHREAD))
		{
			printk("\n-----------------------------------------------------\n");
			printk(KERN_EMERG"pid = %d,comm = %s,last_schedule=%lu\n",p->pid,p->comm,p->last_schedule_jiffies);
			show_stack(p , NULL);
			printk("-----------------------------------------------------\n\n");
		}
	} while_each_thread(g, p);
	//read_unlock(&tasklist_lock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_dump_dead_kernel_thread, dump_dead_kernel_thread_get, 0 , "0x%08llx\n");




static void thread_debug_work_func(struct work_struct *work);
struct workqueue_struct *thread_debug_wq;
static DECLARE_DELAYED_WORK(thread_debug_work, thread_debug_work_func);

static unsigned int print_seconds = 30;
static unsigned int schedule_seconds = 15;


static char process_thread_stoped_name[512] = {0};

static void thread_debug_work_func(struct work_struct *work)
{
	struct task_struct *g, *p;
	int monitor_kernel = !!strstr(process_thread_stoped_name,"kernel");
	freezer_do_not_count();
	for_each_process(g)
	{
		p = g;
		if(p == current)
		{
			continue;
		}
		if(frozen(p))
		{
			continue;
		}
		if((p->flags&PF_KTHREAD))
		{
			if(!monitor_kernel)
			{
				continue;
			}
		}
		else
		{
			if(!strstr(process_thread_stoped_name,p->comm))
			{
				continue;
			}
		}
		
		//task_lock(g);
		do 
		{
			if(p->last_schedule_jiffies == p->last_check_jiffies)
			{
				continue;
			}
			if(time_after(jiffies,p->last_schedule_jiffies + print_seconds*HZ))
			{
				p->last_check_jiffies = p->last_schedule_jiffies;
				printk("\n-----------------------------------------------------\n");
				printk(KERN_EMERG"pid = %d,comm = %s,last_schedule=%lu\n",p->pid,p->comm,p->last_schedule_jiffies);
				show_stack(p , NULL);
				printk("-----------------------------------------------------\n\n");
			}
			else
			{
				continue;
			}
		} while_each_thread(g, p);
		//task_unlock(g);
	}
	queue_delayed_work(thread_debug_wq , &thread_debug_work , schedule_seconds*HZ);
}

static ssize_t process_thread_stoped_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return simple_read_from_buffer(buf,size,ppos,process_thread_stoped_name,strlen(process_thread_stoped_name));
}
static ssize_t process_thread_stoped_write(struct file *file, const char __user *ubuf,size_t len, loff_t *offp)
{
	ssize_t size = 0;
	memset(process_thread_stoped_name , 0 , 512);
	size = simple_write_to_buffer(process_thread_stoped_name,511,offp,ubuf,len);
	if(process_thread_stoped_name[0] == '0')
	{
		//printk("%s:%d\n" , __FUNCTION__,__LINE__);
		cancel_delayed_work(&thread_debug_work);
	}
	else
	{
		//printk("%s:%d\n" , __FUNCTION__,__LINE__); 
		queue_delayed_work(thread_debug_wq , &thread_debug_work , 10*HZ);
	}
	return size;
}

static const struct file_operations process_thread_stoped_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = process_thread_stoped_read,
	.write = process_thread_stoped_write
};

static int thread_debug_init(void)
{
	thread_debug_wq = create_workqueue("thread_debug");
	if (IS_ERR(thread_debug_wq)) {
		pr_err("%s: create_singlethread_workqueue ENOMEM\n", __func__);
		return -ENOMEM;
	}
	return 0;
}

static void init_thread_debug_fs(void)
{
	struct dentry *thread_dir = debugfs_create_dir("thread", debug_wgz);
	debugfs_create_file("process_thread_stoped", S_IRWXUGO, thread_dir, NULL,&process_thread_stoped_fops);
	debugfs_create_x32("print_seconds", S_IRWXUGO, thread_dir,(u32 *)&print_seconds);
	debugfs_create_x32("schedule_seconds", S_IRWXUGO, thread_dir,(u32 *)&schedule_seconds);
	debugfs_create_file("dump_pid", 0777,thread_dir, &dump_pid,&fops_dump_pid);
	debugfs_create_file("dump_dead_kernel", 0777,thread_dir, &dump_dead_kernel_thread,&fops_dump_dead_kernel_thread);
	thread_debug_init();
}
#else
static void init_thread_debug_fs(void)
{
}
#endif
/**************************************************************************************/
/*********************************Sound Code Register************************************/

#ifdef DEBUG_SOUND_REGISTER
extern int msm8x16_wcd_write_for_debug(unsigned int reg,unsigned int value);
extern int msm8x16_wcd_read_for_debug(unsigned int reg);

static unsigned long sound_reg_address = 0;
static int sound_register_get_func(void *data, u64 *val)
{
	int value = msm8x16_wcd_read_for_debug(sound_reg_address);
	*val = value;
	return 0;
}

static int sound_register_set_func(void *data, u64 val)
{
	unsigned int value = val;
	msm8x16_wcd_write_for_debug(sound_reg_address , value);
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(sound_register_fops, sound_register_get_func, sound_register_set_func, "0x%08llx\n");

static void init_sound_debug_fs(void)
{
	struct dentry *sound_dir = debugfs_create_dir("sound", debug_wgz);
	debugfs_create_file("register_value", S_IRWXUGO, sound_dir, NULL,&sound_register_fops);
	debugfs_create_x32("register_address", S_IRWXUGO, sound_dir,(u32 *)&sound_reg_address);
}
	
#else
static void init_sound_debug_fs(void)
{
}
#endif


/**************************************************************************************/
static int __init debug_init(void)
{
	debug_wgz = debugfs_create_dir("debug_wgz", NULL);
	init_debug_memfs();
	init_thread_debug_fs();
	init_sound_debug_fs();
	return 0;
}
static void __exit debug_exit(void)
{
	pr_info("remove\n");
	debugfs_remove_recursive(debug_wgz);	
}

module_init(debug_init);
module_exit(debug_exit);
MODULE_LICENSE("GPL");

