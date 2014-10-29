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





#include <linux/vmalloc.h>
#include <linux/io.h>

#include <asm/cp15.h>
#include <asm/cputype.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/sizes.h>
#include <asm/system_info.h>
#include <linux/delay.h>


#include <asm/mach/map.h>
#define SUSPEND_RESUME_DEBUG
#define SUSPEND_RESUME_DEBUG_DUMP_REGULATOR
#define SUSPEND_RESUME_DEBUG_DUMP_GPIO

#define debug_printk(format, arg...) do{if(debug&1){printk(format, ##arg);}}while(0)

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



static struct dentry *debug_mem_dir;

struct test_buffer
{
	unsigned long test_buf[1024];
}__attribute__((aligned(4096)));

static unsigned long debug = 0x00000000;
static unsigned long address = 0xc0000000;
static unsigned long value = 0x00000000;
static unsigned long printptetable = 0;
static struct test_buffer test_array_buf = {.test_buf = {0}};
static unsigned long test_buf_addr = (unsigned long)&(test_array_buf.test_buf[0]);
static unsigned long count = 1;
static unsigned long printmore = 1;
static unsigned long vir_2_phy = 1;
static unsigned long phy_2_vir = 1;
static unsigned long phy_addr = 0x20000;
static unsigned long phy_value = 0;
static unsigned long phy_printmore = 1;

static unsigned long pid = 0;
static unsigned long task_struct_addr = 0;





#define ARM_FIRST_LEVEL_MASK 0x3
#define ARM_FIRST_LEVEL_1M 0x2
#define ARM_FIRST_LEVEL_1K_TABLE 0x1
#define ARM_FIRST_LEVEL_4K_TABLE 0x3
#define ARM_FIRST_LEVEL_NONE 0x0

#define ARM_FIRST_1M_SHIFT 20
#define ARM_FIRST_LEVEL_SHIFT 20

#define ARM_SEC_LEVEL_MASK 0x3
#define ARM_SEC_LEVEL_4K 0x2
#define ARM_SEC_LEVEL_64K 0x1
#define ARM_SEC_LEVEL_1K 0x3
#define ARM_SEC_LEVEL_NONE 0x0

static int isaddressvalid(int write,unsigned long vir_addr)
{
	pgd_t *pgd;
	pte_t *pte;
	pte_t *pte_hw_arm;

	unsigned long l_pgd = 0 ; 
	unsigned long l_pte = 0;
	
	if(!virt_addr_valid(vir_addr))
	{
		debug_printk("address(0x%08lx) is not valid\n" , vir_addr);
		//return 0;
	}
	pgd = pgd_offset(current->active_mm,vir_addr);
	l_pgd = (unsigned long )(*pgd)[0];
//	debug_printk("init_mm = %p , current->mm = %p taskid = %d , pgd=0x%p , l_pgd=0x%08lx\n" , &init_mm , current->mm , current->pid , pgd , l_pgd);
	if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , not matched\n" , vir_addr , l_pgd);
		return 0;
	}

	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_4K_TABLE)
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , match 4k table , not support\n",vir_addr,l_pgd);
		return 0;
	}
	
	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
	{
		debug_printk("arm 1M table \n");
		debug_printk("l_pgd= %08lx , vir_addr=%08lx , phy_addr = %08lx\n", l_pgd , vir_addr,(l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1)));
		return 1;
	}
	
	else //(l_pgd & ARM_FIRST_LEVEL_MASK == ARM_FIRST_LEVEL_1K_TABLE) only this
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , match 1k table \n" , vir_addr , l_pgd);

		pgd = pgd_offset(current->active_mm,vir_addr);
		pte = pte_offset_map((pmd_t*)pgd, vir_addr);
		if (pte_none(*pte)) 
		{
			debug_printk("address(0x%lx) pte is none\n" , vir_addr);
			return 0;
		}

		pte_hw_arm = pte+PTRS_PER_PTE;
		debug_printk("pte = 0x%08lx  , pte_hw_arm =0x%08lx, pfn = 0x%08lx , phy_addr = 0x%08lx\n", (unsigned long)pte_val(*pte) ,(unsigned long)pte_val(*pte_hw_arm) ,pte_pfn(*pte), (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK));

		l_pte = (unsigned long)(*pte_hw_arm);
		if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
		{
			debug_printk("page not present\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_64K)
		{
			debug_printk("64k page , not support\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_1K)
		{
			debug_printk("extend small 4k ok\n");
		}

		else //4k
		{
			debug_printk("ok 4k page\n");
			return 1;
		}
	}
	
	return 1;
}

static unsigned long get_phy_address(unsigned long vir_addr)
{
	pgd_t *pgd;
	pte_t *pte;
	pte_t *pte_hw_arm;

	unsigned long l_pgd = 0 ; 
	unsigned long l_pte = 0;
	unsigned long phy_address = 0;

	
	if(!virt_addr_valid(vir_addr))
	{
		debug_printk("address(0x%08lx) is not valid\n" , vir_addr);
	}
	pgd = pgd_offset(current->active_mm,vir_addr);
	l_pgd = (unsigned long )(*pgd)[0];
//	debug_printk("init_mm = %p , current->mm = %p taskid = %d , pgd=0x%p , l_pgd=0x%08lx\n" , &init_mm , current->mm , current->pid , pgd , l_pgd);
	if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , not matched\n" , vir_addr , l_pgd);
		return 0;
	}

	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_4K_TABLE)
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , match 4k table , not support\n",vir_addr,l_pgd);
		return 0;
	}
	
	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
	{
		debug_printk("arm 1M table \n");
		debug_printk("l_pgd= %08lx , vir_addr=%08lx , phy_addr = %08lx\n", l_pgd , vir_addr,(l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1)));
		phy_address = (l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1));
		return phy_address;
	}
	
	else //(l_pgd & ARM_FIRST_LEVEL_MASK == ARM_FIRST_LEVEL_1K_TABLE) only this
	{
		debug_printk("address(0x%08lx) pgd = 0x%08lx , match 1k table \n" , vir_addr , l_pgd);

		pgd = pgd_offset(current->active_mm,vir_addr);
		pte = pte_offset_map((pmd_t*)pgd, vir_addr);
		if (pte_none(*pte)) 
		{
			debug_printk("address(0x%lx) pte is none\n" , vir_addr);
			return 0;
		}

		pte_hw_arm = pte+PTRS_PER_PTE;
		debug_printk("pte = %08lx  , pte_hw_arm =%08lx, pfn = %08lx , phy_addr = %08lx\n", (unsigned long)pte_val(*pte) ,(unsigned long)pte_val(*pte_hw_arm) ,pte_pfn(*pte), (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK));

		phy_address = (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK);
			
		l_pte = (unsigned long)(*pte_hw_arm);
		if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
		{
			debug_printk("page not present\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_64K)
		{
			debug_printk("64k page , not support\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_1K)
		{
			debug_printk("extend small 4k ok\n");
		}

		else //4k
		{
			debug_printk("ok 4k page\n");
		}
	}
	
	return phy_address;
}




static int printptetablefunc(void *data, u64 *val)
{
	pgd_t *pgd;
	pte_t *pte;
	pte_t *pte_hw_arm;
	int mpgd_index = 0;
	int mpte_index = 0;
	int mpgd_count = 0; 
	int mpte_count = 0; 
	unsigned long m_address = 0;

	unsigned long l_pgd = 0;

	mpgd_count = pgd_index(0xFFFFFFFF); 
	debug_printk(KERN_EMERG"mpgd_count = %d\n" , mpgd_count);
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset(current->active_mm,m_address);
		debug_printk(KERN_EMERG"mpgd_index= %08d ,#%08lx pgd is %p ,*pgd = %08x , %08x\n" , mpgd_index,m_address , pgd , (*pgd)[0], (*pgd)[1]);
	}
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset(current->active_mm,m_address);
		l_pgd = (*pgd)[0];
		if(pgd_none(*pgd))
		{
			//debug_printk(KERN_EMERG"#%08lx pgd is none\n" , m_address);
			continue;
		}
		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
		{
			debug_printk("\n pgd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			debug_printk(" %08lx-1M ", (unsigned long)(*pgd)[0]);
			debug_printk(" %08lx-1M ", (unsigned long)(*pgd)[1]);
			debug_printk("\n");
		}

		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1K_TABLE)
		{
			mpte_count = PMD_SIZE/PAGE_SIZE;
			debug_printk("\n pgd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpte_index*PAGE_SIZE;
				pte = pte_offset_map((pmd_t*)pgd, m_address);
				if(mpte_index % 16 == 0)
				{
					debug_printk("\n"); 
				}
				if(mpte_index % 1024 == 0)
				{
					debug_printk("\n"); 
				}
				pte_hw_arm = pte+PTRS_PER_PTE;
				debug_printk(" %08lx", (unsigned long)pte_val(*pte_hw_arm));
				pte_unmap(*pte);
			}			
		}
	
	}
	
	return 0;
}

static int debugfs_set_reg(void *data, u64 val)
{
	unsigned long vir_addr = address;
	if(isaddressvalid(1 , vir_addr))
	{
		debug_printk("val is 0x%llx , address = 08x%lx\n" , val , vir_addr);
		*(u32 *)vir_addr = val;
	}
	return 0;
}
static int debugfs_get_reg(void *data, u64 *val)
{
	unsigned long vir_addr = address;
	if(isaddressvalid(1 , vir_addr))
	{
		*val = *(u32 *)vir_addr;
		debug_printk("val is 0x%llx , address = 0x%lx &test_array_buf=0x%p, size is 4096 bytes\n" , *val , vir_addr , &test_array_buf);
	}
	else
	{
		*val = 0x57575777;
	}
	return 0;
}

static int printmemsfunc(void *data, u64 *val)
{
	int index = 0;
	unsigned long addr;
	addr = address;
	for(index =0; index < count;++index)
	{
		if(addr % 1024 == 0)
		{
			debug_printk("\n");
		}
		if(addr % 4096 == 0)
		{
			debug_printk("\n");
		}
		if(addr %16 == 0)
		{
			debug_printk("\n");
			debug_printk("%08x: " , (u32)addr);
		}
		debug_printk("%08x " , *(u32 *)addr);
		addr += 4;
	}
	return 0;
}

static int vir_2_phy_func(void *data, u64 val)
{
	unsigned long vir_addr = val ; 
	vir_2_phy = get_phy_address(vir_addr);
	return 0;
}


static int u32_get_func(void *data, u64 *val)
{
	*val = *(u32 *)data;
	return 0;
}



static int phy_2_vir_func(void *data, u64 val)
{
	pgd_t *pgd;
	pte_t *pte;
	pte_t *pte_hw_arm;
	int mpgd_index = 0;
	int mpte_index = 0;
	int mpgd_count = 0; 
	int mpte_count = 0; 
	unsigned long m_address = 0;
	unsigned long m_phy_address = val;

	unsigned long l_pgd = 0;

	mpgd_count = pgd_index(0xFFFFFFFF); 
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset(current->active_mm,m_address);
		l_pgd = (*pgd)[0];
		if(pgd_none(*pgd))
		{
			continue;
		}
		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
		{
			if(l_pgd>>PGDIR_SHIFT== m_phy_address >> PGDIR_SHIFT)
			{
				phy_2_vir = m_address|(m_phy_address&(PGDIR_SIZE-1));
				debug_printk("\n");
				debug_printk("vir_addr = %08lx ,phy_addr = %08lx ,l_pgd=%08lx  " , m_address|(m_phy_address&(PGDIR_SIZE-1)) , m_phy_address ,l_pgd);
			}	
		}

		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1K_TABLE)
		{
			mpte_count = PMD_SIZE/PAGE_SIZE;
			//debug_printk("\n pmd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpte_index*PAGE_SIZE;
				pte = pte_offset_map((pmd_t*)pgd, m_address);
				if(pte_pfn(*pte) == m_phy_address >> PAGE_SHIFT && pte_present(*pte))
				{
					phy_2_vir = m_address|(m_phy_address&(PGDIR_SIZE-1));
					debug_printk("\n");
					pte_hw_arm = pte+PTRS_PER_PTE;
					debug_printk("vir_addr = %08lx ,phy_addr = %08lx ,pte_hw_arm=%08lx  " , m_address|(m_phy_address&(PAGE_SIZE-1)) , m_phy_address ,(unsigned long)pte_val(*pte_hw_arm));
				}
				pte_unmap(*pte);
			}			
		}
	
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
		//vir_address = mem_io_remap((phys_addr_t)phy_addr_temp , count);

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
		vir_address = ioremap((phys_addr_t)phy_addr_temp , size); 
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
		iounmap(vir_address);
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
	set_phy_value(phy_addr , (u32)val);
	return 0;
}
static int phy_value_get_func(void *data, u64 *val)
{
	void __iomem *vir_address;
	vir_address = remap_io_or_mem((phys_addr_t)phy_addr,count);
	if(vir_address != NULL)
	{
		isaddressvalid(0 , ((unsigned long)vir_address));
		*val = *(u32 *)vir_address;
		unmap_io_or_mem(phy_addr , vir_address);
	}
	return 0;
}

static int phy_printmemsfunc_real(phys_addr_t phy_address , unsigned long length)
{
	void __iomem *vir_address;
	int index;
	vir_address = remap_io_or_mem(phy_address,length);
	
	for(index =0; index < length;++index)
	{
		if((unsigned long)vir_address % 512 == 0)
		{
			debug_printk("\n");
		}
		if((unsigned long)vir_address % 4096 == 0)
		{
			debug_printk("vir_address=%p\n" , vir_address);
		}
		if((unsigned long)vir_address %16 == 0)
		{
			debug_printk("\n");
		}
		debug_printk("%08x " , *(u32 *)vir_address);
		vir_address += 4;
	}
	if(vir_address != NULL)
	{
		unmap_io_or_mem(phy_address , vir_address);
	}
	return 0;
}


static int phy_printmemsfunc(void *data, u64 *val)
{
	return phy_printmemsfunc_real((phys_addr_t)phy_addr,count);
}


static int task_struct_addr_get_func(void *data, u64 *val)
{
	struct task_struct *task = pid ? find_task_by_vpid(pid) : current;
	if(task == 0)
	{
		debug_printk(KERN_EMERG"can not find pid=%ld\n" , pid) ;
		return 0;
	}
	debug_printk(KERN_EMERG"pid = %d\n" , task->pid) ;
	debug_printk(KERN_EMERG"task = %p\n" , task) ;
	debug_printk(KERN_EMERG"real_cred = %p\n" , task->real_cred) ;
	debug_printk(KERN_EMERG"cred = %p\n" , task->cred) ;
	*val = (u32)task;
	return 0;
}




DEFINE_SIMPLE_ATTRIBUTE(fops_reg, debugfs_get_reg, debugfs_set_reg, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_printptetable, printptetablefunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_printmems, printmemsfunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_vir_2_phy, u32_get_func, vir_2_phy_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_2_vir, u32_get_func, phy_2_vir_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_value, phy_value_get_func, phy_value_set_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_printmems, phy_printmemsfunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_task_struct_addr, task_struct_addr_get_func, 0, "0x%08llx\n");









static void init_debug_memfs(void)
{
	debug_mem_dir = debugfs_create_dir("debug_mem", NULL);
	debugfs_create_x32("debug", S_IRWXUGO, debug_mem_dir,(u32 *)&debug);
	debugfs_create_x32("address", S_IRWXUGO, debug_mem_dir,(u32 *)&address);
	debugfs_create_file("value", 0777,debug_mem_dir, &value,&fops_reg);
	debugfs_create_file("printptetable", 0777,debug_mem_dir, &printptetable,&fops_printptetable);
	debugfs_create_x32("test_buf_addr", S_IRWXUGO, debug_mem_dir,(u32 *)&test_buf_addr);
	debugfs_create_file("printmems", 0777,debug_mem_dir, &printmore,&fops_printmems);
	debugfs_create_x32("count", S_IRWXUGO, debug_mem_dir,(u32 *)&count);
	debugfs_create_file("vir_2_phy", 0777,debug_mem_dir, &vir_2_phy,&fops_vir_2_phy);
	debugfs_create_file("phy_2_vir", 0777,debug_mem_dir, &phy_2_vir,&fops_phy_2_vir);
	debugfs_create_x32("phy_addr", S_IRWXUGO, debug_mem_dir,(u32 *)&phy_addr);
	debugfs_create_file("phy_value", 0777,debug_mem_dir, &phy_value,&fops_phy_value);
	debugfs_create_file("phy_printmems", 0777,debug_mem_dir, &phy_printmore,&fops_phy_printmems);
}

static void init_task_structfs(void)
{
	
	struct dentry *task_struct_dir = debugfs_create_dir("task_struct", debug_mem_dir);
	debugfs_create_x32("pid", S_IRWXUGO, task_struct_dir,(u32 *)&pid);
	debugfs_create_file("task_struct_addr", 0777,task_struct_dir, &task_struct_addr,&fops_task_struct_addr);
}

#if 1		//CPU

static unsigned long cpu_id = 0;
static int cpu_id_opcode2 = 0;
static int cpu_id_crn = 0;
static int cpu_id_crm = 0;
static int cpu_id_mrc_mcr = 1;



static unsigned int vir_address_command=0;
static unsigned int command_context = 0xee105f10;//mrc	p15, 0, %0, c0, c0, 0
static void composite_command_context(void)
{
	command_context = *(unsigned int*)vir_address_command;
	command_context = command_context & ~(1<<20|0xf<<16|7<<5|0xf);
	command_context = command_context|cpu_id_mrc_mcr<<20|cpu_id_crn<<16|cpu_id_opcode2<<5|cpu_id_crm;
	debug_printk("command_context = 0x%x \n" , command_context);
}

static int cpu_id_get(void *data, u64 *val)
{
	unsigned int value = 0;
	static int suspend = 0;
	suspend = !suspend;
	if(vir_address_command != 0)
	{
		unsigned long phy_addr = 0 ; 
		phy_addr = get_phy_address(vir_address_command);
		composite_command_context();
		set_phy_value(phy_addr , command_context);
		
	}
	//unsigned int command = 0xcc;
	asm(
			"nop\n\t"
			"readaddr:mrc	p15, 0, %0, c0, c0, 0\n\t"	
			"adr %1,readaddr"
		    : "=r" (value),"=r"(vir_address_command)				
		    : 				
		    : "cc"
		    );						
	debug_printk(KERN_EMERG"value = 0x%x\n" , value) ;
	*val = (u32)value;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_cpu_id, cpu_id_get, 0, "0x%08llx\n");


static void init_cpu_info_structfs(void)
{
	struct dentry *cpu_info_dir = debugfs_create_dir("cpu_info", debug_mem_dir);
	debugfs_create_file("cpu_id", 0777,cpu_info_dir, &cpu_id,&fops_cpu_id);
	debugfs_create_x32("cpu_id_opcode2", S_IRWXUGO, cpu_info_dir,(u32 *)&cpu_id_opcode2);
	debugfs_create_x32("cpu_id_crn", S_IRWXUGO, cpu_info_dir,(u32 *)&cpu_id_crn);
	debugfs_create_x32("cpu_id_crm", S_IRWXUGO, cpu_info_dir,(u32 *)&cpu_id_crm);
	debugfs_create_x32("cpu_id_mrc_mcr", S_IRWXUGO, cpu_info_dir,(u32 *)&cpu_id_mrc_mcr);
}
#endif

#define MSM8916_GPIO_START 0x01000000
#define MSM8916_GPIO_LENGTH 0x300000
#define MSM8916_GPIO_NUM 122


static void dump_gpio_register(void)
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


#ifdef SUSPEND_RESUME_DEBUG
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
				pr_err("suspend_resume_base_addr[i] = 0x%lx ,suspend_value[i] = 0x%lx\n" 
					, suspend_resume_base_addr[i] , suspend_value[i]);
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
				debug_printk("suspend_resume_base_addr[i] = 0x%lx , resume_value[i] = 0x%lx \n" 
					, suspend_resume_base_addr[i] , resume_value[i]);
				set_phy_value(suspend_resume_base_addr[i],resume_value[i]);
				udelay(10);
			}
		}
	}
}

extern void debugfs_dump_gic(void);
void resume_before_irq_enabled(void)
{
	//debugfs_dump_gic();
}

void suspend_after_irq_disabled(void)
{
	//debugfs_dump_gic();
}

void suspend_after_gic_irq_suspended(void)
{
	//debugfs_dump_gic();
}

void resume_before_gic_irq_resumed(void)
{
	if(suspend_resume_debug&(1<<2))debugfs_dump_gic();
}

void suspend_after_msm_tlmm_v4_gp_irq_suspended(void)
{

}

extern void debugfs_msm_tlmm_v4_gp_irq_show(void) ;

void resume_before_msm_tlmm_v4_gp_irq_resumed(void)
{
	if(suspend_resume_debug&(1<<2))
	{
		debugfs_msm_tlmm_v4_gp_irq_show();
	}
}

extern void debugfs_spmi_pmic_arb_resume(void);
void before_spmi_pmic_arb_resumed(void)
{
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

static void init_suspend_resume_fs(void)
{
	struct dentry *suspend_resume_dir = debugfs_create_dir("suspend_resume", debug_mem_dir);
	debugfs_create_file("test", 0777,suspend_resume_dir, &suspend_resume_test,&fops_resume_suspend);
	debugfs_create_x32("suspend_resume_addr", S_IRUGO, suspend_resume_dir,(u32 *)&suspend_resume_addr);
	debugfs_create_x32("suspend_resume_debug", S_IRWXUGO, suspend_resume_dir,(u32 *)&suspend_resume_debug);
}

static void init_suspend_resume(void)
{

		after_dpm_suspend_start = after_dpm_suspend_start_callback;
		first_step_for_resume = resume_first;
		resume_before_irq_enable = resume_before_irq_enabled;
		suspend_after_irq_disable = suspend_after_irq_disabled;
		last_step_for_suspend = last_suspend_callback;
		suspend_resume_addr= (unsigned int) &suspend_resume_base_addr;
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


/**********************************Thread**********************************************/
#define DEBUG_THREAD_MONITOR
#ifdef DEBUG_THREAD_MONITOR
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


static char print_thread_name[512] = {0};

static void thread_debug_work_func(struct work_struct *work)
{
	struct task_struct *g, *p;
	int monitor_kernel = !!strstr(print_thread_name,"kernel");
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
			if(!strstr(print_thread_name,p->comm))
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

static ssize_t thread_info_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return simple_read_from_buffer(buf,size,ppos,print_thread_name,strlen(print_thread_name));
}
static ssize_t thread_info_write(struct file *file, const char __user *ubuf,size_t len, loff_t *offp)
{
	ssize_t size = 0;
	memset(print_thread_name , 0 , 512);
	size = simple_write_to_buffer(print_thread_name,511,offp,ubuf,len);
	if(print_thread_name[0] == '0')
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

static const struct file_operations thread_onfo_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = thread_info_read,
	.write = thread_info_write
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
	struct dentry *thread_dir = debugfs_create_dir("thread", debug_mem_dir);
	debugfs_create_file("thread_info", S_IRWXUGO, thread_dir, NULL,&thread_onfo_fops);
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
static int __init debug_init(void)
{

	init_debug_memfs();
	init_task_structfs();
	init_cpu_info_structfs();
	init_suspend_resume();
	init_thread_debug_fs();

	return 0;
}

static void __exit debug_exit(void)
{
	pr_info("remove\n");
	debugfs_remove_recursive(debug_mem_dir);	
}

module_init(debug_init);
module_exit(debug_exit);
MODULE_LICENSE("GPL");

