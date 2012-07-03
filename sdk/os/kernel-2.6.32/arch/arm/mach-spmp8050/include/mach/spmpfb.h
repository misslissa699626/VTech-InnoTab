#include <asm/types.h>


/* LCD description */
struct spmp_display {
	const char	*name;
	/* LCD type */
	unsigned type;

	/* Screen size */
	unsigned short width;
	unsigned short height;

	/* Screen info */
	unsigned short xres;
	unsigned short yres;
	unsigned short bpp;
	
	int		hsw;		/* Horizontal synchronization pulse width */
	int		hfp;		/* Horizontal front porch */
	int		hbp;		/* Horizontal back porch */
	int		vsw;		/* Vertical synchronization pulse width */
	int		vfp;		/* Vertical front porch */
	int		vbp;		/* Vertical back porch */
	int     dataseq;    /* Data Sequence */
	int     fb_attrib;
	int     transport;  
	int     mWorkFreq;	
	unsigned int currentbl_level;
	int		(*enable)	        (struct spmp_display *panel);
	void	(*disable)	        (struct spmp_display *panel);
	int     (*set_bklight_level)(struct spmp_display *panel,unsigned int level);
	unsigned int (*get_bklight_level)(struct spmp_display *panel);
	unsigned int (*get_bklight_max)  (struct spmp_display *panel);
};


int spmp_regdev_lcdc(void);