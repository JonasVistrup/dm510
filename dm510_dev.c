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
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <asm/switch_to.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/atomic.h>
#include "dm510_dev.h"

/* Prototypes - this would normally go in a .h file */
static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

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
	char *buffer; 			//Adjustable
	int size;
	char *wp;	 		//Write pointer
	struct mutex mutex;		//Buffer is a shared resource, therefore a mutex lock is needed
	wait_queue_head_t wq, rq;	//Wq: Write queue for processes waiting to write. Rq: Reading queue for processes waiting to read
};

static struct buffers buffer0;

static struct buffers buffer1;

struct dm510_dev {
	wait_queue_head_t openq;	//Open queue for processes waiting to open for writing, or reading
	struct buffers *bufferRead;	//Pointer to the read buffer
	struct buffers *bufferWrite;	//Pointer to the write buffer
	struct mutex mutex;		//Dm510_fops operations are shared between all proccess which have opened this device, therefore a mutex lock is needed
	struct cdev cdev;		//Character device driver
	atomic_t number_of_readers;	//Number of readers, is atomic to ensure no race condition in open and release
	int max_readers;		//Numbers of readers allowed on this device, -1 if unlimited number of readers
	int flag_write;			//Boolean, true iff write has been opened on this device, false iff not.
};

static struct dm510_dev dm510_0;
static struct dm510_dev dm510_1;



//Initialize a character device driver, and registers them with the kernel.
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

/*Called when module is loaded
 *Registers a char dev region, and initializes DM510_0, DM510_1, buffer0, buffer1. */
int __init dm510_init_module( void ) {

	int result;
	dev_t dev = 0;

	static atomic_t a0 = ATOMIC_INIT(0);
	static atomic_t a1 = ATOMIC_INIT(0);


	dev = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);
	result = register_chrdev_region(dev, DEVICE_COUNT, DEVICE_NAME);
	if (result < 0) {
		printk(KERN_WARNING "dm510_dev: can't get major %d\n", MAJOR_NUMBER);
		return result;
	}
	/*
	*	Initialize the two buffer and the two drivers. buffer0, buffer1, DM510_0 & DM510_1
	*/
	buffer0.size = 42;
	buffer1.size = 42;
	buffer0.buffer = kmalloc(sizeof(char)*buffer0.size,GFP_KERNEL);
	buffer1.buffer = kmalloc(sizeof(char)*buffer1.size,GFP_KERNEL);
	buffer0.wp = buffer0.buffer;
	buffer1.wp = buffer1.buffer;

	dm510_0.flag_write = 0;		//False
	dm510_1.flag_write = 0;		//False

	dm510_0.number_of_readers = a0;
	dm510_1.number_of_readers = a1;

	dm510_0.max_readers = -1;	//Unlimited
	dm510_1.max_readers = -1;	//Unlimited
	dm510_0.bufferRead = &buffer0;
	dm510_0.bufferWrite = &buffer1;
	dm510_1.bufferRead = &buffer1;
	dm510_1.bufferWrite = &buffer0;


	//Queue initialization
	init_waitqueue_head(&(buffer0.wq));
	init_waitqueue_head(&(buffer0.rq));
	init_waitqueue_head(&(dm510_0.openq));

	init_waitqueue_head(&(buffer1.wq));
	init_waitqueue_head(&(buffer1.rq));
	init_waitqueue_head(&(dm510_1.openq));

	//Mutex initialization
	mutex_init(&(dm510_0.mutex));
	mutex_init(&(dm510_1.mutex));
	mutex_init(&(buffer0.mutex));
	mutex_init(&(buffer1.mutex));

	//Device registration
	setup_cdev(&dm510_0,0);
	setup_cdev(&dm510_1,1);


	printk(KERN_INFO "DM510: Hello from your device!\n");
	return 0;
}



/* Called when module is unloaded.
 * Unregisters the devices, and frees the buffer.
 * Every other initialized attribute are static global varialbes,
 * and therefore "is freed", when the program terminates.*/
void __exit dm510_cleanup_module( void ) {
	dev_t devno = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);

	/* Get rid of our char dev entries */
	cdev_del(&dm510_0.cdev);
	cdev_del(&dm510_1.cdev);

	kfree(buffer0.buffer);
	kfree(buffer1.buffer);

	unregister_chrdev_region(devno, DEVICE_COUNT);

	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file.
 * Checks the f_flags for read, write or both. And only
 * allows the open for write, if no other writers exits.
 * And only allows the open for read, if the amount of
 * readers haven't exceeded maxReaders. Otherwise wait.
 */
static int dm510_open( struct inode *inode, struct file *filp ) {

	struct dm510_dev *dev;
	dev = container_of(inode->i_cdev, struct dm510_dev, cdev);	//Get the surrounding struct of cdev.

	if(mutex_lock_interruptible(&dev->mutex)){			//Mutex lock acquired. Only true if the Kernel chooses to interrupt the sleep.
		return -ERESTARTSYS;
	}

	switch(filp->f_flags & O_ACCMODE){				//Bit-mask to get whether it was opened for reading, writing or both.
		case O_WRONLY:
			while(dev->flag_write){				//If true, then another writer exist, therefore the proccess should sleep until untrue.
				mutex_unlock(&dev->mutex);			//Mutex unlocked before sleep
				if(filp->f_flags & O_NONBLOCK){			//If true, then no sleep allowed, and it returns instead.
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, !dev->flag_write)){	//Proccess is inserted into sleep queue. With the reverse condition of the while guard.
					return -ERESTARTSYS;
				}
				if(mutex_lock_interruptible(&dev->mutex)){			//Mutex lock acqured before continuing.
					return -ERESTARTSYS;
				}								//This while-loop sleep structure is repeated thorughted the program.
			}
			dev->flag_write = 1;
			break;

		case O_RDONLY:
			while(!(((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1))){	//If true, then to many readers exist, therefore the proccess should sleep until untrue.
				mutex_unlock(&dev->mutex);
				if(filp->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, ((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1) )){
					return -ERESTARTSYS;
				}
				if(mutex_lock_interruptible(&dev->mutex)){
					return -ERESTARTSYS;
				}
			}
			atomic_inc(&dev->number_of_readers);
			break;

		case O_RDWR:
			while(!((((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1))& (!dev->flag_write))){	//If true, then to many readers exist and/or another writer exist, therefore the process sholud sleep.
				mutex_unlock(&dev->mutex);
				if(filp->f_flags & O_NONBLOCK){
					return -EAGAIN;
				}
				if(wait_event_interruptible(dev->openq, ( ((atomic_read(&dev->number_of_readers)) < (dev->max_readers)) | (dev->max_readers==-1) ) & (!dev->flag_write) )){
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
	mutex_unlock(&dev->mutex);		//Mutex unlocked before returning.
	filp->private_data = dev;
	return 0;
}


/* Called when a process closes the device file.
 * Updates the number of writers and readers, and
 * wakes the other proccess in the openq. */
static int dm510_release( struct inode *inode, struct file *filp ) {
	struct dm510_dev* dev;
	dev = filp->private_data;

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


/* Called when a process, which already opened the dev file, attempts to read from it.
 * buf = The buffer to fill with data.
 * count = The max number of bytes to read.
 * f_pos = The offset in the file. Isn't used.
 * Sleeps if buffer is not full enough to read the entire message length requested.*/
static ssize_t dm510_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ){

	int failedBytes;
	int readBytes;
	int i;

	struct dm510_dev *dev = filp->private_data;
	if(!access_ok(buf, count)){		//Checks the user-buffer.
		return -EFAULT;
	}
	if(count > dev->bufferRead->size){	//Returns immediatily if length of message-request is larger then size of dev-buffer.
		return 0;
	}
	if(mutex_lock_interruptible(&dev->bufferRead->mutex)){	//Get mutex lock.
		return -ERESTARTSYS;
	}
	while(count > (dev->bufferRead->wp - dev->bufferRead->buffer)){ //Sleeps while count is larger the the number of bytes in the dev-buffer.
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
	failedBytes = copy_to_user(buf, dev->bufferRead->buffer, count);		//Copies the dev-buffer message to the user-buffer.
	readBytes = count - failedBytes;
	for(i = readBytes; i < (dev->bufferRead->wp - dev->bufferRead->buffer); i++){	//Copies the buffer message left to the start of the buffer.
		dev->bufferRead->buffer[i-readBytes] = dev->bufferRead->buffer[i];
	}
	dev->bufferRead->wp -= readBytes;						//Adjusting the write pointer.
	mutex_unlock(&dev->bufferRead->mutex);						//Unlocking the mutex.
	wake_up(&dev->bufferRead->wq);							//Waking up writers from wq.

	return readBytes; //return number of bytes read.
}


/* Called when a process writes to dev file.
 * buf = The buffer to get data from.
 * count = the max number of bytes to write.
 * f_pos = the offset in the file.
 * Sleeps if content cant be written to the dev-buffer,
 * do to too much already in the dev-buffer. */
static ssize_t dm510_write( struct file *filp, const char *buf, size_t count, loff_t *f_pos ){

	int failedBytes;

	struct dm510_dev *dev = filp->private_data;
	if(!access_ok(buf, count)){		//Checks the user-buffer
		return -EFAULT;
	}
	if(count > dev->bufferWrite->size){	//Returns immediatily if wanting to write more then the buffer size.
		return 0;
	}
	if(mutex_lock_interruptible(&dev->bufferWrite->mutex)){	//Acquire mutex lock
		return -ERESTARTSYS;
	}
	while((dev->bufferWrite->wp - dev->bufferWrite->buffer + count) > dev->bufferWrite->size){	//Sleep if not enough space in the dev-buffer is availiable.
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
	failedBytes = copy_from_user(dev->bufferWrite->wp, buf, count);		//Copies the message from user-buffer to dev-buffer at position wp.
	dev->bufferWrite->wp += (count - failedBytes);				//Adjust the wp.
	mutex_unlock(&dev->bufferWrite->mutex);					//Unlock mutex.
	wake_up(&dev->bufferWrite->rq);						//Wake up reading queue.

	return count - failedBytes;
}

/* called by system call icotl.
 * cmd =  command passed from the user.
 * arg = argument of the command.
 * Has three commands:
 *   change read-buffer size. Might delete data if new buffer can't contain old data.
 *   change write-buffer size. Might delete data if new buffer can't contain old data.
 *   change max number of reader.*/
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg ){
	struct dm510_dev *dev = filp->private_data;
	int i;
	char *nBuffer;

	if(!access_ok((void __user *) arg, _IOC_SIZE(cmd))){	//Checks if it is ok to use arg.
		return -EFAULT;
	}

	switch(cmd){
		case IOC_B_READ_SIZE:
			if(mutex_lock_interruptible(&dev->bufferRead->mutex)){			//Aquire mutex lock for bufferRead.
				return -ERESTARTSYS;
			}
			dev->bufferRead->size = arg;						//Change the size.
			nBuffer = kmalloc(sizeof(char)*dev->bufferRead->size, GFP_KERNEL);	//Malloc a new buffer.
			for(i = 0; i < dev->bufferRead->size; i++){				//Move data from the old buffer to the new buffer.
				nBuffer[i] = dev->bufferRead->buffer[i];
			}
			dev->bufferRead->wp = nBuffer + (dev->bufferRead->wp - dev->bufferRead->buffer);	//Move write pointer to the new buffer.
			kfree(dev->bufferRead->buffer);								//Free the old buffer.
			dev->bufferRead->buffer = nBuffer;							//Assign the new buffer to bufferRead
			mutex_unlock(&dev->bufferRead->mutex);							//Unlock mutex

			wake_up(&dev->bufferRead->wq);								//Wake up write queue in case of a larger buffer.
			break;

		case IOC_B_WRITE_SIZE:		//Same as above with bufferWrite instead of bufferRead.
			if(mutex_lock_interruptible(&dev->bufferWrite->mutex)){
				return -ERESTARTSYS;
			}
			dev->bufferWrite->size = arg;
			nBuffer = kmalloc(sizeof(char)*dev->bufferWrite->size, GFP_KERNEL);
			for(i = 0; i < dev->bufferWrite->size; i++){
				nBuffer[i] = dev->bufferWrite->buffer[i];
			}
			dev->bufferWrite->wp = nBuffer + (dev->bufferWrite->wp - dev->bufferWrite->buffer);
			kfree(dev->bufferWrite->buffer);
			dev->bufferWrite->buffer = nBuffer;
			mutex_unlock(&dev->bufferWrite->mutex);

			wake_up(&dev->bufferWrite->wq);
			break;

		case IOC_NUM_OF_READERS:
			if(mutex_lock_interruptible(&dev->mutex)){	//Aquire mutex.
				return -ERESTARTSYS;
			}
			dev->max_readers = arg;		//Change max number of reader.
			mutex_unlock(&dev->mutex);	//Unlock mutex.

			wake_up(&dev->openq);		//Wake up openq for readers in case of max reader increase.
			break;

		default: return -ENOTTY;		//Wrong command.
	}
	printk(KERN_INFO "DM510: ioctl called.\n");

	return 0;
}

module_init( dm510_init_module );		//Declares init function for module.
module_exit( dm510_cleanup_module );		//Declares exit function for module.

MODULE_AUTHOR( "...Jonas Vistrup, Thomas Lindal Winther, Mikkel Hejsel." );	//Do while?
MODULE_LICENSE( "GPL" );
