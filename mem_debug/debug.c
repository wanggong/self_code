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


static unsigned int address = 0xc0000000;
static unsigned int value = 0x00000000;
//static unsigned int count = 1;

static int isaddressvalid(int write)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	if(!virt_addr_valid(address))
	{
		printk(KERN_EMERG"address(0x%x) is not valid\n" , address);
		//return 0;
	}
	pgd = pgd_offset_k(address);
	if(pgd_none(*pgd))
	{
		printk(KERN_EMERG"address(0x%x) pgd is none\n" , address);
		return 0;
	}
	pud = pud_offset(pgd, address);
	if(pud_none(*pud))
	{
		printk(KERN_EMERG"address(0x%x) pud is none\n" , address);
		return 0;
	}
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
	{
		printk(KERN_EMERG"address(0x%x) pmd is none\n" , address);
		return 0;
	}
	pte = pte_offset_map(pmd, address);
	if (pte_none(*pte)) 
	{
		printk(KERN_EMERG"address(0x%x) pte is none\n" , address);
		return 0;
	}
	if(!pte_present(*pte))
	{
		printk(KERN_EMERG"address(0x%x) pte is not present\n" , address);
		return 0;
	}
	if(write&&!pte_write(*pte))
	{
		printk(KERN_EMERG"address(0x%x) is readonly\n" , address);
		return 0;
	}
	
	return 1;
}

static int debugfs_set_reg(void *data, u64 val)
{
	if(isaddressvalid(1))
	{
		printk("val is 0x%llx , address = 0x%x\n" , val , address);
		*(u32 *)address = val;
	}
	return 0;
}
static int debugfs_get_reg(void *data, u64 *val)
{
	if(isaddressvalid(0))
	{
		*val = *(u32 *)address;
		printk("val is 0x%llx , address = 0x%x &address=0x%x\n" , *val , address , (unsigned int)&address);
	}
	else
	{
		*val = 0x57575777;
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_reg, debugfs_get_reg, debugfs_set_reg, "0x%08llx\n");


static void init_debug_memfs(void)
{
	debug_mem_dir = debugfs_create_dir("debug_mem", NULL);
	address_entry = debugfs_create_x32("address", S_IRWXUGO, debug_mem_dir,(u32 *)&address);
	value_dentry = debugfs_create_file("value", 0777,debug_mem_dir, &value,&fops_reg);
}

static void remove_debug_memfs(void)
{
	debugfs_remove(value_dentry);
	debugfs_remove(address_entry);
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
