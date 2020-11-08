#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h> 
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/mutex.h>

#define DEVICE_NAME "echo"
#define BUF_SIZE (ssize_t)(PAGE_SIZE * 10)

static char               *kbuf;                  // buffer for transfering data
static int                 my_major, my_minor = 0;
static const unsigned int  count = 1;
static dev_t               first;
static struct cdev        *dev;
static wait_queue_head_t   wq;
static size_t              writed_bytes = 0;
static struct mutex        mtx;

static int dev_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Open file device with minor = %d\n", iminor(inode));
	return 0;
}

static int dev_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Release the file structure\n");
	return 0;
}

static ssize_t dev_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	int ret;
	size_t fsize;
	/**
	 * I need to come up with some idead concerning how to
	 * perform exit in case of writed_bytes is equal zero, but we
	 * don't have any writers. At this moment we should detect 
	 * the fact that device file opened for writing was released
	 * At this point we need to go out from sleep instruction below.
	 */
	ret = wait_event_interruptible(wq, writed_bytes > 0);
	if (ret != 0) {
		printk(KERN_INFO "signal has delivered while waiting\n");
		return ret;
	}
	mutex_lock(&mtx);
	
	fsize = min(writed_bytes - *offp, count);
	ret = copy_to_user(buff, kbuf + *offp, fsize);
	if (ret != 0) {
		printk(KERN_INFO "error: not all bytes have been transferred: %d\n", ret);
		return -EFAULT;
	}

	*offp += fsize;
	if (*offp == writed_bytes) {
		writed_bytes = 0;
		*offp = 0;
		wake_up_interruptible(&wq);
	}
	printk(KERN_INFO "readed: %zd bytes; offset: %d; general_kbuf: %zd\n ", fsize, (int)*offp, writed_bytes);
	mutex_unlock(&mtx);
	return fsize;
}

static ssize_t dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	int ret;
	size_t fsize;
	ret = wait_event_interruptible(wq, writed_bytes < BUF_SIZE);
	if (ret != 0) {
		printk(KERN_INFO "signal has delivered while waiting\n");
		return ret;
	}
	mutex_lock(&mtx);
	fsize = min(count, BUF_SIZE - writed_bytes);

	if (copy_from_user(kbuf + writed_bytes, buff, fsize))
		return -EFAULT;
	*offp += fsize;
	writed_bytes += fsize;
	printk(KERN_INFO "writed: %zd bytes; offset: %d; bytes_in_kbuf: %zd\n", fsize, (int)*offp, writed_bytes);

	mutex_unlock(&mtx);
	wake_up_interruptible(&wq);
	return fsize;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read  = dev_read,
	.write = dev_write, 
	.open  = dev_open, 
	.release = dev_release
};


static int __init chardev_init(void)
{
	int ret;
	printk(KERN_INFO "device driver (aka module) is loaded\n");
	kbuf = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (!kbuf) {
		printk(KERN_ERR "kmalloc error!\n");
		return -1;
	}
	ret = alloc_chrdev_region(&first, my_minor, count, DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ERR "alloc_chrdev_region: %d\n", ret);
		return -1;
	}
	my_major = MAJOR(first);
	printk(KERN_INFO "major = %d, minor = %d\n", my_major, my_minor);
	dev = cdev_alloc();
	cdev_init(dev, &fops);
	dev->owner = THIS_MODULE;
	ret = cdev_add(dev, first, count);
	if (ret < 0) {
		printk(KERN_ERR "cdev_add: %d\n", ret);
		return -1;
	}
	mutex_init(&mtx);
	init_waitqueue_head(&wq);
	return 0;
}

/**
 * Is called just before the module is rmmoded from the kernel
 */
static void __exit chardev_exit(void)
{
	printk(KERN_INFO "device driver (aka module) is unloaded\n");
	unregister_chrdev_region(first, count);
	cdev_del(dev);
	kfree(kbuf);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
