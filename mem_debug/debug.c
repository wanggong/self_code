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






static int isaddressvalid(int write)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	if(!virt_addr_valid(address))
	{
		printk("address(0x%lx) is not valid\n" , address);
		//return 0;
	}
	pgd = pgd_offset_k(address);
	if(pgd_none(*pgd))
	{
		printk("address(0x%lx) pgd is none\n" , address);
		return 0;
	}
	pud = pud_offset(pgd, address);
	if(pud_none(*pud))
	{
		printk("address(0x%lx) pud is none\n" , address);
		return 0;
	}
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
	{
		printk("address(0x%lx) pmd is none\n" , address);
		return 0;
	}
	pte = pte_offset_map(pmd, address);
	if (pte_none(*pte)) 
	{
		printk("address(0x%lx) pte is none\n" , address);
		return 0;
	}
	printk("pte = %08lx , pfn = %08lx , phy_addr = %08lx\n", (unsigned long)pte_val(*pte) ,pte_pfn(*pte), pte_pfn(*pte)<<PAGE_SHIFT);
	if(!pte_present(*pte))
	{
		printk("address(0x%lx) page is not present\n" , address);
		return 1;
	}
	if(write&&!pte_write(*pte))
	{
		printk("address(0x%lx) is readonly\n" , address);
		return 1;
	}
	
	return 1;
}



static int printptetablefunc(void *data, u64 *val)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int mpgd_index = 0;
	int mpud_index = 0;
	int mpmd_index = 0;
	int mpte_index = 0;
	int mpgd_count = 0; 
	int mpud_count = 0;
	int mpmd_count = 0;
	int mpte_count = 0; 
	unsigned long m_address = 0;

	mpgd_count = pgd_index(0xFFFFFFFF); 
	printk(KERN_EMERG"mpgd_count = %d\n" , mpgd_count);
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset_k(m_address);
		printk(KERN_EMERG"mpgd_index= %08d ,#%08lx pgd is %p ,*pgd = %08x , %08x\n" , mpgd_index,m_address , pgd , (*pgd)[0], (*pgd)[1]);
	}
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset_k(m_address);
		if(pgd_none(*pgd))
		{
			//printk(KERN_EMERG"#%08lx pgd is none\n" , m_address);
			continue;
		}
		mpud_count = PGDIR_SIZE/PUD_SIZE;
		//printk(KERN_EMERG"mpud_count = %d\n" , mpud_count);
		for(mpud_index = 0 ; mpud_index < mpud_count ; ++mpud_index)
		{
			m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE;
			pud = pud_offset(pgd, m_address);
			if(pud_none(*pud))
			{
				//printk(KERN_EMERG"#%08lx pud is none\n" , m_address);
				continue;
			}
			mpmd_count = PUD_SIZE/PMD_SIZE;
			//printk(KERN_EMERG"mpmd_count = %d\n" , mpmd_count);
			for(mpmd_index = 0 ; mpmd_index < mpmd_count ; ++mpmd_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE+PMD_SIZE*mpmd_index;
				//printk(KERN_EMERG"#%08lx befire\n" , m_address);
				pmd = pmd_offset(pud, m_address);
				//printk(KERN_EMERG"#%08lx after\n" , m_address);
				if (pmd_none(*pmd))
				{
					//printk(KERN_EMERG"#%08lx pmd is none\n" , m_address);
					continue;
				}
				mpte_count = PMD_SIZE/PAGE_SIZE;
				//printk(KERN_EMERG"mpte_count = %d\n" , mpte_count);
				printk("pmd = %lx ,mpgd_index=%d , m_address=%lx\n", (unsigned long)pmd_val(*pmd) , mpgd_index , m_address);
				for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
				{
					m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE+PMD_SIZE*mpmd_index+mpte_index*PAGE_SIZE;
					//printk(KERN_EMERG"#%08lx pte befire pmd=%p\n" , m_address , pmd);
					pte = pte_offset_map(pmd, m_address);
					//printk(KERN_EMERG"#%08lx pte=%p\n" , m_address,pte);
					if(mpte_index % 16 == 0)
					{
						printk("\n"); 
					}
					printk(" %08lx-", (unsigned long)pte_val(*pte));
					
					if (pte_none(*pte)) 
					{
						//printk(KERN_EMERG"#%08lx pte is none\n" , m_address);
						printk("NNNNN");
						continue;
					}
					//printk(KERN_EMERG"#%08lx after pte=%p\n" , m_address,pte);
					if(!pte_present(*pte))
					{
						//printk(KERN_EMERG"#%08lx pte is not present\n" , m_address);
						printk("P");
						//continue;
					}
					else
					{
						printk("N");
					}
					//printk(KERN_EMERG"#%08lx after pte_present pte=%p\n" , m_address,pte);
					if(pte_write(*pte))
					{
						//printk(KERN_EMERG"#%08lx\n" , m_address);
						//continue;
						printk("W");
					}
					else
					{
						printk("R");
					}
					if(pte_dirty(*pte))
					{
						printk("D");
					}
					else
					{
						printk("C");
					}
					if(pte_young(*pte))
					{
						printk("Y");
					}
					else
					{
						printk("O");
					}
					if(pte_exec(*pte))
					{
						printk("E");
					}
					else
					{
						printk("D");
					}
					pte_unmap(*pte);
					//printk(KERN_EMERG"#%08lx after pte_write pte=%p\n" , m_address,pte);
					//printk(KERN_EMERG"@%08lx\n" , m_address);  
				}
				printk("\n");
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
		if(addr % 512 == 0)
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
	unsigned long phy_addr = val;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int mpgd_index = 0;
	int mpud_index = 0;
	int mpmd_index = 0;
	int mpte_index = 0;
	int mpgd_count = 0; 
	int mpud_count = 0;
	int mpmd_count = 0;
	int mpte_count = 0; 
	unsigned long m_address = 0;

	mpgd_count = pgd_index(0xFFFFFFFF); 
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset_k(m_address);
	}
	for(mpgd_index = 0 ; mpgd_index <= mpgd_count ; ++mpgd_index)
	{
		m_address = PGDIR_SIZE*mpgd_index;
		pgd = pgd_offset_k(m_address);
		if(pgd_none(*pgd))
		{
			//printk(KERN_EMERG"#%08lx pgd is none\n" , m_address);
			continue;
		}
		mpud_count = PGDIR_SIZE/PUD_SIZE;
		//printk(KERN_EMERG"mpud_count = %d\n" , mpud_count);
		for(mpud_index = 0 ; mpud_index < mpud_count ; ++mpud_index)
		{
			m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE;
			pud = pud_offset(pgd, m_address);
			if(pud_none(*pud))
			{
				//printk(KERN_EMERG"#%08lx pud is none\n" , m_address);
				continue;
			}
			mpmd_count = PUD_SIZE/PMD_SIZE;
			//printk(KERN_EMERG"mpmd_count = %d\n" , mpmd_count);
			for(mpmd_index = 0 ; mpmd_index < mpmd_count ; ++mpmd_index)
			{
				m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE+PMD_SIZE*mpmd_index;
				//printk(KERN_EMERG"#%08lx befire\n" , m_address);
				pmd = pmd_offset(pud, m_address);
				//printk(KERN_EMERG"#%08lx after\n" , m_address);
				if (pmd_none(*pmd))
				{
					//printk(KERN_EMERG"#%08lx pmd is none\n" , m_address);
					continue;
				}
				mpte_count = PMD_SIZE/PAGE_SIZE;
				//printk(KERN_EMERG"mpte_count = %d\n" , mpte_count);
				//printk("pmd = %lx ,mpgd_index=%d , m_address=%lx\n", (unsigned long)pmd_val(*pmd) , mpgd_index , m_address);
				for(mpte_index = 0 ; mpte_index < mpte_count ; ++mpte_index)
				{
					m_address = PGDIR_SIZE*mpgd_index+mpud_index*PUD_SIZE+PMD_SIZE*mpmd_index+mpte_index*PAGE_SIZE;
					//printk(KERN_EMERG"#%08lx pte befire pmd=%p\n" , m_address , pmd);
					pte = pte_offset_map(pmd, m_address);
					//printk(KERN_EMERG"#%08lx pte=%p\n" , m_address,pte);
					
					if (pte_none(*pte)) 
					{
						//printk(KERN_EMERG"#%08lx pte is none\n" , m_address);
						pte_unmap(*pte);
						continue;
					}

					if(pte_pfn(*pte) == phy_addr >> PAGE_SHIFT)
					{
						printk("\n");
						printk("vir_addr = %08lx ,phy_addr = %08lx ,pte=%08lx  " , m_address|(phy_addr&(PAGE_SIZE-1)) , phy_addr ,(unsigned long)pte_val(*pte));
						
						if(!pte_present(*pte))
						{
							//printk(KERN_EMERG"#%08lx pte is not present\n" , m_address);
							printk("P");
							//continue;
						}
						else
						{
							printk("N");
						}
						//printk(KERN_EMERG"#%08lx after pte_present pte=%p\n" , m_address,pte);
						if(pte_write(*pte))
						{
							//printk(KERN_EMERG"#%08lx\n" , m_address);
							//continue;
							printk("W");
						}
						else
						{
							printk("R");
						}
						if(pte_dirty(*pte))
						{
							printk("D");
						}
						else
						{
							printk("C");
						}
						if(pte_young(*pte))
						{
							printk("Y");
						}
						else
						{
							printk("O");
						}
						if(pte_exec(*pte))
						{
							printk("E");
						}
						else
						{
							printk("D");
						}
						printk("\n");
					}
					pte_unmap(*pte);
					//printk(KERN_EMERG"#%08lx after pte_write pte=%p\n" , m_address,pte);
					//printk(KERN_EMERG"@%08lx\n" , m_address);  
				}
				//printk("\n");
			}
			
		}
	}
	
	return 0;	
	return 0;
}


static int phy_value_set(void *data, u64 val)
{
	if(isaddressvalid(1))
	{
		printk("val is 0x%llx , address = 08x%lx\n" , val , address);
		*(u32 *)address = val;
	}
	return 0;
}
static int phy_value_get(void *data, u64 *val)
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





DEFINE_SIMPLE_ATTRIBUTE(fops_reg, debugfs_get_reg, debugfs_set_reg, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_value, phy_value_get, phy_value_set, "0x%08llx\n");

DEFINE_SIMPLE_ATTRIBUTE(fops_printptetable, printptetablefunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_printmems, printmemsfunc, 0, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_vir_2_phy, 0, vir_2_phy_func, "0x%08llx\n");
DEFINE_SIMPLE_ATTRIBUTE(fops_phy_2_vir, 0, phy_2_vir_func, "0x%08llx\n");






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
	phy_value_entry = debugfs_create_x32("phy_value", S_IRWXUGO, debug_mem_dir,(u32 *)&phy_value);
}

static void remove_debug_memfs(void)
{
	debugfs_remove(value_dentry);
	debugfs_remove(address_entry);
	debugfs_remove(debug_mem_dir);
	debugfs_remove(print_entry);
	debugfs_remove(testvalue_entry);
	debugfs_remove(vir_2_phy_entry);
	debugfs_remove(phy_2_vir_entry);
	debugfs_remove(phy_addr_entry);
	debugfs_remove(phy_value_entry);
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
