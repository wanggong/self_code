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


static struct dentry *debug_mem_dir;

struct test_buffer
{
	unsigned long test_buf[1024];
}__attribute__((aligned(4096)));

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
		printk("address(0x%08lx) is not valid\n" , vir_addr);
		//return 0;
	}
	pgd = pgd_offset(current->active_mm,vir_addr);
	l_pgd = (unsigned long )(*pgd)[0];
//	printk("init_mm = %p , current->mm = %p taskid = %d , pgd=0x%p , l_pgd=0x%08lx\n" , &init_mm , current->mm , current->pid , pgd , l_pgd);
	if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , not matched\n" , vir_addr , l_pgd);
		return 0;
	}

	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_4K_TABLE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 4k table , not support\n",vir_addr,l_pgd);
		return 0;
	}
	
	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
	{
		printk("arm 1M table \n");
		printk("l_pgd= %08lx , vir_addr=%08lx , phy_addr = %08lx\n", l_pgd , vir_addr,(l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1)));
		return 1;
	}
	
	else //(l_pgd & ARM_FIRST_LEVEL_MASK == ARM_FIRST_LEVEL_1K_TABLE) only this
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 1k table \n" , vir_addr , l_pgd);

		pgd = pgd_offset(current->active_mm,vir_addr);
		pte = pte_offset_map((pmd_t*)pgd, vir_addr);
		if (pte_none(*pte)) 
		{
			printk("address(0x%lx) pte is none\n" , vir_addr);
			return 0;
		}

		pte_hw_arm = pte+PTRS_PER_PTE;
		printk("pte = 0x%08lx  , pte_hw_arm =0x%08lx, pfn = 0x%08lx , phy_addr = 0x%08lx\n", (unsigned long)pte_val(*pte) ,(unsigned long)pte_val(*pte_hw_arm) ,pte_pfn(*pte), (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK));

		l_pte = (unsigned long)(*pte_hw_arm);
		if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
		{
			printk("page not present\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_64K)
		{
			printk("64k page , not support\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_1K)
		{
			printk("extend small 4k ok\n");
		}

		else //4k
		{
			printk("ok 4k page\n");
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
		printk("address(0x%08lx) is not valid\n" , vir_addr);
	}
	pgd = pgd_offset(current->active_mm,vir_addr);
	l_pgd = (unsigned long )(*pgd)[0];
//	printk("init_mm = %p , current->mm = %p taskid = %d , pgd=0x%p , l_pgd=0x%08lx\n" , &init_mm , current->mm , current->pid , pgd , l_pgd);
	if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , not matched\n" , vir_addr , l_pgd);
		return 0;
	}

	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_4K_TABLE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 4k table , not support\n",vir_addr,l_pgd);
		return 0;
	}
	
	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
	{
		printk("arm 1M table \n");
		printk("l_pgd= %08lx , vir_addr=%08lx , phy_addr = %08lx\n", l_pgd , vir_addr,(l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1)));
		phy_address = (l_pgd&PGDIR_MASK)|(vir_addr&(PGDIR_SIZE-1));
		return phy_address;
	}
	
	else //(l_pgd & ARM_FIRST_LEVEL_MASK == ARM_FIRST_LEVEL_1K_TABLE) only this
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 1k table \n" , vir_addr , l_pgd);

		pgd = pgd_offset(current->active_mm,vir_addr);
		pte = pte_offset_map((pmd_t*)pgd, vir_addr);
		if (pte_none(*pte)) 
		{
			printk("address(0x%lx) pte is none\n" , vir_addr);
			return 0;
		}

		pte_hw_arm = pte+PTRS_PER_PTE;
		printk("pte = %08lx  , pte_hw_arm =%08lx, pfn = %08lx , phy_addr = %08lx\n", (unsigned long)pte_val(*pte) ,(unsigned long)pte_val(*pte_hw_arm) ,pte_pfn(*pte), (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK));

		phy_address = (pte_pfn(*pte)<<PAGE_SHIFT)&(vir_addr&PAGE_MASK);
			
		l_pte = (unsigned long)(*pte_hw_arm);
		if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
		{
			printk("page not present\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_64K)
		{
			printk("64k page , not support\n");
		}
		else if((l_pte&ARM_SEC_LEVEL_MASK) == ARM_SEC_LEVEL_1K)
		{
			printk("extend small 4k ok\n");
		}

		else //4k
		{
			printk("ok 4k page\n");
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
	printk(KERN_EMERG"mpgd_count = %d\n" , mpgd_count);
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset(current->active_mm,m_address);
		printk(KERN_EMERG"mpgd_index= %08d ,#%08lx pgd is %p ,*pgd = %08x , %08x\n" , mpgd_index,m_address , pgd , (*pgd)[0], (*pgd)[1]);
	}
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset(current->active_mm,m_address);
		l_pgd = (*pgd)[0];
		if(pgd_none(*pgd))
		{
			//printk(KERN_EMERG"#%08lx pgd is none\n" , m_address);
			continue;
		}
		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
		{
			printk("\n pgd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			printk(" %08lx-1M ", (unsigned long)(*pgd)[0]);
			printk(" %08lx-1M ", (unsigned long)(*pgd)[1]);
			printk("\n");
		}

		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1K_TABLE)
		{
			mpte_count = PMD_SIZE/PAGE_SIZE;
			printk("\n pgd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpte_index*PAGE_SIZE;
				pte = pte_offset_map((pmd_t*)pgd, m_address);
				if(mpte_index % 16 == 0)
				{
					printk("\n"); 
				}
				if(mpte_index % 1024 == 0)
				{
					printk("\n"); 
				}
				pte_hw_arm = pte+PTRS_PER_PTE;
				printk(" %08lx", (unsigned long)pte_val(*pte_hw_arm));
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
		printk("val is 0x%llx , address = 08x%lx\n" , val , vir_addr);
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
		printk("val is 0x%llx , address = 0x%lx &test_array_buf=0x%p, size is 4096 bytes\n" , *val , vir_addr , &test_array_buf);
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
			printk("\n");
		}
		if(addr % 4096 == 0)
		{
			printk("\n");
		}
		if(addr %16 == 0)
		{
			printk("\n");
			printk("%08x: " , (u32)addr);
		}
		printk("%08x " , *(u32 *)addr);
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
				printk("\n");
				printk("vir_addr = %08lx ,phy_addr = %08lx ,l_pgd=%08lx  " , m_address|(m_phy_address&(PGDIR_SIZE-1)) , m_phy_address ,l_pgd);
			}	
		}

		else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1K_TABLE)
		{
			mpte_count = PMD_SIZE/PAGE_SIZE;
			//printk("\n pmd = 0x%lx ,mpgd_index=%d , m_address=0x%lx\n", (unsigned long)pgd_val(*pgd) , mpgd_index , m_address);
			for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpte_index*PAGE_SIZE;
				pte = pte_offset_map((pmd_t*)pgd, m_address);
				if(pte_pfn(*pte) == m_phy_address >> PAGE_SHIFT && pte_present(*pte))
				{
					phy_2_vir = m_address|(m_phy_address&(PGDIR_SIZE-1));
					printk("\n");
					pte_hw_arm = pte+PTRS_PER_PTE;
					printk("vir_addr = %08lx ,phy_addr = %08lx ,pte_hw_arm=%08lx  " , m_address|(m_phy_address&(PAGE_SIZE-1)) , m_phy_address ,(unsigned long)pte_val(*pte_hw_arm));
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
		printk("map memory area, phy_addr_temp=0x%lx,size=0x%lx\n" , (unsigned long)phy_addr_temp, size);
		//vir_address = mem_io_remap((phys_addr_t)phy_addr_temp , count);

		first_page = phys_to_page(phy_addr_temp);
		printk("first_page = 0x%p\n" , first_page);

		vir_address = vmap(&first_page, (size+PAGE_SIZE)>>PAGE_SHIFT, VM_MAP, PAGE_KERNEL);
		printk("vir_address = 0x%p\n" , vir_address);
		vir_address = (void*)(((unsigned long)vir_address&PAGE_MASK)| (phy_addr_temp&~PAGE_MASK));
		printk("last vir_address = 0x%p\n" , vir_address);
	}
	else
	{
		printk("map io area\n");
		vir_address = ioremap((phys_addr_t)phy_addr_temp , size); 
	}
	if(vir_address == 0)
	{
		printk("error vir_base is null");
	}
	printk("phy_addr = 0x%lx ,  vir_address = 0x%p" , (unsigned long)phy_addr_temp  , vir_address);
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


static int phy_printmemsfunc(void *data, u64 *val)
{
	void __iomem *vir_address;
	int index;
	vir_address = remap_io_or_mem((phys_addr_t)phy_addr,count);
	
	for(index =0; index < count;++index)
	{
		if((unsigned long)vir_address % 512 == 0)
		{
			printk("\n");
		}
		if((unsigned long)vir_address % 4096 == 0)
		{
			printk("vir_address=%p\n" , vir_address);
		}
		if((unsigned long)vir_address %16 == 0)
		{
			printk("\n");
		}
		printk("%08x " , *(u32 *)vir_address);
		vir_address += 4;
	}
	if(vir_address != NULL)
	{
		unmap_io_or_mem(phy_addr , vir_address);
	}
	return 0;
}


static int task_struct_addr_get_func(void *data, u64 *val)
{
	struct task_struct *task = pid ? find_task_by_vpid(pid) : current;
	if(task == 0)
	{
		printk(KERN_EMERG"can not find pid=%ld\n" , pid) ;
		return 0;
	}
	printk(KERN_EMERG"pid = %d\n" , task->pid) ;
	printk(KERN_EMERG"task = %p\n" , task) ;
	printk(KERN_EMERG"real_cred = %p\n" , task->real_cred) ;
	printk(KERN_EMERG"cred = %p\n" , task->cred) ;
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
	printk("command_context = 0x%x \n" , command_context);
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
	printk(KERN_EMERG"value = 0x%x\n" , value) ;
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


#ifdef SUSPEND_RESUME_DEBUG
#define SUSPEND_RESUME_COUNT 1024
unsigned long int  suspend_resume_base_addr[SUSPEND_RESUME_COUNT*3];
void suspend_last(void)
{
	int i = 0;
	unsigned long int *suspend_value=suspend_resume_base_addr+SUSPEND_RESUME_COUNT;
	pr_err("wgz %s\n",__FUNCTION__);
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

void resume_first(void)
{
	int i = 0;
	unsigned long int *resume_value=suspend_resume_base_addr+SUSPEND_RESUME_COUNT*2;
	pr_err("wgz %s\n",__FUNCTION__);
	for(i = 0 ; i < SUSPEND_RESUME_COUNT; ++i)
	{
		if(suspend_resume_base_addr[i] != 0)
		{
			pr_err("suspend_resume_base_addr[i] = 0x%lx , resume_value[i] = 0x%lx \n" 
				, suspend_resume_base_addr[i] , resume_value[i]);
			set_phy_value(suspend_resume_base_addr[i],resume_value[i]);
			udelay(10);
		}
	}
}

static int resume_suspend_set(void *data, u64 val)
{
	*(u32*)data = (u32)val;
	if(val == 0)
	{
		suspend_last();
	}
	else
	{
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
}

extern void (*last_step_for_suspend)(void)  ;
extern void (*first_step_for_resume)(void) ;
#endif
static int __init debug_init(void)
{

	init_debug_memfs();
	init_task_structfs();
	init_cpu_info_structfs();
#ifdef SUSPEND_RESUME_DEBUG
	last_step_for_suspend = suspend_last;
	first_step_for_resume = resume_first;
	suspend_resume_addr= (unsigned int) &suspend_resume_base_addr;
	init_suspend_resume_fs();
#endif
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

