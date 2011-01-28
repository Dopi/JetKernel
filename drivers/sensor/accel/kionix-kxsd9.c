/* kionix-kxsd9.c
 *
 * G-Sensor found in HTC Raphael (Touch Pro) and HTC Diamond mobile phones
 *
 * also acts as pedometer and free-fall detector
 * reports g-force as x,y,z
 * reports step count

    ^
 +y |
    ___
-x | -'| +x
<--|   |-->
   |___| / -z
   |_O_|/
 -y |  /
    v / +z

 * TODO: calibration, report free fall, ..
 *
 * Job Bolle <jb@b4m.com>
 */

#include <linux/interrupt.h>
#include <linux/irq.h>

#include <plat/pm.h>
#include <plat/s3c64xx-dvfs.h>
#include <linux/i2c/pmic.h>
#include <mach/param.h>
#include <linux/random.h>
#include <plat/gpio-cfg.h>
#include <mach/jet.h>

#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#ifdef CONFIG_ANDROID_POWER
#include <linux/android_power.h>
#endif

#define MODULE_NAME "kionix-kxsd9"

#define I2C_READ_RETRY_TIMES 10
#define I2C_WRITE_RETRY_TIMES 10

#define KXSD9_DEBUG  0
#define KXSD9_DUMP   0

#define KXSD9_FREE_FALL 0x800
#define KXSD9_WROP_BUF	30

#if KXSD9_DEBUG
 #define DLOG(fmt, arg...) printk(KERN_DEBUG MODULE_NAME ", %s: " fmt "\n", __FUNCTION__, ## arg);
#else
 #define DLOG(fmt, arg...) do {} while(0)
#endif

struct kxsd9 {
	struct i2c_client *client;
	struct input_dev *inputdev;
	struct hrtimer timer;
	struct delayed_work work;
	struct mutex lock;
#ifdef CONFIG_ANDROID_POWER
	android_suspend_lock_t suspend_lock;
#endif
	int on,scale,rate,susp;
	unsigned short pedo_count;
	int pedo_up,pedo_lim;

	unsigned short wrop[KXSD9_WROP_BUF];
	int head,tail;
};

//static struct kxsd9 *kxsd9_global;

#define KXSD9_REG_RST	0x0A
#define KXSD9_RST_KEY	0xCA
#define KXSD9_REG_A	0x0E	// --- --- --- --- --- --- Mhi ---
#define KXSD9_REG_B	0x0D	// ClH ENA ST  -0- -0- Men -0- -0-
#define KXSD9_REG_C	0x0C	// LP2 LP1 LP0 Mle Mla -0- FS1 FS0

#define KXSD9_B_CLKHLD	(1<<7)
#define KXSD9_B_ENABLE	(1<<6)
#define KXSD9_B_SLFTST	(1<<5)
#define KXSD9_C_MI_LEV	(1<<4)	// moti extra sensitive
#define KXSD9_C_MI_LAT	(1<<3)	// moti latched vs. non-latched
#define KXSD9_B_MI_ON	(1<<2)	// moti enabled
#define KXSD9_A_MI_INT	(1<<1)	// moti has happened (read clears)
#define KXSD9_BWIDTH	0xE0	// 111- =50Hz

enum {	/* operation     	   param */
	KXSD9_CTL_RESET,	// ignored
	KXSD9_CTL_ENABLE,	// 0 = disabled
	KXSD9_CTL_SCALE,	// 1 (2G) .. 4 (8G)
	KXSD9_CTL_RATE		// samples per 10 seconds
};


/****************************** JetDroid *****************************************/
#define KXSD9_SADDR 		0x18 	// [0011000]
#define I2C_DF_NOTIFY       0x01
#define IRQ_ACC_INT IRQ_EINT(22)

#define TIMER_OFF 	0
#define TIMER_ON	1

//#define INTERRUPT_DRIVEN

static int kxsd9_timer_oper = TIMER_ON;
static int interrupted = 0;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { I2C_CLIENT_END };

#ifdef CONFIG_JET_OPTION
static unsigned short probe_addr[] = { 0, KXSD9_SADDR, I2C_CLIENT_END };
#else
static unsigned short probe_addr[] = { 5/*0*/, KXSD9_SADDR, I2C_CLIENT_END };
#endif

static struct i2c_client_address_data addr_data = {
	.normal_i2c     = normal_addr,
	.probe          = probe_addr,
	.ignore         = ignore,
};

/************************************************************************************/

#if 0 // This function is not used anymore
static int kxsd9_wrop_enq(struct kxsd9 *kxsd9,unsigned char reg,unsigned char val)
{
	int nt;

	nt = (kxsd9->tail + 1) % KXSD9_WROP_BUF;
	if (nt == kxsd9->head) {
		// buffer full
		return -1;
	}
	kxsd9->wrop[kxsd9->tail] = (reg << 8) | val;
	kxsd9->tail = nt;
	kxsd9_timer_oper = TIMER_ON;
	return 0;
}
#endif

static int kxsd9_wrop_deq(struct kxsd9 *kxsd9,char *buf)
{
	if (kxsd9->head == kxsd9->tail) {
		// buffer empty
		return -1;
	}
	buf[0] = kxsd9->wrop[kxsd9->head] >> 8;
	buf[1] = kxsd9->wrop[kxsd9->head] & 0xFF;
	kxsd9->head = (kxsd9->head + 1) % KXSD9_WROP_BUF;
	return 0;
}

static int kxsd9_i2c_write(struct i2c_client *client, const uint8_t *sendbuf, int len)
{
	int rc;
	int retry;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = len,
			.buf = sendbuf,
		},
	};

	for (retry = 0; retry <= I2C_WRITE_RETRY_TIMES; retry++) {
		rc = i2c_transfer(client->adapter, msg, 1);
		if (rc == 1)
			return 0;
		msleep(10);
		printk(KERN_WARNING "kxsd9, i2c write retry\n");
	}
	dev_err(&client->dev, "i2c_write_block retry over %d\n",
			I2C_WRITE_RETRY_TIMES);
	return rc;
}

static int kxsd9_i2c_read(struct i2c_client *client, uint8_t id,
						uint8_t *recvbuf, int len)
{
	int retry;
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &id,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = recvbuf,
		}
	};
	for (retry = 0; retry <= I2C_READ_RETRY_TIMES; retry++) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)
			return 0;
		msleep(10);
		printk(KERN_WARNING "kxsd9, i2c read retry\n");
	}
	dev_err(&client->dev, "i2c_read_block retry over %d\n",
			I2C_READ_RETRY_TIMES);
	return -EIO;
}

int kxsd9_control(struct kxsd9 *kxsd9,int oper,int param)
{
	int restart;
	uint8_t buf[2];
	DLOG(": %s(%d, %d)\n", __func__, oper, param);
	mutex_lock(&kxsd9->lock);
	restart = (kxsd9->head == kxsd9->tail);
	switch (oper)
	{
		case KXSD9_CTL_RESET:
			//kxsd9_wrop_enq(kxsd9,KXSD9_REG_RST,KXSD9_RST_KEY);
			buf[0] = KXSD9_REG_RST;
			buf[1] = KXSD9_RST_KEY;
			kxsd9_i2c_write(kxsd9->client, buf, 2);
			DLOG("KB: write %02x to %02x\n", buf[1], buf[0]);
			kxsd9->pedo_up = kxsd9->pedo_lim = kxsd9->pedo_count = 0;
			break;

		case KXSD9_CTL_ENABLE:
			kxsd9->on = !!param;
			//kxsd9_wrop_enq(kxsd9,KXSD9_REG_B,param ?
			//		KXSD9_B_CLKHLD | KXSD9_B_ENABLE | KXSD9_B_MI_ON : 0x00);
			//kxsd9_i2c_read(kxsd9->client, KXSD9_REG_B, buf, 1);
			//printk("KB: Register B value: %d\n", buf[0]);
			buf[0] = KXSD9_REG_B;
#ifdef INTERRUPT_DRIVEN
			buf[1] = param ? KXSD9_B_CLKHLD | KXSD9_B_ENABLE | KXSD9_B_MI_ON : 0x00;
#else
			buf[1] = param ? KXSD9_B_CLKHLD | KXSD9_B_ENABLE : 0x00;
#endif
			DLOG("KB: write %02x to %02x\n", buf[1], buf[0]);
			kxsd9_i2c_write(kxsd9->client, buf, 2);
			break;

		case KXSD9_CTL_SCALE:
			if (param < 1)
				param = 1;
			else if (param > 4)
				param = 4;
			kxsd9->scale = param;
			param = 4 - param;
			//kxsd9_wrop_enq(kxsd9,KXSD9_REG_C,KXSD9_BWIDTH | KXSD9_C_MI_LAT | KXSD9_C_MI_LEV | param);
			buf[0] = KXSD9_REG_C;
			buf[1] = KXSD9_BWIDTH | KXSD9_C_MI_LAT | KXSD9_C_MI_LEV | param;
			//buf[1] = KXSD9_BWIDTH | KXSD9_C_MI_LEV | param;
			DLOG("KB: write %02x to %02x\n", buf[1], buf[0]);
			kxsd9_i2c_write(kxsd9->client, buf, 2);

			break;

		case KXSD9_CTL_RATE:
			param &= 0x1FFF;
			restart = (param > kxsd9->rate);
			kxsd9->rate = param;
			break;
	}
#ifndef INTERRUPT_DRIVEN
	if (restart) {
		hrtimer_start(&kxsd9->timer, ktime_set(0,16 * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);
	}
#endif
	mutex_unlock(&kxsd9->lock);
	return 0;
}

static enum hrtimer_restart kxsd9_poll_timer(struct hrtimer *timer)
{
	struct kxsd9 *kxsd9;

	kxsd9 = container_of(timer, struct kxsd9, timer);
#ifdef CONFIG_ANDROID_POWER
	android_lock_suspend(&kxsd9->suspend_lock);
#endif
	schedule_work(&kxsd9->work.work);
	return HRTIMER_NORESTART;
}

static void kxsd9_work(struct work_struct *work)
{
	struct kxsd9 *kxsd9;
	int err;
	uint8_t buf[6], status[2];
	int x,y,z;
	unsigned long long gabs;
	ktime_t restart_time = {0};

	kxsd9 = container_of(work, struct kxsd9, work.work);
	mutex_lock(&kxsd9->lock);
	/*if (kxsd9_wrop_deq(kxsd9,buf) == 0) {
		DLOG(": write %02x to %02x\n", buf[1], buf[0]);

		kxsd9_i2c_write(kxsd9->client, buf, 2);

		restart_time.tv.nsec = 4 * NSEC_PER_MSEC;
		hrtimer_start(&kxsd9->timer, restart_time, HRTIMER_MODE_REL);
	} else*/ {
		//status[0] = KXSD9_REG_B;
		//status[1] = 0xC0;
		//kxsd9_i2c_write(kxsd9->client, status, 2);

		kxsd9_i2c_read(kxsd9->client, 0, buf, 6);
		
		x = ((buf[0] << 4) + (buf[1] >> 4) - 0x800 + 0); //Modified for JetDroid
		y = (0x800 - (buf[2] << 4) - (buf[3] >> 4) - 0); //Modified for JetDroid
		z = (0x800 - (buf[4] << 4) - (buf[5] >> 4) + 128); // calib?
		//x = -(((int)buf[0]<<4) ^ ((int)buf[1]>>4)); //Used in default Samsung driver
		//y = (((int)buf[2]<<4) ^ ((int)buf[3]>>4));  //Didn't work with JetDroid
		//z = (((int)buf[4]<<4) ^ ((int)buf[5]>>4));  //Kept just for reference

		// detect step
		gabs = x * x + y * y + z * z;
		if (kxsd9->pedo_up) {
			if (gabs > kxsd9->pedo_lim) {
				kxsd9->pedo_up = 0;
				kxsd9->pedo_lim = gabs / 2;
				kxsd9->pedo_count++;
				input_report_abs(kxsd9->inputdev, ABS_GAS,
						kxsd9->pedo_count);
			} else if (kxsd9->pedo_lim > gabs * 2) {
				kxsd9->pedo_lim = gabs * 2;
			}
		} else {
			if (gabs < kxsd9->pedo_lim) {
				kxsd9->pedo_up = 1;
				kxsd9->pedo_lim = gabs * 2;
			} else if (kxsd9->pedo_lim < gabs / 2) {
				kxsd9->pedo_lim = gabs / 2;
			}
		}

#if KXSD9_DUMP
#if 1
		printk(KERN_INFO "G=(%6d, %6d, %6d) P=%d %s\n",
				x, y, z, kxsd9->pedo_count,
				gabs < KXSD9_FREE_FALL ? "FF" : ""); // free-fall
#else
		printk(KERN_INFO "G=( %02X %02X  %02X %02X  %02X %02X )\n",
				buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
#endif
#endif
		input_report_abs(kxsd9->inputdev, ABS_X, x);
		input_report_abs(kxsd9->inputdev, ABS_Y, y);
		input_report_abs(kxsd9->inputdev, ABS_Z, z);
		input_sync(kxsd9->inputdev);

#ifndef INTERRUPT_DRIVEN
		if (kxsd9->on && kxsd9->rate && !kxsd9->susp)
		{
			restart_time.tv.nsec = (10000 / kxsd9->rate)
					* NSEC_PER_MSEC;
			hrtimer_start(&kxsd9->timer, restart_time,
					HRTIMER_MODE_REL);
		}
#else
		//if (interrupted) {
		kxsd9_i2c_read(kxsd9->client, KXSD9_REG_A, status, 1);
		//printk("KB: Register A value: %d\n", status[0]);
		kxsd9_i2c_read(kxsd9->client, KXSD9_REG_B, status, 1);
		//printk("KB: Register B value: %d\n", status[0]);
		status[0] = KXSD9_REG_B;
		status[1] = 0xC4;
		kxsd9_i2c_write(kxsd9->client, status, 2);
		//printk("KB: Register B value written: %d\n", status[1]);
		//interrupted = 0;
		enable_irq(IRQ_ACC_INT);

		//}
#endif
	}
#ifdef CONFIG_ANDROID_POWER
	android_unlock_suspend(&kxsd9->suspend_lock);
#endif
	mutex_unlock(&kxsd9->lock);

}


static ssize_t kxsd9_ctl_rate_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));

	DLOG(" %s\n", __func__);
	return sprintf(buf, "%u\n", kxsd9 ? kxsd9->rate : 0);
}

static ssize_t kxsd9_ctl_rate_store(struct device *dev, struct device_attribute *attr,
				const char *buf,size_t count)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));
	unsigned long val = simple_strtoul(buf, NULL, 10);

	DLOG(" %s\n", __func__);
	kxsd9_control(kxsd9,KXSD9_CTL_RATE,val);
        return count;
}

static ssize_t kxsd9_ctl_scale_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));

	DLOG(" %s\n", __func__);
	return sprintf(buf, "%u\n", kxsd9 ? kxsd9->scale : 0);
}

static ssize_t kxsd9_ctl_scale_store(struct device *dev, struct device_attribute *attr,
				const char *buf,size_t count)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));
	unsigned long val = simple_strtoul(buf, NULL, 10);

	DLOG(" %s\n", __func__);
	kxsd9_control(kxsd9,KXSD9_CTL_SCALE,val);
        return count;
}

static ssize_t kxsd9_ctl_enable_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));

	DLOG(" %s\n", __func__);
	return sprintf(buf, "%u\n", kxsd9 && kxsd9->on ? 1 : 0);
}

static ssize_t kxsd9_ctl_enable_store(struct device *dev, struct device_attribute *attr,
				const char *buf,size_t count)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(to_i2c_client(dev));
	unsigned long val = simple_strtoul(buf, NULL, 10);

	DLOG(" %s\n", __func__);
	kxsd9_control(kxsd9,KXSD9_CTL_ENABLE,!!val);
        return count;
}

struct device_attribute kxsd9_sysfs_ctl_rate =
{
	.attr = {	.name = "rate",
			.mode = S_IWUSR | S_IRUGO | S_IRWXO }, //File premissions modified for JetDroid
	.show	= kxsd9_ctl_rate_show,
	.store	= kxsd9_ctl_rate_store,
};

struct device_attribute kxsd9_sysfs_ctl_scale =
{
	.attr = {	.name = "scale",
			.mode = S_IWUSR | S_IRUGO | S_IRWXO }, //File premissions modified for JetDroid
	.show	= kxsd9_ctl_scale_show,
	.store	= kxsd9_ctl_scale_store,
};

struct device_attribute kxsd9_sysfs_ctl_enable =
{
	.attr = {	.name = "enable",
			.mode = S_IWUSR | S_IRUGO | S_IRWXO }, //File premissions modified for JetDroid
	.show	= kxsd9_ctl_enable_show,
	.store	= kxsd9_ctl_enable_store,
};


#if 1
static irqreturn_t kxsd9_interrupt_handler(int irq, void *dev_id)
{
	int err;	
	struct kxsd9 *kxsd9;
	disable_irq(IRQ_ACC_INT);
	printk("KB: Inside kxsd9 Interrupt Handler\n");
	kxsd9 = (struct kxsd9 *)(dev_id);
//	interrupted = 1;
	err = schedule_work(&kxsd9->work.work);
	if (!err)
		printk("KB: error in queueing the work\n");

	return IRQ_HANDLED;
}
#endif

#if 0
struct kxsd9 *publish_kxsd9 (void)
{
	return kxsd9_global;
}
EXPORT_SYMBOL(publish_kxsd9);
#endif


static int kxsd9_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct kxsd9 *kxsd9;
	struct input_dev *idev;

	//struct i2c_client *tmp_client;

	//struct akm8973_data *akm;
	int ret;

	printk(KERN_INFO MODULE_NAME ": Initializing Kionix KXSD9 driver "
					"at addr: 0x%02x\n", client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		printk(KERN_ERR MODULE_NAME ": i2c bus not supported\n");
		return -EINVAL;
	}

	kxsd9 = kzalloc(sizeof *kxsd9, GFP_KERNEL);
	if (kxsd9 < 0) {
		printk(KERN_ERR MODULE_NAME ": Not enough memory\n");
		return -ENOMEM;
	}
	mutex_init(&kxsd9->lock);
	kxsd9->client = client;
	i2c_set_clientdata(client, kxsd9);

	// Tried to use same event for both sensors in Jet but didn't work
	//tmp_client = this_client_share();
	//akm = i2c_get_clientdata(tmp_client);

	/*if(akm->input_dev == NULL) {
		printk("Driver Exists\n");
	}
	else {
		printk("Driver Does Not Exist\n");
		//kxsd9->inputdev = akm->input_dev;
	}*/

	idev = input_allocate_device();
	if (idev) {
		idev->name = MODULE_NAME;
		idev->phys=kzalloc(12, GFP_KERNEL);
		snprintf(idev->phys, 11, "i2c/0-%04x", client->addr);
		set_bit(EV_ABS, idev->evbit);
		//input_set_abs_params(idev, ABS_X, -2048, 2047, 0, 0);
		//input_set_abs_params(idev, ABS_Y, -2048, 2047, 0, 0);
		//input_set_abs_params(idev, ABS_Z, -2048, 2047, 0, 0);
		input_set_abs_params(idev, ABS_X, 0, 4095, 0, 0); //Modified for JetDroid
		input_set_abs_params(idev, ABS_Y, 0, 4095, 0, 0); //Modified for JetDroid
		input_set_abs_params(idev, ABS_Z, 0, 4095, 0, 0); //Modified for JetDroid
		input_set_abs_params(idev, ABS_GAS, 0, 65535, 0, 0); //Modified for JetDroid
		if (!input_register_device(idev)) {
			kxsd9->inputdev = idev;
		} else {
			kxsd9->inputdev = 0;
			printk(KERN_ERR MODULE_NAME
					": Failed to register input device\n");
		}
	}


	if (device_create_file(&client->dev, &kxsd9_sysfs_ctl_enable) != 0)
		printk(KERN_ERR MODULE_NAME ": Failed to create 'enable' file\n");
	if (device_create_file(&client->dev, &kxsd9_sysfs_ctl_scale) != 0)
		printk(KERN_ERR MODULE_NAME ": Failed to create 'scale' file\n");
	if (device_create_file(&client->dev, &kxsd9_sysfs_ctl_rate) != 0)
		printk(KERN_ERR MODULE_NAME ": Failed to create 'rate' file\n");
#ifdef CONFIG_ANDROID_POWER
	kxsd9->suspend_lock.name = MODULE_NAME;
	android_init_suspend_lock(&kxsd9->suspend_lock);
	android_lock_suspend(&kxsd9->suspend_lock);
#endif
	INIT_DELAYED_WORK(&kxsd9->work, kxsd9_work);

#ifdef INTERRUPT_DRIVEN
	//ret = request_irq(IRQ_ACC_INT, kxsd9_interrupt_handler, IRQF_TRIGGER_HIGH, "kionix-kxsd9", kxsd9);
	ret = request_irq(IRQ_ACC_INT, kxsd9_interrupt_handler, IRQF_DISABLED, "kionix-kxsd9", kxsd9);
	if (ret < 0) {
		printk("request() irq failed!\n");
		//goto exit;
	}
#endif
	hrtimer_init(&kxsd9->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	kxsd9->timer.function = kxsd9_poll_timer;

	kxsd9_control(kxsd9,KXSD9_CTL_RESET,0);
	kxsd9_control(kxsd9,KXSD9_CTL_SCALE,1);
	kxsd9_control(kxsd9,KXSD9_CTL_RATE,100);
#if KXSD9_DEBUG
	kxsd9_control(kxsd9,KXSD9_CTL_ENABLE,1);
#else
	kxsd9_control(kxsd9,KXSD9_CTL_ENABLE,0);
#endif

	//kxsd9_global = kxsd9;
	return 0;
}

static int kxsd9_remove(struct i2c_client * client)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(client);

	input_unregister_device(kxsd9->inputdev);
	input_free_device(kxsd9->inputdev);
#ifdef CONFIG_ANDROID_POWER
	android_uninit_suspend_lock(&kxsd9->suspend_lock);
#endif
	kfree(kxsd9);
	return 0;
}

#if CONFIG_PM
static int kxsd9_suspend(struct i2c_client * client, pm_message_t mesg)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(client);

	DLOG(": suspending device...\n");
	kxsd9->susp = 1;
	if (kxsd9->on) {
		kxsd9_control(kxsd9,KXSD9_CTL_ENABLE,0);
		kxsd9->on = 1;
	}
	return 0;
}

static int kxsd9_resume(struct i2c_client * client)
{
	struct kxsd9 *kxsd9 = i2c_get_clientdata(client);

	DLOG(": resuming device...\n");
	kxsd9->susp = 0;
	if (kxsd9->on)
		kxsd9_control(kxsd9,KXSD9_CTL_ENABLE,1);
	return 0;
}
#else
#define kxsd9_suspend NULL
#define kxsd9_resume NULL
#endif

static const struct i2c_device_id kxsd9_ids[] = {
        { MODULE_NAME, 0 },
        { }
};

/* Return 0 if detection is successful, -ENODEV otherwise */
/*************** Added for JetDroid ***********************/
static int kxsd9_detect(struct i2c_client *client, int kind,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -ENODEV;

	strlcpy(info->type, MODULE_NAME, I2C_NAME_SIZE);

	return 0;
}

static struct i2c_driver kxsd9_driver = {
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
	},
/*************** Added for JetDroid ***********************/
	.class = I2C_CLASS_HWMON,
	.id_table = kxsd9_ids,
	.probe = kxsd9_probe,
	.remove = kxsd9_remove,
	.detect = kxsd9_detect,
	.address_data = &addr_data,
/**********************************************************/
#if CONFIG_PM
	.suspend = kxsd9_suspend,
	.resume = kxsd9_resume,
#endif
};

static int __init kxsd9_init(void)
{
	printk(KERN_INFO MODULE_NAME ": Registering Kionix KXSD9 driver\n");

#ifdef INTERRUPT_DRIVEN
	s3c_gpio_cfgpin(GPIO_ACC_INT, S3C_GPIO_SFN(GPIO_ACC_INT_AF));
	s3c_gpio_setpull(GPIO_ACC_INT, S3C_GPIO_PULL_NONE);

	set_irq_type(IRQ_ACC_INT, IRQ_TYPE_EDGE_RISING);
	//set_irq_type(IRQ_ACC_INT, IRQ_TYPE_LEVEL_HIGH);
#endif
	return i2c_add_driver(&kxsd9_driver);
}

static void __exit kxsd9_exit(void)
{
	printk(KERN_INFO MODULE_NAME ": Unregistered Kionix KXSD9 driver\n");
	i2c_del_driver(&kxsd9_driver);
}

MODULE_AUTHOR("Job Bolle");
MODULE_DESCRIPTION("Kionix KXSD9 driver");
MODULE_LICENSE("GPL");

module_init(kxsd9_init);
module_exit(kxsd9_exit);
