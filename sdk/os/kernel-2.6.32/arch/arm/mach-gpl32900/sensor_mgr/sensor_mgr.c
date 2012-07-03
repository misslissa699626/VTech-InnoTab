 /**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
 
 /**
 * @file gp_csi.c
 * @brief CSI interface
 * @author Simon Hsu
 */
//#include <stdlib.h>
//#include <stdio.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-subdev.h>

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/sensor_mgr.h>
#include <mach/gp_board.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	SENSOR_MAX_SBDEV	3

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
	#define DEBUG	DIAG_ERROR
#else
	#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_sensormgr_dev_s {
	/*Sub device*/
	int32_t sdcnt;
	int32_t sdidx;
	char port[SENSOR_MAX_SBDEV][16];
	callbackfunc_t cb[SENSOR_MAX_SBDEV];
	struct v4l2_subdev *sd[SENSOR_MAX_SBDEV];
	sensor_config_t *sensor[SENSOR_MAX_SBDEV];
	struct semaphore sem;
} gp_sensormgr_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
gp_sensormgr_dev_t *sensormgr_devices=NULL;
gp_board_sensor_t *sensor_config=NULL;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

/**
 * @brief   use to select sensor
 * @return  success: 0, fail: -1
 * @see
 */ 
int32_t gp_get_sensorinfo( int sel, int* sdaddr, int* cbaddr, int* port, int* sensor)
{
	int32_t ret;

	if( down_interruptible(&sensormgr_devices->sem) ) {
		return -ERESTARTSYS;
	}
	
	if( sel > sensormgr_devices->sdcnt ) {
		DIAG_ERROR("sensor number out of insmod sensor number\n");
		ret = -1;
	}
	
	if( sensormgr_devices->sd[sel]==0 )	{
		DIAG_ERROR("invalid address or has unresgiter\n");
		ret = -1;
	}
	else {
		*sdaddr = (int)sensormgr_devices->sd[sel];
		*cbaddr = (int)&sensormgr_devices->cb[sel];
		*port = (int)sensormgr_devices->port[sel];
		*sensor = (int)sensormgr_devices->sensor[sel];
		ret = 0;
	}
		
	up(&sensormgr_devices->sem);
	return ret;
}
EXPORT_SYMBOL(gp_get_sensorinfo);

/**
 * @brief   use to register sensor
 * @return  success: 0, fail: -1
 * @see
 */ 
int32_t register_sensor(struct v4l2_subdev *sd, int *port, sensor_config_t *config)
{
	int i, sdnum=0;
	
	DEBUG("%s:%d\n", __FUNCTION__, __LINE__);

	if( down_interruptible(&sensormgr_devices->sem) ) {
		return -ERESTARTSYS;
	}

	if( sensormgr_devices==NULL )
	{
		DIAG_ERROR("sensor manager module has not insert yet\n");
		return -1;
	}
	if(sensormgr_devices->sdcnt>=SENSOR_MAX_SBDEV)
	{
		DIAG_ERROR("register too many sensor module\n");
		return -1;
	}
	for( i=0; i<SENSOR_MAX_SBDEV*2; i+=2 )	{
		if( strncmp( (char*)*(port+i+1), "PORT0", strlen( (char*)*(port+i+1) ) )==0 || strncmp( (char*)*(port+i+1), "MIPI", strlen( (char*)*(port+i+1) ) )==0 )	{
			if(strncmp( (char*)*(port+i), "0", strlen( (char*)*(port+i) ) ) == 0)	{
				sensor_config = gp_board_get_config("sensor0", gp_board_sensor_t);
				sdnum=0;
			}
			else if(strncmp( (char*)*(port+i), "1", strlen( (char*)*(port+i) ) ) == 0)	{
				sensor_config = gp_board_get_config("sensor1", gp_board_sensor_t);
				sdnum=1;
			}
			else if(strncmp( (char*)*(port+i), "2", strlen( (char*)*(port+i) ) ) == 0)	{
				sensor_config = gp_board_get_config("sensor2", gp_board_sensor_t);
				sdnum=2;
			}
			sensormgr_devices->sd[sdnum] = sd;
			strcpy( sensormgr_devices->port[sdnum], (char*)*(port+i+1) );
			
			if( sensor_config!=NULL )	{
				sensormgr_devices->cb[sdnum].powerctl = sensor_config->set_sensor_power;
				sensormgr_devices->cb[sdnum].standby = sensor_config->set_sensor_standby;
				sensormgr_devices->cb[sdnum].reset = sensor_config->set_sensor_reset;
				sensormgr_devices->cb[sdnum].set_port = sensor_config->set_sensor_port;
				sensormgr_devices->sensor[sdnum] = config;
			}
			else {
				sensormgr_devices->cb[sdnum].powerctl = NULL;
				sensormgr_devices->cb[sdnum].standby = NULL;
				sensormgr_devices->cb[sdnum].reset = NULL;
				sensormgr_devices->cb[sdnum].set_port = NULL;
				sensormgr_devices->sensor[sdnum] = NULL;
			}				
			sensormgr_devices->sdcnt++;	
		}
	}
	/*
	sensormgr_devices->sd[sensormgr_devices->sdcnt] = sd;
	if( sensormgr_devices->sdcnt==0 )
		sensor_config = gp_board_get_config("sensor0", gp_board_sensor_t);
	else if( sensormgr_devices->sdcnt==1 )
		sensor_config = gp_board_get_config("sensor1", gp_board_sensor_t);
	else if( sensormgr_devices->sdcnt==2 )
		sensor_config = gp_board_get_config("sensor2", gp_board_sensor_t);
	
	if (strncmp(port, "PORT0", strlen(port)) == 0)
		strcpy( sensormgr_devices->port[sensormgr_devices->sdcnt], "PORT0" );
	else if(strncmp(port, "MIPI", strlen(port)) == 0)
		strcpy( sensormgr_devices->port[sensormgr_devices->sdcnt], "MIPI" );
		
	if( sensor_config == NULL )
	{
		DIAG_ERROR("No board config\n");
		sensor_config->set_sensor_power=NULL;
		sensor_config->set_sensor_reset=NULL;
		sensor_config->set_sensor_port=NULL;
	}
	if( sensor_config->set_sensor_power==NULL )
		DIAG_ERROR("Sensor Power pin not found\n");
	if( sensor_config->set_sensor_reset==NULL )
		DIAG_ERROR("Sensor reset pin not found\n");
	if( sensor_config->set_sensor_port==NULL )
		DIAG_ERROR("Sensor set port not found\n");
		
	sensormgr_devices->cb[sensormgr_devices->sdcnt].powerctl = sensor_config->set_sensor_power;
	sensormgr_devices->cb[sensormgr_devices->sdcnt].reset = sensor_config->set_sensor_reset;
	sensormgr_devices->cb[sensormgr_devices->sdcnt].set_port = sensor_config->set_sensor_port;
//	sensormgr_devices->cb[sensormgr_devices->sdcnt].suspend = sd->ops->ext->suspend;
//	sensormgr_devices->cb[sensormgr_devices->sdcnt].resume = sd->ops->ext->resume;
	
	sensormgr_devices->sdcnt++;*/
	
	up(&sensormgr_devices->sem);

	return 0;
}
EXPORT_SYMBOL(register_sensor);

/**
 * @brief   use to unregister sensor
 * @return  success: 0, fail: -1
 * @see
 */ 
int32_t unregister_sensor(struct v4l2_subdev *sd)
{
	int32_t i;

	if( down_interruptible(&sensormgr_devices->sem) ) {
		return -ERESTARTSYS;
	}
	
	if( sensormgr_devices==NULL )
	{
		DIAG_ERROR("csi module has not isert yet\n");
		return -1;
	}
	if( sensormgr_devices->sdcnt==0 )
	{
		DIAG_ERROR("No sensor register\n");
		return -1;
	}
	for( i=0; i<sensormgr_devices->sdcnt; i++ )
	{
		if( sensormgr_devices->sd[i]==sd )
		{
			sensormgr_devices->sd[i]=0;
			sensormgr_devices->sdcnt--;
			break;
		}
	}
	
	up(&sensormgr_devices->sem);
	
	return 0;
}
EXPORT_SYMBOL(unregister_sensor);

/**
 * @brief   character device module exit function
 * @see
 */ 
static void __exit gp_sensor_mgr_exit(void)
{
	kfree(sensormgr_devices);
	sensormgr_devices = NULL;
}

/**
 * @brief   character device module init function
 * @return  success: 0
 * @see
 */ 
static int32_t __init gp_sensor_mgr_init(void)
{
	int32_t ret;
	
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	/* allocate a structure for csi device */
	sensormgr_devices = kmalloc(sizeof(gp_sensormgr_dev_t), GFP_KERNEL);
	if(!sensormgr_devices) {
		DIAG_ERROR("CSI: can't kmalloc\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}
	memset(sensormgr_devices, 0, sizeof(gp_sensormgr_dev_t));

	/* initial the semaphore */
	init_MUTEX(&(sensormgr_devices->sem));
	
	return 0;

fail_kmalloc:
	return ret;
}

module_init(gp_sensor_mgr_init);
module_exit(gp_sensor_mgr_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP Sensor_mgr driver");
MODULE_LICENSE_GP;
