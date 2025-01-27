#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static int test_hello_init(void)
{
	printk(KERN_INFO "hello");
	return 0;
}

static void test_hello_exit(void)
{
	printk(KERN_INFO "exiting");
}

module_init(test_hello_init);
module_exit(test_hello_exit);
