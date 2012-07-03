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
/**
 * @file fntmgr.c
 * @brief font manager interface
 * @author Gabriel Liao
 */

#include <string.h>
#include <mach/typedef.h>

#include "SDL_ttf.h"
#include "fntMgr.h"
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#if 0
#define DEBUG printf
#else
#define DEBUG(...)
#endif

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define LIST_MAX_FONTS 5

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void releaseFontFromList(font_node_t *pFontNode);
static void cacheFontToList(font_node_t *pFontNode);
TTF_Font* fntMgrOpenFont(char *typeface, int fontSize);
void fntMgrCloseFont(char *typeface, int fontSize);
int fntMgrInit(void);
int fntMgrRelease(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static LIST_HEAD(fontList);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void
releaseFontFromList(
	font_node_t *pFontNode
)
{
	font_node_t *f;

	/* remove the least reference and oldest font node */
	if (!pFontNode) {
		list_for_each_entry(f, &fontList, list) {
			if(!pFontNode || pFontNode->ref >= f->ref) {
				pFontNode = f;
			}
		}
	}
	
	list_del(&pFontNode->list);
	TTF_CloseFont(pFontNode->hFont);
	DEBUG("---- remove cached font: name=%s, size=%d, ref=%d\n", pFontNode->name, pFontNode->size, pFontNode->ref);

	free(pFontNode);
}

static void
cacheFontToList(
	font_node_t *pFontNode
)
{
	font_node_t *f;
	int count = 0;

	list_for_each_entry(f, &fontList, list) {
		count++;
	}

	if (count >= LIST_MAX_FONTS) {
		releaseFontFromList(NULL);
	}
	
	list_add(&pFontNode->list, &fontList);
}

static TTF_Font*
openCacheFont(
	char *typeface,
	int fontSize
)
{
	font_node_t *f;

	list_for_each_entry(f, &fontList, list) {
		if (f->name && !strcmp(typeface, f->name)) {
			if (f->size && f->size == fontSize) {
				f->ref++;
				
				DEBUG("**** find cache font: %s, %d\n", typeface, fontSize);
				return f->hFont;
			}
		}
	}

	return NULL;
}

TTF_Font*
fntMgrOpenFont(
	char *typeface,
	int fontSize
)
{
	TTF_Font *font;
	font_node_t *pFontNode;
	
	font = openCacheFont(typeface, fontSize);

	if(!font) {
		font = TTF_OpenFont(typeface, fontSize);
		if (font) {
			pFontNode = (font_node_t*) malloc(sizeof(font_node_t));
			if (pFontNode == NULL) {
				DEBUG("allocate memory faile. No cache font\n");
				return font;
			}

			pFontNode->hFont = font;
			pFontNode->size = fontSize;
			pFontNode->ref = 0;
			strcpy(pFontNode->name, typeface);

			cacheFontToList(pFontNode);
			DEBUG("++++ cache font: %s, %d\n", typeface, fontSize);
		}
	}

	return font;
}

void
fntMgrCloseFont(
	char *typeface,
	int fontSize
)
{
	font_node_t *f;

	list_for_each_entry(f, &fontList, list) {
		if (!strcmp(typeface, f->name) && fontSize == f->size ) {
			(f->ref > 0) ? f->ref-- : 0;
			break;
		}
	}
}

int
fntMgrInit(
	void
)
{
	return 0;
}

int
fntMgrRelease(
	void
)
{
	font_node_t *f;

	list_for_each_entry(f, &fontList, list) {
		if(f) {
			list_del(&f->list);
			TTF_CloseFont(f->hFont);
			DEBUG("---- remove cached font: name=%s, size=%d, ref=%d\n", f->name, f->size, f->ref);

			free(f);
		}
	}

	return 0;
}
