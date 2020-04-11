/* Prototype module for second mandatory DM510 assignment */
#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>	
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>
#include <linux/semaphore.h>
/* #include <asm/system.h> */
#include <asm/switch_to.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
/* Prototypes - this would normally go in a .h file */
static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#define DEVICE_NAME "dm510_dev" /* Dev name as it appears in /proc/devices */
#define MAJOR_NUMBER 254
#define MIN_MINOR_NUMBER 0
#define MAX_MINOR_NUMBER 1

#define DEVICE_COUNT 2
/* end of what really should have been in a .h file */

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
        .unlocked_ioctl   = dm510_ioctl
};
struct buffers{
	char *buffer; //Adjustable
	int size;
	char *wp; //write pointer
	static mutex mutex;
};

static struct buffers buffer0;

static struct buffers buffer1;

struct dm510_dev {
	wait_queue_head_t wq, rq;
	struct buffers *bufferRead;
	struct buffers *bufferWrite;
	struct mutex mutex;
	struct cdev cdev;
	int number_of_readers;
	int max_readers;
	int flag_write;
};

static struct dm510_dev dm510_0;
static struct dm510_dev dm510_1;


static void setup_cdev(struct dm510_dev *dev, int index){
	int err, devno = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER + index);

	cdev_init(&dev->cdev, &dm510_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &dm510_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding dm510%d", err, index);
}

/* called when module is loaded */
int __init dm510_init_module( void ) {

	/* initialization code belongs here */
	int result;
	dev_t dev = 0;
	dev = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);
	result = register_chrdev_region(dev, DEVICE_COUNT, DEVICE_NAME);
	if (result < 0) {
		printk(KERN_WARNING "dm510_dev: can't get major %d\n", MAJOR_NUMBER);
		return result;
	}
	/*
	*	initialize the two drivers. DM510_0, DM510_1
	*/
	buffer0.size = 42;
	buffer1.size = 42;
	buffer0.buffer = kmalloc(sizeof(char)*buffer0.size,GFP_KERNEL);
	buffer1.buffer = kmalloc(sizeof(char)*buffer1.size,GFP_KERNEL);
	buffer0.wp = buffer0.buffer;
	buffer1.wp = buffer1.buffer;
	mutex_init(&buffer0.mutex);
	mutex_init(&buffer1.mutex);

	dm510_0.flag_write = 0;
	dm510_1.flag_write = 0;
	dm510_0.number_of_readers = 0;
	dm510_1.number_of_readers = 0;
	dm510_0.max_readers = -1;
	dm510_1.max_readers = -1;
	dm510_0.bufferRead = &buffer0;
	dm510_0.bufferWrite = &buffer1;
	dm510_1.bufferRead = &buffer1;
	dm510_1.bufferWrite = &buffer0;
	dm510_0.wq = DECLARE_WAIT_QUEUE_HEAD(wq);
	mutex_init(&dm510_0.mutex);
	mutex_inti(&dm510_1.mutex);

	setup_cdev(&dm510_0,0);
	setup_cdev(&dm510_1,1);


	printk(KERN_INFO "DM510: Hello from your device!\n");
	return 0;
}



/* Called when module is unloaded */
void __exit dm510_cleanup_module( void ) {
	dev_t devno = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);

	/* Get rid of our char dev entries */
	cdev_del(&dm510_0.cdev);
	cdev_del(&dm510_1.cdev);

	kfree(buffer0.buffer);
	kfree(buffer1.buffer);


	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, DEVICE_COUNT);
	/* clean up code belongs here */

	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {

	struct dm510_dev *dev;
	dev = container_of(inode->i_cdev, struct dm510_dev, cdev);

	//write or read?
	switch(filp->f_flags & O_ACCMODE){				//f_mode istead of f_flags
		case O_WRONLY:
			if(dev->flag_write){
				return -1
			}else{
				dev->flag_write = 1;
			}
			break;

		case O_RDONLY:
			if(((dev->number_of_readers) < (dev->max_readers)) | (dev->max_readers==-1)){
				dev->number_of_readers++;
			}else{
				return -1;
			}
			break;

		case O_RDWR:
			if((((dev->number_of_reader) < (dev->max_readers)) | (dev->max_readers==-1)) & !(dev->flag_write)){
				dev->number_of_readers++;
				dev->flag_write = 1;
			}else{
				return -1;
			}
			break;
	}

	filp->private_data = dev;
	/* device claiming code belongs here */
	return 0;
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {

	struct dm510_dev* dev = filp->private_data;

	switch(filp->flags & O_ACCMODE){
		case O_WRONLY:
			dev->flag_write = 0;
			break;

		case O_RDONLY:
			dev->number_of_readers--;
			break;

		case O_RDWR:
			dev->number_of_readers--;
			dev->flag_write = 0;
			break;
	}
	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp, char *buf,      /* The buffer to fill with data     */
    size_t count,   /* The max number of bytes to read  */
    loff_t *f_pos )  /* The offset in the file           */
{

	/* read code belongs here */

	return 0; //return number of bytes read
}


/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
    const char *buf,/* The buffer to get data from      */
    size_t count,   /* The max number of bytes to write */
    loff_t *f_pos )  /* The offset in the file           */
{

	/* write code belongs here */	
	
	return 0; //return number of bytes written
}

/* called by system call icotl */ 
long dm510_ioctl( 
    struct file *filp, 
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	/* ioctl code belongs here */
	printk(KERN_INFO "DM510: ioctl called.\n");

	return 0; //has to be changed
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_AUTHOR( "...Your names here. Do not delete the three dots in the beginning." );
MODULE_LICENSE( "GPL" );
