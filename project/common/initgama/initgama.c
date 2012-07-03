#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include <mach/gp_board.h>
#include "mach/typedef.h"
#include "mach/gp_display.h"


#define GAMA_DATA_FILE "/etc/color.lut"
#define COLOR_DATA_FILE "/etc/color.cc"


static UINT32
initColorMatrix(
	void
)
{
	int32_t dispDev;
	int32_t colorFd;
	uint8_t colorData[48];
	gp_disp_colormatrix_t matrix;
	int32_t i;
	int32_t *colorMatrix;
	
	/* Open color data file */
	colorFd = open(COLOR_DATA_FILE, O_RDONLY);
	if (colorFd < 0) {
		printf("Open %s fail\n", COLOR_DATA_FILE);
		return -1;
	}

	/* Opening the device dispDev */
	dispDev = open("/dev/disp0", O_RDWR);
	if (dispDev < 0) {
		printf("Open disp0 fail\n");
		goto __Err_open_disp_color;
	}

	if ( 48 != read(colorFd, colorData, 48) ) {
		printf("read color data fail\n");
		goto __Err_read_color;
	}

	colorMatrix = (int32_t*)colorData;
	matrix.a00 = (uint16_t)colorMatrix[0];
	matrix.a01 = (uint16_t)colorMatrix[1];
	matrix.a02 = (uint16_t)colorMatrix[2];
	matrix.a10 = (uint16_t)colorMatrix[3];
	matrix.a11 = (uint16_t)colorMatrix[4];
	matrix.a12 = (uint16_t)colorMatrix[5];
	matrix.a20 = (uint16_t)colorMatrix[6];
	matrix.a21 = (uint16_t)colorMatrix[7];
	matrix.a22 = (uint16_t)colorMatrix[8];
	matrix.b0 = (uint16_t)colorMatrix[9];
	matrix.b1 = (uint16_t)colorMatrix[10];
	matrix.b2 = (uint16_t)colorMatrix[11];

	ioctl( dispDev, DISPIO_SET_CMATRIX_PARAM, &matrix );
	ioctl( dispDev, DISPIO_SET_UPDATE, NULL );

	close(colorFd);
	close(dispDev);	
	return 0;
	
__Err_read_color:
	close(dispDev);
__Err_open_disp_color:
	close(colorFd);
	return -1;
	
}

static UINT32
initgama(
	void
)
{
	int32_t dispDev;
	int32_t gamaFd;
	uint8_t gamaData[768];
	gp_disp_gammatable_t gamma_table_r, gamma_table_g, gamma_table_b;
	int32_t i;

	
	/* Open gama data file */
	gamaFd = open(GAMA_DATA_FILE, O_RDONLY);
	if (gamaFd < 0) {
		printf("Open %s fail\n", GAMA_DATA_FILE);
		return -1;
	}

	/* Opening the device dispDev */
	dispDev = open("/dev/disp0", O_RDWR);
	if (dispDev < 0) {
		printf("Open disp0 fail\n");
		goto __Error_open_disp_gama;
	}

	/* read gama data */
	if ( 768 != read(gamaFd, gamaData, 768) ) {
		printf("Read gama data fail\n");
		goto __Error_open_disp_gama;
	}	

	for( i = 0 ; i < 256 ; i++ ) {
		gamma_table_r.table[i] = gamaData[i];
	}
	for( i = 256 ; i < 512 ; i++ ) {
		gamma_table_g.table[i-256] = gamaData[i];
	}
	for( i = 512 ; i < 768 ; i++ ) {
		gamma_table_b.table[i-512] = gamaData[i];
	}
							
	ioctl( dispDev, DISPIO_SET_GAMMA_ENABLE, 0 );
	ioctl( dispDev, DISPIO_SET_UPDATE, NULL );

	gamma_table_r.id = SP_DISP_GAMMA_R;
	gamma_table_g.id = SP_DISP_GAMMA_G;
	gamma_table_b.id = SP_DISP_GAMMA_B;

	ioctl( dispDev, DISPIO_WAIT_FRAME_END, 0 );
	ioctl( dispDev, DISPIO_SET_GAMMA_PARAM, &gamma_table_r );
	ioctl( dispDev, DISPIO_SET_GAMMA_PARAM, &gamma_table_g );
	ioctl( dispDev, DISPIO_SET_GAMMA_PARAM, &gamma_table_b );

	ioctl( dispDev, DISPIO_SET_GAMMA_ENABLE, 1 );
	ioctl( dispDev, DISPIO_SET_UPDATE, NULL );
	
	close(gamaFd);
	close(dispDev);
	return 0;

__Err_read_gama:
	close(dispDev);
__Error_open_disp_gama:
	close(gamaFd);
	return -1;
}

int
main(
	int argc,
	char **argv
)
{

	/*Initialize color matrix*/
	initColorMatrix();
		
	/*Initialize gama*/
	initgama();

	return 0;
}

