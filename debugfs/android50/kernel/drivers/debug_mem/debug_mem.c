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
#include <linux/delay.h>
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
#include <linux/gpio.h>


//#define WGZ_DEBUG_SCHEDULE
//#define DEBUG_SOUND_REGISTER
//#define SUSPEND_RESUME_DEBUG
//#define SUSPEND_RESUME_DEBUG_DUMP_REGULATOR
//#define SUSPEND_RESUME_DEBUG_DUMP_GPIO
//#define GPIO_SIGNAL_GENERATE
#define MEMORY_ATTR_TEST


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
static unsigned int dump_task = -1;

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

static char process_dump_task_name[512] = {0};
static void thread_debug_dump_task_func(void)
{
	struct task_struct *g, *p;
	freezer_do_not_count();
	for_each_process(g)
	{
		p = g;
		if(!strstr(process_dump_task_name,p->comm))
		{
			continue;
		}
		do 
		{
			p->last_check_jiffies = p->last_schedule_jiffies;
			printk("\n-----------------------------------------------------\n");
			printk(KERN_EMERG"pid = %d,comm = %s,last_schedule=%lu\n",p->pid,p->comm,p->last_schedule_jiffies);
			show_stack(p , NULL);
			printk("-----------------------------------------------------\n\n");
		} while_each_thread(g, p);
	}
}

void thread_debug_dump_task(struct task_struct *task)
{
	struct task_struct *g, *p;
	g = p = task;
	do 
	{
		p->last_check_jiffies = p->last_schedule_jiffies;
		printk("\n-----------------------------------------------------\n");
		printk(KERN_EMERG"pid = %d,comm = %s,last_schedule=%lu\n",p->pid,p->comm,p->last_schedule_jiffies);
		show_stack(p , NULL);
		printk("-----------------------------------------------------\n\n");
	} while_each_thread(g, p);
}


static int dump_task_set(void *data, u64 val)
{
	thread_debug_dump_task_func();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_dump_task, 0 , dump_task_set, "0x%08llx\n");
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

static ssize_t process_dump_task_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return simple_read_from_buffer(buf,size,ppos,process_dump_task_name,strlen(process_dump_task_name));
}
static ssize_t process_dump_task_write(struct file *file, const char __user *ubuf,size_t len, loff_t *offp)
{
	ssize_t size = 0;
	memset(process_dump_task_name , 0 , 512);
	size = simple_write_to_buffer(process_dump_task_name,511,offp,ubuf,len);
	return size;
}
static const struct file_operations process_dump_task_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = process_dump_task_read,
	.write = process_dump_task_write
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
	debugfs_create_file("dump_task_comm", S_IRWXUGO, thread_dir, NULL,&process_dump_task_fops);
	debugfs_create_file("process_thread_stoped", S_IRWXUGO, thread_dir, NULL,&process_thread_stoped_fops);
	debugfs_create_x32("print_seconds", S_IRWXUGO, thread_dir,(u32 *)&print_seconds);
	debugfs_create_x32("schedule_seconds", S_IRWXUGO, thread_dir,(u32 *)&schedule_seconds);
	debugfs_create_file("dump_pid", 0777,thread_dir, &dump_pid,&fops_dump_pid);
	debugfs_create_file("dump_task", 0777,thread_dir, &dump_task,&fops_dump_task);
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


/******************************Suspend Resume Debug**************************************/
#ifdef SUSPEND_RESUME_DEBUG
#define MSM8916_GPIO_START 0x01000000
#define MSM8916_GPIO_LENGTH 0x300000
#define MSM8916_GPIO_NUM 122

extern void (*last_step_for_suspend)(void)  ;
extern void (*first_step_for_resume)(void) ;
extern void (*resume_before_irq_enable)(void);
extern void (*suspend_after_irq_disable)(void);
extern void (*after_dpm_suspend_start)(void) ;
extern void (*resume_before_gic_irq_resume)(void) ;
extern void (*suspend_after_gic_irq_suspend)(void);
extern void (*resume_before_msm_tlmm_v4_gp_irq_resume)(void) ;
extern void (*suspend_after_msm_tlmm_v4_gp_irq_suspend)(void) ;
extern void (*before_spmi_pmic_arb_resume)(void);

void dump_gpio_register(void)
{
	void __iomem *vir_address;
	int __iomem *vir_address_int;
	int index;
	unsigned int io_config = 0;
	unsigned int io_inout = 0;
	unsigned int io_intr_cfg = 0;
	unsigned int io_intr_status = 0;
	static char *pull_str[4] = {"NO","DOWN","KEEP","UP"};
	unsigned int wake_enable0 = 0;
	unsigned int wake_enable1 = 0;
	static int wake_enable0_gpio_index[32] = {115,114,113,112,111,110,109,108
									   ,107,98 ,97 ,69 ,62 ,54 ,52 ,51
									   ,50 ,49 ,38 ,37 ,36 ,35 ,34 ,31
									   ,28 ,25 ,21 ,20 ,13 ,12 ,5  ,1};
	static int wake_enable1_gpio_index[32] = {777,777,777,777,777,777,777,777
									   ,777,777,777,777,777,777,777,777
									   ,777,777,777,68 ,66 ,205,204 ,203
									   ,202,201,121,120,9  ,118,117,11};

	
	vir_address = remap_io_or_mem(MSM8916_GPIO_START,MSM8916_GPIO_LENGTH);
	vir_address_int = (int __iomem *)vir_address;
	printk(KERN_EMERG"%-4s%"
		"-11s%-3s%-4s%-4s%-5s"
		"%-11s%-2s%-2s"
		"%-11s%-11s\n"
		,"NUM"
		,"config","OE","DRV","FUN","PULL"
		,"value","O","I"
		,"intr_cfg","intr_stat") ;
	for(index = 0 ; index < MSM8916_GPIO_NUM ; index++)
	{
		io_config = *(vir_address_int + index*0x1000/4);
		io_inout = *(vir_address_int + (index*0x1000+4)/4);
		io_intr_cfg = *(vir_address_int + (index*0x1000+8)/4);
		io_intr_status = *(vir_address_int + (index*0x1000+0xC)/4);
		printk(KERN_EMERG"%-4d"
			"0x%-8x %-3s%-4d%-4d%-5s"
			"0x%-8x %-2d%-2d"
			"0x%-8x 0x%-8x\n" 
			, index 
			,io_config,((io_config&(1<<9))&&(((io_config>>2)&0xF)==0))?"EN":"NO",((((io_config>>6)&7)+1)*2),(io_config>>2)&0xF,pull_str[io_config&3]
			,io_inout,io_inout&2,io_inout&1
			, io_intr_cfg,io_intr_status) ;
	}
	wake_enable0 = *(vir_address_int + 0x100008/4);
	wake_enable1 = *(vir_address_int + 0x10000C/4);
	printk(KERN_EMERG"-------------WAKEUP_GPIO-------------------");
	printk(KERN_EMERG"0x%x  0x%x\n" , wake_enable0,wake_enable1);
	printk(KERN_EMERG"wakeup gpio:");
	for(index = 0 ; index < 32 ; index ++)
	{
		if(wake_enable0&(1<<index))
		{
			printk(" %d " , wake_enable0_gpio_index[31-index]);
		}
		if(wake_enable1&(1<<index))
		{
			printk(" %d " , wake_enable1_gpio_index[31-index]);
		}
	}
	printk("\n");
}

#define SUSPEND_RESUME_COUNT 1024
unsigned long int  suspend_resume_base_addr[SUSPEND_RESUME_COUNT*3];
//suspend_resume_debug
//bit[0]		dupm all ldo status
//bit[1]		dump gpio status
//bit[2]		dump interrupts
//bit[31]		exec command set

unsigned long int suspend_resume_debug = 0;
#ifdef SUSPEND_RESUME_DEBUG_DUMP_REGULATOR
extern void reg_debug_consumers_show_all(void);
extern void spmi_debug_dump_all_ldo(void);
#endif

#ifdef SUSPEND_RESUME_DEBUG_DUMP_GPIO
void gpio_debug_show_all(void);
#endif
static void print_current_time(char *annotation)
{
	struct timespec ts;
	struct rtc_time tm;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	printk("PM: suspend %s %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
		annotation, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
}

void after_dpm_suspend_start_callback(void)
{
	int i = 0;
	unsigned long int *suspend_value=suspend_resume_base_addr+SUSPEND_RESUME_COUNT;
	pr_err("wgz %s\n",__FUNCTION__);
	if(suspend_resume_debug&(1<<31))
	{
		for(i = 0 ; i < SUSPEND_RESUME_COUNT; ++i)
		{
			if(suspend_resume_base_addr[i] != 0)
			{
				//resume_value[i] = get_phy_value(suspend_resume_base_addr[i]);
				set_phy_value(suspend_resume_base_addr[i],suspend_value[i]);
				pr_err("suspend_resume_base_addr[%d] = 0x%lx ,suspend_value[%d] = 0x%lx\n" 
					,i, suspend_resume_base_addr[i] , i,suspend_value[i]);
				udelay(10);
			}
		}
	}
	print_current_time("after_dpm_suspend_start_callback");
	
}

void last_suspend_callback(void)
{
	debug_printk("wgz %s\n",__FUNCTION__);
#ifdef SUSPEND_RESUME_DEBUG_DUMP_REGULATOR
	if(suspend_resume_debug&(1<<0))reg_debug_consumers_show_all();
	if(suspend_resume_debug&(1<<0))spmi_debug_dump_all_ldo();
#endif
#ifdef SUSPEND_RESUME_DEBUG_DUMP_GPIO
	if(suspend_resume_debug&(1<<1))gpio_debug_show_all();
	if(suspend_resume_debug&(1<<1))dump_gpio_register();
#endif
	print_current_time("last_suspend_callback");
	
}


void resume_first(void)
{
	int i = 0;
	unsigned long int *resume_value=suspend_resume_base_addr+SUSPEND_RESUME_COUNT*2;
	debug_printk("wgz %s\n",__FUNCTION__);
	print_current_time("resume_first");
#ifdef SUSPEND_RESUME_DEBUG_DUMP_REGULATOR
	if(suspend_resume_debug&(1<<0))spmi_debug_dump_all_ldo();
	if(suspend_resume_debug&(1<<0))reg_debug_consumers_show_all();
#endif
#ifdef SUSPEND_RESUME_DEBUG_DUMP_GPIO
	if(suspend_resume_debug&(1<<1))dump_gpio_register();
	if(suspend_resume_debug&(1<<1))gpio_debug_show_all();
#endif
	if(suspend_resume_debug&(1<<31))
	{
		for(i = 0 ; i < SUSPEND_RESUME_COUNT; ++i)
		{
			if(suspend_resume_base_addr[i] != 0)
			{
				debug_printk("suspend_resume_base_addr[%d] = 0x%lx , resume_value[%d] = 0x%lx \n" 
					,i, suspend_resume_base_addr[i] ,i, resume_value[i]);
				set_phy_value(suspend_resume_base_addr[i],resume_value[i]);
				udelay(10);  
			}
		}
	}
}

extern void debugfs_dump_gic(void);
void resume_before_irq_enabled(void)
{
	pr_err("wgz resume_before_irq_enabled\n");
	//debugfs_dump_gic();
}

void suspend_after_irq_disabled(void)
{
	pr_err("wgz suspend_after_irq_disabled\n");
	//debugfs_dump_gic();
}

void suspend_after_gic_irq_suspended(void)
{
	pr_err("wgz suspend_after_gic_irq_suspended\n");
	if(suspend_resume_debug&(1<<2))debugfs_dump_gic();
}

void resume_before_gic_irq_resumed(void)
{
	pr_err("wgz resume_before_gic_irq_resumed\n");
	if(suspend_resume_debug&(1<<2))debugfs_dump_gic();
}

extern void debugfs_msm_tlmm_v4_gp_irq_show(void) ;
void suspend_after_msm_tlmm_v4_gp_irq_suspended(void)
{
	pr_err("%s enter\n",__FUNCTION__);
	if(suspend_resume_debug&(1<<2))
	{
		debugfs_msm_tlmm_v4_gp_irq_show();
	}
}

void resume_before_msm_tlmm_v4_gp_irq_resumed(void)
{
	pr_err("%s enter\n",__FUNCTION__);
	if(suspend_resume_debug&(1<<2))
	{
		debugfs_msm_tlmm_v4_gp_irq_show();
	}
}

extern void debugfs_spmi_pmic_arb_resume(void);
void before_spmi_pmic_arb_resumed(void)
{
	pr_err("%s enter\n",__FUNCTION__);
	if(suspend_resume_debug&(1<<2))
	{
		debugfs_spmi_pmic_arb_resume();
	}

}





static int resume_suspend_set(void *data, u64 val)
{
	*(u32*)data = (u32)val;
	if(val == 0)
	{
		after_dpm_suspend_start_callback();
		last_suspend_callback();
	}
	else
	{
		resume_before_msm_tlmm_v4_gp_irq_resumed();
		resume_before_gic_irq_resumed();
		resume_first();
	}
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(fops_resume_suspend, 0 , resume_suspend_set, "0x%08llx\n");

static unsigned long suspend_resume_test = 0;
static unsigned long suspend_resume_addr= 0;

unsigned int gic_wakeup_irqs[32] = {0};
static unsigned long gic_wakeup_irqs_addr=(unsigned long)&gic_wakeup_irqs;
static void init_suspend_resume_fs(void)
{
	struct dentry *suspend_resume_dir = debugfs_create_dir("suspend_resume", debug_wgz);
	debugfs_create_file("test", 0777,suspend_resume_dir, &suspend_resume_test,&fops_resume_suspend);
	debugfs_create_x64("suspend_resume_addr", S_IRUGO, suspend_resume_dir,(u64 *)&suspend_resume_addr);
	debugfs_create_x64("suspend_resume_debug", S_IRWXUGO, suspend_resume_dir,(u64 *)&suspend_resume_debug);
	debugfs_create_x64("gic_wakeup_irqs", S_IRWXUGO, suspend_resume_dir,(u64 *)&gic_wakeup_irqs_addr);
}

static void init_suspend_resume(void)
{

		after_dpm_suspend_start = after_dpm_suspend_start_callback;
		first_step_for_resume = resume_first;
		resume_before_irq_enable = resume_before_irq_enabled;
		suspend_after_irq_disable = suspend_after_irq_disabled;
		last_step_for_suspend = last_suspend_callback;
		suspend_resume_addr= (unsigned long) &suspend_resume_base_addr;
		resume_before_gic_irq_resume = resume_before_gic_irq_resumed;
		suspend_after_gic_irq_suspend = suspend_after_gic_irq_suspended;
		resume_before_msm_tlmm_v4_gp_irq_resume = resume_before_msm_tlmm_v4_gp_irq_resumed;
		suspend_after_msm_tlmm_v4_gp_irq_suspend = suspend_after_msm_tlmm_v4_gp_irq_suspended;
		before_spmi_pmic_arb_resume = before_spmi_pmic_arb_resumed;
		init_suspend_resume_fs();
}
#else
static void init_suspend_resume(void)
{
}
#endif

/**************************************************************************************/

/************************************gpio signal generate***************************************/
#ifdef GPIO_SIGNAL_GENERATE
struct sig_set
{
	int value;
	int usec;
};

struct sig_set sig_high[] = {
	{0,20},
	{1,70},
};

struct sig_set sig_low[] = {
	{0,70},
	{1,20},
};

struct sig_set eod[] = {
	{0,20},
	{1,180},
};

static unsigned long gpio_no = 192;
static unsigned long light = 0xc9;
static int gpio_signal_output(int value)
{
	int rc;
	struct sig_set *s = value ? sig_high : sig_low;
	rc = gpio_direction_output(gpio_no, s[0].value);
	if(rc){
		pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
		return rc;
	}
	udelay(s[0].usec);
	rc = gpio_direction_output(gpio_no, s[1].value);
	if(rc){
		pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
		return rc;
	}
	udelay(s[1].usec);
	return rc;
}
static int gpio_signal_generate_trigger(void *data, u64 val)
{
	int rc;
	int index;
	unsigned int value = val;
	value = value;
	rc = gpio_request(gpio_no, "gpio_sig_gen");
	if (rc) {
		pr_err("%s: Failed to request gpio %ld,rc = %d\n",
				__func__, gpio_no, rc);

		//return -1;
	}
	//enter standby mode
	rc = gpio_signal_output(1);
	if(rc){
		pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
		goto FREE_GPIO;
	}
	udelay(50);
	//output address
	for(index = 0 ; index < 3;index++) {
		rc = gpio_signal_output(0);
		if(rc){
			pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
			goto FREE_GPIO;
		}
	}
	//output value
	for(index = 7 ; index >= 0;index--){
		rc = gpio_signal_output(light&(1<<index));
		if(rc){
			pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
			goto FREE_GPIO;
		}
	}
	//output eod
	for(index = 0 ; index < 2;index++){
		rc = gpio_direction_output(gpio_no, eod[index].value);
		if(rc){
			pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
			goto FREE_GPIO;
		}
		udelay(eod[index].usec);
	}
	printk("gpio signal generate completed\n");
FREE_GPIO:
	gpio_free(gpio_no);
	return rc;
}

static int gpio_signal_set_value(void *data, u64 val)
{
	int rc = 0;
	unsigned int value = val;
	value = value;
	rc = gpio_request(gpio_no, "gpio_sig_gen");
	if (rc) {
		pr_err("%s: Failed to request gpio %ld,rc = %d\n",
				__func__, gpio_no, rc);

		//return -1;
	}
	//output address
	rc = gpio_direction_output(gpio_no, value);
	if(rc){
		pr_err("%s:%d failed gpio:%ld,rc:%d",__func__,__LINE__,gpio_no,rc);
	}	
	gpio_free(gpio_no);
	return rc;
}

static int gpio_signal_get_value(void *data, u64 *val)
{
	int rc;
	rc = gpio_request(gpio_no, "gpio_sig_gen");
	if (rc) {
		pr_err("%s: Failed to request gpio %ld,rc = %d\n",
				__func__, gpio_no, rc);
		//return -1;
	}
	*val = gpio_get_value(gpio_no);
	gpio_free(gpio_no);
	return rc;
}


DEFINE_SIMPLE_ATTRIBUTE(gpio_signal_generate_fops, NULL, gpio_signal_generate_trigger, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_signal_value, gpio_signal_get_value, gpio_signal_set_value, "0x%08llx\n");


static void init_gpio_signal_generate(void)
{
	struct dentry *gpio_signal_dir = debugfs_create_dir("signal_generate", debug_wgz);
	debugfs_create_file("start", S_IRWXUGO, gpio_signal_dir, NULL,&gpio_signal_generate_fops);
	debugfs_create_file("value", S_IRWXUGO, gpio_signal_dir, NULL,&gpio_signal_value);
	debugfs_create_x32("gpio", S_IRWXUGO, gpio_signal_dir,(u32 *)&gpio_no);
	debugfs_create_x32("light", S_IRWXUGO, gpio_signal_dir,(u32 *)&light);
}
#else
static void init_gpio_signal_generate(void)
{
}
#endif
/******************************************************************************************/


/************************************memory module test***************************************/
#ifdef MEMORY_ATTR_TEST
static int memory_attr_test_size = 1024*32/2;
static int memory_attr_test_method = 0 ; //0:copy,1:read,2:write
static int memory_attr_test_times = 1;

extern void *vmalloc_attr(unsigned long size , pgprot_t prot);

static void test_copy(pgprot_t prot ,long size,long times, char* pre_print)
{
	int i = 0;
	void *src,*dest;
	struct timespec tmstart,tmend,tmcost;
	unsigned long long nsec;
	src = vmalloc_attr(size , prot);
	dest = vmalloc_attr(size , prot);
	tmstart = CURRENT_TIME;
	
	for(i = 0 ; i < times ; i++)
	{
		memcpy(dest , src , size);
	}
	
	tmend = CURRENT_TIME;
	tmcost = timespec_sub(tmend,tmstart);
	nsec = timespec_to_ns(&tmcost);
	printk("wgz copy size:%8ld k,times:%8ld k,%8s,%8lld ms,%8lld ns\n",size/1024,times/1024,pre_print,nsec/(1000*1000) , nsec%(1000*1000));
	vfree(src);
	vfree(dest);
}

static void test_write(pgprot_t prot ,long size,long times, char* pre_print)
{
	int i = 0;
	void *src,*dest;
	struct timespec tmstart,tmend,tmcost;
	unsigned long long nsec;
	src = vmalloc_attr(size , prot);
	dest = vmalloc_attr(size , prot);
	tmstart = CURRENT_TIME;
	
	for(i = 0 ; i < times ; i++)
	{
		memstore(dest , src , size);
	}
	
	tmend = CURRENT_TIME;
	tmcost = timespec_sub(tmend,tmstart);
	nsec = timespec_to_ns(&tmcost);
	printk("wgz write size:%8ld k,times:%8ld k,%8s,%8lld ms,%8lld ns\n",size/1024,times/1024,pre_print,nsec/(1000*1000) , nsec%(1000*1000));
	vfree(src);
	vfree(dest);
}

static void test_read(pgprot_t prot ,long size,long times, char* pre_print)
{
	int i = 0;
	void *src,*dest;
	struct timespec tmstart,tmend,tmcost;
	unsigned long long nsec;
	src = vmalloc_attr(size , prot);
	dest = vmalloc_attr(size , prot);
	tmstart = CURRENT_TIME;
	
	for(i = 0 ; i < times ; i++)
	{
		memload(dest , src , size);
	}

	tmend = CURRENT_TIME;
	tmcost = timespec_sub(tmend,tmstart);
	nsec = timespec_to_ns(&tmcost);
	printk("wgz read size:%8ld k,times:%8ld k,%8s,%8lld ms,%8lld ns\n",size/1024,times/1024,pre_print,nsec/(1000*1000) , nsec%(1000*1000));
	vfree(src);
	vfree(dest);
}



/********************1. 使用PAGE_KERNEL分配内存*******************************
使用memcpy测试数据如下（lc1861，L1D=32k,L2=512k）,拷贝4G数据，
工作集大小	花费时间
		4K		680ms
		16k		650ms
		256k	800ms
		1M		3000ms
从上面数据可以看出，在L2 cache范围内的速度是很快的	4G/800ms = 5G/s
但是当工作集超出L2后，速度迅速下降 4G/3000ms = 1.33G/S
*****************************************************************************/


#define NORMAL_WRITEBACK __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL))
#define NORMAL_WRITETHROUGH __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_WT))
#define NORMAL_NONCACHEABLE __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_NC))

#define DEVICE_NGNRNE __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRnE))
#define DEVICE_NGNRE __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRE))
#define DEVICE_GRE __pgprot(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_GRE))

//使用下面标志分配的内存，既可以被kernel访问，也可以被user访问。
#define PAGE_USER_KERNEL (PAGE_KERNEL | PTE_USER)

#if 1
/********************************使用不同memory属性的测试数据如下************************************

action	size(k)	times(k)	memattr		time(ms)
copy	   4	1024	writeback   	  680
copy	   4	1024	writethrough	39296
copy	   4	1024	noncache    	54876
copy	   4	1024	NGNRNE      	07672
copy	   4	1024	NGNRE       	53420
copy	   4	1024	GRE         	36788
copy	   8	 512	writeback   	  476
copy	   8	 512	writethrough	36780
copy	   8	 512	noncache    	36776
copy	   8	 512	NGNRNE      	55208
copy	   8	 512	NGNRE       	45904
copy	   8	 512	GRE         	37596
copy	  16	 256	writeback   	  656
copy	  16	 256	writethrough	37648
copy	  16	 256	noncache    	37676
copy	  16	 256	NGNRNE      	61360
copy	  16	 256	NGNRE       	51940
copy	  16	 256	GRE         	37616
copy	  32	 128	writeback   	  668
copy	  32	 128	writethrough	37588
copy	  32	 128	noncache    	37584
copy	  32	 128	NGNRNE      	58564
copy	  32	 128	NGNRE       	48912
copy	  32	 128	GRE         	37592
copy	  64	  64	writeback   	  752
copy	  64	  64	writethrough	37576
copy	  64	  64	noncache    	37576
copy	  64	  64	NGNRNE      	58536
copy	  64	  64	NGNRE       	48864
copy	  64	  64	GRE         	37576
copy	 128	  32	writeback   	  772
copy	 128	  32	writethrough	37576
copy	 128	  32	noncache    	37568
copy	 128	  32	NGNRNE      	57892
copy	 128	  32	NGNRE       	48140
copy	 128	  32	GRE         	37576
copy	 256	  16	writeback   	  792
copy	 256	  16	writethrough	37592
copy	 256	  16	noncache    	37600
copy	 256	  16	NGNRNE      	58608
copy	 256	  16	NGNRE       	48920
copy	 256	  16	GRE         	37600
copy	 512	   8	writeback   	 1204
copy	 512	   8	writethrough	37572
copy	 512	   8	noncache    	37576
copy	 512	   8	NGNRNE      	57412
copy	 512	   8	NGNRE       	48328
copy	 512	   8	GRE         	37584
copy	1024	   4	writeback   	 2664
copy	1024	   4	writethrough	37584
copy	1024	   4	noncache    	37576
copy	1024	   4	NGNRNE      	58556
copy	1024	   4	NGNRE       	49116
copy	1024	   4	GRE         	37596
copy	2048	   2	writeback   	 2292
copy	2048	   2	writethrough	37620
copy	2048	   2	noncache    	37576
copy	2048	   2	NGNRNE      	58436
copy	2048	   2	NGNRE       	48916
copy	2048	   2	GRE         	37584
copy	4096	   1	writeback   	 2328
copy	4096	   1	writethrough	37584
copy	4096	   1	noncache    	37576
copy	4096	   1	NGNRNE      	58104
copy	4096	   1	NGNRE       	49864
copy	4096	   1	GRE         	37588
writ	   4	1024	writeback   	  672
writ	   4	1024	writethrough	  780
writ	   4	1024	noncache    	  760
writ	   4	1024	NGNRNE      	 2268
writ	   4	1024	NGNRE       	 2236
writ	   4	1024	GRE         	  748
writ	   8	 512	writeback   	  656
writ	   8	 512	writethrough	  740
writ	   8	 512	noncache    	  744
writ	   8	 512	NGNRNE      	 2288
writ	   8	 512	NGNRE       	 2244
writ	   8	 512	GRE         	  760
writ	  16	 256	writeback   	  652
writ	  16	 256	writethrough	  736
writ	  16	 256	noncache    	  772
writ	  16	 256	NGNRNE      	 2292
writ	  16	 256	NGNRE       	 2268
writ	  16	 256	GRE         	  740
writ	  32	 128	writeback   	  652
writ	  32	 128	writethrough	  736
writ	  32	 128	noncache    	  760
writ	  32	 128	NGNRNE      	 2300
writ	  32	 128	NGNRE       	 2260
writ	  32	 128	GRE         	  736
writ	  64	  64	writeback   	  656
writ	  64	  64	writethrough	  736
writ	  64	  64	noncache    	  740
writ	  64	  64	NGNRNE      	 2300
writ	  64	  64	NGNRE       	 2272
writ	  64	  64	GRE         	  740
writ	 128	  32	writeback   	  652
writ	 128	  32	writethrough	  740
writ	 128	  32	noncache    	  736
writ	 128	  32	NGNRNE      	 2300
writ	 128	  32	NGNRE       	 2264
writ	 128	  32	GRE         	  740
writ	 256	  16	writeback   	  660
writ	 256	  16	writethrough	  736
writ	 256	  16	noncache    	  740
writ	 256	  16	NGNRNE      	 2304
writ	 256	  16	NGNRE       	 2272
writ	 256	  16	GRE         	  740
writ	 512	   8	writeback   	  668
writ	 512	   8	writethrough	  736
writ	 512	   8	noncache    	  736
writ	 512	   8	NGNRNE      	 2304
writ	 512	   8	NGNRE       	 2264
writ	 512	   8	GRE         	  736
writ	1024	   4	writeback   	  672
writ	1024	   4	writethrough	  740
writ	1024	   4	noncache    	  752
writ	1024	   4	NGNRNE      	 2304
writ	1024	   4	NGNRE       	 2276
writ	1024	   4	GRE         	  740
writ	2048	   2	writeback   	  684
writ	2048	   2	writethrough	  756
writ	2048	   2	noncache    	  764
writ	2048	   2	NGNRNE      	 2308
writ	2048	   2	NGNRE       	 2268
writ	2048	   2	GRE         	  756
writ	4096	   1	writeback   	  692
writ	4096	   1	writethrough	  756
writ	4096	   1	noncache    	  772
writ	4096	   1	NGNRNE      	 2304
writ	4096	   1	NGNRE       	 2280
writ	4096	   1	GRE         	  744
read	   4	1024	writeback   	  672
read	   4	1024	writethrough	36924
read	   4	1024	noncache    	36912
read	   4	1024	NGNRNE      	36924
read	   4	1024	NGNRE       	36928
read	   4	1024	GRE         	36924
read	   8	 512	writeback   	  656
read	   8	 512	writethrough	37240
read	   8	 512	noncache    	37224
read	   8	 512	NGNRNE      	37220
read	   8	 512	NGNRE       	37236
read	   8	 512	GRE         	37224
read	  16	 256	writeback   	  668
read	  16	 256	writethrough	37376
read	  16	 256	noncache    	37368
read	  16	 256	NGNRNE      	37368
read	  16	 256	NGNRE       	37348
read	  16	 256	GRE         	60148
read	  32	 128	writeback   	  724
read	  32	 128	writethrough	37664
read	  32	 128	noncache    	37688
read	  32	 128	NGNRNE      	37260
read	  32	 128	NGNRE       	36668
read	  32	 128	GRE         	36648
read	  64	  64	writeback   	  532
read	  64	  64	writethrough	44212
read	  64	  64	noncache    	36820
read	  64	  64	NGNRNE      	36672
read	  64	  64	NGNRE       	36652
read	  64	  64	GRE         	37056
read	 128	  32	writeback   	  764
read	 128	  32	writethrough	37224
read	 128	  32	noncache    	36696
read	 128	  32	NGNRNE      	36664
read	 128	  32	NGNRE       	53108
read	 128	  32	GRE         	37708
read	 256	  16	writeback   	  772
read	 256	  16	writethrough	37720
read	 256	  16	noncache    	37724
read	 256	  16	NGNRNE      	44456
read	 256	  16	NGNRE       	37008
read	 256	  16	GRE         	37228
read	 512	   8	writeback   	  724
read	 512	   8	writethrough	36708
read	 512	   8	noncache    	37520
read	 512	   8	NGNRNE      	37616
read	 512	   8	NGNRE       	37476
read	 512	   8	GRE         	37056
read	1024	   4	writeback   	 1628
read	1024	   4	writethrough	36692
read	1024	   4	noncache    	37812
read	1024	   4	NGNRNE      	37780
read	1024	   4	NGNRE       	37812
read	1024	   4	GRE         	49908
read	2048	   2	writeback   	 4716
read	2048	   2	writethrough	39744
read	2048	   2	noncache    	37512
read	2048	   2	NGNRNE      	36676
read	2048	   2	NGNRE       	36664
read	2048	   2	GRE         	36672
read	4096	   1	writeback   	 1760
read	4096	   1	writethrough	37040
read	4096	   1	noncache    	37336
read	4096	   1	NGNRNE      	37168
read	4096	   1	NGNRE       	37828
read	4096	   1	GRE         	37784

对上面的数据可使用excel根据需要排序。
1. 对于writeback模式
	a. 写入时间670ms左右，工作集的大小对于写入速度基本没有影响，可能是系统对写入做了优化，待查明。
	b. 对于读入，在工作集小于L2Cache时，读取速度变化不大700ms以内，但是当工作集大于L2Cache时，
		需要时间便很快增加1600ms以上。
		原因：大于L2cache之后，需要从内存中读取，所以时间变长。

2. writethrough和Non Cache模式
	a. 写入速度和writeback差不多，在700ms左右。
		原因：没有cache的写入理论应该需要更长时间，可能系统对写入做了优化。
	b. 读取速度在37000ms左右，原大于writeback模式时的时间。
		原因：没有cache，每次都要从总线读取，所以时间很长。
	c.	wirtethrough和Non Cache表现一致的原因：
		见cortex_a72_mpcore_trm_100095_0003_06_en.pdf page291
		The Cortex-A72 processor memory system treats all Write-Through pages as Non-cacheable.

3. Device NGNRNE模式
	a.	写入速度要慢于memory模式，在2300ms左右。
		原因：不能缓存，不能合并，所以时间最长。
	b.	读取速度，37000ms左右，和writeback差不多，device是不能预读的，理论应该比Memory 的NonCacheable
		慢才对，但实际差不多，原因待查。

4. Device的NGNRE
	a.	写入速度2200ms左右，比NGNRNE稍快一点，因为是可以early ack。
	b.	读取速度37000ms左右，没变化。

5.	Device的GRE模式
	a.	写入时间740ms左右，为什么会这么快，跟writeback模式差不多了，难道Gathering对速度的影响这么大吗？
		待查

	b . 读取时间37000ms左右，没变化

******************************************************************************************************/
#endif

void memory_attr_test_copy(void)
{
	long total_size = 4*1024*1024*1024L;
	long malloc_size = 4096;
	for(malloc_size = 4096;malloc_size<=4*1024*1024;malloc_size*=2)
	{
		test_copy(NORMAL_WRITEBACK ,malloc_size,total_size/malloc_size ,    "writeback   ");
		test_copy(NORMAL_WRITETHROUGH ,malloc_size,total_size/malloc_size , "writethrough");
		test_copy(NORMAL_NONCACHEABLE ,malloc_size,total_size/malloc_size , "noncache    ");

		test_copy(DEVICE_NGNRNE ,malloc_size,total_size/malloc_size ,    	"NGNRNE      ");
		test_copy(DEVICE_NGNRE ,malloc_size,total_size/malloc_size ,		"NGNRE       ");
		test_copy(DEVICE_GRE ,malloc_size,total_size/malloc_size , 			"GRE         ");
	}
}

void memory_attr_test_write(void)
{
	long total_size = 4*1024*1024*1024L;
	long malloc_size = 4096;
	for(malloc_size = 4096;malloc_size<=4*1024*1024;malloc_size*=2)
	{
		test_write(NORMAL_WRITEBACK ,malloc_size,total_size/malloc_size ,    "writeback   ");
		test_write(NORMAL_WRITETHROUGH ,malloc_size,total_size/malloc_size , "writethrough");
		test_write(NORMAL_NONCACHEABLE ,malloc_size,total_size/malloc_size , "noncache    ");

		test_write(DEVICE_NGNRNE ,malloc_size,total_size/malloc_size ,    	 "NGNRNE      ");
		test_write(DEVICE_NGNRE ,malloc_size,total_size/malloc_size ,		 "NGNRE       ");
		test_write(DEVICE_GRE ,malloc_size,total_size/malloc_size ,			 "GRE         ");
	}
}

void memory_attr_test_read(void)
{
	long total_size = 4*1024*1024*1024L;
	long malloc_size = 4096;
	for(malloc_size = 4096;malloc_size<=4*1024*1024;malloc_size*=2)
	{
		test_read(NORMAL_WRITEBACK ,malloc_size,total_size/malloc_size ,    "writeback   ");
		test_read(NORMAL_WRITETHROUGH ,malloc_size,total_size/malloc_size , "writethrough");
		test_read(NORMAL_NONCACHEABLE ,malloc_size,total_size/malloc_size , "noncache    ");

		test_read(DEVICE_NGNRNE ,malloc_size,total_size/malloc_size ,  		"NGNRNE      ");
		test_read(DEVICE_NGNRE ,malloc_size,total_size/malloc_size , 		"NGNRE       ");
		test_read(DEVICE_GRE ,malloc_size,total_size/malloc_size , 			"GRE         ");
	}
}


static int memory_attr_test_trigger(void *data, u64 val)
{
	val = val;
	data = data;
	if(memory_attr_test_method == 0)
	{
		//memory_attr_test_copy();
		memory_attr_test_write();
		memory_attr_test_read();
	}
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(memory_attr_test_fops, NULL, memory_attr_test_trigger, "0x%08llx\n");


static void init_memory_attr_test(void)
{
	struct dentry *memory_attr_test = debugfs_create_dir("memory_attr_test", debug_wgz);
	debugfs_create_file("start", S_IRWXUGO, memory_attr_test, NULL,&memory_attr_test_fops);
	debugfs_create_x32("size", S_IRWXUGO, memory_attr_test,(u32 *)&memory_attr_test_size);
	debugfs_create_x32("times", S_IRWXUGO, memory_attr_test,(u32 *)&memory_attr_test_times);
	debugfs_create_x32("method", S_IRWXUGO, memory_attr_test,(u32 *)&memory_attr_test_method);
}
#else
static void init_memory_attr_test(void)
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
	init_suspend_resume();
	init_gpio_signal_generate();
	init_memory_attr_test();
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

