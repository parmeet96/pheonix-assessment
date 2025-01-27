#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Parmeet Singh");
MODULE_DESCRIPTION("Kprobe ioctl version");

#define SET_MODE _IOW('a','a',int)
#define SET_BLOCK_SYSCALL _IOW('b','b',struct ProcessInfo)
#define SET_BLOCK_PID _IOW('c','c',pid_t)
#define SET_LOG_SYSCALL_FSM _IOW('d','d',int)
#define GET_LOG_SYSCALL_FSM _IOR('e','e',int)

struct ProcessInfo
{
        int pid;
        int syscall;
};


#define MODE_OFF 0
#define MODE_LOG 1
#define MODE_BLOCK 2
#define MODE_LOG_FSM 3
#define MAJOR_NO 250
#define MINOR_NO 0

static int mode = MODE_OFF;
static int block_syscall =-1;
static int log_syscall_fsm=-1;
static pid_t block_pid =-1;
static int major_number;
static struct class *dev_class;
static struct ProcessInfo kernel_process_info;

static struct kprobe kp_open, kp_read, kp_write;
static int syscall_observed =-1;
// implement writing to file
//

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	syscall_observed=-1;
	if(mode == MODE_OFF)
	{
		printk(KERN_INFO "Module is in disable mode");
	}

	if(mode == MODE_LOG)
	{
		printk(KERN_INFO "(PID : %d) with system call :: %s \n",current->pid,p->symbol_name);

	}
	if(mode == MODE_LOG_FSM)
	{
		
		int local_syscall=-1;
                if( strcmp(p->symbol_name, "do_sys_open")==0)
                {
                        local_syscall=0;
			syscall_observed=0;
                }
                else if( strcmp(p->symbol_name,"vfs_read")==0)
                {
                        local_syscall=1;
			syscall_observed=1;
                }
                else if( strcmp (p->symbol_name , "vfs_write")==0)
                {
                        local_syscall=2;
			syscall_observed=2;
                }
		if(local_syscall == log_syscall_fsm)
		{
			printk(KERN_INFO "(PID : %d) with system call :: %s \n",current->pid,p->symbol_name);
		}
	
	}

	if(mode == MODE_BLOCK )
	{
		int local_syscall=-1;
		if(strcmp(p->symbol_name , "do_sys_open")==0)
		{
			local_syscall=0;
		}
		else if(strcmp(p->symbol_name,"vfs_read")==0)
		{
			local_syscall=1;
		}
		else if(strcmp(p->symbol_name, "vfs_write")==0)
		{
			local_syscall=2;
		}

		if(current->pid == kernel_process_info.pid && local_syscall == kernel_process_info.syscall)
		{
			printk(KERN_INFO "Process :: %s (PID : %d) with system call :: %s is blocked \n",current->comm,current->pid,p->symbol_name);
         	       return -1;
		}

	}

	return 0;

}


static long control_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case SET_MODE:
			if(arg >=2)
			{
				printk(KERN_ERR "Error argument list is greater or equal to 2\n");	
				return -EINVAL;
			}
			mode = arg;
			printk(KERN_INFO "IOCTL :: kprobe mode set to %d\n",mode);
			break;
		case SET_BLOCK_SYSCALL:
			if(arg >=2)
                        {
                                printk(KERN_ERR "Error argument list is greater or equal to 2\n");
                                return -EINVAL;
                        }
			if(copy_from_user(&kernel_process_info,(struct ProcessInfo __user *)arg,sizeof(kernel_process_info)))
			{
				printk(KERN_ERR "Failed to copy data from user space\n");
				return -EFAULT;
			
			}
			printk(KERN_INFO "Recieved pid %d for syscall %d ",kernel_process_info.pid,kernel_process_info.syscall);
			block_pid=kernel_process_info.pid;
			block_syscall=kernel_process_info.syscall;
			mode =MODE_BLOCK;
			printk(KERN_INFO "IOCTL :: IOCTL Blocked System Call for %d\n",block_syscall);
                        break;
		case SET_BLOCK_PID:
			if(arg >=2)
                        {
                                printk(KERN_ERR "Error argument list is greater or equal to 2\n");
                                return -EINVAL;
                        }
			block_pid =arg;
			printk(KERN_INFO "IOCTL :: IOCTL Blocked Process Call for %d\n",block_pid);
			break;
		case SET_LOG_SYSCALL_FSM:
			if(arg >=2)
                        {
                                printk(KERN_ERR "Error argument list is greater or equal to 2\n");
                                return -EINVAL;
                        }
			mode = MODE_LOG_FSM;
			log_syscall_fsm = arg;
			printk(KERN_INFO "IOCTL :: IOCTL Loggin FSM for system call :: %d",log_syscall_fsm);
			break;
		case GET_LOG_SYSCALL_FSM:
			if(syscall_observed <0)
			{
				printk(KERN_INFO "No system call observered returing negative value\n");
				return -EAGAIN;
			}
			else if(syscall_observed >=0)
			{
				syscall_observed =0;
				if(copy_to_user((int __user *)arg,&syscall_observed,sizeof(syscall_observed)))
				{
					return -EFAULT;
				}
				printk(KERN_INFO "Sent value back to user space :: %d");
			}

		default : return -EINVAL;
		
	}
	return 0;
}

static struct file_operations fops={
	.unlocked_ioctl = control_ioctl,
};

static int __init kprobe_module_init(void)
{
	int ret;
	major_number = register_chrdev(MAJOR_NO,"Char_device_driver",&fops);
	if(major_number <0)
	{
		printk(KERN_ERR "Init ::: Failed to initialize character device driver \n");
		return major_number;
	}


	dev_class = class_create("Char_device_driver_class");
	if(IS_ERR(dev_class))
	{
		unregister_chrdev(major_number,"Char_device_driver");
		return PTR_ERR(dev_class);
	}

	if(IS_ERR(device_create(dev_class,NULL,MKDEV(major_number,0),NULL,"Char_device_driver")))
	{
		class_destroy(dev_class);
		unregister_chrdev(major_number,"Char_device_driver");
		return PTR_ERR(dev_class);
	}


        kp_read.symbol_name = "vfs_read";
        kp_read.pre_handler = handler_pre;

        ret = register_kprobe(&kp_read);
	//TODO:: THIS CLEANUP CAN BE IMPROVED USING GENERALISED METHOD
        if(ret<0)
        {
                printk(KERN_ERR "Failed to register kprobe for system read\n");
		device_destroy(dev_class,MKDEV(major_number,0));
		class_destroy(dev_class);
                unregister_chrdev(major_number,"Char_device_driver");
                return ret;
        }

        kp_open.symbol_name="do_sys_open";
        kp_open.pre_handler= handler_pre;

        ret = register_kprobe(&kp_open);
	//TODO:: SAME HERE
        if(ret <0)
        {
                printk(KERN_ERR "Failed to register kprobe for system open\n");
                unregister_kprobe(&kp_read);
		device_destroy(dev_class,MKDEV(major_number,0));
                class_destroy(dev_class);
                unregister_chrdev(major_number,"Char_device_driver");
                return ret;
        }

        kp_write.symbol_name = "vfs_write";
        kp_write.pre_handler = handler_pre;

        ret = register_kprobe(&kp_write);

	//TODO:: SAME HERE ALSO
        if(ret <0)
        {
                printk(KERN_ERR "Failed to register kprobe for system write\n");
                unregister_kprobe(&kp_read);
                unregister_kprobe(&kp_open);
		device_destroy(dev_class,MKDEV(major_number,0));
                class_destroy(dev_class);
                unregister_chrdev(major_number,"Char_device_driver");
                return ret;
        }
	
	printk(KERN_INFO "kprobe loaded successfully initialisation is done \n");
	return ret;
}


static void __exit kprobe_module_exit(void)
{
	printk(KERN_INFO "Exiting to deregister kprobe modulez\n");
        unregister_kprobe(&kp_read);
        unregister_kprobe(&kp_open);
	unregister_kprobe(&kp_write);
        device_destroy(dev_class,MKDEV(major_number,0));
       	class_destroy(dev_class);
	unregister_chrdev(major_number,"Char_device_driver");
	printk(KERN_INFO "Exiting to deregister kprobe module done\n");
}


module_init(kprobe_module_init);
module_exit(kprobe_module_exit);

