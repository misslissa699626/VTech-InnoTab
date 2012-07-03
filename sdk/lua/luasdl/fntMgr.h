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
#ifndef _FONT_MGR_H_
#define _FONT_MGR_H_

#include "typedef.h"
#include <linux/list.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MAX_FONT_NAME 256

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({					\
															\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct font_node_s {
	struct list_head list;
	TTF_Font *hFont;
	char name[MAX_FONT_NAME];
	int size;
	int ref;
} font_node_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
TTF_Font* 
fntMgrOpenFont(
	char *typeface,
	int fontSize
);

void
fntMgrCloseFont(
	char *typeface,
	int fontSize
);

int
fntMgrInit(
	void
);

int
fntMgrRelease(
	void
);

#endif /* _FONT_MGR_H_ */
