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
	char buffer[42];
	int size;
};

static struct buffers buffer0;

static struct buffers buffer1;

struct dm510_dev {
	int number;
	struct buffers *bufferRead;
	struct buffers *bufferWrite;
	struct mutex mutex;
	struct cdev cdev;
};

static struct dm510_dev dm510_0;
static struct dm510_dev dm510_1;

static int flag_write_0 = 0;
static int flag_write_1 = 0;

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
	//if (MAJOR_NUMBER) {
		dev = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);
		result = register_chrdev_region(dev, DEVICE_COUNT, DEVICE_NAME);
	//} else {
	//	result = alloc_chrdev_region(&dev, MIN_MINOR_NUMBER, DEVICE_COUNT,
	//			DEVICE_NAME);
	//	MAJOR_NUMBER = MAJOR(dev);
	//}
	if (result < 0) {
		printk(KERN_WARNING "dm510_dev: can't get major %d\n", MAJOR_NUMBER);
		return result;
	}
	/*
	*	initialize the two drivers. DM510_0, DM510_1
	*/
	buffer0.size = 0;
	buffer1.size = 0;
	dm510_0.number = 0;
	dm510_1.number = 1;
	dm510_0.bufferRead = &buffer0;
	dm510_0.bufferWrite = &buffer1;
	dm510_1.bufferRead = &buffer1;
	dm510_1.bufferWrite = &buffer0;
	// MANGLER MUTEX!!!!
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
	if(!((filp->f_flags & O_ACCMODE) == O_RDONLY)){				//f_mode istead of f_flags
		if((dev->number==0) & flag_write_0){
			return -1;
		}else{
			flag_write_0 = 1;
		}

		if((dev->number==1) & flag_write_1){
			return -1;
		}else{
			flag_write_1 = 1;
		}
	}

	filp->private_data = dev;
	/* device claiming code belongs here */
	return 0;
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {

	struct dm510_dev* dev = filp->private_data;

	if(!((filp->f_flags & O_ACCMODE) == O_RDONLY)){
		if(dev->number==0){
			flag_write_0 = 0;
		}else{
			flag_write_1 = 0;
		}
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
