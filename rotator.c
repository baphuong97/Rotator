#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/timer.h>

#define GPIO_ADDR_BASE		0x3F200000
#define GPIO_PIN			18
#define GPIOFSEL1			1
#define GPIO_SET0			7
#define GPIO_CLR0			10
#define GPIO_LVL0			13

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HaNQ - PhuongNB");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("This module to control servo motor by raspberry pi 3");

static unsigned int *gpio_base;
static int duty_cycle = 0;
static char message[5] = {0};

unsigned int *set_mode;
unsigned int *set_high;
unsigned int *set_low;
unsigned int last_value;

static struct kobject *kobj;

static void my_udelay(int m)
{
	int i;
	for(i = 0; i < m; i++)
		udelay(100);
}

static int get_value_pin(void)
{
	return (*((unsigned int*)(gpio_base + GPIO_LVL0))) & (1 << GPIO_PIN);
}

static int get_direction(void)
{
	return (*((unsigned int *)(gpio_base + GPIOFSEL1))) & (1 << 24);
}

static void set_pin_mode(char *mode)
{
	set_mode = (unsigned int *)gpio_base + GPIOFSEL1;

	/*mode = 1 set output, mode = 0 set input*/
	if(0 == strcmp(mode, "in"))
	{
		printk(KERN_INFO "set mode in\n");
		(*set_mode) = ((*set_mode) & (~(7 << 24))) | (0 << 24);
	}
	else if(0 == strcmp(mode, "out"))
	{
		printk(KERN_INFO "set mode out\n");
		(*set_mode) = ((*set_mode) & (~(7 << 24))) | (1 << 24);
	}
	else
	{
		printk(KERN_ALERT "String not suitable\n");
		return;
	}

	printk(KERN_INFO "value of set mode reg after set is %d\n", *set_mode);
}

static void set_pin_value(int value)
{
	set_high = (unsigned int *)(gpio_base + GPIO_SET0);
	set_low = (unsigned int *)(gpio_base + GPIO_CLR0);
	/*value = 1 set high, value = 0 set low*/
	if(1 == value)
	{
		printk(KERN_INFO "set pin high\n");
		(*set_high) = 1 << GPIO_PIN;
	}
	else
	{
		printk(KERN_INFO "set pin low\n");
		(*set_low) = 1 << GPIO_PIN;
	}
	printk(KERN_INFO "Value of SET reg is: %d, CLR reg is: %d\n", *set_high, *set_low);
}


static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	printk(KERN_INFO "value of pin is: %d\n", last_value);
	return sprintf(buf, "%d\n", last_value);
}
static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int i;
	printk(KERN_INFO "Val_store function is called\n");
	sscanf(buf, "%d", &duty_cycle);
	printk(KERN_INFO "get from user: %d\n", duty_cycle);
	if(0 != get_direction()) //check if pin mode is output
	{
		for(i = 0; i < 50; i++)
		{
			set_pin_value(1);
			my_udelay(duty_cycle);
			set_pin_value(0);
			my_udelay(200 - duty_cycle);
		}
		last_value = (duty_cycle - 15) * 10;
	}
	else
	{
		printk(KERN_ALERT "direction is input so cannot set pin value\n");
	}
    return count;
}

static ssize_t direction_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if(0 != get_direction())
	{
		return sprintf(buf, "%s", "out\n");
	}
    return sprintf(buf, "%s", "in\n");
}

static ssize_t direction_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "direction store function is called\n");
	sscanf(buf, "%s", message);
	printk(KERN_INFO "get from user: %s\n", message);
	set_pin_mode(message);
    return count;
}

static struct kobj_attribute val_kobj_attr = __ATTR(value, 0644, val_show, val_store);
static struct kobj_attribute dir_kobj_attr = __ATTR(direction, 0644, direction_show, direction_store);

static struct attribute *attr[] = {
	&val_kobj_attr.attr,
	&dir_kobj_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attr,
};

static int __init exam_init(void)
{
	int ret;

	printk(KERN_ALERT "Call from init function\n");

	gpio_base = (unsigned int *)ioremap(GPIO_ADDR_BASE, 0x100);
	if(gpio_base == NULL)
	{
		printk(KERN_ALERT "Cannot map to virtual addr\n");
		return -ENOMEM;
	}

	/*default pin mode is out*/
	set_pin_mode("out");

	/*default value is low*/
	set_pin_value(0);

	kobj = kobject_create_and_add("my_gpio_module2", kernel_kobj->parent);
	if(NULL == kobj)
	{
		printk(KERN_ALERT "cannot create kobject\n");
		return -1;
	}
	printk(KERN_INFO "create successfully kobject\n");

	ret = sysfs_create_group(kobj, &attr_group);
	if(0 != ret)
	{
		printk(KERN_ALERT "cannot create group files\n");
		kobject_put(kobj);
		return -1;
	}
	printk(KERN_INFO "created group file\n");
	return ret;
}

static void __exit exam_exit(void)
{
	printk(KERN_INFO "goodbye\n");
	kobject_put(kobj);
	set_pin_value(0);
	iounmap(gpio_base);
	printk(KERN_INFO "exit\n");
}
module_init(exam_init);
module_exit(exam_exit);
