#ifndef __TC358749_H__
#define __TC358749_H__

struct gpioctl {
	int gpio;
	int active;
};

struct tc358749_hdmiin {
	struct i2c_client		*client;
	struct device			*dev;
	struct class			*reg_class;
	struct gpioctl io_power;
	struct gpioctl io_reset;
	struct gpioctl io_stanby;
	struct gpioctl io_int;
};

struct reg_val_sz {
	unsigned int reg;
	unsigned int val;
	unsigned int val_sz;
};

#define TC358749_I2C_RATE	(100*1000)
/*int  i2c_write_8bits(const int reg, char value);
int  i2c_write_16bits(const int reg, int value);
char i2c_read_8bits(int reg);
int i2c_read_16bits(int reg);*/
int tc_read_reg(struct reg_val_sz *rvs);
int tc_write_reg(struct reg_val_sz *rvs);

#endif

