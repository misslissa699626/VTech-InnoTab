#ifndef _APP_DEV_MODULE_
#define _APP_DEV_MODULE_

#define APP_TAG  0x50415047   		//GPAP
#define APP_HDR_TAG                 0
#define APP_AREA_FORMAT  			1
#define APP_AREA_INIT    			3
#define APP_AREA_WRITE_SECTOR   	4
#define APP_AREA_READ_SECTOR    	5
#define APP_AREA_FLUSH		    	6
#define APP_AREA_GET_NANDINFO   	7
#define APP_AREA_GET_PROCESS     	8
#define APP_AREA_GET_APP_HEADER   	9

#define APP_AREA_HEADER_INFO			     10
#define APP_AREA_PARTITION_INFO			     11
#define APP_AREA_PARTITION_WRITE_SECTOR      12
#define APP_AREA_PARTITION_READ_SECTOR     	 13
#define APP_AREA_PARTITION_WRITE_SECTOR_END  14


/* Partiton Header Information Start*/
#define APP_PART_TAG					0x41445041		//"APDP"

#define APP_PART_TAG_ADD            	0
#define APP_PART_TAG_LEN            	4
#define APP_PART_VERSION_ADD        	4
#define APP_PART_VERSION_LEN        	1
#define APP_PART_DRV_TYPE_ADD       	5
#define APP_PART_DRV_TYPE_LEN       	1
#define APP_PART_SEC_SIZE_ADD	    	8
#define APP_PART_SEC_SIZE_LEN   		4
#define APP_PART_IMG_CHKSUM_ADD	    	20
#define APP_PART_IMG_CHKSUM_LEN   		4
#define APP_PART_IMG_STARTSEC_ADD	    24
#define APP_PART_IMG_STARTSEC_LEN  		4
#define APP_PART_IMG_SIZESEC_ADD	    28
#define APP_PART_IMG_SIZESEC_LEN   		4
#define APP_PART_IMG_TYPE_ADD	    	32
#define APP_PART_IMG_TYPE_LEN   		1
#define APP_PART_IMG_DEST_ADDR	        36
#define APP_PART_IMG_DEST_ADDR_LEN	    4
/* Partiton Header Information End*/

typedef struct
{
	void *buffer;				// 数据buffer
	unsigned int start_sector;	// 起始sector
	unsigned int sector_cnt;	// sector个数	
}Xsfer_arg;

typedef struct
{
	unsigned int block_size;
	unsigned int page_size;	
}nand_info_arg;

typedef struct
{
	unsigned int targe_sectors;
	unsigned int xsfered_sectors;	
}process_arg;

typedef struct gpAppPart_s{
	unsigned int	startSector;	// [24+16N .. 27+16N], (sectors)
	unsigned int	partSize;		// [28+16N .. 31+16N], (sectors)
	char	partType;		// [32+16N]         
	char	rfu33[3];		// [33+16N .. 35+16N]
	unsigned int	dstAddress;		// [36+16N .. 39+16N]
}gpAppPart_t;

typedef struct gpAppHeader_s{
	unsigned int	headerTag;		// [0..3]
	char	imageVersion;	// [4]
	char	driverType;		// [5]
	char	headerSize;		// [6], (Sectors)
	char	rfu7;			// [7]
	unsigned int	totalAppSize;	// [8..11], (Sectors)
	unsigned short	totalPartNumber;// [12..13]
	//INT8U	rfu14[6];		// [14..19]
	char	upgradeKeyPort;	// [14], port, bit[0..3]:port, bit[4..7] low/high active
	char	upgradeKeyId;	// [15], id
	char	rfu14[4];		// [16..19]
	unsigned int	checkSum;		// [20..23]
	gpAppPart_t	*appPartN;	// [24...]
}gpAppHeader_t;

typedef struct gpAppHeaderUser_s{
	unsigned int		headerTag;		// [0..3]
	char				imageVersion;	// [4]
	char				headerSize;		// [6], (Sectors)	
	unsigned int		totalAppSize;	// [8..11], (Sectors)
	unsigned short	    totalPartNumber;// [12..13]	
	unsigned int		checkSum;		// [20..23]
}gpAppHeaderUser_t;

typedef struct gpAppPartUser_s{
	unsigned int	partNumber;
	unsigned int	startSector;	// [24+16N .. 27+16N], (sectors)
	unsigned int	partSize;		// [28+16N .. 31+16N], (sectors)
	char			partType;		// [32+16N]         
	unsigned int	dstAddress;		// [36+16N .. 39+16N]
	unsigned int	CurSectorId;
	unsigned int  	ImageSize;
	unsigned int    ImageStartSector;
}gpAppPartUser_t;

typedef struct gpAppPartArg_s
{
	gpAppPartUser_t  partparam;
	unsigned int sector_cnt;	// sector个数
	void *buffer;				// 数据buffer

}XsferPartArg_t;

typedef enum
{
  APP_TYPE_RUNTIME_CODE = 0,
  APP_TYPE_RESOUECE_BIN = 1,
  APP_TYPE_FASTBOOT_BIN = 2,
  APP_TYPE_QUICK_IMAGE = 3,
  APP_TYPE_HIBERATE_IMAGE = 4,
  APP_TYPE_IMAGE_FLAG = 5,
  APP_TYPE_CUSTOMIZE_BTL = 6,
} APP_TYPE_ENUM;

#endif