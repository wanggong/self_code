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

#include <asm/mach/map.h>



static struct dentry *debug_mem_dir;
static struct dentry *value_dentry;
static struct dentry *address_entry;
static struct dentry *print_entry;
static struct dentry *testvalue_entry;
static struct dentry *printmore_entry;
static struct dentry *count_entry;
static struct dentry *vir_2_phy_entry;
static struct dentry *phy_2_vir_entry;
static struct dentry *phy_addr_entry;
static struct dentry *phy_value_entry;
static struct dentry *phy_printmore_entry;






static unsigned long address = 0xc0000000;
static unsigned long value = 0x00000000;
static unsigned long printptetable = 0;
static unsigned long testvalue = 0xc0000000;
static unsigned long count = 1;
static unsigned long printmore = 1;
static unsigned long vir_2_phy = 1;
static unsigned long phy_2_vir = 1;
static unsigned long phy_addr = 0x20000;
static unsigned long phy_value = 0;
static unsigned long phy_printmore = 1;




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


#if 0
#define pgd_addr(pgd , v_addr)((pgd&(~((1<<21)-1)))|(v_addr&((1<<21)-1)))


#define arm_pgd_index(addr)		((addr) >> ARM_FIRST_LEVEL_SHIFT)
#define arm_pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))
#define arm_pgd_offset_k(addr)	pgd_offset(&init_mm, addr)




#define VM_ARM_SECTION_MAPPING	0x80000000
#define VM_ARM_STATIC_MAPPING	0x40000000
#define VM_ARM_EMPTY_MAPPING	0x20000000
#define VM_ARM_MTYPE(mt)		((mt) << 20)
#define VM_ARM_MTYPE_MASK	(0x1f << 20)
#define VM_ARM_DMA_CONSISTENT	0x20000000

struct mem_type {
	pteval_t prot_pte;
	pmdval_t prot_l1;
	pmdval_t prot_sect;
	unsigned int domain;
};


static void __iomem * mem_arm_ioremap_pfn_caller(unsigned long pfn,
	unsigned long offset, size_t size, unsigned int mtype, void *caller)
{
	const struct mem_type *type;
	int err;
	unsigned long addr;
 	struct vm_struct * area;

#ifndef CONFIG_ARM_LPAE
	/*
	 * High mappings must be supersection aligned
	 */
	if (pfn >= 0x100000 && (__pfn_to_phys(pfn) & ~SUPERSECTION_MASK))
		return NULL;
#endif

	type = get_mem_type(mtype);
	if (!type)
		return NULL;

	/*
	 * Page align the mapping size, taking account of any offset.
	 */
	size = PAGE_ALIGN(offset + size);


#if 0	//wgz remove
	/*
	 * Try to reuse one of the static mapping whenever possible.
	 */
	read_lock(&vmlist_lock);
	for (area = vmlist; area; area = area->next) {
		if (!size || (sizeof(phys_addr_t) == 4 && pfn >= 0x100000))
			break;
		if (!(area->flags & VM_ARM_STATIC_MAPPING))
			continue;
		if ((area->flags & VM_ARM_MTYPE_MASK) != VM_ARM_MTYPE(mtype))
			continue;
		if (__phys_to_pfn(area->phys_addr) > pfn ||
		    __pfn_to_phys(pfn) + size-1 > area->phys_addr + area->size-1)
			continue;
		/* we can drop the lock here as we know *area is static */
		read_unlock(&vmlist_lock);
		addr = (unsigned long)area->addr;
		addr += __pfn_to_phys(pfn) - area->phys_addr;
		return (void __iomem *) (offset + addr);
	}
	read_unlock(&vmlist_lock);
#endif
	/*
	 * Don't allow RAM to be mapped - this causes problems with ARMv6+
	 */
#if 0  //wgz remove
	if (WARN_ON(pfn_valid(pfn)))
		return NULL;
#endif
	area = get_vm_area_caller(size, VM_IOREMAP, caller);
 	if (!area)
 		return NULL;
 	addr = (unsigned long)area->addr;

#if !defined(CONFIG_SMP) && !defined(CONFIG_ARM_LPAE)
	if (DOMAIN_IO == 0 &&
	    (((cpu_architecture() >= CPU_ARCH_ARMv6) && (get_cr() & CR_XP)) ||
	       cpu_is_xsc3()) && pfn >= 0x100000 &&
	       !((__pfn_to_phys(pfn) | size | addr) & ~SUPERSECTION_MASK)) {
		area->flags |= VM_ARM_SECTION_MAPPING;
		err = remap_area_supersections(addr, pfn, size, type);
	} else if (!((__pfn_to_phys(pfn) | size | addr) & ~PMD_MASK)) {
		area->flags |= VM_ARM_SECTION_MAPPING;
		err = remap_area_sections(addr, pfn, size, type);
	} else
#endif
		err = ioremap_page_range(addr, addr + size, __pfn_to_phys(pfn),
					 __pgprot(type->prot_pte));

	if (err) {
 		vunmap((void *)addr);
 		return NULL;
 	}

	flush_cache_vmap(addr, addr + size);
	return (void __iomem *) (offset + addr);
}


static void __iomem *mem_arm_ioremap_caller(phys_addr_t phys_addr, size_t size,
	unsigned int mtype, void *caller)
{
	phys_addr_t last_addr;
	phys_addr_t offset = phys_addr & ~PAGE_MASK;
 	unsigned long pfn = __phys_to_pfn(phys_addr);

 	/*
 	 * Don't allow wraparound or zero size
	 */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	return mem_arm_ioremap_pfn_caller(pfn, offset, size, mtype,caller);
}



static void __iomem *mem_arm_ioremap(phys_addr_t phys_addr, size_t size, unsigned int mtype)
{
	return mem_arm_ioremap_caller(phys_addr, size, mtype,
		__builtin_return_address(0));
}

#define mem_io_remap(cookie,size)		mem_arm_ioremap((cookie), (size), MT_DEVICE)

#endif






static int isaddressvalid(int write)
{
	pgd_t *pgd;
	pte_t *pte;
	pte_t *pte_hw_arm;

	unsigned long l_pgd = 0 ; 
	unsigned long l_pte = 0;

	
	if(!virt_addr_valid(address))
	{
		printk("address(0x%08lx) is not valid\n" , address);
		//return 0;
	}
	pgd = pgd_offset(current->active_mm,address);
	l_pgd = (unsigned long )(*pgd)[0];
//	printk("init_mm = %p , current->mm = %p taskid = %d , pgd=0x%p , l_pgd=0x%08lx\n" , &init_mm , current->mm , current->pid , pgd , l_pgd);
	if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_NONE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , not matched\n" , address , l_pgd);
		return 0;
	}

	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_4K_TABLE)
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 4k table , not support\n",address,l_pgd);
		return 0;
	}
	
	else if((l_pgd&ARM_FIRST_LEVEL_MASK) == ARM_FIRST_LEVEL_1M)
	{
		printk("arm 1M table \n");
		printk("l_pgd= %08lx , vir_addr=%08lx , phy_addr = %08lx\n", l_pgd , address,(l_pgd&PGDIR_MASK)|(address&(PGDIR_SIZE-1)));
		return 1;
	}
	
	else //(l_pgd & ARM_FIRST_LEVEL_MASK == ARM_FIRST_LEVEL_1K_TABLE) only this
	{
		printk("address(0x%08lx) pgd = 0x%08lx , match 1k table \n" , address , l_pgd);

		pgd = pgd_offset(current->active_mm,address);
		pte = pte_offset_map((pmd_t*)pgd, address);
		if (pte_none(*pte)) 
		{
			printk("address(0x%lx) pte is none\n" , address);
			return 0;
		}

		pte_hw_arm = pte+PTRS_PER_PTE;
		printk("pte = %08lx  , pte_hw_arm =%08lx, pfn = %08lx , phy_addr = %08lx\n", (unsigned long)pte_val(*pte) ,(unsigned long)pte_val(*pte_hw_arm) ,pte_pfn(*pte), (pte_pfn(*pte)<<PAGE_SHIFT)&(address&PAGE_MASK));

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
	if(isaddressvalid(1))
	{
		printk("val is 0x%llx , address = 08x%lx\n" , val , address);
		*(u32 *)address = val;
	}
	return 0;
}
static int debugfs_get_reg(void *data, u64 *val)
{
	if(isaddressvalid(0))
	{
		*val = *(u32 *)address;
		printk("val is 0x%llx , address = 0x%lx &testvalue=0x%p\n" , *val , address , &testvalue);
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
		}
		printk("%08x " , *(u32 *)addr);
		addr += 4;
	}
	return 0;
}

static int vir_2_phy_func(void *data, u64 val)
{
	address = val;
	isaddressvalid(1);
				
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
		printk("first_page = %p\n" , first_page);

		vir_address = vmap(&first_page, (size+PAGE_SIZE)>>PAGE_SHIFT, VM_IOREMAP, PAGE_KERNEL);
		printk("vir_address = %p\n" , vir_address);
		vir_address = (void*)(((unsigned long)vir_address&PAGE_MASK)| (phy_addr_temp&~PAGE_MASK));
	}
	else
	{
		printk("map io area\n");
		vir_address = ioremap((phys_addr_t)phy_addr_temp , count); 
	}
	if(vir_address == 0)
	{
		printk("error vir_base is null");
	}
	printk("phy_addr = %lx ,  vir_address = %p" , (unsigned long)phy_addr_temp  , vir_address);
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



static int phy_value_set_func(void *data, u64 val)
{
	
	void __iomem *vir_address;
	vir_address = remap_io_or_mem((phys_addr_t)phy_addr,count);
	*(u32 *)vir_address = (u32)val;
	if(vir_address != NULL)
	{
		unmap_io_or_mem(phy_addr , vir_address);
	}
	return 0;
}
static int phy_value_get_func(void *data, u64 *val)
{
	void __iomem *vir_address;
	vir_address = remap_io_or_mem((phys_addr_t)phy_addr,count);
	if(vir_address != NULL)
	{
		*val = *(u32 *)vir_address;
		address = (unsigned long)vir_address;
		isaddressvalid(0);
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






DEFINE_SIMPLE_ATTRIBUTE(fops_reg, debugfs_get_reg, debugfs_set_reg, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_printptetable, printptetablefunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_printmems, printmemsfunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_vir_2_phy, 0, vir_2_phy_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_2_vir, 0, phy_2_vir_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_value, phy_value_get_func, phy_value_set_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_printmems, phy_printmemsfunc, 0, "0x%08llx\n");








static void init_debug_memfs(void)
{
	debug_mem_dir = debugfs_create_dir("debug_mem", NULL);
	address_entry = debugfs_create_x32("address", S_IRWXUGO, debug_mem_dir,(u32 *)&address);
	value_dentry = debugfs_create_file("value", 0777,debug_mem_dir, &value,&fops_reg);
	print_entry = debugfs_create_file("printptetable", 0777,debug_mem_dir, &printptetable,&fops_printptetable);
	testvalue_entry = debugfs_create_x32("testvalue", S_IRWXUGO, debug_mem_dir,(u32 *)&testvalue);
	printmore_entry = debugfs_create_file("printmems", 0777,debug_mem_dir, &printmore,&fops_printmems);
	count_entry = debugfs_create_x32("count", S_IRWXUGO, debug_mem_dir,(u32 *)&count);
	vir_2_phy_entry = debugfs_create_file("vir_2_phy", 0777,debug_mem_dir, &vir_2_phy,&fops_vir_2_phy);
	phy_2_vir_entry = debugfs_create_file("phy_2_vir", 0777,debug_mem_dir, &phy_2_vir,&fops_phy_2_vir);
	phy_addr_entry = debugfs_create_x32("phy_addr", S_IRWXUGO, debug_mem_dir,(u32 *)&phy_addr);
	phy_value_entry = debugfs_create_file("phy_value", 0777,debug_mem_dir, &phy_value,&fops_phy_value);
	phy_printmore_entry = debugfs_create_file("phy_printmems", 0777,debug_mem_dir, &phy_printmore,&fops_phy_printmems);
}

static void remove_debug_memfs(void)
{
	debugfs_remove(value_dentry);
	debugfs_remove(address_entry);
	debugfs_remove(print_entry);
	debugfs_remove(testvalue_entry);
	debugfs_remove(vir_2_phy_entry);
	debugfs_remove(phy_2_vir_entry);
	debugfs_remove(phy_addr_entry);
	debugfs_remove(phy_value_entry);
	debugfs_remove(phy_printmore_entry);
	debugfs_remove(count_entry);
	debugfs_remove(printmore_entry);
	debugfs_remove(debug_mem_dir);
}


static int __init debug_init(void)
{

	init_debug_memfs();
	return 0;
}

static void __exit debug_exit(void)
{
	pr_info("remove\n");
	remove_debug_memfs();
}

module_init(debug_init);
module_exit(debug_exit);
MODULE_LICENSE("GPL");
