/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>




#define BP_CREATE 1
#define BP_DEL 2

struct bp_list {
    struct list_head list;
    struct perf_event * __percpu *bp;
    struct perf_event_attr attr;
};

static int bp_dev_open(struct inode *inode, struct file *file)
{
    struct list_head *list = (struct list_head*)kmalloc(sizeof(struct list_head) , GFP_KERNEL);
    if(!list) {
        pr_err("malloc list failed !");
        return -ENOMEM;
    }
    INIT_LIST_HEAD(list);
    file->private_data = list;
    return 0;
}

static int bp_dev_release(struct inode *inode, struct file *file)
{
    struct list_head *list = (struct list_head*)file->private_data;
    while(!list_empty(list)){
        struct bp_list *bp_l = (struct bp_list*)list->next;
        unregister_wide_hw_breakpoint(bp_l->bp);
        list_del(&bp_l->list);
        kfree(bp_l);
    }
    file->private_data = NULL;
    kfree(list);
    return 0;
}

static int create_bp(struct file *file , struct perf_event_attr *user_attr)
{
    struct perf_event * __percpu *bp;
    struct perf_event_attr attr;
    struct bp_list *bp_l;
    struct list_head *list = file->private_data;

    ptrace_breakpoint_init(&attr);
    attr.bp_addr = user_attr->bp_addr;
    attr.bp_len = user_attr->bp_len;
    attr.bp_type = user_attr->bp_type;
    attr.disabled = 0;
    bp = register_wide_hw_breakpoint(&attr, NULL, NULL);
    if (IS_ERR(bp)){
        pr_err("create breakpoint error\n");
        return -1;
    }
    bp_l = (struct bp_list*)kmalloc(sizeof(struct bp_list) , GFP_KERNEL);
    if(!bp_l){
        pr_err("kmalloc bp_list failed \n");
        unregister_wide_hw_breakpoint(bp);
        return -ENOMEM;
    }
    bp_l->bp = bp;
    bp_l->attr = attr;
    list_add(&bp_l->list , list);
    return 0;
}

static void del_bp(struct file *file , struct perf_event_attr *user_attr)
{
    struct list_head *head = (struct list_head*)file->private_data;
    struct bp_list *bp_l = NULL;
    int find = 0;
    list_for_each_entry(bp_l, head , list){
        if(bp_l->attr.bp_addr == user_attr->bp_addr){
            find = 1;
            break;
        }
    }
    if(find) {
        unregister_wide_hw_breakpoint(bp_l->bp);
        list_del(&bp_l->list);
        kfree(bp_l);
    }
}

static long bp_dev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
    long rc = 0;
    struct perf_event_attr bp_attr;
    
    if(copy_from_user(&bp_attr , (void __user *)arg , sizeof(bp_attr))) {
        pr_err("bp copy user failed\n");
        return -EFAULT;
    }
    switch(cmd) {
        case BP_CREATE:
            rc = create_bp(file , &bp_attr);
            break;
        case BP_DEL:
            del_bp(file , &bp_attr);
            break;
        default:
            break;
    }
    return rc;
}

const struct file_operations bp_dev_operations = {
	.owner		= THIS_MODULE,
	.open		= bp_dev_open,
	.release	= bp_dev_release,
	.unlocked_ioctl = bp_dev_ioctl,
	.compat_ioctl   = bp_dev_ioctl,
};

static struct miscdevice bp_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "breakpoint",
	.fops = &bp_dev_operations,
};

static int __init breakpoint_init(void)
{
	int err = -ENOMEM;
	err = misc_register(&bp_miscdevice);
	if (err){
		pr_err("register breakpoint misc dev failed err:%d\n" , err);
            return -1;
       }
        printk("wgz register breakpoint misc dev success\n");
	return err;

}

static void __exit breakpoint_exit(void)
{
    misc_deregister(&bp_miscdevice);
}

module_init(breakpoint_init);
module_exit(breakpoint_exit);

MODULE_AUTHOR("wanggongzhen <wanggongzhen@hotmail.com>");
MODULE_DESCRIPTION("user watchpoint");
MODULE_LICENSE("GPL");
