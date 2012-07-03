#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <asm/io.h>

#include "support.InnoTab.h"

void innotab_endlessWarning( const char *str, int line, char *msg )
{
	if ( msg == 0 )
		while (1)
			printk( "MTD for InnoTab should not reach here: %s() - %d\n", str, line );
	else
		while (1)
			printk( "%s() - %d, Warning: %s\n", str, line, msg );
}
EXPORT_SYMBOL_GPL(innotab_endlessWarning);

void innotab_notice( const char *str, int line, char *msg )
{
	if ( msg == 0 )
		printk( TERMCOL_red"MTD for InnoTab reaches here: %s() - %d\n"TERMCOL_white, str, line );
	else
		printk( TERMCOL_red"%s() - %d: %s\n"TERMCOL_white, str, line, msg );
}
EXPORT_SYMBOL_GPL(innotab_notice);