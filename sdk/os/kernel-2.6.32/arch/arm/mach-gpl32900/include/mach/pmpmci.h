
#define S3CMCI_DMA 0

enum pmpmci_waitfor {
	COMPLETION_NONE,
	COMPLETION_FINALIZE,
	COMPLETION_CMDSENT,
	COMPLETION_RSPFIN,
	COMPLETION_XFERFINISH,
	COMPLETION_XFERFINISH_RSPFIN,
};

struct pmp_mci_pdata {
	unsigned int	wprotect_invert : 1;
	unsigned int	detect_invert : 1;   /* set => detect active high. */

	unsigned int	gpio_detect;
	unsigned int	gpio_wprotect;
	unsigned long	ocr_avail;
	void		(*set_power)(unsigned char power_mode,
				     unsigned short vdd);
};

struct pmpmci_host {
	struct platform_device	*pdev;
	struct s3c24xx_mci_pdata *pdata;
	struct mmc_host		*mmc;
	struct resource		*mem;
	struct clk		*clk;
	void __iomem		*base;
	int			irq;
	int			irq_cd;
	int			dma;

	unsigned long		clk_rate;
	unsigned long		clk_div;
	unsigned long		real_rate;
	u8				prescaler;

	unsigned		sdiimsk;
	unsigned		sdiwdata;	
	unsigned		sdirdata;
	int			dodma;
    unsigned char  ucCardVersion;	
    unsigned char  g_uiIsSD_20_SDA;			
	int			dmatogo;

	struct mmc_request	*mrq;
	int			cmd_is_stop;

	spinlock_t		complete_lock;
	enum s3cmci_waitfor	complete_what;

	int			dma_complete;

	u32			pio_sgptr;
	u32			pio_bytes;
	u32			pio_count;
	u32			*pio_ptr;
#define XFER_NONE 0
#define XFER_READ 1
#define XFER_WRITE 2
	u32			pio_active;

	int			bus_width;

	char 			dbgmsg_cmd[301];
	char 			dbgmsg_dat[301];
	char			*status;

	unsigned int		ccnt, dcnt;
	struct tasklet_struct	pio_tasklet;

#ifdef CONFIG_CPU_FREQ
	struct notifier_block	freq_transition;
#endif
};
