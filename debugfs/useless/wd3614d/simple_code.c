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
#include <linux/delay.h>
#include <linux/gpio.h>

static struct dentry *debug_wgz;


/************************************gpio signal generate***************************************/
struct sig_set
{
	int value;
	int usec;
};

static struct sig_set sig_high[] = {
	{0,20},
	{1,70},
};

static struct sig_set sig_low[] = {
	{0,70},
	{1,20},
};

static struct sig_set eod[] = {
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
/******************************************************************************************/

static int __init debug_init(void)
{
	debug_wgz = debugfs_create_dir("debug_wgz", NULL);
	init_gpio_signal_generate();
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

