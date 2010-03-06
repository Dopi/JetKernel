#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>

#include <asm/io.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/hardware.h>

#include "i2c-emul.h"

#define S3C_ADDR_BASE   (0xF4000000)
#ifndef __ASSEMBLY__
#define S3C_ADDR(x) ((void __iomem __force *)S3C_ADDR_BASE + (x))
#else
#define S3C_ADDR(x) (S3C_ADDR_BASE + (x))
#endif
#define S3C_VA_GPIO S3C_ADDR(0x00600000)    /* GPIO */
#define S3C24XX_VA_GPIO     S3C_VA_GPIO
#define S3C_GPIOREG(x) ((x) + S3C24XX_VA_GPIO)
#define S3C_GPBDAT     S3C_GPIOREG(0x24)
#define S3C_GPCDAT     S3C_GPIOREG(0x44)

#define S3C_GPIONO(bank,offset) ((bank) + (offset))
#define S3C_GPIO_BANKB   (32*1)
 
#define S3C_GPB2       S3C_GPIONO(S3C_GPIO_BANKB, 2)
#define S3C_GPB2_INP            (0)
#define S3C_GPB2_OUTP           (1)
#define S3C_GPB2_UART_RXD3      (2)
#define S3C_GPB2_IRDA_RXD       (3)
#define S3C_GPB2_EXT_DMA_REQ    (4)
#define S3C_GPB2_ADDR_CF2       (5)
#define S3C_GPB2_RESERVED       (6)
#define S3C_GPB2_I2C_SCL        (6) //+anthony [10th Oct.2008]
#define S3C_GPB2_EXT_INT_G1_10  (7)

#define S3C_GPB3       S3C_GPIONO(S3C_GPIO_BANKB, 3)
#define S3C_GPB3_INP            (0)
#define S3C_GPB3_OUTP           (1)
#define S3C_GPB3_UART_TXD3      (2)
#define S3C_GPB3_IRDA_RXD       (3)
#define S3C_GPB3_EXT_DMA_ACK    (4)
#define S3C_GPB3_RESERVED1      (5)
#define S3C_GPB3_RESERVED2      (6)
#define S3C_GPB3_I2C_SDA        (6) //+anthony [10th Oct.2008]
#define S3C_GPB3_EXT_INT_G1_11  (7)













#if 0
#define SCL_Hi  { gpdat = __raw_readl(S3C_GPCDAT);\
                    gpdat |= (0x1 << 7);\
                    __raw_writel(gpdat,S3C_GPCDAT);\
                    udelay(DELAY); }
#define SCL_Lo  { gpdat = __raw_readl(S3C_GPCDAT);\
                    gpdat &= ~(0x1 << 7);\
                    __raw_writel(gpdat,S3C_GPCDAT);\
                    udelay(DELAY); }  
#define SDA_Hi  { gpdat = __raw_readl(S3C_GPCDAT);\
                    gpdat |= (0x1 << 6);\
                    __raw_writel(gpdat,S3C_GPCDAT);\
                    udelay(DELAY); }
#define SDA_Lo  { gpdat = __raw_readl(S3C_GPCDAT);\
                    gpdat &= ~(0x1 << 6);\
                    __raw_writel(gpdat,S3C_GPCDAT); \
                    udelay(DELAY); } 
#endif

#if 0	// GPC case
#define SCL_Hi  { s3c_gpio_setpin(S3C_GPC7,1);}
#define SCL_Lo  { s3c_gpio_setpin(S3C_GPC7,0);}
#define SDA_Hi  { s3c_gpio_setpin(S3C_GPC6,1);}
#define SDA_Lo  { s3c_gpio_setpin(S3C_GPC6,0);}
#else



#if 0
#define SCL_Hi  { s3c_gpio_setpin(S3C_GPB2,1);}
#define SCL_Lo  { s3c_gpio_setpin(S3C_GPB2,0);}
#define SDA_Hi  { s3c_gpio_setpin(S3C_GPB3,1);}
#define SDA_Lo  { s3c_gpio_setpin(S3C_GPB3,0);}
#else

#define SCL_Hi  { gpio_set_value(GPIO_I2C1_SCL, GPIO_LEVEL_HIGH); }
#define SCL_Lo  { gpio_set_value(GPIO_I2C1_SCL, GPIO_LEVEL_LOW); }
#define SDA_Hi  { gpio_set_value(GPIO_I2C1_SDA, GPIO_LEVEL_HIGH); }
#define SDA_Lo  { gpio_set_value(GPIO_I2C1_SDA, GPIO_LEVEL_LOW); }

#endif


#endif


//sktlinux
//SDA - GPB3
//SCL - GPB2

#define DELAY       10
#define	SDA_BIT	3

#define I2C_EMUL_DBG    0
#if I2C_EMUL_DBG
#define dbg(x...)      printk(x)
#else
#define dbg(x...)
#endif

static inline void SET_SDA(int val)
{
    //dbg("   before SET_SDA: val = %d, GPCDAT = 0x%x\n",val,__raw_readl(S3C_GPCDAT));
    if(val)
    {
        SDA_Hi;
    }
    else
    {
        SDA_Lo;
    }
    //dbg("   after SET_SDA: GPCDAT = 0x%x\n", __raw_readl(S3C_GPCDAT));
}
static inline int GET_SDA(void)
{
    int ret;
    //unsigned long reg;
    unsigned long dat;
    
    //s3c_gpio_cfgpin(S3C_GPC6, S3C_GPC6_INP); 
    
    dat = __raw_readl(S3C_GPBDAT);
    //dbg("GET_SDA: GPCDAT = 0x%x\n", dat);
    
    if( (0x1 << SDA_BIT) & dat)
        ret = 1;
    else
        ret = 0;
    
    //s3c_gpio_cfgpin(S3C_GPC6, S3C_GPC6_OUTP);
    
    return ret;       
}

// read ACK(SDA)
static inline int GET_ACK(void)
{
    int ret;
    //unsigned long reg;
    unsigned long ack = 0;
    
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_INP);

    //udelay(DELAY);
    udelay(DELAY);

    SCL_Hi;
    udelay(DELAY);
    ack = __raw_readl(S3C_GPBDAT);
    udelay(DELAY);
    SCL_Lo;
    //udelay(DELAY);
    //dbg("ACK: GPCDAT = 0x%x\n", ack);
    
    
    if( (0x1 << SDA_BIT) & ack)
        ret = 1;
    else
        ret = 0;
    
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
    
    return ret;       
}
/* --- other auxiliary functions --------------------------------------	*/
static void i2c_emul_start(void) 
{
	/* assert: scl, sda are high */
	//unsigned long gpdat;
	
    SDA_Hi;
    SCL_Hi;
    udelay(DELAY);
    
	SDA_Lo;
    udelay(DELAY);
	SCL_Lo;
    udelay(DELAY);
}

static void i2c_emul_repstart(void) 
{
	/* scl, sda may not be high */
	
	SDA_Hi;
	SCL_Hi;
    udelay(DELAY);
	
	SDA_Lo;
    udelay(DELAY);
	SCL_Lo;
    udelay(DELAY);
}
static void i2c_emul_stop(void) 
{
	/* assert: scl is low */
	
	SDA_Lo;
    udelay(DELAY);
	SCL_Hi;
    udelay(DELAY);
	SDA_Hi;
    udelay(DELAY);
}

static int i2c_emul_outb(char c)
{
	int i;
	int sb;
	int ack;
	
    dbg("[i2c-emul] Enter %s, char  = 0x%x\n",__FUNCTION__,c);

	/* assert: scl is low */
	for ( i=7 ; i>=0 ; i-- ) {
		sb = c & ( 1 << i );

		SET_SDA(sb);
		udelay(DELAY);
		
		SCL_Hi;
		udelay(DELAY);
		
		SCL_Lo;
		//udelay(DELAY);
		//udelay(DELAY);
	}
	udelay(DELAY);
	SDA_Hi;   // set SDA Hi before reading ack
    //dbg("   after SDA_Hi GPCDAT = 0x%x\n", __raw_readl(S3C_GPCDAT));
	//udelay(DELAY);
	udelay(DELAY);
	
    ack = GET_ACK();
	//ack = GET_SDA();
    dbg("   ack = %d\n",ack);
	
	//SCL_Hi;
	//udelay(DELAY);
	
	SCL_Lo;
	udelay(DELAY);
	
	if(ack == 0)    return 0;   // OK
	else            return -1;  // error
}


static unsigned char i2c_emul_inb(void) 
{
	/* read byte via i2c port, without start/stop sequence	*/
	/* acknowledge is sent in i2c_read.			*/
	int i;
	unsigned char indata=0;
	
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_INP);
	for (i = 0;i < 8;i++)
	{
	    udelay(DELAY);
	    SCL_Hi;
	    udelay(DELAY);
	    indata *= 2;    // shift left 
	    
	    if(GET_SDA())
	        indata |= 0x01;
	        
	    SCL_Lo;
	}
	
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
    
	//return (int)(indata & 0xff);

    //dbg(" inb : 0x%x\n",indata);
    return indata;
}

static inline int doAddress(unsigned char addr, int rw)
{
    unsigned char tmp;
    //unsigned long gpdat;
    
    dbg("[i2c-emul] Enter %s\n",__FUNCTION__);
    
    //i2c_start();
    tmp = addr << 1;
    tmp |= rw;
    
    if(i2c_emul_outb((char)tmp) < 0)
    {
        dbg("[i2c-emul] error in doAddress\n");
        return -1;
    }
        
    dbg("[i2c-emul]   doAddress OK\n");
    return 0;
}


int i2c_emul_test(void)
{
    int ret = 0;
    unsigned char read_val;

    dbg("[i2c-emul] Enter %s\n",__FUNCTION__);

    s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_OUTP);
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
    
    //mdelay(3000);
    dbg("   set GPC6,7 pull up,down disable\n");
    //s3c_gpio_pullup(S3C_GPB3, 2);
    //s3c_gpio_pullup(S3C_GPB2, 2);
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)

    mdelay(3000);
    i2c_emul_write(0x1a, 0x01, 0x00);
    i2c_emul_write(0x1a, 0x10, 0x13);
    i2c_emul_write(0x1a, 0x00, 0x01);
    ret = i2c_emul_read(0x1a,0x10,&read_val);
    dbg("   i2c_read : reg 0x10 -> val 0x%x\n",read_val);


	 // setup GPIO for I2C
	s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_I2C_SCL);	// IIC SCL (GPB5)
	s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_I2C_SDA);	// IIC SDA (GPB6)
	//s3c_gpio_pullup(S3C_GPB2, 0x2); // IIC SCL Pull-Up enable (GPB5)
	//s3c_gpio_pullup(S3C_GPB3, 0x2); // IIC SCL Pull-Up enable (GPB6)
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)
    
    return 0;
}

void gpio_test(void)
{
    unsigned char reg;

	dbg("   before : GPBCON = 0x%x, GPBDAT = 0x%x, GPBPU = 0x%x\n",__raw_readl(S3C_GPBCON), __raw_readl(S3C_GPBDAT),__raw_readl(S3C_GPBPU));
	
	s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_OUTP);
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
    
    //mdelay(3000);
    dbg("   set GPC6,7 pull up,down disable\n");
    //s3c_gpio_pullup(S3C_GPB3, 2);
    //s3c_gpio_pullup(S3C_GPB2, 2);
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)
	dbg("   set GPC6 to 0(Low) \n");
    reg = __raw_readl(S3C_GPBDAT);
    reg &= ~(0x1 << 6);
    __raw_writel(reg,S3C_GPBDAT);

	dbg("   after : GPBCON = 0x%x, GPBDAT = 0x%x, GPBPU = 0x%x\n",__raw_readl(S3C_GPBCON), __raw_readl(S3C_GPBDAT),__raw_readl(S3C_GPBPU));

    reg = __raw_readl(S3C_GPBDAT);
    reg |= 0x1 << 3;
    __raw_writel(reg,S3C_GPBDAT);
    mdelay(3000);
    reg &= ~(0x1 << 3);
    __raw_writel(reg,S3C_GPBDAT);
    mdelay(3000);
    reg |= 0x1 << 3;
    __raw_writel(reg,S3C_GPBDAT);
    mdelay(3000);

}
int i2c_emul_init(void)
{
    int ret;
    unsigned long   reg;
    unsigned char   read_val=0;
    unsigned char   msg[2],addr;
    
    addr = 0x1a;
    
    dbg("[i2c-emul] Enter %s\n",__FUNCTION__);
    
    
    dbg("   before : GPCCON = 0x%x, GPCDAT = 0x%x, GPCPU = 0x%x\n",__raw_readl(S3C_GPCCON), __raw_readl(S3C_GPCDAT),__raw_readl(S3C_GPCPU));

    //mdelay(3000);
    dbg("   set GPC6,7 output port \n");
    s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_OUTP);
    s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
    
    //mdelay(3000);
    dbg("   set GPC6,7 pull up,down disable\n");
    //s3c_gpio_pullup(S3C_GPB3, 2);
    //s3c_gpio_pullup(S3C_GPB2, 2);
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)

    dbg("   set GPC6 to 0(Low) \n");
    reg = __raw_readl(S3C_GPCDAT);
    reg &= ~(0x1 << 6);
    __raw_writel(reg,S3C_GPCDAT);
    
    dbg("   after : GPCCON = 0x%x, GPCDAT = 0x%x, GPCPU = 0x%x\n",__raw_readl(S3C_GPCCON), __raw_readl(S3C_GPCDAT),__raw_readl(S3C_GPCPU));
   
    //mdelay(3000);
    CODEC_CLK_EN_SET;

    //while(1);
    //gpio_test();
    mdelay(3000);

    dbg("   power on LM49350\n");
/*    msg[0] = 0x01;
    msg[1] = 0x00;  // 
    i2c_emul_write(addr, 0x01, 0x00);
    i2c_emul_write(addr, 0x10, 0x30);
    i2c_emul_write(addr, 0x18, 0x00);
    i2c_emul_write(addr, 0x19, 0x00);
    i2c_emul_write(addr, 0x00, 0x01);
*/

    mdelay(3000);
    ret = i2c_emul_read(0x1a,0x10,&read_val);
    dbg("   i2c_read : reg 0x10 -> val 0x%x\n",read_val);

    return 0;
}

int i2c_emul_read(unsigned char dev_addr,unsigned char reg_addr, unsigned char*  reg_val)
{
    int ret = 0;
    int ack;

    dbg("[i2c-emul] Enter %s\n",__FUNCTION__);
    
	i2c_emul_start();

	if(doAddress(dev_addr, 0) < 0)
	{
	    return -1;
    }
    if((ret = i2c_emul_outb(reg_addr)) < 0)
    {
	    dbg("[i2c-emul] error in i2c-read\n");
	    return -1;
    }
    i2c_emul_repstart();
	if(doAddress(dev_addr, 1) < 0)
	{
	    return -1;
    }
    
   
    *(reg_val) = i2c_emul_inb();
    printk("i2c_emul_read : reg = 0x%x, val = 0x%x\n",reg_addr,*reg_val);

    ack = GET_ACK();
    // send ACK
    /*
    SCL_Lo;
    SDA_Lo;
    udelay(DELAY);
    SCL_Hi;
    udelay(DELAY);
    SCL_Lo;
    */

	i2c_emul_stop();

    return ret;
}

int i2c_emul_write(unsigned char dev_addr,unsigned char reg_addr , unsigned char reg_val)
{
	int ret;
	
    dbg("[i2c-emul] Enter %s\n",__FUNCTION__);
    
	i2c_emul_start();
	
	if(doAddress(dev_addr, 0) < 0)
	{
	    return -1;
	}
		
	ret = i2c_emul_outb(reg_addr);
	if(ret < 0)
	{
	    dbg("[i2c-emul] error in writing reg_addr\n");
	    return -1;
	}
    else
    {
        if((ret = i2c_emul_outb(reg_val)) < 0)
        {
            dbg("[i2c-emul] error in writing reg_val\n");
            return -1;
        }
    }
	
	i2c_emul_stop();
	return 0;
}

//EXPORT_SYMBOL(i2c_init);
//EXPORT_SYMBOL(i2c_xfer);
