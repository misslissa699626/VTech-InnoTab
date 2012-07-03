/**head file************************/

#ifndef _GP_GPIO_KEYS_H
#define _GP_GPIO_KEYS_H 

struct gp_gpio_keys_button {
	/* Configuration parameters */
	int code;		/* input event code (KEY_*, SW_*) */
	int gpio;		/*GPIO index*/
	int active_low;
	char *desc;
};

struct gp_gpio_keys_platform_data {
	struct gp_gpio_keys_button *buttons;
	int nr_buttons;
};

#endif



