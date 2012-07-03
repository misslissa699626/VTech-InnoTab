/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/

/*!
 * @file gp_board.h
 * @brief The configuration interface of board
 */
#ifndef _GP_BOARD_H_
#define _GP_BOARD_H_

#ifdef   __cplusplus
extern "C" {
#endif

#include <mach/typedef.h>
#ifdef __KERNEL__
#include <linux/fs.h>
#endif

#define GP_BOARD_COUNT_OF(a) (sizeof(a)/sizeof(a[0]))
#define GP_BOARD_ITEM(name, item) { (name), (void*)item, sizeof(*item) }

#define GP_BOARD_NO_WAIT		0
#define GP_BOARD_WAIT_FOREVER	0xFFFFFFFFUL


/* IOCTL */
#define GP_BOARD_MAGIC		'B'
#define IOCTL_BOARD_GET_CONFIG  _IOWR(GP_BOARD_MAGIC, 0x01, gp_board_remote_config_t)
#define IOCTL_BOARD_INVOKE      _IOWR(GP_BOARD_MAGIC, 0x02, gp_board_rpc_t)

#define GB_BOARD_RPC_MAX_PARAM 5

enum {
	GP_BOARD_INVOKE_VV,
	GP_BOARD_INVOKE_IV,
	GP_BOARD_INVOKE_VI,
	GP_BOARD_INVOKE_II,
	GP_BOARD_INVOKE_IU,
	GP_BOARD_INVOKE_IIU,
	GP_BOARD_INVOKE_III,
	GP_BOARD_INVOKE_IIII,
	GP_BOARD_INVOKE_IUUU,
	GP_BOARD_INVOKE_IIIII,
};

typedef struct gp_board_remote_config_s {
	char tag[16];
	int len;
	int len_out;
	void *config;
} gp_board_remote_config_t;

typedef struct gp_board_rpc_s {
	int type;
	void *func;
	int ret;
	int iparam[GB_BOARD_RPC_MAX_PARAM];
} gp_board_rpc_t;

// pin function
enum {
	GP_PIN_DISP0_LCD = 0,
	GP_PIN_SD0,
	GP_PIN_SD1,
	GP_PIN_PWM,
	GP_PIN_SPI,
	GP_PIN_MAX,
};

// board config
typedef struct gp_board_s {
	char name[32];
	int (*init)(void);
	void (*power_off)(void);
	void (*power_reset)(void);
	int (*test)(int param1, int param2);	
} gp_board_t;

// pin function config
typedef struct gp_board_pin_func_s {
	int32_t (*pin_func_request)(int32_t type, uint32_t timeout);
	void (*pin_func_release)(int32_t handle);
} gp_board_pin_func_t;

// panel config
typedef struct gp_board_panel_s {
	int (*set_backlight)(int enable);
	int (*set_brightness)(int duty);
	int (*set_panelpowerOn0)(int enable);
	int (*set_panelpowerOn1)(int enable);
	int (*set_panel_spi_cs)(UINT32 val );
	int (*set_panel_spi_scl)(UINT32 val );
	int (*set_panel_spi_sda)(UINT32 val );
	int (*set_panel_mirror)(int enable);
} gp_board_panel_t;

// sd config
typedef struct gp_board_sd_s {
	int (*set_power)(int enable);
	int (*detect)(void);
	int (*is_write_protected)(void);
	int (*set_standby)(int enable);	
	int (*get_func)(void);	
	int (*set_io)(int mode, int out_value);	
} gp_board_sd_t;

// ms config
typedef struct gp_board_ms_s {
	int (*set_power)(int enable);
	int (*detect)(void);
} gp_board_ms_t;

// nand config
typedef struct gp_board_nand_s {
	int (*set_cs)(	int csn, int status);
	int (*set_wp)(int enable);	
} gp_board_nand_t;


// usb config
typedef struct gp_board_usb_s {
	int (*phy0_func_en_get)(void);
	int (*phy1_func_sel_get)(void);
	int (*get_host_speed)(void);
	int (*set_power)(int enable);
	int (*slave_detect)(void);
	int (*otg_switch)(int mode);
	int (*get_host_pg)(void);
	int (*get_slave_vbus_config)(void);
} gp_board_usb_t;


// audio config
typedef struct gp_board_audio_s {
	int (*get_headphone_detect)(void);
	int (*set_speaker_power)(int enable);
} gp_board_audio_t;

// sensor config
typedef struct gp_board_sensor_s {
	int (*set_sensor_reset )(int enable);
	int (*set_sensor_standby)(int enable);
	int (*set_sensor_power)(int enable);
} gp_board_sensor_t;

// LED config
typedef struct gp_board_led_s {
	int (*set_led_light)( int type, int enable);
	int (*set_led_brightness)(	int channel, int enable,  int value);
} gp_board_led_t;

// angle_sw config
typedef struct gp_board_angle_sw_s {
	int (*detect_angle_sw)(void);
} gp_board_angle_sw_t;

// PWM config
typedef struct gp_board_pwm_s {
	int *channel;
	int count;
} gp_board_pwm_t;

// PCB config
typedef struct gp_board_pcb_s {
	int (*detect_pcb_version)(void);
} gp_board_pcb_t;

// System Power config
typedef struct gp_board_system_s {
	int (*detect_dc_in)(void);
	int (*detect_battery_voltage)(void);
	int (*detect_battery_charger)(void);
	int (*set_ssc)(	int sid, int srate, int shz, int en );
	int (*prepare)(void);
	void (*finish)(void);
} gp_board_system_t;

// register
typedef struct gp_board_item_s {
	char *tag;
	void *data;
	int len;
} gp_board_item_t;

#ifdef __KERNEL__
typedef struct gp_board_config_s {
	gp_board_item_t *items;
	int count;
	long (*ioctl)(struct file *filp, uint32_t cmd, unsigned long arg);
} gp_board_config_t;

extern void gp_board_register(const gp_board_config_t *config);
#endif

extern void *__gp_board_get_config(const char *tag, int len, int *len_out);

#define gp_board_get_config(tag, type) \
	((type *) __gp_board_get_config((tag), sizeof(type), 0))

extern int32_t gp_board_malloc_pin_conf_entry(int32_t type);
extern void gp_board_free_pin_conf_entry(int32_t type);
extern int32_t gp_board_pin_func_request(int32_t type,uint32_t timeout);
extern void gp_board_pin_func_release(uint32_t handle);

#ifdef   __cplusplus
}
#endif

#endif
