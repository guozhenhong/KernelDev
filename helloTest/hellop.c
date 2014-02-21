#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("Dual BSD/GPL");

static char *whom = "world";
static int howmany = 2;

module_param(whom, charp, S_IRUGO | S_IWUSR);
module_param(howmany, int, S_IRUGO | S_IWUSR);

static int hello_init(void)
{
    int i = 0;
    for(; i < howmany; i++)
    {
        printk(KERN_ALERT "Hello %s\n", whom);
    }
    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "GoodBye world\n");
}

module_init(hello_init);
module_exit(hello_exit);
