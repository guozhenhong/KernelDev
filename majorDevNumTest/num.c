#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");

static int major = 0;
static int minor = 1;

module_param(major, int, S_IRUGO | S_IWUSR);
module_param(minor, int, S_IRUGO | S_IWUSR);

dev_t dev;
unsigned int nNum = 3;

static int num_init(void)
{
    int result;

    if(major){
        dev = MKDEV(major, minor);
        result = register_chrdev_region(dev, nNum, "gzhNUMTest");         
    }else{
        result = alloc_chrdev_region(&dev, minor, nNum, "gzhNUMTest");
        major = MAJOR(dev);
    }

    if(result < 0){
        printk(KERN_ALERT "cannot get major %d\n", major);
        return result;
    }

    printk(KERN_ALERT "%d\n", major);

    return 0;
}

static void num_exit(void)
{
    unregister_chrdev_region(dev, nNum);
    printk(KERN_ALERT "ByeBye!\n");

}


module_init(num_init);
module_exit(num_exit);
