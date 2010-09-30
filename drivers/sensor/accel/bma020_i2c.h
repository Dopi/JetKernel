#ifndef __BMA020_I2C_HEADER__
#define __BMA020_I2C_HEADER__

char  i2c_acc_bma020_read (u8, u8 *, unsigned int);
char  i2c_acc_bma020_write(u8 reg, u8 *val);

int  i2c_acc_bma020_init(void);
void i2c_acc_bma020_exit(void);

#endif
