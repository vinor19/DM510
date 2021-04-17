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
#include <linux/cdev.h>
/* #include <asm/system.h> */
#include <asm/switch_to.h>
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
#define MAXREADERS 6

#define BUFFERSIZE0 4000
#define BUFFERSIZE1 4000

#define DEVICE_COUNT 2

#define DM510_MR _IOW('a','a',int32_t*)
#define DM510_BS _IOW('a','b',int32_t*)

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

struct dm510_mod{
	wait_queue_head_t inq, outq; /* In and out queue for this device buffer */
	char *buffer, *end; /* Beginning and end of this device's buffer */
	int buffersize; /* Size of this device's buffer */
	char *rp, *wp; /* Read and write pointer for this device */
	size_t nreaders, nwriters; /* Number of openings for reading and writing */
				
	struct mutex mutex; /* mutual exclusion semaphore */
	struct cdev cdev; /* Char device structure */
						
	/* Other device driver */
	struct dm510_mod *other;
};

static char *buffer0, *buffer1;
static int buffersize0 = BUFFERSIZE0, buffersize1 = BUFFERSIZE1;
static int maxreaders = MAXREADERS;
dev_t dm510_devno;

static struct dm510_mod *devices;

/* Setting up cdev */
static void setup_cdev(struct dm510_mod *dev, int index){
	int err;
			
	cdev_init(&dev->cdev, &dm510_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&(dev->cdev), MKDEV(MAJOR_NUMBER,index), 1);
	if(err)
		printk(KERN_NOTICE "Error %d adding dm510-dev-%d", err, index);
}


static void setup_dev_p(struct dm510_mod *dev){	
	dev->buffersize = buffersize0;
	dev->buffer = buffer0;
	dev->end = dev->buffer + dev->buffersize;
	dev->other->buffersize = buffersize1;
	dev->other->buffer = buffer1;
	dev->other->end = dev->other->buffer + dev->other->buffersize;
	dev->rp = dev->buffer;
	dev->wp = dev->other->buffer;
	dev->other->rp = dev->wp;
	dev->other->wp = dev->rp;
}

/* called when module is loaded */
int dm510_init_module( void ) {
		
	int i, result;
	dev_t firstdev = MKDEV(MAJOR_NUMBER,MIN_MINOR_NUMBER);
	//printk(KERN_INFO "%d\n", firstdev);
	result = register_chrdev_region(firstdev, 2, "dm510-");
	if(result < 0){
		printk(KERN_NOTICE "Unable to get dm510- region, error %d\n", result);
		return 0;
	}
	
	/* Initialize buffers */
	buffer0 = kmalloc(buffersize0, GFP_KERNEL);
	if(!buffer0){
		unregister_chrdev_region(firstdev, 2);
		return -ENOMEM;
	}
	buffer1 = kmalloc(buffersize1, GFP_KERNEL);
	if(!buffer1){
		kfree(buffer0);
		unregister_chrdev_region(firstdev, 2); /* if fail, need to unregister */
		return -ENOMEM;
	}

	dm510_devno = firstdev;
	devices = kmalloc(2 * sizeof(struct dm510_mod), GFP_KERNEL);
	if(devices == NULL){
		kfree(buffer0);
		kfree(buffer1);
		unregister_chrdev_region(firstdev, 2);
		return 0;
	}
						
	memset(devices, 0, 2 * sizeof(struct dm510_mod));
	for(i = 0; i < 2; i++){
		init_waitqueue_head(&(devices[i].inq));
		init_waitqueue_head(&(devices[i].outq));
		mutex_init(&devices[i].mutex);
		setup_cdev(&devices[i], i);
		//printk(KERN_INFO "This device MINOR number is: %d\n", MINOR((&devices[i].cdev)->dev));
	}
	(&devices[0])->other = &devices[1];
	(&devices[1])->other = &devices[0];
	setup_dev_p(&devices[0]);
	printk(KERN_INFO "DM510: Hello from your device!\n");
	
	return 0;
}

/* Called when module is unloaded */
void dm510_cleanup_module( void ) {
	int i;
	if(!devices)
		return;
	/* clean up code belongs here */
	kfree(buffer0);
	kfree(buffer1);
				
	for(i = 0; i < 2; i++)
		cdev_del(&devices[i].cdev);	
					
	kfree(devices);
						
	unregister_chrdev_region(dm510_devno, 2);
	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {
	struct dm510_mod *dev;

	dev = container_of(inode -> i_cdev, struct dm510_mod, cdev);
	if(dev->nreaders >= maxreaders)
		return -EBUSY;
	filp->private_data = dev;
//	printk(KERN_INFO "nreaders is: %ld\n", dev->nreaders);	
//	printk(KERN_INFO "Called open\n");

	if(mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;	

	if(filp->f_mode & FMODE_READ)
		dev->nreaders++;

	if(filp->f_mode & FMODE_WRITE)
		dev->other->nwriters++;

	mutex_unlock(&dev->mutex);
	//printk(KERN_INFO "Setup dm510-dev-%d\n", MINOR((&dev->cdev)->dev));
	return 0;
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {
	
	struct dm510_mod *dev = filp->private_data;
//	printk(KERN_INFO "Release started");
	mutex_lock(&dev->mutex);

	if(filp->f_mode & FMODE_READ)
		dev->nreaders--;
	if(filp->f_mode & FMODE_WRITE)
		dev->other->nwriters--;
	mutex_unlock(&dev->mutex);

	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp,
	char *buf,      /* The buffer to fill with data     */
	size_t count,   /* The max number of bytes to read  */
	loff_t *f_pos ) /* The offset in the file           */
{
	struct dm510_mod *dev = filp->private_data;
//	printk(KERN_INFO "Read started");	
	if(mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;
	
	while (dev->rp == dev->other->wp){
		mutex_unlock(&dev->mutex);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
//		printk(KERN_INFO "Waiting");				
		if(wait_event_interruptible(dev->inq,(dev->rp != dev->other->wp)))
			return -ERESTARTSYS;
//		printk(KERN_INFO "Finished waiting");				
		if(mutex_lock_interruptible(&dev->mutex))
			return -ERESTARTSYS;
	}

	if (dev->other->wp > dev->rp)
		count = min(count, (size_t)(dev->other->wp - dev->rp));
	else 
		count = min(count, (size_t)(dev->end - dev->rp));
	if(copy_to_user(buf, dev->rp, count)){
		mutex_unlock(&dev->mutex);
		return -EFAULT;
	}
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->buffer; /* wrapped */
	wake_up_interruptible(&dev->outq);
	wake_up_interruptible(&dev->inq);
	mutex_unlock(&dev->mutex);
	
//	printk(KERN_INFO "Read finished");
	return count; //return number of bytes read	
}

/* How much space is free? */
static int spacefree(struct dm510_mod *dev)
{
	//printk(KERN_INFO "Spacefree started\n");
	if (dev->other->rp == dev->wp){
	//	printk(KERN_INFO "(dev->other->rp == dev->wp)\n");
        	return (dev->other->buffersize - 1);
	}
	//printk(KERN_INFO "(dev->other->rp != dev->wp)\n");
	return ((dev->other->rp + dev->other->buffersize - dev->wp) % dev->other->buffersize) - 1;
}

/* Wait for space for writing; caller must hold device semaphore.  On
 *  * error the semaphore will be released before returning. */
static int dm510_getwritespace(struct dm510_mod *dev, 
		struct file *filp)
{
//	printk(KERN_INFO "getwritespace started");	
	while (spacefree(dev) == 0) { /* full */
//		printk(KERN_INFO "Inside while loop");
		mutex_unlock(&dev->other->mutex);

//		printk(KERN_INFO "Lock released\n");
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
//		
		if (wait_event_interruptible(dev->other->outq, (spacefree(dev) != 0)))
			return -ERESTARTSYS;
//		printk(KERN_INFO "Acquiring lock\n");
		if (mutex_lock_interruptible(&dev->other->mutex))
			return -ERESTARTSYS;
//		printk(KERN_INFO "Lock acquired\n");
	}
//	printk(KERN_INFO "getwritespace ended");
	return 0;
}	

/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
	const char *buf,/* The buffer to get data from      */
	size_t count,   /* The max number of bytes to write */
	loff_t *f_pos ) /* The offset in the file           */
{
	struct dm510_mod *dev = filp->private_data;	
	int result;
//	printk(KERN_INFO "Started write\n");
	if(mutex_lock_interruptible(&dev->other->mutex))
		return -ERESTARTSYS;
//	printk(KERN_INFO "Acquired the lock\n");
	result = dm510_getwritespace(dev, filp);
//	printk(KERN_INFO "Getwritespace after");
	if(result)
		return result;
//	printk(KERN_INFO "Acquired enough space to write\n");
	count = min(count, (size_t)spacefree(dev));
	if (dev->wp >= dev->other->rp)
		count = min(count, (size_t)(dev->other->end - dev->wp));
	else
		count = min(count, (size_t)(dev->other->rp - dev->wp - 1));
//	printk(KERN_INFO "Found how much space I can print in\n");
	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock (&dev->other->mutex);
		return -EFAULT;
	}
	dev->wp += count;
	if (dev->wp == dev->other->end)
		dev->wp = dev->other->buffer;
//	printk(KERN_INFO "Write pointer has been updated\n");
	wake_up_interruptible(&dev->other->outq);
	wake_up_interruptible(&dev->other->inq);
	mutex_unlock(&dev->other->mutex);
//	printk(KERN_INFO "Write finished\n");
	return count; //return number of bytes written	
}

static int updatebuffer(struct file *filp, int32_t *arg){
	
	struct dm510_mod *dev = filp->private_data;
	char *newbuffer;
	int newbuffersize;
	printk(KERN_INFO "updateBuffer started\n");	
	if(get_user(newbuffersize, arg))
		return -EFAULT;	
	if(mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	while (dev->rp != dev->other->wp){
		mutex_unlock(&dev->mutex);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if(wait_event_interruptible(dev->outq,(dev->rp == dev->other->wp)))
			return -ERESTARTSYS;
		

		if(mutex_lock_interruptible(&dev->mutex))
			return -ERESTARTSYS;
	}
	
	newbuffer = kmalloc(newbuffersize, GFP_KERNEL);
	if(!newbuffer){
		mutex_unlock(&dev->mutex);
		return -ENOMEM;
	}
	if(MINOR((&dev->cdev)->dev) == 0){
		buffersize0 = newbuffersize;
		buffer0 = newbuffer;
		dev->buffersize = buffersize0;
		dev->buffer = buffer0;
		dev->end = dev->buffer + dev->buffersize;
		dev->rp = dev->buffer;
		dev->other->wp = dev->rp;
	}
	else{
		buffersize1 = newbuffersize;
		buffer1 = newbuffer;
		dev->buffersize = buffersize1;
		dev->buffer = buffer1;
		dev->end = dev->buffer + dev->buffersize;
		dev->rp = dev->buffer;
		dev->other->wp = dev->rp;
	}
	mutex_unlock(&dev->mutex);
	return 0;
}

static int update_readers(struct file *filp, int32_t *arg){
	struct dm510_mod *dev = filp->private_data;
	int newmax;
	if(get_user(newmax, arg))
		return -EFAULT;
	maxreaders = newmax;
	return 0;
}

/* called by system call icotl */ 
long dm510_ioctl( 
	struct file *filp, 
	unsigned int cmd,   /* command passed from the user */
	unsigned long arg ) /* argument of the command */
{
	printk(KERN_INFO "DM510: ioctl called.\n");
	
	switch(cmd){
	case DM510_MR:{
			update_readers(filp, (int32_t*) arg);
			break;
		      }
	case DM510_BS:{
			updatebuffer(filp, (int32_t*) arg);
			break;
		      }
	}
	return 0; //has to be changed
	
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_AUTHOR( "...Viktor Bjerg Nordstrøm." );
MODULE_LICENSE( "GPL" );
