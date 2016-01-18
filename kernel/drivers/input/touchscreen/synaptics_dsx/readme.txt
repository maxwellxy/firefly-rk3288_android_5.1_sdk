	
1.platform data:
	static int synaptics_gpio_setup(int gpio, bool configure, int dir, int state);
	static unsigned char regulator_name[] = "";
	static unsigned char cap_button_codes[] = {KEY_MENU, KEY_HOME, KEY_BACK};

	static struct synaptics_dsx_cap_button_map cap_button_map = {
		.nbuttons = ARRAY_SIZE(cap_button_codes),
		.map = cap_button_codes,
	};

	static struct synaptics_dsx_board_data dsx_board_data = {
		.irq_gpio = (GPIO_TO_PIN(1, 16)),
		.irq_flags = IRQF_TRIGGER_FALLING,
		.power_gpio = -1,
		.power_on_state = 1,
		.power_delay_ms = 160,
		.reset_gpio = (GPIO_TO_PIN(1, 28)),
		.reset_on_state = 0,
		.reset_delay_ms = 100,
		.reset_active_ms = 20,
 		.gpio_config = synaptics_gpio_setup,
 		.regulator_name = regulator_name,
 		.cap_button_map = &cap_button_map,

	};

	I2C_BOARD_INFO(I2C_DRIVER_NAME, 0x20),//Synaptics_rmi4 S3402 IC
	.platform_data = &dsx_board_data,

2.把synaptics_dsx放入firefly_rk3288_mipi_linux/drivers/input/touchscreen/

3.firefly_rk3288_mipi_linux/drivers/input/touchscreen/：
		Makefile+：
			obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_DSX) += synaptics_dsx/
		Kconfig+：
			source "drivers/input/touchscreen/synaptics_dsx/Kconfig"

4.#make menuconfig:
		Synaptics DSX touchscreen -> 
			Synaptics DSX touchscreen bus interface (I2C)
			Synaptics DSX core driver module
	
5.在rk3288-tb.dts添加:
		&i2c1 {
        		status = "okay";
        		synaptics_ts@4b {
                		compatible = "synaptics,synaptics_dsx";
                		reg = <0x4b>;
                		touch-gpio = <&gpio8 GPIO_A7 IRQ_TYPE_EDGE_FALLING>;
                		reset-gpio = <&gpio8 GPIO_A6 GPIO_ACTIVE_LOW>;
                		//power-gpio = <&gpio0 GPIO_C5 GPIO_ACTIVE_LOW>;
               		 	max-x = <720>;
                		max-y = <1280>;
        		};
		};

6.已调通
