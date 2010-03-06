#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>

#include <linux/dprintk.h>

static unsigned long long iPrintFlag;

extern int debug_check(unsigned long long flag)
{
	unsigned long long    id;

	/* check "flag" here */
	/* if skip, return 0.*/
	id = flag & 0xffffffffffffffff;
	if( !(id & iPrintFlag) )
		return 0;

	return 1;
}
//EXPORT_SYMBOL(debug_check);
//EXPORT_SYMBOL(iPrintFlag);

extern struct class *sec_class;
struct device *debug_dev;

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	sprintf(buf, "[SEC DEBUG] Print flag: %LX\n", iPrintFlag);
	if(iPrintFlag & ODR_WR)		sprintf(buf, "%sPrint ODR_WR  flag ON\n", buf);
	if(iPrintFlag & ODR_RD)		sprintf(buf, "%sPrint ODR_RD  flag ON\n", buf);
	if(iPrintFlag & ODR_IRQ)	sprintf(buf, "%sPrint ODR_IRQ flag ON\n", buf);
	if(iPrintFlag & ODR_MAP)	sprintf(buf, "%sPrint ODR_MAP flag ON\n", buf);
	if(iPrintFlag & TSP_KEY)	sprintf(buf, "%sPrint TSP_KEY flag ON\n", buf);
	if(iPrintFlag & TSP_ABS)	sprintf(buf, "%sPrint TSP_ABS flag ON\n", buf);
	if(iPrintFlag & KPD_PRS)	sprintf(buf, "%sPrint KPD_PRS flag ON\n", buf);
	if(iPrintFlag & KPD_RLS)	sprintf(buf, "%sPrint KPD_RLS flag ON\n", buf);
	
	return sprintf(buf, "%s", buf);
}

static ssize_t onedram_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "WRITE", 5) == 0 || strncmp(buf, "write", 5) == 0) {
		if(iPrintFlag & ODR_WR) {
			iPrintFlag &= ~ODR_WR;
			printk("[SEC DEBUG] Print ODR_WR flag OFF.\n");
		}
		else {
			iPrintFlag |= ODR_WR;
			printk("[SEC DEBUG] Print ODR_WR flag ON.\n");
		}	
	}
	else if(strncmp(buf, "READ", 4) == 0 || strncmp(buf, "read", 4) == 0) {
		if(iPrintFlag & ODR_RD) {
			iPrintFlag &= ~ODR_RD;
			printk("[SEC DEBUG] Print ODR_RD flag OFF.\n");
		}
		else {
			iPrintFlag |= ODR_RD;
			printk("[SEC DEBUG] Print ODR_RD flag ON.\n");
		}
	}
	else if(strncmp(buf, "IRQ", 3) == 0 || strncmp(buf, "irq", 3) == 0) {
		if(iPrintFlag & ODR_IRQ) {
			iPrintFlag &= ~ODR_IRQ;
			printk("[SEC DEBUG] Print ODR_IRQ flag OFF.\n");
		}
		else {
			iPrintFlag |= ODR_IRQ;
			printk("[SEC DEBUG] Print ODR_IRQ flag ON.\n");
		}		
	}
	else if(strncmp(buf, "MAP", 3) == 0 || strncmp(buf, "map", 3) == 0) {
		if(iPrintFlag & ODR_MAP) {
			iPrintFlag &= ~ODR_MAP;
			printk("[SEC DEBUG] Print ODR_MAP flag OFF.\n");
		}
		else {
			iPrintFlag |= ODR_MAP;
			printk("[SEC DEBUG] Print ODR_MAP flag ON.\n");
		}		
	}
	else
		printk("[SEC DEBUG] invalid command - %s.\n", buf);

	printk("[%s] iPrintFlag : %LX \n", __func__, iPrintFlag);


	return size;
}

static ssize_t touch_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "KEY", 3) == 0 || strncmp(buf, "key", 3) == 0) {
		if(iPrintFlag & TSP_KEY) {
			iPrintFlag &= ~TSP_KEY;
			printk("[SEC DEBUG] Print TSP_KEY flag OFF.\n");
		}
		else {
			iPrintFlag |= TSP_KEY;
			printk("[SEC DEBUG] Print TSP_KEY flag ON.\n");
		}
	
	}
	else if(strncmp(buf, "ABS", 3) == 0 || strncmp(buf, "abs", 3) == 0) {
		if(iPrintFlag & TSP_ABS) {
			iPrintFlag &= ~TSP_ABS;
			printk("[SEC DEBUG] Print TSP_ABS flag OFF.\n");
		}
		else {
			iPrintFlag |= TSP_ABS;
			printk("[SEC DEBUG] Print TSP_ABS flag ON.\n");
		}
	}
	else 
		printk("[SEC DEBUG] invalid command - %s.\n", buf);

	printk("[%s] iPrintFlag : %LX \n", __func__, iPrintFlag);

	return size;
}

static ssize_t keypad_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "PRESS", 5) == 0 || strncmp(buf, "press", 5) == 0) {
		if(iPrintFlag & KPD_PRS) {
			iPrintFlag &= ~KPD_PRS;
			printk("[SEC DEBUG] Print KPD_PRS flag OFF.\n");
		}
		else {
			iPrintFlag |= KPD_PRS;
			printk("[SEC DEBUG] Print KPD_PRS flag ON.\n");
		}
	}
	else if(strncmp(buf, "RELEASE", 6) == 0 || strncmp(buf, "release", 6) == 0) {
		if(iPrintFlag & KPD_RLS) {
			iPrintFlag &= ~KPD_RLS;
			printk("[SEC DEBUG] Print KPD_RLS flag OFF.\n");
		}
		else {
			iPrintFlag |= KPD_RLS;
			printk("[SEC DEBUG] Print KPD_RLS flag ON.\n");
		}
	}
	else
		printk("[SEC DEBUG] invalid command - %s.\n", buf);

	printk("[%s] iPrintFlag : %LX \n", __func__, iPrintFlag);

	return size;
}

static DEVICE_ATTR(status, S_IRUGO | S_IWUSR, status_show, NULL);
static DEVICE_ATTR(onedram, S_IRUGO | S_IWUSR, NULL, onedram_store);
static DEVICE_ATTR(touch, S_IRUGO | S_IWUSR, NULL, touch_store);
static DEVICE_ATTR(keypad, S_IRUGO | S_IWUSR, NULL, keypad_store);

int __init dprintk_init(void)
{

	debug_dev = device_create_drvdata(sec_class, NULL, 0, NULL, "debug");
	if (IS_ERR(debug_dev))
		pr_err("Failed to create device(debug)!\n");
	if (device_create_file(debug_dev, &dev_attr_status) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_status.attr.name);
	if (device_create_file(debug_dev, &dev_attr_onedram) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_onedram.attr.name);
	if (device_create_file(debug_dev, &dev_attr_touch) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touch.attr.name);
	if (device_create_file(debug_dev, &dev_attr_keypad) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_keypad.attr.name);

	iPrintFlag = 0x0;
	iPrintFlag |= (KPD_PRS|KPD_RLS|TSP_KEY);

	return 0;

}

void __exit dprintk_exit(void)
{
	return;
}
module_init(dprintk_init);
module_exit(dprintk_exit);

MODULE_DESCRIPTION("Dprintk Support Module");
MODULE_LICENSE("GPL");
