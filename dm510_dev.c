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
#include <linux/atomic.h>
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

#define IOC_MAGIC 'k'
/*
*	CB means Change buffer
*
*/

#define IOC_B_READ_SIZE		_IOW(IOC_MAGIC, 0, int)
#define IOC_B_WRITE_SIZE 	_IOW(IOC_MAGIC, 1, int)
#define IOC_NUM_OF_READERS 	_IOW(IOC_MAGIC, 2, int)

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
	struct mutex mutex;
	wait_queue_head_t wq, rq;
};

static struct buffers buffer0;

static struct buffers buffer1;

struct dm510_dev {
	wait_queue_head_t openq;
	struct buffers *bufferRead;
	struct buffers *bufferWrite;
	struct mutex mutex;
	struct cdev cdev;
	atomic_t number_of_readers;
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

	DEFINE_MUTEX(buffermutex0);
	DEFINE_MUTEX(buffermutex1);
	DEFINE_MUTEX(dm510mutex0);
	DEFINE_MUTEX(dm510mutex1);

	static atomic_t a0 = ATOMIC_INIT(0);
	static atomic_t a1 = ATOMIC_INIT(0);

	DECLARE_WAIT_QUEUE_HEAD(wq0);
	DECLARE_WAIT_QUEUE_HEAD(rq0);
	DECLARE_WAIT_QUEUE_HEAD(openq0);
	DECLARE_WAIT_QUEUE_HEAD(wq1);
	DECLARE_WAIT_QUEUE_HEAD(rq1);
	DECLARE_WAIT_QUEUE_HEAD(openq1);


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

	buffer0.mutex = buffermutex0;
	buffer1.mutex = buffermutex1;

	dm510_0.flag_write = 0;
	dm510_1.flag_write = 0;
	dm510_0.number_of_readers = a0;
	dm510_1.number_of_readers = a1;
	dm510_0.max_readers = -1;
	dm510_1.max_readers = -1;
	dm510_0.bufferRead = &buffer0;
	dm510_0.bufferWrite = &buffer1;
	dm510_1.bufferRead = &buffer1;
	dm510_1.bufferWrite = &buffer0;


	buffer0.wq = wq0;
	buffer0.rq = rq0;
	dm510_0.openq = openq0;

	buffer1.wq = wq1;
	buffer1.rq = rq1;
	dm510_1.openq = openq1;


	dm510_0.mutex = dm510mutex0;
	dm510_1.mutex = dm510mutex1;

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

	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {

	struct dm510_dev *dev;
	dev = container_of(inode->i_cdev, struct dm510_dev, cdev);

	//write or read?
	if(mutex_lock_interruptible(&dev->mutex)){
		return -ERESTARTSYS;
	}
	switch(filp->f_flags & O_ACCMODE){				//f_mode istead of f_flags
		case O_WRONLY:
			while(dev->flag_write){
				mutex_unlock(&dev->mutex);
				if(filp->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, !dev->flag_write)){
					return -ERESTARTSYS;
				}
				if(mutex_lock_interruptible(&dev->mutex)){
					return -ERESTARTSYS;
				}
			}
			dev->flag_write = 1;
			break;

		case O_RDONLY:
			while(!(((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1))){
				mutex_unlock(&dev->mutex);
				if(filp->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, !dev->flag_write)){ //rewrite flag
					return -ERESTARTSYS;
				}
				if(mutex_lock_interruptible(&dev->mutex)){
					return -ERESTARTSYS;
				}
			}
			atomic_inc(&dev->number_of_readers);
			break;

		case O_RDWR:
			while(!((((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1))& (!dev->flag_write))){
				mutex_unlock(&dev->mutex);
				if(filp->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, !dev->flag_write)){ //rewrite flag
					return -ERESTARTSYS;
				}
				if(mutex_lock_interruptible(&dev->mutex)){
					return -ERESTARTSYS;
				}
			}
			atomic_inc(&dev->number_of_readers);
			dev->flag_write = 1;
			break;
	}
	mutex_unlock(&dev->mutex);
	filp->private_data = dev;
	return 0;
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {

	struct dm510_dev* dev = filp->private_data;
	switch(filp->f_flags & O_ACCMODE){
		case O_WRONLY:
			dev->flag_write = 0;
			wake_up(&dev->openq);
			break;

		case O_RDONLY:
			atomic_dec(&dev->number_of_readers);
			wake_up(&dev->openq);
			break;

		case O_RDWR:
			atomic_dec(&dev->number_of_readers);
			dev->flag_write = 0;
			wake_up(&dev->openq);
			break;
	}
	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. buf = The buffer to fill with data. count = The max number of bytes to read. f_pos = The offset in the file. */
static ssize_t dm510_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ){

	int failedBytes;
	int readBytes;
	int i;

	struct dm510_dev *dev = filp->private_data;
	if(!access_ok(buf, count)){
		return -EFAULT;
	}
	if(count > dev->bufferRead->size){
		return 0;
	}
	if(mutex_lock_interruptible(&dev->bufferRead->mutex)){
		return -ERESTARTSYS;
	}
	while(count > (dev->bufferRead->wp - dev->bufferRead->buffer)){ //Buffer full
		mutex_unlock(&dev->bufferRead->mutex);
		if(filp->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		if(wait_event_interruptible(dev->bufferRead->rq, !(count > (dev->bufferRead->wp - dev->bufferRead->buffer)))){
			return -ERESTARTSYS;
		}
		if(mutex_lock_interruptible(&dev->bufferRead->mutex)){
			return -ERESTARTSYS;
		}
	}
	failedBytes = copy_to_user(buf, dev->bufferRead->buffer, count);
	readBytes = count - failedBytes;
	for(i = readBytes; i < (dev->bufferRead->wp - dev->bufferRead->buffer); i++){
		dev->bufferRead->buffer[i-readBytes] = dev->bufferRead->buffer[i];
	}
	dev->bufferRead->wp -= readBytes;
	mutex_unlock(&dev->bufferRead->mutex);
	wake_up(&dev->bufferRead->wq);

	return readBytes; //return number of bytes read
}


/* Called when a process writes to dev file, buf = The buffer to get data from. count = the max number of bytes to write. f_pos = the offset in the file */
static ssize_t dm510_write( struct file *filp, const char *buf, size_t count, loff_t *f_pos ){

	int failedBytes;

	struct dm510_dev *dev = filp->private_data;
	if(!access_ok(buf, count)){
		return -EFAULT;
	}
	if(count > dev->bufferWrite->size){
		return 0; //0 bytes written, since the offset is bigger than the buffer.
	}
	if(mutex_lock_interruptible(&dev->bufferWrite->mutex)){
		return -ERESTARTSYS;
	}
	while((dev->bufferWrite->wp - dev->bufferWrite->buffer + count) > dev->bufferWrite->size){ //Buffer full
		mutex_unlock(&dev->bufferWrite->mutex);
		if(filp->f_flags & O_NONBLOCK){
			return -EAGAIN;
		}
		if(wait_event_interruptible(dev->bufferWrite->wq, !((dev->bufferWrite->wp - dev->bufferWrite->buffer + count) > dev->bufferWrite->size))){
			return -ERESTARTSYS;
		}
		if(mutex_lock_interruptible(&dev->bufferWrite->mutex)){
			return -ERESTARTSYS;
		}
	}
	failedBytes = copy_from_user(dev->bufferWrite->wp, buf, count);
	dev->bufferWrite->wp += (count - failedBytes);
	mutex_unlock(&dev->bufferWrite->mutex);
	wake_up(&dev->bufferWrite->rq);

	return count - failedBytes;
}

/* called by system call icotl, cmd =  command passed from the user. arg = argument of the command*/ 
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg ){
	struct dm510_dev *dev = filp->private_data;
	int i;
	char *nBuffer;

	if(!access_ok((void __user *) arg, _IOC_SIZE(cmd))){
		return -EFAULT;
	}

	switch(cmd){
		case IOC_B_READ_SIZE:
			if(mutex_lock_interruptible(&dev->bufferRead->mutex)){
				return -ERESTARTSYS;
			}
			if(__get_user(dev->bufferRead->size, (int __user *)arg)){
				return -EFAULT;
			}
			nBuffer = kmalloc(sizeof(char)*dev->bufferRead->size, GFP_KERNEL);
			for(i = 0; i < dev->bufferRead->size; i++){
				nBuffer[i] = dev->bufferRead->buffer[i];
			}
			kfree(dev->bufferRead->buffer);
			dev->bufferRead->buffer = nBuffer;
			mutex_unlock(&dev->bufferRead->mutex);

			wake_up(&dev->bufferRead->wq);
			break;

		case IOC_B_WRITE_SIZE:
			if(mutex_lock_interruptible(&dev->bufferWrite->mutex)){
				return -ERESTARTSYS;
			}
			if(__get_user(dev->bufferWrite->size, (int __user *)arg)){
				return -EFAULT;
			}
			nBuffer = kmalloc(sizeof(char)*dev->bufferWrite->size, GFP_KERNEL);
			for(i = 0; i < dev->bufferWrite->size; i++){
				nBuffer[i] = dev->bufferWrite->buffer[i];
			}
			kfree(dev->bufferWrite->buffer);
			dev->bufferWrite->buffer = nBuffer;
			mutex_unlock(&dev->bufferWrite->mutex);

			wake_up(&dev->bufferWrite->wq);
			break;

		case IOC_NUM_OF_READERS:
			if(mutex_lock_interruptible(&dev->mutex)){
				return -ERESTARTSYS;
			}
			if(__get_user(dev->max_readers, (int __user *)arg)){
				return -EFAULT;
			}
			mutex_unlock(&dev->mutex);

			wake_up(&dev->openq);
			break;

		default: return -ENOTTY;
	}
	printk(KERN_INFO "DM510: ioctl called.\n");

	return 0;
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_AUTHOR( "...Jonas Vistrup, Thomas Lindal Winther, Mikkel Hejsel." );
MODULE_LICENSE( "GPL" );
