#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include "tc358749.h"

static struct tc358749_hdmiin *tc358749_hdmiin;
static int audio_start = 0;

int tc_read_reg(struct reg_val_sz *rvs)
{
    int err, i, retry = 2, tmp, num_msg;
    unsigned char buf[8];
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;
	unsigned int reg, val, val_sz;

	adapter = tc358749_hdmiin->client->adapter;
	reg = rvs->reg;
	val = rvs->val;
	val_sz = rvs->val_sz;

	buf[0] = reg>>8;
	buf[1] = reg;

	num_msg = 0;
	msgs[0].addr = tc358749_hdmiin->client->addr;
	msgs[0].flags = tc358749_hdmiin->client->flags;
	msgs[0].len = 2;
	msgs[0].buf = buf;
	msgs[0].scl_rate = TC358749_I2C_RATE;

	num_msg++;

	msgs[1].addr = tc358749_hdmiin->client->addr;
	msgs[1].flags = tc358749_hdmiin->client->flags | I2C_M_RD;
	msgs[1].len = val_sz;
	msgs[1].buf = buf;
	msgs[1].scl_rate = TC358749_I2C_RATE;

	err = -EAGAIN;    
	num_msg++;

	while ((retry-- > 0) && (err < 0)) {
		if (num_msg==1) {			
			err = i2c_transfer(adapter, &msgs[1], num_msg);
		} else {
			err = i2c_transfer(adapter, msgs, num_msg);
		}
	
		if (err >= 0) {
            err = 0;
		} else {
			dev_err(tc358749_hdmiin->dev, 
				"i2c read dev(addr:0x%x) failed,try again-%d!",
				tc358749_hdmiin->client->addr, retry);
			udelay(10);
		}
	}


    if (err==0) { 
        val = 0x00;
        for(i=0; i<val_sz; i++) {
            tmp = buf[i];
            val |= (tmp <<((val_sz-1-i)*8));
        }
		return val;
    }

    return err;
}

int tc_write_reg(struct reg_val_sz *rvs)
{
	struct i2c_msg msgs;
	int ret = -1;
	int i;
	unsigned char buf[8];
	unsigned int reg, val, val_sz;

	reg = rvs->reg;
	val = rvs->val;
	val_sz = rvs->val_sz;
	buf[0] = reg >> 8;
	buf[1] = reg;

	for (i = 0; i < val_sz; i++) {
		buf[2+i] = (val>>((val_sz-1-i)*8))&0xff;
	}

	
	msgs.addr = tc358749_hdmiin->client->addr;
	msgs.flags = tc358749_hdmiin->client->flags;
	msgs.len = 2 + val_sz;
	msgs.buf = buf;
	msgs.scl_rate = TC358749_I2C_RATE;

	ret = i2c_transfer(tc358749_hdmiin->client->adapter, &msgs, 1);
	if (ret < 0)
		dev_err(tc358749_hdmiin->dev,
			"%s:i2c_transfer fail =%d\n", __func__, ret);
	return ret;
}

static struct reg_val_sz tc358749_reg_def[] = {
	{0x0004, 0x0400,     2},
	{0x0002, 0x807f,     2},
	{0x0002, 0x0000,     2},
	{0x0020, 0x5b30,     2},
	{0x0022, 0x0302,     2},
	{0x0022, 0x1302,     2},
	{0x0006, 0xf401,     2},
	{0x0060, 0x0100,     2},
	{0x7080, 0x0000,     2},
	{0x0014, 0x0000,     2},
	{0x0016, 0xff05,     2},
	{0x0140, 0x00000000, 4},
	{0x0144, 0x00000000, 4},
	{0x0148, 0x00000000, 4},
	{0x014c, 0x00000000, 4},
	{0x0150, 0x00000000, 4},
	{0x0210, 0x70170000, 4},
	{0x0214, 0x05000000, 4},
	{0x0218, 0x05260000, 4},
	{0x021c, 0x02000000, 4},
	{0x0220, 0x05050000, 4},
	{0x0224, 0x68420000, 4},
	{0x0228, 0x09000000, 4},
	{0x022c, 0x03000000, 4},
	{0x0230, 0x05000000, 4},
	{0x0234, 0x1f000000, 4},
	{0x0238, 0x00000000, 4},
	{0x0204, 0x01000000, 4},
	{0x0518, 0x01000000, 4},
	{0x0500, 0x868000a3, 4},
	{0x0012, 0x0200,     2},
	{0x8502, 0x01,       1},
	{0x8512, 0xfe,       1},
	{0x8514, 0x00,       1},
	{0x8515, 0x00,       1},
	{0x8516, 0x00,       1},
	{0x8531, 0x01,       1},
	{0x8540, 0x8c,       1},
	{0x8541, 0x0a,       1},
	{0x8630, 0xb0,       1},
	{0x8631, 0x1e,       1},
	{0x8632, 0x04,       1},
	{0x8670, 0x01,       1},
	{0x8532, 0x80,       1},
	{0x8536, 0x40,       1},
	{0x853f, 0x0a,       1},
	{0x8543, 0x32,       1},
	{0x8544, 0x10,       1},
	{0x8545, 0x31,       1},
	{0x8546, 0x2d,       1},
	{0x85c7, 0x01,       1},
	{0x85ca, 0x00,       1},
	{0x85cb, 0x01,       1},
	{0x8c00, 0x00,       1},
	{0x8c01, 0xff,       1},
	{0x8c02, 0xff,       1},
	{0x8c03, 0xff,       1},
	{0x8c04, 0xff,       1},
	{0x8c05, 0xff,       1},
	{0x8c06, 0xff,       1},
	{0x8c07, 0x00,       1},
	{0x8c08, 0x52,       1},
	{0x8c09, 0x62,       1},
	{0x8c0a, 0x88,       1},
	{0x8c0b, 0x88,       1},
	{0x8c0c, 0x00,       1},
	{0x8c0d, 0x88,       1},
	{0x8c0e, 0x88,       1},
	{0x8c0f, 0x88,       1},
	{0x8c10, 0x1c,       1},
	{0x8c11, 0x15,       1},
	{0x8c12, 0x01,       1},
	{0x8c13, 0x03,       1},
	{0x8c14, 0x80,       1},
	{0x8c15, 0x00,       1},
	{0x8c16, 0x00,       1},
	{0x8c17, 0x78,       1},
	{0x8c18, 0x0a,       1},
	{0x8c19, 0x0d,       1},
	{0x8c1a, 0xc9,       1},
	{0x8c1b, 0xa0,       1},
	{0x8c1c, 0x57,       1},
	{0x8c1d, 0x47,       1},
	{0x8c1e, 0x98,       1},
	{0x8c1f, 0x27,       1},
	{0x8c20, 0x12,       1},
	{0x8c21, 0x48,       1},
	{0x8c22, 0x4c,       1},
	{0x8c23, 0x00,       1},
	{0x8c24, 0x00,       1},
	{0x8c25, 0x00,       1},
	{0x8c26, 0x01,       1},
	{0x8c27, 0x01,       1},
	{0x8c28, 0x01,       1},
	{0x8c29, 0x01,       1},
	{0x8c2a, 0x01,       1},
	{0x8c2b, 0x01,       1},
	{0x8c2c, 0x01,       1},
	{0x8c2d, 0x01,       1},
	{0x8c2e, 0x01,       1},
	{0x8c2f, 0x01,       1},
	{0x8c30, 0x01,       1},
	{0x8c31, 0x01,       1},
	{0x8c32, 0x01,       1},
	{0x8c33, 0x01,       1},
	{0x8c34, 0x01,       1},
	{0x8c35, 0x01,       1},
	{0x8c36, 0x02,       1},
	{0x8c37, 0x3a,       1},
	{0x8c38, 0x80,       1},
	{0x8c39, 0x18,       1},
	{0x8c3a, 0x71,       1},
	{0x8c3b, 0x38,       1},
	{0x8c3c, 0x2d,       1},
	{0x8c3d, 0x40,       1},
	{0x8c3e, 0x58,       1},
	{0x8c3f, 0x2c,       1},
	{0x8c40, 0x45,       1},
	{0x8c41, 0x00,       1},
	{0x8c42, 0xc4,       1},
	{0x8c43, 0x8e,       1},
	{0x8c44, 0x21,       1},
	{0x8c45, 0x00,       1},
	{0x8c46, 0x00,       1},
	{0x8c47, 0x1e,       1},
	{0x8c48, 0x01,       1},
	{0x8c49, 0x1d,       1},
	{0x8c4a, 0x00,       1},
	{0x8c4b, 0x72,       1},
	{0x8c4c, 0x51,       1},
	{0x8c4d, 0xd0,       1},
	{0x8c4e, 0x1e,       1},
	{0x8c4f, 0x20,       1},
	{0x8c50, 0x6e,       1},
	{0x8c51, 0x28,       1},
	{0x8c52, 0x55,       1},
	{0x8c53, 0x00,       1},
	{0x8c54, 0xc4,       1},
	{0x8c55, 0x8e,       1},
	{0x8c56, 0x21,       1},
	{0x8c57, 0x00,       1},
	{0x8c58, 0x00,       1},
	{0x8c59, 0x1e,       1},
	{0x8c5a, 0x00,       1},
	{0x8c5b, 0x00,       1},
	{0x8c5c, 0x00,       1},
	{0x8c5d, 0xfc,       1},
	{0x8c5e, 0x00,       1},
	{0x8c5f, 0x54,       1},
	{0x8c60, 0x6f,       1},
	{0x8c61, 0x73,       1},
	{0x8c62, 0x68,       1},
	{0x8c63, 0x69,       1},
	{0x8c64, 0x62,       1},
	{0x8c65, 0x61,       1},
	{0x8c66, 0x2d,       1},
	{0x8c67, 0x48,       1},
	{0x8c68, 0x32,       1},
	{0x8c69, 0x43,       1},
	{0x8c6a, 0x50,       1},
	{0x8c6b, 0x0a,       1},
	{0x8c6c, 0x00,       1},
	{0x8c6d, 0x00,       1},
	{0x8c6e, 0x00,       1},
	{0x8c6f, 0xfd,       1},
	{0x8c70, 0x00,       1},
	{0x8c71, 0x17,       1},
	{0x8c72, 0x3d,       1},
	{0x8c73, 0x0f,       1},
	{0x8c74, 0x8c,       1},
	{0x8c75, 0x17,       1},
	{0x8c76, 0x00,       1},
	{0x8c77, 0x0a,       1},
	{0x8c78, 0x20,       1},
	{0x8c79, 0x20,       1},
	{0x8c7a, 0x20,       1},
	{0x8c7b, 0x20,       1},
	{0x8c7c, 0x20,       1},
	{0x8c7d, 0x20,       1},
	{0x8c7e, 0x01,       1},
	{0x8c7f, 0x63,       1},
	{0x8c80, 0x02,       1},
	{0x8c81, 0x03,       1},
	{0x8c82, 0x17,       1},
	{0x8c83, 0x74,       1},
	{0x8c84, 0x47,       1},
	{0x8c85, 0x10,       1},
	{0x8c86, 0x04,       1},
	{0x8c87, 0x05,       1},
	{0x8c88, 0x05,       1},
	{0x8c89, 0x05,       1},
	{0x8c8a, 0x05,       1},
	{0x8c8b, 0x05,       1},
	{0x8c8c, 0x23,       1},
	{0x8c8d, 0x09,       1},
	{0x8c8e, 0x07,       1},
	{0x8c8f, 0x01,       1},
	{0x8c90, 0x66,       1},
	{0x8c91, 0x03,       1},
	{0x8c92, 0x0c,       1},
	{0x8c93, 0x00,       1},
	{0x8c94, 0x30,       1},
	{0x8c95, 0x00,       1},
	{0x8c96, 0x80,       1},
	{0x8c97, 0x8c,       1},
	{0x8c98, 0x0a,       1},
	{0x8c99, 0xd0,       1},
	{0x8c9a, 0x01,       1},
	{0x8c9b, 0x1d,       1},
	{0x8c9c, 0x80,       1},
	{0x8c9d, 0x18,       1},
	{0x8c9e, 0x71,       1},
	{0x8c9f, 0x38,       1},
	{0x8ca0, 0x16,       1},
	{0x8ca1, 0x40,       1},
	{0x8ca2, 0x58,       1},
	{0x8ca3, 0x2c,       1},
	{0x8ca4, 0x25,       1},
	{0x8ca5, 0x00,       1},
	{0x8ca6, 0x80,       1},
	{0x8ca7, 0x38,       1},
	{0x8ca8, 0x74,       1},
	{0x8ca9, 0x00,       1},
	{0x8caa, 0x00,       1},
	{0x8cab, 0x18,       1},
	{0x8cac, 0x01,       1},
	{0x8cad, 0x1d,       1},
	{0x8cae, 0x80,       1},
	{0x8caf, 0x18,       1},
	{0x8cb0, 0x71,       1},
	{0x8cb1, 0x38,       1},
	{0x8cb2, 0x16,       1},
	{0x8cb3, 0x40,       1},
	{0x8cb4, 0x58,       1},
	{0x8cb5, 0x2c,       1},
	{0x8cb6, 0x25,       1},
	{0x8cb7, 0x00,       1},
	{0x8cb8, 0x80,       1},
	{0x8cb9, 0x38,       1},
	{0x8cba, 0x74,       1},
	{0x8cbb, 0x00,       1},
	{0x8cbc, 0x00,       1},
	{0x8cbd, 0x18,       1},
	{0x8cbe, 0x01,       1},
	{0x8cbf, 0x1d,       1},
	{0x8cc0, 0x80,       1},
	{0x8cc1, 0x18,       1},
	{0x8cc2, 0x71,       1},
	{0x8cc3, 0x38,       1},
	{0x8cc4, 0x16,       1},
	{0x8cc5, 0x40,       1},
	{0x8cc6, 0x58,       1},
	{0x8cc7, 0x2c,       1},
	{0x8cc8, 0x25,       1},
	{0x8cc9, 0x00,       1},
	{0x8cca, 0x80,       1},
	{0x8ccb, 0x38,       1},
	{0x8ccc, 0x74,       1},
	{0x8ccd, 0x00,       1},
	{0x8cce, 0x00,       1},
	{0x8ccf, 0x18,       1},
	{0x8cd0, 0x01,       1},
	{0x8cd1, 0x1d,       1},
	{0x8cd2, 0x80,       1},
	{0x8cd3, 0x18,       1},
	{0x8cd4, 0x71,       1},
	{0x8cd5, 0x38,       1},
	{0x8cd6, 0x16,       1},
	{0x8cd7, 0x40,       1},
	{0x8cd8, 0x58,       1},
	{0x8cd9, 0x2c,       1},
	{0x8cda, 0x25,       1},
	{0x8cdb, 0x00,       1},
	{0x8cdc, 0x80,       1},
	{0x8cdd, 0x38,       1},
	{0x8cde, 0x74,       1},
	{0x8cdf, 0x00,       1},
	{0x8ce0, 0x00,       1},
	{0x8ce1, 0x18,       1},
	{0x8ce2, 0x00,       1},
	{0x8ce3, 0x00,       1},
	{0x8ce4, 0x00,       1},
	{0x8ce5, 0x00,       1},
	{0x8ce6, 0x00,       1},
	{0x8ce7, 0x00,       1},
	{0x8ce8, 0x00,       1},
	{0x8ce9, 0x00,       1},
	{0x8cea, 0x00,       1},
	{0x8ceb, 0x00,       1},
	{0x8cec, 0x00,       1},
	{0x8ced, 0x00,       1},
	{0x8cee, 0x00,       1},
	{0x8cef, 0x00,       1},
	{0x8cf0, 0x00,       1},
	{0x8cf1, 0x00,       1},
	{0x8cf2, 0x00,       1},
	{0x8cf3, 0x00,       1},
	{0x8cf4, 0x00,       1},
	{0x8cf5, 0x00,       1},
	{0x8cf6, 0x00,       1},
	{0x8cf7, 0x00,       1},
	{0x8cf8, 0x00,       1},
	{0x8cf9, 0x00,       1},
	{0x8cfa, 0x00,       1},
	{0x8cfb, 0x00,       1},
	{0x8cfc, 0x00,       1},
	{0x8cfd, 0x00,       1},
	{0x8cfe, 0x00,       1},
	{0x8cff, 0xb5,       1},
	{0x8573, 0x81,       1},
	{0x8600, 0x00,       1},
	{0x8602, 0xf3,       1},
	{0x8603, 0x02,       1},
	{0x8604, 0x0c,       1},
	{0x8606, 0x05,       1},
	{0x8607, 0x00,       1},
	{0x8620, 0x2a,       1},
	{0x8621, 0x02,       1},
	{0x8640, 0x01,       1},
	{0x8641, 0x65,       1},
	{0x8642, 0x07,       1},
	{0x8652, 0x02,       1},
	{0x8665, 0x10,       1},
	{0x8709, 0xff,       1},
	{0x870b, 0x2c,       1},
	{0x870c, 0x53,       1},
	{0x870d, 0x01,       1},
	{0x870e, 0x30,       1},
	{0x9007, 0x10,       1},
	{0x854a, 0x01,       1},
	{0x854a, 0x01,       1},
	{0x0004, 0xd70c,     2},
	{0X0000, 0X00,       0}, /* table end */
};


static ssize_t  hdmiin_reg_show(struct class *cls,
struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "hdmi reg  = *****\n");
	
}

static ssize_t hdmiin_reg_store(struct class *cls,
struct class_attribute *attr, const char *buf, size_t count)
{
	char *string;
	int regval;

	regval = simple_strtol(buf,&string,0);
	pr_info("write  = 0x%x\n", regval);
	return strnlen(buf, PAGE_SIZE);
}

extern int snd_start_hdmi_in_audio_route(void);
extern int snd_stop_hdmi_in_audio_route(void);

static ssize_t  hdmiin_audio_show(struct class *cls,
struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "audio statas %d \n",audio_start);
}

extern void es8323_codec_set_reg(int loopback);
static ssize_t hdmiin_audio_store(struct class *cls,
struct class_attribute *attr, const char *buf, size_t count)
{
	char *string;
	int val;

	val = simple_strtol(buf,&string,0);
    if (val == 0) {
        audio_start = 0;
        snd_stop_hdmi_in_audio_route();
        pr_info("end hdmiin detect \n");
    } else if (val == 1) {
        audio_start = 1;
        pr_info("start hdmiin detect \n");
        snd_start_hdmi_in_audio_route();
    } else if (val == 2) {
        audio_start = 1;
        pr_info("stop local , then open hdmiin\n");
        //snd_stop_hdmi_in_audio_route();
        //snd_start_hdmi_in_audio_route();
    }

	return strnlen(buf, PAGE_SIZE);
}

static CLASS_ATTR(hdmiin, 0666, hdmiin_reg_show, hdmiin_reg_store);
static CLASS_ATTR(hdmiin_audio, 0666, hdmiin_audio_show, hdmiin_audio_store);
 
static int tc358749_hdmiin_thread(void *__unused)
{
	static struct reg_val_sz tc358749_reg_test[] = {{0x8520, 0x00, 1}, };
	int val;

	while(1) {
        msleep(1000);
        if (audio_start) {
            msleep(100);
            val = tc_read_reg(&tc358749_reg_test[0]);
            pr_info("%s val = 0x%x\n", __func__, val);
            if ((val > 0 ) && ((val & 0x01) == 1)) {
                snd_start_hdmi_in_audio_route();
            } else {
                snd_stop_hdmi_in_audio_route();
            }
        //} else {
        //    msleep(1000);
        //    snd_stop_hdmi_in_audio_route();
        }
    }
	return 0;
}

static int tc358749_hdmiin_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct device_node *np;
	enum of_gpio_flags flags;
	int ret;

	int i = 0;

	pr_info("%s!\n", __func__);
	tc358749_hdmiin = devm_kzalloc(&client->dev,
				       sizeof(*tc358749_hdmiin),
				       GFP_KERNEL);
	if (!tc358749_hdmiin) {
		dev_err(&client->dev, "tc358749 hdmiin device kmalloc fail!\n");
		return -ENOMEM;
	}
	memset(tc358749_hdmiin, 0, sizeof(*tc358749_hdmiin));
	tc358749_hdmiin->client = client;
	tc358749_hdmiin->dev = &client->dev;
	np = tc358749_hdmiin->dev->of_node;

	tc358749_hdmiin->io_power.gpio = of_get_named_gpio_flags(np,
								 "gpio-power",
								 0, &flags);
	if (!gpio_is_valid(tc358749_hdmiin->io_power.gpio)) {
		dev_err(tc358749_hdmiin->dev, "invalid hdmiin->io_power.gpio: %d\n",
			tc358749_hdmiin->io_power.gpio);
		return -EINVAL;
		}
	ret = devm_gpio_request(tc358749_hdmiin->dev,
				tc358749_hdmiin->io_power.gpio,
				"hdmiin-power-io");
	if (ret != 0) {
		dev_err(tc358749_hdmiin->dev,
			"gpio_request hdmiin->io_power.gpio invalid: %d\n",
			tc358749_hdmiin->io_power.gpio);
		return ret;
		}
	tc358749_hdmiin->io_power.active = (flags & OF_GPIO_ACTIVE_LOW);
	gpio_direction_output(tc358749_hdmiin->io_power.gpio,
			      !(tc358749_hdmiin->io_power.active));

	tc358749_hdmiin->io_stanby.gpio = of_get_named_gpio_flags(np,
								  "gpio-stanby",
								  0, &flags);
	if (!gpio_is_valid(tc358749_hdmiin->io_stanby.gpio)) {
		dev_err(tc358749_hdmiin->dev,
			"invalid hdmiin->io_stanby.gpio: %d\n",
			tc358749_hdmiin->io_stanby.gpio);
		return -EINVAL;
		}
	ret = devm_gpio_request(tc358749_hdmiin->dev,
				tc358749_hdmiin->io_stanby.gpio,
				"hdmiin-stanby-io");
	if (ret != 0) {
		dev_err(tc358749_hdmiin->dev,
			"gpio_request hdmiin->io_stanby.gpio invalid: %d\n",
			tc358749_hdmiin->io_stanby.gpio);
		return ret;
		}
	tc358749_hdmiin->io_stanby.active = (flags & OF_GPIO_ACTIVE_LOW);
	gpio_direction_output(tc358749_hdmiin->io_stanby.gpio,
			      !(tc358749_hdmiin->io_stanby.active));

	tc358749_hdmiin->io_reset.gpio = of_get_named_gpio_flags(np,
								 "gpio-reset",
								 0, &flags);
	if (!gpio_is_valid(tc358749_hdmiin->io_reset.gpio)) {
		dev_err(tc358749_hdmiin->dev,
			"invalid hdmiin->io_reset.gpio: %d\n",
			tc358749_hdmiin->io_reset.gpio);
		return -EINVAL;
		}
	ret = devm_gpio_request(tc358749_hdmiin->dev,
				tc358749_hdmiin->io_reset.gpio,
				"hdmiin-reset-io");
	if (ret != 0) {
		dev_err(tc358749_hdmiin->dev,
			"gpio_request hdmiin->io_reset.gpio invalid: %d\n",
			tc358749_hdmiin->io_reset.gpio);
		return ret;
		}
	tc358749_hdmiin->io_reset.active = (flags & OF_GPIO_ACTIVE_LOW);
	gpio_direction_output(tc358749_hdmiin->io_reset.gpio,
			      !(tc358749_hdmiin->io_reset.active));

	tc358749_hdmiin->io_int.gpio = of_get_named_gpio_flags(np, "gpio-int",
								0, &flags);
	if (!gpio_is_valid(tc358749_hdmiin->io_int.gpio)) {
		dev_err(tc358749_hdmiin->dev,
			"invalid hdmiin->io_int.gpio: %d\n",
			tc358749_hdmiin->io_int.gpio);
		return -EINVAL;
		}
	ret = devm_gpio_request(tc358749_hdmiin->dev,
				tc358749_hdmiin->io_int.gpio,
				"hdmiin-interrupt-io");
	if (ret != 0) {
		dev_err(tc358749_hdmiin->dev,
			"gpio_request hdmiin->io_int.gpio invalid: %d\n",
			tc358749_hdmiin->io_int.gpio);
		return ret;
		}
	gpio_direction_input(tc358749_hdmiin->io_int.gpio);

	for (i = 0; tc358749_reg_def[i].reg != 0; i++) {
		tc_write_reg(&tc358749_reg_def[i]);
		if (0x22 == tc358749_reg_def[i].reg)
			mdelay(1);
	}

	/*
	for (i = 0; tc358749_reg_def[i].reg != 0; i++) {
		pr_info("0x%x, 0x%x\n", tc358749_reg_def[i].reg, tc_read_reg(&tc358749_reg_def[i]));
	}
	*/
	tc358749_hdmiin->reg_class = class_create(THIS_MODULE, "hdmiin_reg");
	if(IS_ERR(tc358749_hdmiin->reg_class))
		pr_err("Failed to create hdmiin class!.\n");
	ret = class_create_file(tc358749_hdmiin->reg_class, &class_attr_hdmiin);
	ret = class_create_file(tc358749_hdmiin->reg_class, &class_attr_hdmiin_audio);

	kthread_run(tc358749_hdmiin_thread, NULL, "hdmi-in");
	pr_info("%s OK!\n", __func__);
	return 0;
}

static int  tc358749_hdmiin_remove(struct i2c_client *client)
{
	class_remove_file(tc358749_hdmiin->reg_class, &class_attr_hdmiin);
	class_destroy(tc358749_hdmiin->reg_class);
	tc358749_hdmiin->reg_class = NULL;

	return 0;
}

static const struct i2c_device_id id_table[] = {
	{"tc358749_hdmiin", 0 },
	{ }
};

static struct of_device_id tc358749_dt_ids[] = {
	{ .compatible = "rockchip,tc358749" },
	{ }
};

static struct i2c_driver tc358749_hdmiin_driver  = {
	.probe	= &tc358749_hdmiin_probe,
	.remove = &tc358749_hdmiin_remove,
	.driver = {
		.name  = "tc358749_hdmiin",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(tc358749_dt_ids),
	},
	.id_table	= id_table,
};


static int __init tc358749_hdmiin_init(void)
{
	return i2c_add_driver(&tc358749_hdmiin_driver);
}

static void __exit tc358749_hdmiin_exit(void)
{
	i2c_del_driver(&tc358749_hdmiin_driver);
}

fs_initcall(tc358749_hdmiin_init);
module_exit(tc358749_hdmiin_exit);

MODULE_DESCRIPTION("ROCKCHIP TC358749 HDMI-IN");
MODULE_AUTHOR("Rock-chips, <www.rock-chips.com>");
