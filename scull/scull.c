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
#include <asm/uaccess.h>

#include "scull.h"

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
	printk(KERN_ALERT "In scull_open()\n");
	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "In scull_release()\n");

	return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int index)
{
	struct scull_qset *res = NULL;

	int n = index;

	if(!dev)
		return NULL;
	
	printk(KERN_ALERT "In scull_follow()\n");

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
	// if(down_interruptible(&dev->sem))
	// 	return -ERESTARTSYS;

	printk(KERN_ALERT "In scull_read()\n");

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
		// up(&dev->sem);
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

	// if(down_interruptible(&dev->sem))
	// 	return -ERESTARTSYS;

	printk(KERN_ALERT "In scull_write()\n");

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
		// up(&dev->sem);
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

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	// .ioctl = scull_ioctl,
	.open = scull_open,
	.release = scull_release,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;

	int devno = MKDEV(scull_major, scull_minor + index);
	printk(KERN_ALERT "tht major is %d\n", scull_major);
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
	
	// if(scull_devices != NULL)
	// {
	// 	printk(KERN_ALERT "In clean up !!!\n");
	// 	kfree(scull_devices);		
	// }

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

    printk(KERN_ALERT "In init!!!\n");

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
