int i2c_emul_init(void);
int i2c_emul_write(unsigned char addr, unsigned char reg, unsigned char val);
int i2c_emul_read(unsigned char addr, unsigned char reg, unsigned char* val);
int i2c_emul_test(void);
void gpio_test(void);

