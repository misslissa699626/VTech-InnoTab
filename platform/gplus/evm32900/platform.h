#ifndef _PLATFORM_H_
#define _PLATFORM_H_

//#define MK_GPIO_INDEX(channel,func,gid,pin) (((channel)<<24)|((func)<<16)|((gid)<<8)|(pin))

/**************************************************************************
 * Panel power & Backlight
 **************************************************************************/
#define	panel_set_backlight_channel		0xff//GPIO_CHANNEL_0
#define	panel_set_backlight_func		0
#define	panel_set_backlight_gid			23
#define	panel_set_backlight_pin			6
#define	panel_set_backlight_level		1

#define	panel_brightness_backlight_id		1//0xff		/* No Used */
#define	panel_brightness_backlight_freq		300//hz
#define	panel_brightness_backlight_init_duty	15

#define	panel_set_powerOn0_start_delay	0	/*mS*/

#define	panel_set_powerOn0_0_channel	0xff
#define	panel_set_powerOn0_0_func		2
#define	panel_set_powerOn0_0_gid		23
#define	panel_set_powerOn0_0_pin		12
#define	panel_set_powerOn0_0_level		0

#define	panel_set_powerOn0_0to1_delay	10	/*mS*/

#define	panel_set_powerOn0_1_channel	0xff
#define	panel_set_powerOn0_1_func		2
#define	panel_set_powerOn0_1_gid		23
#define	panel_set_powerOn0_1_pin		12
#define	panel_set_powerOn0_1_level		0

#define	panel_set_powerOn0_1to2_delay	10	/*mS*/

#define	panel_set_powerOn0_2_channel	0xff
#define	panel_set_powerOn0_2_func		2
#define	panel_set_powerOn0_2_gid		23
#define	panel_set_powerOn0_2_pin		12
#define	panel_set_powerOn0_2_level		0

#define	panel_set_powerOn0_2to3_delay	10	/*mS*/

#define	panel_set_powerOn0_3_channel	0xff
#define	panel_set_powerOn0_3_func		2
#define	panel_set_powerOn0_3_gid		23
#define	panel_set_powerOn0_3_pin		12
#define	panel_set_powerOn0_3_level		0

#define	panel_set_powerOn0_end_delay	10	/*mS*/

#define	panel_set_powerOn1_start_delay	0	/*mS*/

#define	panel_set_powerOn1_0_channel	0xff
#define	panel_set_powerOn1_0_func		2
#define	panel_set_powerOn1_0_gid		23
#define	panel_set_powerOn1_0_pin		12
#define	panel_set_powerOn1_0_level		0

#define	panel_set_powerOn1_0to1_delay	10	/*mS*/

#define	panel_set_powerOn1_1_channel	0xff
#define	panel_set_powerOn1_1_func		2
#define	panel_set_powerOn1_1_gid		23
#define	panel_set_powerOn1_1_pin		12
#define	panel_set_powerOn1_1_level		0

#define	panel_set_powerOn1_1to2_delay	10	/*mS*/

#define	panel_set_powerOn1_2_channel	0xff
#define	panel_set_powerOn1_2_func		2
#define	panel_set_powerOn1_2_gid		23
#define	panel_set_powerOn1_2_pin		12
#define	panel_set_powerOn1_2_level		0

#define	panel_set_powerOn1_2to3_delay	10	/*mS*/

#define	panel_set_powerOn1_3_channel	0xff
#define	panel_set_powerOn1_3_func		2
#define	panel_set_powerOn1_3_gid		23
#define	panel_set_powerOn1_3_pin		12
#define	panel_set_powerOn1_3_level		0

#define	panel_set_powerOn1_end_delay	10	/*mS*/

#define	panel_set_powerOff0_start_delay	0	/*mS*/

#define	panel_set_powerOff0_0_channel	0xff
#define	panel_set_powerOff0_0_func		2
#define	panel_set_powerOff0_0_gid		23
#define	panel_set_powerOff0_0_pin		12
#define	panel_set_powerOff0_0_level		0

#define	panel_set_powerOff0_0to1_delay	10	/*mS*/

#define	panel_set_powerOff0_1_channel	0xff
#define	panel_set_powerOff0_1_func		2
#define	panel_set_powerOff0_1_gid		23
#define	panel_set_powerOff0_1_pin		12
#define	panel_set_powerOff0_1_level		0

#define	panel_set_powerOff0_1to2_delay	10	/*mS*/

#define	panel_set_powerOff0_2_channel	0xff
#define	panel_set_powerOff0_2_func		2
#define	panel_set_powerOff0_2_gid		23
#define	panel_set_powerOff0_2_pin		12
#define	panel_set_powerOff0_2_level		0

#define	panel_set_powerOff0_2to3_delay	10	/*mS*/

#define	panel_set_powerOff0_3_channel	0xff
#define	panel_set_powerOff0_3_func		2
#define	panel_set_powerOff0_3_gid		23
#define	panel_set_powerOff0_3_pin		12
#define	panel_set_powerOff0_3_level		0

#define	panel_set_powerOff0_end_delay	10	/*mS*/

#define	panel_set_powerOff1_start_delay	0	/*mS*/

#define	panel_set_powerOff1_0_channel	0xff
#define	panel_set_powerOff1_0_func		2
#define	panel_set_powerOff1_0_gid		23
#define	panel_set_powerOff1_0_pin		12
#define	panel_set_powerOff1_0_level		0

#define	panel_set_powerOff1_0to1_delay	10	/*mS*/

#define	panel_set_powerOff1_1_channel	0xff
#define	panel_set_powerOff1_1_func		2
#define	panel_set_powerOff1_1_gid		23
#define	panel_set_powerOff1_1_pin		12
#define	panel_set_powerOff1_1_level		0

#define	panel_set_powerOff1_1to2_delay	10	/*mS*/

#define	panel_set_powerOff1_2_channel	0xff
#define	panel_set_powerOff1_2_func		2
#define	panel_set_powerOff1_2_gid		23
#define	panel_set_powerOff1_2_pin		12
#define	panel_set_powerOff1_2_level		0

#define	panel_set_powerOff1_2to3_delay	10	/*mS*/

#define	panel_set_powerOff1_3_channel	0xff
#define	panel_set_powerOff1_3_func		2
#define	panel_set_powerOff1_3_gid		23
#define	panel_set_powerOff1_3_pin		12
#define	panel_set_powerOff1_3_level		0

#define	panel_set_powerOff1_end_delay	10	/*mS*/

#define	panel_set_spi_cs_channel	0xff
#define	panel_set_spi_cs_func		2
#define	panel_set_spi_cs_gid		23
#define	panel_set_spi_cs_pin		12

#define	panel_set_spi_scl_channel	0xff
#define	panel_set_spi_scl_func		2
#define	panel_set_spi_scl_gid		23
#define	panel_set_spi_scl_pin		12

#define	panel_set_spi_sda_channel	0xff
#define	panel_set_spi_sda_func		2
#define	panel_set_spi_sda_gid		23
#define	panel_set_spi_sda_pin		12

#define	panel_set_mirror0_channel	0xff
#define	panel_set_mirror0_func		2
#define	panel_set_mirror0_gid		23
#define	panel_set_mirror0_pin		12

#define	panel_set_mirror1_channel	0xff
#define	panel_set_mirror1_func		2
#define	panel_set_mirror1_gid		23
#define	panel_set_mirror1_pin		12

#define	panel_set_mirror_normal0	1
#define	panel_set_mirror_normal1	1
#define	panel_set_mirror_mirror0	0
#define	panel_set_mirror_mirror1	0

/**************************************************************************
 * SD
 **************************************************************************/

#define	sd0_clk_channel			0x0//IOA23
#define	sd0_clk_func			0
#define	sd0_clk_gid				29
#define	sd0_clk_pin				23
#define	sd0_clk_level			0
#define	sd0_clk_pull_level		GPIO_PULL_HIGH
#define	sd0_clk_driving			1

#define	sd0_cmd_channel			0x0//IOA24
#define	sd0_cmd_func			0
#define	sd0_cmd_gid				30
#define	sd0_cmd_pin				24
#define	sd0_cmd_level			0
#define	sd0_cmd_pull_level		GPIO_PULL_HIGH
#define	sd0_cmd_driving		1

#define	sd0_data0_channel		0x0//IOA25
#define	sd0_data0_func			0
#define	sd0_data0_gid			30
#define	sd0_data0_pin			25
#define	sd0_data0_level			0
#define	sd0_data0_pull_level	GPIO_PULL_HIGH
#define	sd0_data0_driving		1

#define	sd0_data1_channel		0x0//IOA26
#define	sd0_data1_func			0
#define	sd0_data1_gid			34
#define	sd0_data1_pin			26
#define	sd0_data1_level			0
#define	sd0_data1_pull_level	GPIO_PULL_HIGH
#define	sd0_data1_driving		1

#define	sd0_data2_channel		0x0//IOA27
#define	sd0_data2_func			0
#define	sd0_data2_gid			34
#define	sd0_data2_pin			27
#define	sd0_data2_level			0
#define	sd0_data2_pull_level	GPIO_PULL_HIGH
#define	sd0_data2_driving		1

#define	sd0_data3_channel		0x0//IOA28
#define	sd0_data3_func			0
#define	sd0_data3_gid			34
#define	sd0_data3_pin			28
#define	sd0_data3_level			0
#define	sd0_data3_pull_level	GPIO_PULL_HIGH
#define	sd0_data3_driving		1
 
#define	sd0_set_power_channel		0xff
#define	sd0_set_power_func			2
#define	sd0_set_power_gid			23
#define	sd0_set_power_pin			12
#define	sd0_set_power_level			0
#define	sd0_set_power_driving		0

#define	sd0_detect_channel			0//0//0xff//0xff//GPIO_CHANNEL_0//IOA1
#define	sd0_detect_func			0//	0
#define	sd0_detect_gid			19//	18//19//
#define	sd0_detect_pin			1//	0//1//0
#define	sd0_detect_level			0//0
#define	sd0_detect_pull_level		GPIO_PULL_HIGH
#define	sd0_detect_driving			0

#define	sd0_is_write_protected_channel		0x0//ioa0
#define	sd0_is_write_protected_func		0//2
#define	sd0_is_write_protected_gid		18//23
#define	sd0_is_write_protected_pin		0//12
#define	sd0_is_write_protected_level	GPIO_PULL_HIGH
#define	sd0_is_write_protected_pull_level	GPIO_PULL_HIGH
#define	sd0_is_write_protected_driving		0

#define	sdio0_set_standby_channel	0xff		/* No Used */
#define	sdio0_set_standby_func		2
#define	sdio0_set_standby_gid		30
#define	sdio0_set_standby_pin		24
#define	sdio0_set_standby_level		0

#define	sd1_clk_channel			0xff
#define	sd1_clk_func			2
#define	sd1_clk_gid				20
#define	sd1_clk_pin				2
#define	sd1_clk_level			0
#define	sd1_clk_pull_level		GPIO_PULL_HIGH
#define	sd1_clk_driving			0

#define	sd1_cmd_channel			0xff
#define	sd1_cmd_func			2
#define	sd1_cmd_gid				21
#define	sd1_cmd_pin				3
#define	sd1_cmd_level			0
#define	sd1_cmd_pull_level		GPIO_PULL_HIGH
#define	sd1_cmd_driving			0

#define	sd1_data0_channel		0xff
#define	sd1_data0_func			2
#define	sd1_data0_gid			22
#define	sd1_data0_pin			4
#define	sd1_data0_level			0
#define	sd1_data0_pull_level	GPIO_PULL_HIGH
#define	sd1_data0_driving		0

#define	sd1_data1_channel		0xff
#define	sd1_data1_func			2
#define	sd1_data1_gid			22
#define	sd1_data1_pin			5
#define	sd1_data1_level			0
#define	sd1_data1_pull_level	GPIO_PULL_HIGH
#define	sd1_data1_driving		0

#define	sd1_data2_channel		0xff
#define	sd1_data2_func			2
#define	sd1_data2_gid			23
#define	sd1_data2_pin			6
#define	sd1_data2_level			0
#define	sd1_data2_pull_level	GPIO_PULL_HIGH
#define	sd1_data2_driving		0

#define	sd1_data3_channel		0xff
#define	sd1_data3_func			2
#define	sd1_data3_gid			24
#define	sd1_data3_pin			7
#define	sd1_data3_level			0
#define	sd1_data3_pull_level	GPIO_PULL_HIGH
#define	sd1_data3_driving		0

#define	sd1_set_power_channel	0xff
#define	sd1_set_power_func		2
#define	sd1_set_power_gid		23
#define	sd1_set_power_pin		12
#define	sd1_set_power_level		0
#define	sd1_set_power_driving	0

#define	sd1_detect_channel		0xff
#define	sd1_detect_func			0
#define	sd1_detect_gid			19
#define	sd1_detect_pin			1
#define	sd1_detect_level		0
#define	sd1_detect_pull_level	GPIO_PULL_HIGH
#define	sd1_detect_driving		0

#define	sd1_is_write_protected_channel		0xff
#define	sd1_is_write_protected_func			2
#define	sd1_is_write_protected_gid			23
#define	sd1_is_write_protected_pin			12
#define	sd1_is_write_protected_level		0
#define	sd1_is_write_protected_pull_level	GPIO_PULL_HIGH
#define	sd1_is_write_protected_driving		0

#define	sdio1_set_standby_channel	0xff		/* B_KSD_CARD1, WIFI_DISABLE_L */
#define	sdio1_set_standby_func		2
#define	sdio1_set_standby_gid		30
#define	sdio1_set_standby_pin		24
#define	sdio1_set_standby_level		0

/**************************************************************************
 * MS
 **************************************************************************/
#define	ms_clk_channel			0x0
#define	ms_clk_func			    1
#define	ms_clk_gid				29
#define	ms_clk_pin				23
#define	ms_clk_level			0
#define	ms_clk_pull_level		GPIO_PULL_HIGH
#define	ms_clk_driving			0

#define	ms_bs_channel			0x0
#define	ms_bs_func			    1
#define	ms_bs_gid				30
#define	ms_bs_pin				24
#define	ms_bs_level			    0
#define	ms_bs_pull_level		GPIO_PULL_HIGH
#define	ms_bs_driving			0

#define	ms_data0_channel		0x0
#define	ms_data0_func			1
#define	ms_data0_gid			30
#define	ms_data0_pin			25
#define	ms_data0_level			0
#define	ms_data0_pull_level	    GPIO_PULL_HIGH
#define	ms_data0_driving		0

#define	ms_data1_channel		0x0
#define	ms_data1_func			1
#define	ms_data1_gid			34
#define	ms_data1_pin			28
#define	ms_data1_level			0
#define	ms_data1_pull_level	    GPIO_PULL_HIGH
#define	ms_data1_driving		0

#define	ms_data2_channel		0x0
#define	ms_data2_func			1
#define	ms_data2_gid			34
#define	ms_data2_pin			27
#define	ms_data2_level			0
#define	ms_data2_pull_level	    GPIO_PULL_HIGH
#define	ms_data2_driving		0

#define	ms_data3_channel		0x0
#define	ms_data3_func			1
#define	ms_data3_gid			34
#define	ms_data3_pin			26
#define	ms_data3_level			0
#define	ms_data3_pull_level	    GPIO_PULL_HIGH
#define	ms_data3_driving		0

#define	ms_set_power_channel	0xff
#define	ms_set_power_func		2
#define	ms_set_power_gid		23
#define	ms_set_power_pin		12
#define	ms_set_power_level		0

#define	ms_detect_channel	0xff
#define	ms_detect_func		0
#define	ms_detect_gid		GPIO_GID_UNKOWN
#define	ms_detect_pin		24
#define	ms_detect_level		0

/**************************************************************************
 * NAND - CSn & WP
 **************************************************************************/
#define	nand_set_cs0_channel	0xff
#define	nand_set_cs0_func		2
#define	nand_set_cs0_gid		23
#define	nand_set_cs0_pin		12
#define	nand_set_cs0_active		0

#define	nand_set_cs1_channel	0xff
#define	nand_set_cs1_func		2
#define	nand_set_cs1_gid		23
#define	nand_set_cs1_pin		12
#define	nand_set_cs1_active		0

#define	nand_set_cs2_channel	0xff
#define	nand_set_cs2_func		2
#define	nand_set_cs2_gid		23
#define	nand_set_cs2_pin		12
#define	nand_set_cs2_active		0

#define	nand_set_cs3_channel	0xff
#define	nand_set_cs3_func		2
#define	nand_set_cs3_gid		23
#define	nand_set_cs3_pin		12
#define	nand_set_cs3_active		0

#define	nand_set_wp_channel	0x0      //IOA6
#define	nand_set_wp_func		0
#define	nand_set_wp_gid		23
#define	nand_set_wp_pin		6
#define	nand_set_wp_active		1

#define	nand1_set_wp_channel		0x0//IOA5 nand card
#define	nand1_set_wp_func		0
#define	nand1_set_wp_gid		22
#define	nand1_set_wp_pin		5
#define	nand1_set_wp_active		1

/**************************************************************************
 * USB HOST / SLAVE
 **************************************************************************/ 
#define	usb_host_en_power_channel		0xff
#define	usb_host_en_power_func		2
#define	usb_host_en_power_gid			23
#define	usb_host_en_power_pin			12
#define	usb_host_en_power_active		1

#define	usb_switch_bus_channel	0xff
#define	usb_switch_bus_func		2
#define	usb_switch_bus_gid		23
#define	usb_switch_bus_pin		12
#define	usb_switch_bus_active		1

#define	usb_power_good_channel	0xff	/* No Used */
#define	usb_power_good_func		2
#define	usb_power_good_gid		23
#define	usb_power_good_pin		12
#define	usb_power_good_level		1

#define        usb_slave_detect_config USB_SLAVE_VBUS_GPIO//USB_SLAVE_VBUS_POWERON1 //USB_SLAVE_VBUS_GPIO//USB_SLAVE_VBUS_POWERON1 /*0x0 POWERON1, 0x1 GPIO, 0xff NONE*/IOC1 
#define	usb_slave_detect_power_channel		2//GPIO_CHANNEL_2
#define	usb_slave_detect_power_func			2
#define	usb_slave_detect_power_gid				4
#define	usb_slave_detect_power_pin				1

/**************************************************************************
 * Audio out
 **************************************************************************/
#define	headphone_detect_channel	0x0//0xff //Modified by Kevin, IOA13, for Vtech use
#define	headphone_detect_func		2
#define	headphone_detect_gid		2//23
#define	headphone_detect_pin		13//12
#define	headphone_detect_level		0

#define	speaker_set_power_channel	0xff
#define	speaker_set_power_func		2
#define	speaker_set_power_gid		23
#define	speaker_set_power_pin		12
#define	speaker_set_power_active	1

/**************************************************************************
 * Sensor port
 **************************************************************************/ 
#define sensor_port0_mclk_channel	0xff
#define sensor_port0_mclk_func		4
#define sensor_port0_mclk_gid		0
#define sensor_port0_mclk_pin		10
#define sensor_port0_mclk_level		0

#define sensor_port0_pclk_channel	0xff
#define sensor_port0_pclk_func		1
#define sensor_port0_pclk_gid		0
#define sensor_port0_pclk_pin		11
#define sensor_port0_pclk_level		0

#define sensor_port0_vsync_channel	0xff
#define sensor_port0_vsync_func		1
#define sensor_port0_vsync_gid		1
#define sensor_port0_vsync_pin		12
#define sensor_port0_vsync_level	0

#define sensor_port0_hsync_channel	0xff
#define sensor_port0_hsync_func		1
#define sensor_port0_hsync_gid		2
#define sensor_port0_hsync_pin		13
#define sensor_port0_hsync_level	0

#define sensor_port0_data_channel	0xff//0x2
#define sensor_port0_data_func		0
#define sensor_port0_data_gid		4
#define sensor_port0_data_startpin	0
#define sensor_port0_data_level		0

#define sensor_port1_mclk_channel	0xff
#define sensor_port1_mclk_func		1
#define sensor_port1_mclk_gid		0
#define sensor_port1_mclk_pin		10
#define sensor_port1_mclk_level		0

#define sensor_port1_pclk_channel	0xff
#define sensor_port1_pclk_func		1
#define sensor_port1_pclk_gid		0
#define sensor_port1_pclk_pin		11
#define sensor_port1_pclk_level		0

#define sensor_port1_vsync_channel	0xff
#define sensor_port1_vsync_func		1
#define sensor_port1_vsync_gid		1
#define sensor_port1_vsync_pin		12
#define sensor_port1_vsync_level	0

#define sensor_port1_hsync_channel	0xff
#define sensor_port1_hsync_func		1
#define sensor_port1_hsync_gid		2
#define sensor_port1_hsync_pin		13
#define sensor_port1_hsync_level	0

#define sensor_port1_data_channel	0xff
#define sensor_port1_data_func		0
#define sensor_port1_data_gid		4
#define sensor_port1_data_startpin	0
#define sensor_port1_data_level		0

#define sensor_port_cus_data_channel	0x01//IOB0 ~ IOB7
#define sensor_port_cus_data_func		0
#define sensor_port_cus_data_gid		8
#define sensor_port_cus_data_startpin	0
#define sensor_port_cus_data_level		0
/**************************************************************************
 * Sensor reset/standby/power
 **************************************************************************/ 
#define	sensor0_set_reset_channel	0 //IOA29
#define	sensor0_set_reset_func		2
#define	sensor0_set_reset_gid		31
#define	sensor0_set_reset_pin		29
#define	sensor0_set_reset_active	    1

#define	sensor0_set_standby_channel	0 //IOA30
#define	sensor0_set_standby_func	    2
#define	sensor0_set_standby_gid		32
#define	sensor0_set_standby_pin		30
#define	sensor0_set_standby_active	1

#define	sensor0_set_power_channel	0//IOA12
#define	sensor0_set_power_func		2
#define	sensor0_set_power_gid		1
#define	sensor0_set_power_pin		12
#define	sensor0_set_power_active	1

#define	sensor1_set_reset_channel	0xff
#define	sensor1_set_reset_func		2
#define	sensor1_set_reset_gid		23
#define	sensor1_set_reset_pin		12
#define	sensor1_set_reset_active	0

#define	sensor1_set_standby_channel	0xff
#define	sensor1_set_standby_func	2
#define	sensor1_set_standby_gid		23
#define	sensor1_set_standby_pin		12
#define	sensor1_set_standby_active	1

#define	sensor1_set_power_channel	0xff
#define	sensor1_set_power_func		2
#define	sensor1_set_power_gid		23
#define	sensor1_set_power_pin		12
#define	sensor1_set_power_active	1

#define	sensor2_set_reset_channel	0xff
#define	sensor2_set_reset_func		2
#define	sensor2_set_reset_gid		23
#define	sensor2_set_reset_pin		12
#define	sensor2_set_reset_active	0

#define	sensor2_set_standby_channel	0xff
#define	sensor2_set_standby_func	2
#define	sensor2_set_standby_gid		23
#define	sensor2_set_standby_pin		12
#define	sensor2_set_standby_active	1

#define	sensor2_set_power_channel	0xff
#define	sensor2_set_power_func		2
#define	sensor2_set_power_gid		23
#define	sensor2_set_power_pin		12
#define	sensor2_set_power_active	1

/**************************************************************************
 * Mipi
 **************************************************************************/ 
#define mipi_mclk_channel		0
#define mipi_mclk_func			1
#define mipi_mclk_gid			0
#define mipi_mclk_pin			10
#define mipi_mclk_level			0

#define mipi_clkn_channel		0xff
#define mipi_clkn_func			4
#define mipi_clkn_gid			0
#define mipi_clkn_pin			26
#define mipi_clkn_level			0

#define mipi_clkp_channel		0xff
#define mipi_clkp_func			4
#define mipi_clkp_gid			0
#define mipi_clkp_pin			27
#define mipi_clkp_level			0

#define mipi_data0n_channel		0xff
#define mipi_data0n_func		4
#define mipi_data0n_gid			0
#define mipi_data0n_pin			24
#define mipi_data0n_level		0

#define mipi_data0p_channel		0xff
#define mipi_data0p_func		4
#define mipi_data0p_gid			0
#define mipi_data0p_pin			25
#define mipi_data0p_level		0

#define mipi_data1n_channel		0xff
#define mipi_data1n_func		4
#define mipi_data1n_gid			0
#define mipi_data1n_pin			28
#define mipi_data1n_level		0

#define mipi_data1p_channel		0xff
#define mipi_data1p_func		4
#define mipi_data1p_gid			0
#define mipi_data1p_pin			29
#define mipi_data1p_level		0

/**************************************************************************
 * LED Control
 **************************************************************************/
#define	led_set_light0_channel	0xff
#define	led_set_light0_func		2
#define	led_set_light0_gid		23
#define	led_set_light0_pin		12
#define	led_set_light0_active	1

#define	led_set_light1_channel	0xff
#define	led_set_light1_func		2
#define	led_set_light1_gid		23
#define	led_set_light1_pin		12
#define	led_set_light1_active	1

#define	led_set_light2_channel	0xff
#define	led_set_light2_func		2
#define	led_set_light2_gid		23
#define	led_set_light2_pin		12
#define	led_set_light2_active	1

#define	led_set_light3_channel	0xff
#define	led_set_light3_func		2
#define	led_set_light3_gid		23
#define	led_set_light3_pin		12
#define	led_set_light3_active	1

#define	led_set_light4_channel	0xff
#define	led_set_light4_func		2
#define	led_set_light4_gid		23
#define	led_set_light4_pin		12
#define	led_set_light4_active	1

#define	led_set_light5_channel	0xff
#define	led_set_light5_func		2
#define	led_set_light5_gid		23
#define	led_set_light5_pin		12
#define	led_set_light5_active	1

#define	led_set_light6_channel	0xff
#define	led_set_light6_func		2
#define	led_set_light6_gid		23
#define	led_set_light6_pin		12
#define	led_set_light6_active	1

#define	led_set_light7_channel	0xff
#define	led_set_light7_func		2
#define	led_set_light7_gid		23
#define	led_set_light7_pin		12
#define	led_set_light7_active	1

#define	led_set_light8_channel	0xff
#define	led_set_light8_func		2
#define	led_set_light8_gid		23
#define	led_set_light8_pin		12
#define	led_set_light8_active	1

#define	led_set_light9_channel	0xff
#define	led_set_light9_func		2
#define	led_set_light9_gid		23
#define	led_set_light9_pin		12
#define	led_set_light9_active	1

#define	led_set_brightness_id0		0xff	/* No Used */
#define	led_set_brightness_freq0	10000
#define	led_set_brightness_init_duty0	0
#define	led_set_brightness_id1		0xff	/* No Used */
#define	led_set_brightness_freq1	10000
#define	led_set_brightness_init_duty1	0
#define	led_set_brightness_id2		0xff	/* No Used */
#define	led_set_brightness_freq2	10000
#define	led_set_brightness_init_duty2	0

/**************************************************************************
 * I2C
 **************************************************************************/
#define	i2c_sda_channel			0x1//IOB16
#define	i2c_sda_func			0
#define	i2c_sda_gid				13
#define	i2c_sda_pin				16
#define	i2c_scl_channel			0x1
#define	i2c_scl_func			0
#define	i2c_scl_gid				13
#define	i2c_scl_pin				17//IOB17

/**************************************************************************
 * angle switch
 **************************************************************************/
#define	angle_sw_detect_channel		0xff
#define	angle_sw_detect_func			2
#define	angle_sw_detect_gid				23
#define	angle_sw_detect_pin				12
#define	angle_sw_detect_normal_level	0
#define	angle_sw_detect_normal			0		/*degree*/
#define	angle_sw_detect_rot				90		/*degree*/

/**************************************************************************
 * PCB Version Detection
 **************************************************************************/


/**************************************************************************
 * Sytem Power status
 **************************************************************************/
#define	dc_in_detect_channel	0xff
#define	dc_in_detect_func		2
#define	dc_in_detect_gid		23
#define	dc_in_detect_pin		12
#define	dc_in_detect_level		0

#define	battery_voltage_detect_adc_ch	0xff

#define	battery_charger_status_channel	0xff
#define	battery_charger_status_func		2
#define	battery_charger_status_gid		23
#define	battery_charger_status_pin		12
#define	battery_charger_status_level		0

/**************************************************************************
 * PWM channel
 **************************************************************************/
#define	G32900_pwm1_channel		0//IOA3
#define	G32900_pwm1_func		5
#define	G32900_pwm1_gid			21
#define	G32900_pwm1_pin			3

#define	G32900_pwm2_channel		0xff//1
#define	G32900_pwm2_func		6
#define	G32900_pwm2_gid			13
#define	G32900_pwm2_pin			16

/**************************************************************************
 * ps2_uart_mouse pin sel
 **************************************************************************/
#define	ps2mouse_set_clk_channel		0xff
#define	ps2mouse_set_clk_func		2
#define	ps2mouse_set_clk_gid			31
#define	ps2mouse_set_clk_pin			29

#define	ps2mouse_set_dat_channel		0xff
#define	ps2mouse_set_dat_func		2
#define	ps2mouse_set_dat_gid			32
#define	ps2mouse_set_dat_pin			30
#endif // _PLATFORM_H_
