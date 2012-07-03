/* 
*/

#include <linux/spinlock.h>

struct clkchain_s
{
    int cnt;
	char ** chainlist;
};
struct clk {
	struct list_head      list;
	struct module        *owner;
	struct clk           *parent;
	struct clkchain_s    *clkchain;
	const char			*name;
	int					id;
	int					usage;
	unsigned long		rate;
	unsigned long		ctrlbit;
    unsigned int		pValAddr;
	int					(*enable)(struct clk *, int enable);
	int					(*set_rate)(struct clk *c, unsigned long rate);
	unsigned long		(*get_rate)(struct clk *c);
	unsigned long		(*round_rate)(struct clk *c, unsigned long rate);
	int					(*set_parent)(struct clk *c, struct clk *parent);
};

extern spinlock_t clocks_lock;

extern struct clk clk_arm;
extern struct clk clk_arm_ahb;
extern struct clk clk_arm_apb;
extern struct clk clk_ceva;
extern struct clk clk_ceva_ahb;
extern struct clk clk_ceva_apb;
extern struct clk clk_sys;
extern struct clk clk_sys_ahb;
extern struct clk clk_sys_apb;

extern struct clk clk_lcd;
extern struct clk clk_uart;
extern struct clk clk_csi;
extern struct clk clk_i2s_mclk;
extern struct clk clk_i2s_bck;

extern int __init spmp_register_baseclocks(unsigned long xtal);
extern void __init spmp_setup_clocks(void);
extern int __init spmp_baseclk_add(void);
extern void __init dumpclk(void);
extern int spmp_register_clock(struct clk *clk);


//void clk_put(struct clk *clk);
//int clk_enable(struct clk *clk);
//void clk_disable(struct clk *clk);
//unsigned long clk_get_rate(struct clk *clk);
//long clk_round_rate(struct clk *clk, unsigned long rate);
//int clk_set_rate(struct clk *clk, unsigned long rate);
//struct clk *clk_get_parent(struct clk *clk);
//int clk_set_parent(struct clk *clk, struct clk *parent);
