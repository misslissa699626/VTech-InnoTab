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
 * @file showlogo.c
 * @brief Show logo
 * @author Anson Chuang
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <mach/gp_board.h>
#include "mach/typedef.h"
#include "mach/gp_chunkmem.h"
#include "config/sysconfig.h"
#include "diag.h"
#include "mach/gp_display.h"
#include "mach/gp_display1.h"


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define LOGO_DIR "/etc/logo/"

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	printf
#else
	#define DEBUG(...)
#endif

#define DISPLAY_COUNT 3

#define DEFAULT_SLEEP_SECOND 9
static int g_sleep_sec = 0;


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_logo_info_s {
	char* path;
	int16_t num;
	int16_t width;
	int16_t height;
} gp_logo_info_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/



/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
int32_t gChunkDev;
char gLogoPath[64];
chunk_block_t gDispBlk[DISPLAY_COUNT] = {0};
uint8_t *g_addr;
static int showlogocounter = 0;
#define startx 225
#define starty 373
static unsigned int showlogoflag = 0;
static unsigned char logoBuf[26*28*2];

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void drawicon(uint x, uint y, unsigned char *pdata,unsigned char *icon){

	unsigned char *p = pdata + (800*2)*y + x*2;
	int i = 0;
	/*printf("[%s][%d]\n",__FUNCTION__,__LINE__);*/

	for(i=0;i<28;i++){
		memcpy(p,icon,26*1*2);
		p+=(800*2);
		icon+=(26*1*2);
		
	}
}

static UINT32
backlightEnable(
	UINT32 en
)
{
	gp_board_panel_t *panel_config;
	panel_config = gp_board_get_config("panel", gp_board_panel_t);
	if (panel_config != NULL && panel_config->set_backlight != NULL) {
		panel_config->set_backlight(en);
	}
	else {
		return SP_FAIL;
	}

	return SP_OK;
}
static void
logo_memset16(void *_ptr, unsigned short val, unsigned count)
{
    unsigned short *ptr = _ptr;
    while(count--)
        *ptr++ = val;
}

static int32_t
showlogo(
	gp_logo_info_t info
)
{
	int32_t dispDev;
	char dispPath[16];
	int32_t logoFd;
	struct stat s;
	unsigned short *data, *bits, *ptr;
	unsigned count, max;
	gp_disp_res_t panelRes;
	gp_bitmap_t bitmap;

	/* Opening the device dispDev */
	sprintf(dispPath, "/dev/disp%d", info.num);
	dispDev = open(dispPath, O_RDWR);
	if (dispDev < 0) {
		printf("[showlogo] Open disp%d fail\n", info.num);
		return -1;
	}
	ioctl(dispDev, DISPIO_GET_PANEL_RESOLUTION, &panelRes);

	/* Allocate panel bitmap buffer */
	gDispBlk[info.num].size = panelRes.width * panelRes.height * 2;
	if (ioctl(gChunkDev, CHUNK_MEM_ALLOC, (unsigned long)&gDispBlk[info.num]) < 0) {
		printf("[showlogo] Buff allocate fail\n");
		goto __Error_panelblk_alloc;
	}

	/* Open logo files */
	logoFd = open(info.path, O_RDONLY);
	if (logoFd < 0) {
		printf("[showlogo] Open %s fail\n", info.path);
		goto __Error_open_logo;
	}

	if (fstat(logoFd, &s) < 0) {
        goto __Error_close_file;
    }

	data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, logoFd, 0);
	if (data == MAP_FAILED)
		goto __Error_close_file;

	max = panelRes.width * panelRes.height;
	ptr = data;
	count = s.st_size;
	bits = gDispBlk[info.num].addr;
	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;

		logo_memset16(bits, ptr[1], n);
		bits += n;
		max -= n;
		ptr += 2;
		count -= 4;
	}

	munmap(data, s.st_size);

	/* Set bitmap */
	bitmap.width = panelRes.width;
	bitmap.height = panelRes.height;
	bitmap.bpl = panelRes.width * 2;
	bitmap.type = SP_BITMAP_RGB565;
	g_addr =  bitmap.pData = gDispBlk[info.num].addr;
	ioctl(dispDev, DISPIO_SET_PRI_BITMAP, &bitmap);

	/* Enable primary layer */
	ioctl(dispDev, DISPIO_SET_PRI_ENABLE, 1);

	ioctl(dispDev, DISPIO_SET_UPDATE, 0);

	close(logoFd);
	close(dispDev);

	return 0;

__Error_close_file:
	close(logoFd);
__Error_open_logo:
	ioctl(gChunkDev, CHUNK_MEM_FREE, (unsigned long)&gDispBlk[info.num]);
	gDispBlk[info.num].addr = NULL;
__Error_panelblk_alloc:
	close(dispDev);

	return -1;
}


void iconcnt( int sigNum )
{
	showlogocounter++;
	printf("[iconcnt] playlogo counter %d\n", showlogocounter);	
}


void PrePareLight()
{
	int x = 0;
	int y = 0;
	FILE *fp = 0;
	int i = 0;
	fp = fopen("/etc/resource/dot_off.raw","rb");
	
	if(fp == NULL){
		printf("Open /etc/resource/ Fail!\n");
		return;
	}
	fread(logoBuf,1 , 26*28*2, fp);
	showlogoflag = 1;
	if(showlogocounter == 0){
		for(i=0;i<10;i++){
			y = starty;
			x = i*(10+26) + startx;
			drawicon(x, y, g_addr, logoBuf);
			usleep(100000);
		}
	
	}
	fclose(fp);
	fp = 0;
	

}
void playlogo()
{
	
	int flashflag = 0;
	int oldshowlogocounter = 0;
	int x = 0;
	int y = 0;
	FILE *fp = 0;
	int i = 0;
	
	
	#if 0
	printf("[%s][%d]\n",__FUNCTION__,__LINE__);
	printf("======================================================\n");
	printf("[playlogo] playlogo counter %d\n", showlogocounter);
	printf("======================================================\n");
	#endif
	
	if(showlogocounter == 0){
		/* do nothing*/
	}
	else{

		oldshowlogocounter = showlogocounter;

	    while(oldshowlogocounter == showlogocounter){
		
			if(flashflag == 0)
		 		fp = fopen("/etc/resource/dot_on.raw","rb");
	  		else
				fp = fopen("/etc/resource/dot_off.raw","rb");
		
			if(fp == NULL){
				printf("Open /etc/resource/ Fail!\n");
				return ;
			}
	
			fread(logoBuf,1 , 26*28*2, fp);
			fclose(fp);
			fp = 0;
			y = starty;
			x = (showlogocounter-1)*(10+26) + startx;
			drawicon(x, y, g_addr, logoBuf);

			usleep(500000);
		
			flashflag ^= 0x01;

		}

		fp = fopen("/etc/resource/dot_on.raw","rb");
		if(fp == NULL){
			printf("Open /etc/resource/ Fail!\n");
			return;
		}
	
		fread(logoBuf,1 , 26*28*2, fp);

		y = starty;
		x = (oldshowlogocounter-1)*(10+26) + startx;
		drawicon(x, y, g_addr, logoBuf);
		fclose(fp);
		fp = 0;
		flashflag = 0;
		oldshowlogocounter = 0;
	}


	
}


int
main(
	int argc,
	char **argv
)
{
	int i;
	DIR *dir;
	int ret;
	struct dirent *entry;
	gp_logo_info_t logo_info;
	signal( SIGUSR1 , iconcnt );
	

	if ( argc > 1) {
		g_sleep_sec = atoi(argv[1]);
	}
	else {
		g_sleep_sec = DEFAULT_SLEEP_SECOND;
	}

#ifndef SYSCONFIG_SHOWLOGO
	return 0;
#endif
	/* Open logo dir */
	dir = opendir(LOGO_DIR);
	if (dir == NULL) {
		printf("[showlogo] %s dir not exist\n", LOGO_DIR);
		return -1;
	}

	/* Opening /dev/chunkmem */
	gChunkDev = open("/dev/chunkmem", O_RDWR);
	if (gChunkDev < 0) {
		printf("[showlogo] Open chunkMem fail\n");
		goto __Error_open_chunk;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] != 0 && (entry->d_type & DT_REG)) { /* Is File */
			DEBUG("[%s] <%s>\n", __FUNCTION__, entry->d_name);
			if (strncmp(entry->d_name, "logo_", 5) == 0) {
				char *pEnd;
				char *p = entry->d_name + 4;
				sprintf(gLogoPath, "%s%s", LOGO_DIR, entry->d_name);
				logo_info.path = gLogoPath;

				p = strchr(p, '_');
				if (p == NULL)
					continue;
				logo_info.num = (uint16_t)strtol(p+1, &pEnd, 10);

				p = strchr(pEnd, '_');
				if (p == NULL)
					continue;
				logo_info.width = (uint16_t)strtol(p+1, &pEnd, 10);

				p = strchr(pEnd, '_');
				if (p == NULL)
					continue;
				logo_info.height = (uint16_t)strtol(p+1, &pEnd, 10);

				DEBUG("num=%d, width=%d, height=%d\n", logo_info.num, logo_info.width, logo_info.height);
				showlogo(logo_info);
			}
		}
	}

	sleep(1);

	backlightEnable(1);

	closedir(dir);
	#if 1
	PrePareLight();
	while(showlogoflag){
		playlogo();
	}
	sleep(g_sleep_sec);
	/* Free frame buffer */
	for (i=0; i<DISPLAY_COUNT; i++) {
		if (gDispBlk[i].addr) {
			ioctl(gChunkDev, CHUNK_MEM_FREE, (unsigned long)&gDispBlk[i]);
		}
	}

	close(gChunkDev);
	#endif
	return 0;
__Error_open_chunk:
	closedir(dir);
	return -1;
}
