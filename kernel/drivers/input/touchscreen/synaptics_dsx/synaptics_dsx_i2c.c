/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

#include "synaptics_dsx.h"
#include "synaptics_dsx_core.h"

#define SYN_I2C_RETRY_TIMES 10
#define RMI4_I2C_SPEED		(400*1000)

static unsigned char cap_button_codes[] =
		{KEY_BACK, KEY_HOMEPAGE, KEY_MENU};

static struct synaptics_dsx_cap_button_map cap_button_map = {
	.nbuttons = ARRAY_SIZE(cap_button_codes),
	.map = cap_button_codes,
};

static int synaptics_rmi4_i2c_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr)
{
	int retval;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);

	page = ((addr >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				dev_err(rmi4_data->pdev->dev.parent,
						"%s: I2C retry %d\n",
						__func__, retry + 1);
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);
	struct i2c_msg msg[] = {
		{
			.scl_rate = RMI4_I2C_SPEED,
			.addr = i2c->addr,
			.flags = 0x00,
			.len = 1,
			.buf = &buf,
		},
		{
			.scl_rate = RMI4_I2C_SPEED,
			.addr = i2c->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	retval = synaptics_rmi4_i2c_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN) {
		retval = -EIO;
		goto exit;
	}

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(i2c->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	return retval;
}

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);
	struct i2c_msg msg[] = {
		{
			.scl_rate = RMI4_I2C_SPEED,
			.addr = i2c->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	retval = synaptics_rmi4_i2c_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN) {
		retval = -EIO;
		goto exit;
	}

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(i2c->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(rmi4_data->pdev->dev.parent,
				"%s: I2C write over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	return retval;
}

static struct synaptics_dsx_bus_access bus_access = {
	.type = BUS_I2C,
	.read = synaptics_rmi4_i2c_read,
	.write = synaptics_rmi4_i2c_write,
};

static struct synaptics_dsx_hw_interface hw_if;

static struct platform_device *synaptics_dsx_i2c_device;

static void synaptics_rmi4_i2c_dev_release(struct device *dev)
{
	kfree(synaptics_dsx_i2c_device);

	return;
}

static int synaptics_rmi4_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval;
	u32 val;
	struct device_node *np = client->dev.of_node;
	struct synaptics_dsx_board_data *board_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "%s: SMBus byte data commands not supported by host\n", __func__);
		return -EIO;
	}
	if (!np) {
		dev_err(&client->dev, "no device tree\n");
		return -EINVAL;
	}

	board_data = kzalloc(sizeof(struct synaptics_dsx_board_data), GFP_KERNEL);
	if (!board_data) {
		dev_err(&client->dev, "%s: no memory allocated\n", __func__);
		return -ENOMEM;
	}
	board_data->irq_gpio = of_get_named_gpio_flags(np, "touch-gpio", 0, (enum of_gpio_flags *)&board_data->irq_flags);

	board_data->reset_gpio = of_get_named_gpio_flags(np, "reset-gpio", 0, (enum of_gpio_flags*)&board_data->reset_on_state);
	if (gpio_is_valid(board_data->reset_gpio)) {
		retval = devm_gpio_request_one(&client->dev, board_data->reset_gpio, 
				(board_data->reset_on_state & OF_GPIO_ACTIVE_LOW) ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW, 
				"dsx-reset");
		if (retval != 0) {
			dev_err(&client->dev, "dsx gpio_request error\n");
			return -EIO;
		}
		gpio_direction_output(board_data->reset_gpio, 
				(board_data->reset_on_state & OF_GPIO_ACTIVE_LOW) ? 1 : 0);
		mdelay(20);
	} else {
		dev_info(&client->dev, "dsx reset pin invalid\n");
	}

	if (of_property_read_u32(np, "max-x", &val)) {
			dev_err(&client->dev, "no max-x defined\n");
			return -EINVAL;
	}
	board_data->panel_x = val;
	if (of_property_read_u32(np, "max-y", &val)) {
			dev_err(&client->dev, "no max-y defined\n");
			return -EINVAL;
	}
	board_data->panel_y = val;
	if (!of_property_read_u32(np, "flip-x", &val)) {
		board_data->x_flip= val;
	}
	if (!of_property_read_u32(np, "flip-y", &val)) {
		board_data->y_flip= val;
	}


	board_data->power_gpio = -1;
	board_data->power_on_state = 1;
	board_data->power_delay_ms = 160;
	board_data->reset_on_state = 0;
	board_data->reset_delay_ms = 100;
	board_data->reset_active_ms = 20;
	board_data->gpio_config = NULL;
	board_data->regulator_name = "";
	board_data->cap_button_map = &cap_button_map;

	synaptics_dsx_i2c_device = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
	if (!synaptics_dsx_i2c_device) {
		dev_err(&client->dev, "%s: Failed to allocate memory for synaptics_dsx_i2c_device\n", __func__);
		return -ENOMEM;
	}
	
	client->dev.platform_data = board_data;
	hw_if.board_data = client->dev.platform_data;
	hw_if.bus_access = &bus_access;

	synaptics_dsx_i2c_device->name = PLATFORM_DRIVER_NAME;
	synaptics_dsx_i2c_device->id = 0;
	synaptics_dsx_i2c_device->num_resources = 0;
	synaptics_dsx_i2c_device->dev.parent = &client->dev;
	synaptics_dsx_i2c_device->dev.platform_data = &hw_if;
	synaptics_dsx_i2c_device->dev.release = synaptics_rmi4_i2c_dev_release;

	retval = platform_device_register(synaptics_dsx_i2c_device);
	if (retval) {
		dev_err(&client->dev, "%s: Failed to register platform device\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static int synaptics_rmi4_i2c_remove(struct i2c_client *client)
{
	platform_device_unregister(synaptics_dsx_i2c_device);

	return 0;
}

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{I2C_DRIVER_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);

static struct of_device_id synaptics_rmi4_dt_ids[] = {
	{ .compatible = "synaptics,synaptics_dsx" },
	{ }
};

static struct i2c_driver synaptics_rmi4_i2c_driver = {
	.driver = {
		.name = I2C_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(synaptics_rmi4_dt_ids),
	},
	.probe = synaptics_rmi4_i2c_probe,
	.remove = synaptics_rmi4_i2c_remove,
	.id_table = synaptics_rmi4_id_table,
};

int synaptics_rmi4_bus_init(void)
{
	return i2c_add_driver(&synaptics_rmi4_i2c_driver);
}
EXPORT_SYMBOL(synaptics_rmi4_bus_init);

void synaptics_rmi4_bus_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_i2c_driver);
	return;
}
EXPORT_SYMBOL(synaptics_rmi4_bus_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX I2C Bus Support Module");
MODULE_LICENSE("GPL v2");
