#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/errno.h> 
#include <linux/fcntl.h>  
#include <linux/seq_file.h>  
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/capability.h>

#include "scull.h"

#define SCULL_IOC_MAGIK 'k'

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIK, 0)
#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIK, 1, int)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIK, 2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIK, 3)
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIK, 4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIK, 5, int)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIK, 6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIK, 7)
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIK, 8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIK, 9, int)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIK, 10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIK, 11)
#define SCULL_IOCHQSET _IO(SCULL_IOC_MAGIK, 12)

#define SCULL_IOC_MAXNR 14

#define __DEBUG_INFO

module_param(scull_major, int, S_IRUGO | S_IWUSR);
module_param(scull_minor, int, S_IRUGO | S_IWUSR);
module_param(scull_num, int, S_IRUGO | S_IWUSR);

struct scull_dev *scull_devices;

int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *set_ptr, *next;
	int i;
	int qset;


	if(dev == NULL)
        return 0;

	qset = dev->qset;
	set_ptr = dev->data;

	if(!set_ptr)
		return 0;

    for(; set_ptr; set_ptr = next){

		if(set_ptr->data){
			for(i = 0; i < qset; i ++){
                if(set_ptr->data[i])
				    kfree(set_ptr->data[i]);
			}
			kfree(set_ptr->data);
			set_ptr->data = NULL;
		}
		next = set_ptr->next;
		kfree(set_ptr);
	}

	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;

	return 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;
	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	if(!dev)
	{
		printk(KERN_ALERT "In scull_open(), dev is NULL");
		return 0;
	}

	filp->private_data = dev;

	if((filp->f_flags & O_ACCMODE) == O_WRONLY){
		scull_trim(dev);
	}

#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In scull_open()\n");
#endif

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In scull_release()\n");
#endif

	return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int index)
{
	struct scull_qset *res = NULL;

	int n = index;

	if(!dev)
		return NULL;
	
#ifdef __DEBUG_INFO	
	printk(KERN_ALERT "In scull_follow()\n");
#endif

	if(!(dev->data))
	{
		dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if((dev->data) == NULL)
			return NULL;
		memset(dev->data, 0, sizeof(struct scull_qset));
	}

	res = dev->data;

	while(n--){
		if(!(res->next)){
			res->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if(!(res->next))
				return NULL;
			memset(res->next, 0, sizeof(struct scull_qset));
		}
		res = res->next;
	}

	return res;
}

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{

	struct scull_dev *dev = filp->private_data;
	struct scull_qset *qset_ptr = dev->data;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;

	unsigned long res;

	int item;
	int rest;
	int s_pos;
	int q_pos;

	int offset = (int)(*f_pos);

	ssize_t retval = 0;
	
#ifdef __DEBUG_INFO
    printk(KERN_ALERT "Waiting for Write proccess! I am %i\n", (get_current())->pid);
#endif

    wait_for_completion(&dev->comp);

	if(down_interruptible(&dev->sem))
	 	return -ERESTARTSYS;
	
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In scull_read()\n");
#endif

	if(offset >= dev->size)
		goto out;

	if((offset + count) > dev->size)
		count = dev->size - offset;

	item = offset / itemsize;
	rest = offset % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;

	qset_ptr = scull_follow(dev, item);

	if(!qset_ptr || !(qset_ptr->data) || !(qset_ptr->data[s_pos]))
		goto out;

	if(count > (quantum - q_pos))
		count = quantum - q_pos;

	res = copy_to_user(buf, qset_ptr->data[s_pos] + q_pos, count);
	if(res != 0){
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

	out:
		up(&dev->sem);
		return retval;
}

ssize_t scull_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{

	struct scull_dev *dev = filp->private_data;
	struct scull_qset *qset_ptr;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;

	ssize_t retval = -ENOMEM;
	int item;
	int rest;
	int s_pos;
	int q_pos;

	int offset = (int)*f_pos;

#ifdef __DEBUG_INFO  
    printk(KERN_ALERT "I am Write proccess! I am %i\n", (get_current())->pid);
#endif

	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In scull_write()\n");
#endif

	item = offset / itemsize;
	rest = offset % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;

	qset_ptr = scull_follow(dev, item);

	if(!qset_ptr){
		goto out;
	}
	if(!(qset_ptr->data)){
		qset_ptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if(!(qset_ptr->data)){
			goto out;
		}
		memset(qset_ptr->data, 0, qset * sizeof(char *));
	}
	if(!((qset_ptr->data)[s_pos])){
		qset_ptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if(!qset_ptr->data[s_pos])
			goto out;
	}

	if(count > quantum - q_pos)
		count = quantum - q_pos;

	if(copy_from_user((qset_ptr->data[s_pos]) + q_pos, buf, count)){
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

	if(dev->size < *f_pos)
		dev->size = *f_pos;

	out:
		up(&dev->sem);
        complete(&dev->comp);
		return retval;
}

loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
	struct scull_dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence){
		case 0:
			newpos = off;
			break;
		case 1:
			newpos = filp->f_pos + off;
			break;
		case 2:
			newpos = dev->size + off;
			break;
		default:
			return -EINVAL;
	}
	if(newpos < 0)
		return -EINVAL;

	filp->f_pos = newpos;

	return newpos;
}

long scull_ioctl(struct file* filp, unsigned	int cmd, unsigned long arg)
{
	int err = 1, tmp;
	int retval = 0;
	
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In scull_ioctl, the cmd is %i\n", cmd);
#endif

	if(_IOC_TYPE(cmd)  != SCULL_IOC_MAGIK)
	{	
		printk(KERN_ALERT "ERROR 1\n");
		return -ENOTTY;
	}
		

	if(_IOC_NR(cmd) > SCULL_IOC_MAXNR)
	{	
		printk(KERN_ALERT "ERROR 2\n");
		return -ENOTTY;
	}
	
	if(_IOC_DIR(cmd) & _IOC_READ)
		err = access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = access_ok(VIRIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));

	if(!err)
	{	
		printk(KERN_ALERT "ERROR 3\n");
		return -EFAULT;
	}

	switch(cmd)
	{
		case SCULL_IOCRESET:
			scull_quantum = 1000;
			scull_qset = 4;
			break;
			
		case SCULL_IOCSQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(scull_quantum, (int __user*)arg);
			break;
			
		case SCULL_IOCSQSET:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(scull_qset, (int __user*)arg);
			break;
			
		case SCULL_IOCTQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			scull_quantum = arg;
			break;
			
		case SCULL_IOCGQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			retval = __put_user(scull_quantum, (int __user*)arg);
			break;
			
		case SCULL_IOCQQUANTUM:
		//	if(!capable(CAP_SYS_ADMIN))
			//	return -EPERM;
			printk("In SCULL_IOCQQUANTUM, %d\n", scull_quantum);
			return scull_quantum;
			
		case SCULL_IOCXQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp= scull_quantum;
			retval = __get_user(scull_quantum, (int __user*)arg);
			if(retval == 0)
				retval = __put_user(tmp, (int __user*)arg);
			break;
			
		case SCULL_IOCHQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
				return -EPERM;
			tmp = scull_quantum;
			scull_quantum = arg;
			return tmp;

		default:
			return -ENOTTY;
	}

	return retval;
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl,
	.open = scull_open,
	.release = scull_release,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;

	int devno = MKDEV(scull_major, scull_minor + index);
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "tht major is %d\n", scull_major);
#endif
	cdev_init(&(dev->cdev), &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;

	err = cdev_add(&(dev->cdev), devno, 1);
	if(err < 0)
		printk(KERN_ALERT "Error %d adding scull%d", err, index);
}

static void scull_cleanup_module(void)
{
	int i;
	dev_t dev = MKDEV(scull_major, scull_minor);
	
#ifdef __DEBUG_INFO
	printk(KERN_ALERT "In clean up !!!/n");
 #endif
   
	if(scull_devices != NULL){
		printk(KERN_ALERT "In clean up !!!/n");
			
		for(i = 0; i < scull_num; i++)
		{
			scull_trim(scull_devices + i);
			cdev_del(&(scull_devices[i].cdev));
		}
		kfree(scull_devices);		
	}else{
		printk(KERN_ALERT "In clean up,devices is NULL !!!/n");
    }
	unregister_chrdev_region(dev, scull_num);
}

static int scull_init_module(void)
{
	int res = 0;
	dev_t dev;
	int i;
	
#ifdef __DEBUG_INFO
     printk(KERN_ALERT "In init!!!\n");
#endif

	if(scull_major){
		dev = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(dev, scull_num, "gzhSCULL");
	}else{
		res = alloc_chrdev_region(&dev, scull_minor, scull_num, "gzhSCULL");		
		scull_major = MAJOR(dev);
	}
	if(res != 0 ){
		printk(KERN_ALERT "In scull_init_module(), register dev error");
		return res;
	}

	printk(KERN_ALERT "tht major is %d\n", scull_major);


	scull_devices = kmalloc(scull_num * sizeof(struct scull_dev), GFP_KERNEL);

	if(scull_devices == NULL){
		printk(KERN_ALERT "scull_devices malloc error\n");
		goto fail;
	}


	memset(scull_devices, 0, scull_num * sizeof(struct scull_dev));

	for(i = 0; i < scull_num; i++)
	{
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
        sema_init(&scull_devices[i].sem, 1);
        init_completion(&scull_devices[i].comp);
		scull_setup_cdev(&(scull_devices[i]), i);
	}
	
	return 0;

	fail:
	{
		printk(KERN_ALERT "In Failed\n");
		scull_cleanup_module();
	}

	return 0;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);


MODULE_LICENSE("GPL");
