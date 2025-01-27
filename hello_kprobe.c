#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ParmeetSingh");
MODULE_DESCRIPTION("Hello world for kprobes");

static struct kprobe kp_read,kp_write,kp_open;

static int handler_pre(struct kprobe *p,struct pt_regs *regs)
{
	printk(KERN_INFO "Kprobe :: %s called by process : %s {PID : %d}\n",p->symbol_name,current->comm,current->pid);
	return 0;
}

static int __init hello_kprobe_init(void)
{
	int ret;
	kp_read.symbol_name = "vfs_read";
	kp_read.pre_handler = handler_pre;

	ret = register_kprobe(&kp_read);
	if(ret<0)
	{
		printk(KERN_ERR "Failed to register kprobe for system read\n");
		return ret;
	}

	kp_open.symbol_name="do_sys_open";
	kp_open.pre_handler= handler_pre;

	ret = register_kprobe(&kp_open);
	if(ret <0)
	{
		printk(KERN_ERR "Failed to register kprobe for system open\n");
		unregister_kprobe(&kp_read);
		return ret;
	}

	kp_write.symbol_name = "vfs_write";
	kp_write.pre_handler = handler_pre;

	ret = register_kprobe(&kp_write);

	if(ret <0)
	{
		printk(KERN_ERR "Failed to register kprobe for system write\n");
		unregister_kprobe(&kp_read);
		unregister_kprobe(&kp_open);
		return ret;
	}

	printk(KERN_INFO "Kprobe Hello World module loaded \n");
	return 0;

}

static void __exit hello_kprobe_exit(void)
{
	unregister_kprobe(&kp_read);
	unregister_kprobe(&kp_write);
	unregister_kprobe(&kp_open);
	printk(KERN_INFO "Kprobe Hello World module unloaded \n");
}


module_init(hello_kprobe_init);
module_exit(hello_kprobe_exit);
