#include <stdio.h>
#include <sys/types.h>	//open
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>	//malloc
#include <unistd.h>	//sync
#include <string.h> //memset
#include <sys/ioctl.h>


#define	SPMP_DEBUG_PRINT(fmt, args...) 	 printf("Debug %d[%s]: " fmt, __LINE__, __FUNCTION__, ##args)
#define SECTOR_SIZE	512

#define NF_IOCTL_START    0x80000
#define NF_IOCTL_FLUASH   (0 + NF_IOCTL_START)
#define NF_IOCTL_TEST   (1 + NF_IOCTL_START)
#define NF_IOCTL_DBG_MSG   (2 + NF_IOCTL_START)
#define NF_IOCTL_EREASEALL   (3 + NF_IOCTL_START)

#define IOCTL_NAND_GET_BLK_INFO (4 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_OPEN		 (5 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_CLOSE	 (6 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_WRITE  (7 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_READ  (8 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_READ_1K	(9 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_WRITE_1K	(10 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_READ_PAGE	(11 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_WRITE_PAGE	(12 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_OPEN		 (13 + NF_IOCTL_START)


#define TEST_FILE_NAME	"nand_test"
#define TEST_DEVICE_NAME "/dev/nanda3"

int nf_memcmp(unsigned char* ptr1, unsigned char ptr2, int len)
{
	int i;
	int count = (len + 3) >> 2;

	for (i = 0; i < count; i++)
	{
		//if (ptr1[i] != ptr2[i])
		if (ptr1[i] != ptr2)
			return i;
	}

	return 0;
}

int Test_KernelDebug_Set(unsigned long op)
{
	int fd = -1;
	char temp = 0;

	fd = open("/dev/spmp8k_nanda1", O_RDONLY );
	if( fd <= 0 )
	{
		SPMP_DEBUG_PRINT("Open Block file failed\n");
		return -1;
	}
	if(ioctl(fd, NF_IOCTL_DBG_MSG, op)!=0)
	{
		SPMP_DEBUG_PRINT("IOCTl failed\n");
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}

void print_buf(unsigned char* buf, int len)
{
	int i;
	unsigned char* buf_NC;

	buf_NC = (unsigned char*) buf;

	for (i = 0; i < len; i++)
	{

		if ((i % 16) == 8)
			printf("- ");
		if ((i % 16) == 0)
			printf("\n");

		printf("%02x ", buf_NC[i]);
	}
	printf("\n");
}

int Test_singleHugeFile(void)
{
	int fd = -1, i=0;
	int TestData_Length = 100*1024*1024; //100MB
	unsigned char *ptrBuffer=NULL;
	
	fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT|O_TRUNC);
	if(fd<0)
	{
		SPMP_DEBUG_PRINT("Open/Create file failed\n");
		return -1;
	}
	ptrBuffer = malloc(SECTOR_SIZE);
	if(ptrBuffer == NULL)
	{
		SPMP_DEBUG_PRINT("Alloc memory failed\n");
		close(fd);
		return -1;
	}

	memset(ptrBuffer, 0x5a, SECTOR_SIZE);
	for(i = 0; i<TestData_Length; i+= SECTOR_SIZE)
	{
		if(write(fd, ptrBuffer, SECTOR_SIZE) < 0)
		{
			SPMP_DEBUG_PRINT("Write data failed\n");
			free(ptrBuffer);
			close(fd);
			return -1;
		}
	}
	
	close(fd);

	sync();

	fd = open(TEST_FILE_NAME, O_RDONLY);
	if(fd<0)
	{
		SPMP_DEBUG_PRINT("Open file failed\n");
		return -1;
	}
	for(i = 0; i<TestData_Length; i+= SECTOR_SIZE)
	{
		if(read(fd, ptrBuffer, SECTOR_SIZE) <= 0)
		{
			SPMP_DEBUG_PRINT("Read data failed\n");
			free(ptrBuffer);
			close(fd);
			return -1;
		}
		if(nf_memcmp(ptrBuffer, 0x5a, SECTOR_SIZE) != 0)
		{
			SPMP_DEBUG_PRINT("Compare Data failed\n");
		}
	}
	close(fd);
	free(ptrBuffer);

	SPMP_DEBUG_PRINT("Compare Data End:  OK !\n");

	return 0;
}

#define MULTI_FILE_NUM 1024

int Test_MultiNormalFile(char flag)
{
	int fd = -1, i=0, j = 0;
	int TestData_Length = 600*1024; //100KB * 1024
	unsigned char *ptrBuffer=NULL;
	char FileName[50] ;
	int Err_Write_Num = 0, Err_Compare_Num = 0;
	int ret= 0 ;

	ptrBuffer = malloc(SECTOR_SIZE);
	if(ptrBuffer == NULL)
	{
		SPMP_DEBUG_PRINT("Alloc memory failed\n");
		close(fd);
		return -1;
	}

	if (flag & 0x01)
	{
		for(i = 0; i < MULTI_FILE_NUM; i++)	
		{
			sprintf(FileName, "%s%04d", TEST_FILE_NAME, i);
			fd = open(FileName, O_RDWR | O_CREAT | O_TRUNC);
			if(fd < 0)
			{
				SPMP_DEBUG_PRINT("Open/Create file %s failed\n", FileName);
				continue;
			}
			
			memset(ptrBuffer, 0x5a, SECTOR_SIZE);
			for(j = 0; j<TestData_Length; j += SECTOR_SIZE)
			{
				if(write(fd, ptrBuffer, SECTOR_SIZE) < 0)
				{
					SPMP_DEBUG_PRINT("%s: Write data failed\n", FileName);
					close(fd);
					
					break;
				}
			}
			if(j<TestData_Length)
			{
				Err_Write_Num ++ ;
				continue;
			}
			close(fd);
			sync();
		}
	}
	if(flag &0x02)
	{
		for(i = 0; i < MULTI_FILE_NUM; i++)	
		{
			sprintf(FileName, "%s%04d", TEST_FILE_NAME, i);
			fd = open(FileName, O_RDONLY);
			if( fd < 0 )
			{
				SPMP_DEBUG_PRINT("Open file %s failed\n", FileName);
				Err_Compare_Num++;
				continue;
			}
			
			for(j = 0; j<TestData_Length; j+= SECTOR_SIZE)
			{
				memset(ptrBuffer, 0, SECTOR_SIZE);
				if(read(fd, ptrBuffer, SECTOR_SIZE) <= 0)
				{
					SPMP_DEBUG_PRINT("Read data failed\n");
					break;
				}
				ret = nf_memcmp(ptrBuffer, 0x5a, SECTOR_SIZE);
				if( ret != 0)
				{
					SPMP_DEBUG_PRINT("Compare Data failed: %s %d %d\n", FileName, j, ret);
					break;
				}
			}
			if(j<TestData_Length)
				Err_Compare_Num++;
			close(fd);
		}
	}
	free(ptrBuffer);

	SPMP_DEBUG_PRINT("Compare Data End: wr:%d Comp: %d !\n",  Err_Write_Num , Err_Compare_Num );

	return 0;
}



int Test_DisplayFile(char *FileName,unsigned long start_addr, long len, unsigned long op)
{
	int fd = -1, i=0, j = 0;
	int TestData_Length = 600*1024; //100KB * 1024
	unsigned char *ptrBuffer=NULL;
//	char FileName[50] ;
	int Err_Write_Num = 0, Err_Compare_Num = 0;
	int ret= 0 ;
	

	if(len<SECTOR_SIZE)
		TestData_Length = SECTOR_SIZE;
	ptrBuffer = malloc(TestData_Length);
	if(ptrBuffer == NULL)
	{
		SPMP_DEBUG_PRINT("Alloc memory failed\n");
		close(fd);
		return -1;
	}

	fd = open(FileName, O_RDONLY);
	if( fd < 0 )
	{
		SPMP_DEBUG_PRINT("Open file %s failed\n", FileName);
		Err_Compare_Num++;
		return -1;
	}
	lseek(fd, start_addr,SEEK_SET);
	memset(ptrBuffer, 0, TestData_Length);
	Test_KernelDebug_Set(op);
	if(read(fd, ptrBuffer, TestData_Length) <= 0)
	{
		SPMP_DEBUG_PRINT("Read data failed\n");
	}else{
		SPMP_DEBUG_PRINT("%s:  %x(%d) %d\n", FileName,start_addr, start_addr, len);
		print_buf(ptrBuffer, len);
	}
	close(fd);
	
	free(ptrBuffer);
	Test_KernelDebug_Set(0);
	return 0;
}



int Test_singleHugeFile_Read(void)
{
	int fd = -1, i=0;
	int TestData_Length = 100*1024*1024; //100MB
	unsigned char *ptrBuffer=NULL;
	
	ptrBuffer = malloc(SECTOR_SIZE);
	if(ptrBuffer == NULL)
	{
		SPMP_DEBUG_PRINT("Alloc memory failed\n");
		return -1;
	}

	fd = open(TEST_FILE_NAME, O_RDONLY);
	if( fd < 0 )
	{
		SPMP_DEBUG_PRINT("Open file failed\n");
		free(ptrBuffer);
		return -1;
	}
	
	for(i = 0; i<TestData_Length; i+= SECTOR_SIZE)
	{
		if(read(fd, ptrBuffer, SECTOR_SIZE) <= 0)
		{
			SPMP_DEBUG_PRINT("Read data failed\n");
			free(ptrBuffer);
			close(fd);
			return -1;
		}
		if(nf_memcmp(ptrBuffer, 0x5a, SECTOR_SIZE) != 0)
		{
			SPMP_DEBUG_PRINT("Compare Data failed (offset = %d(%x))\n", i, i);
			print_buf(ptrBuffer, SECTOR_SIZE);
		}
	}
	close(fd);
	free(ptrBuffer);

	SPMP_DEBUG_PRINT("Compare Data End:  OK !\n");

	return 0;
}
int Test_EnableKernelDebug(unsigned int flag)
{
	int fd = -1;
	char temp = 0;

	fd = open(TEST_DEVICE_NAME, O_RDONLY );
	if( fd <= 0 )
	{
		SPMP_DEBUG_PRINT("Open Block file failed\n");
		return -1;
	}
	
	SPMP_DEBUG_PRINT("Set kernel debug flag: %x\n", flag);
	if(ioctl(fd, NF_IOCTL_DBG_MSG, (unsigned long)flag)!=0)
	{
		SPMP_DEBUG_PRINT("IOCTl failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}


int Test_KernelNand_Ioctl(unsigned int cmd, unsigned long option)
{
	int fd = -1;
	char temp = 0;

	fd = open("/dev/spmp8k_nanda1", O_RDONLY );
	if( fd <= 0 )
	{
		SPMP_DEBUG_PRINT("Open Block file failed\n");
		return -1;
	}
	SPMP_DEBUG_PRINT("Ioctl Cmd: %x %d\n", cmd, option);
	sleep(1);
	if(ioctl(fd, cmd, option)!=0)
	{
		SPMP_DEBUG_PRINT("IOCTl failed\n");
		close(fd);
		return -1;
	}
	
	close(fd);

	return 0;
}


typedef struct gp_nand_blk_info_s {
	int block_size; 	/* in bytes */
	int page_size; 		/* in bytes */
} gp_nand_blk_info_t;

typedef struct gp_nand_phy_partiton_s {
	int offset; 	/* in bytes */
	int size;		/* in bytes */
} gp_nand_phy_partition_t;

typedef struct gp_nand_write_buffer_s {
	void *buf;
	int size; 	/* in bytes */
} gp_nand_write_buffer_t;

#define LINUX_IMAGE_SIZE 0x700000
#define LINUX_IMAGE_NF_STARTBLK (64*1024*1024)
#define LINUX_IMAGE_SDRAM_COMPARE_ADDR 0x2000000
#define LINUX_IMAGE_SDRAM_STARTADDR 0x1000000

int Test_NF_PHYS_PartitionWrite(unsigned char RW_Func, unsigned char RW_flag)
{
	int fd = -1, i=0, j = 0;
	char temp = 0;
	unsigned char *testBuffer;
	gp_nand_blk_info_t info;
	gp_nand_phy_partition_t Test_partition;
	gp_nand_write_buffer_t test_buf;

	fd = open(TEST_DEVICE_NAME, O_RDWR);
	if( fd <= 0 )
	{
		SPMP_DEBUG_PRINT("Open Block file failed\n");
		return -1;
	}
	SPMP_DEBUG_PRINT("Begin\n" );
	sleep(1);
	if(ioctl(fd, IOCTL_NAND_GET_BLK_INFO, &info)!=0)
	{
		SPMP_DEBUG_PRINT("IOCTl failed\n");
		close(fd);
		return -1;
	}
	SPMP_DEBUG_PRINT("info.block_size=%d, info.page_size=%d  RW_Func=%u, RW_flag=%u\n", 
				info.block_size, info.page_size, RW_Func, RW_flag);
	//sleep(1);

	//close(fd); return 0;
	
	Test_partition.offset = LINUX_IMAGE_NF_STARTBLK;
	Test_partition.size = info.block_size;
	if( (RW_flag & 0x01) == 0x01)
		i = ioctl(fd, IOCTL_NAND_PHY_OPEN, &Test_partition);
	else
		i = ioctl(fd, IOCTL_NAND_ISP_OPEN, &Test_partition);
	if(i!=0)
	{
		SPMP_DEBUG_PRINT("IOCTl failed\n");
		close(fd);
		return -1;
	}

	test_buf.buf = malloc(info.page_size);
	test_buf.size = info.page_size;

	if( (RW_flag & 0x01) == 0x01)
	{
		SPMP_DEBUG_PRINT("write:\n");
		for(i=0, j=0; i<Test_partition.size; i+=test_buf.size, j++)
		{
			memset(test_buf.buf, (i>>12)%0xff, test_buf.size);
			switch(RW_Func)
			{
				case 1:
					ioctl(fd, IOCTL_NAND_PHY_WRITE, &test_buf);
					break;
				case 2:
					ioctl(fd, IOCTL_NAND_ISP_WRITE_1K, &test_buf);
					break;
				case 3:
					ioctl(fd, IOCTL_NAND_ISP_WRITE_PAGE, &test_buf);
					break;
			}
			printf("%02x ", *(unsigned char *)test_buf.buf);
			if(j%16 == 0)
				printf("\n");
		}
		printf("\n");
	}
	if( (RW_flag & 0x02) == 0x02)
	{
		SPMP_DEBUG_PRINT("Read:\n");
		for(i=0, j=0; i<Test_partition.size; i+=test_buf.size, j++)
		{
			memset(test_buf.buf, 0, test_buf.size);
			
			switch(RW_Func)
			{
				case 1:
					ioctl(fd, IOCTL_NAND_PHY_READ, &test_buf);
					break;
				case 2:
					ioctl(fd, IOCTL_NAND_ISP_READ_1K, &test_buf);
					break;
				case 3:
					ioctl(fd, IOCTL_NAND_ISP_READ_PAGE, &test_buf);
					break;
			}
			printf("%02x ", *(unsigned char *)test_buf.buf);
			if(j%16 == 0)
				printf("\n");
		}
		printf("\n");
	}
	ioctl(fd, IOCTL_NAND_PHY_CLOSE, &Test_partition);//close
	close(fd);

	return 0;
}


#if 1
#define SUPPORT_NF_TEST_CMD_NUM 32
typedef struct nf_test_cmd_head_{
	char cmd;
	char para_num;
	char para_len;
}nf_test_cmd_head;

typedef struct nf_test_cmd_{
	nf_test_cmd_head head;
	char hint[64];
	unsigned long para[8];
}nf_test_cmd;

#define NF_TEST_CMD_PW_PWRITE		6
#define NF_TEST_CMD_PR_PREAD		7
#define NF_TEST_CMD_LW_SWRITE		8
#define NF_TEST_CMD_LR_SREAD		9
#define NF_TEST_CMD_PW_SWRITE		10
#define NF_TEST_CMD_PR_SREAD		11

#define NF_TEST_CMD_FILE_READ		13

#define NF_TEST_CMD_INVALIDATE	15
#define NF_TEST_CMD_FDISK			16
#define NF_TEST_CMD_SETDBGPRT		17
#define NF_TEST_CMD_CACHEFLUSH	18
#define NF_TEST_CMD_FILLDISK		19
#define NF_TEST_CMD_WRITE_FIXEDLOG		20

nf_test_cmd nf_cmd[SUPPORT_NF_TEST_CMD_NUM]={
	{{1, 3, 12},"logic sector: write"},	//para: dst start_sector, sector_num, pattern
	{{2, 3, 12},"logic sector:  read"},	//para: dst start_sector, sector_num, display_start, display_len
	
	{{3, 3, 12},"logic page: write"},	//para: dst start_page, page_num, pattern
	{{4, 3, 12},"logic page:  read"},	//para: dst start_page, page_num, display_start, display_len
	
	{{5, 3, 12},"logic block: write"},	//para: dst start_block, page_block, pattern
	{{6, 3, 12},"logic block:  read"},	//para: dst start_block, page_block, display_start, display_len

	{{7, 3, 12},"physical page: write"},	//para: dst start_block, page_block, pattern
	{{8, 2, 8}, "physical page:  read"},	//para: dst start_block, page_block, display_start, display_len

	{{9, 3, 12},"logic random: write"},	//para: dst start_block, page_block, pattern
	{{10, 2, 8},"logic random:  read"},	//para: dst start_block, page_block, display_start, display_len
	{{11, 3, 12},"physical sector random: write"},	//para: dst start_block, page_block, pattern
	{{12, 3, 12},"physical sector random: read"},	//para: dst phys addr, display_len

	{{13, 3, 12},"one file:  write"},	//para: dst start_block, page_block, pattern
	{{14, 3, 12},"one file:   read"},	//para: dst start_block, page_block, display_start, display_len
	{{15, 3, 12},"one file:compare"},	//para: dst start_block, page_block, display_start, display_len

	{{20, 0, 0},"Set Nand invalidate"},	//para: dst start_block, page_block, display_start, display_len
	{{21, 0, 0},"fdisk nand"},	//para: dst start_block, page_block, display_start, display_len
	{{22, 1, 4},"Set debug info print"},	//para: dst start_block, page_block, display_start, display_len
	{{23, 0, 0},"Flush Cache Data"},	//para: dst start_block, page_block, display_start, display_len
	{{24, 2, 8},"Write All User sector"},	//para: dst start_block, page_block, display_start, display_len
	{{25, 2, 8},"Write Fixed logic sector"},	//para: dst start_block, page_block, display_start, display_len
	{{26, 0, 0},"write physical partition"}, 

	{{0xff, 0, 0},"return"}
};
int nf_test_do_cmd(nf_test_cmd* cmd)
{
	int i = 0;
	unsigned char *para= (unsigned char *)cmd->para;
	printf("Head:\n\tcmd=%d\n", cmd->head.cmd);
	printf("\tpara_num=%d\n", cmd->head.para_num);
	printf("\tpara_len=%d\n", cmd->head.para_len);
	printf("\tHint:%s\n\tpara:", cmd->hint);
	
	for(i=0; i<cmd->head.para_len; i++)
		printf("%02x ", para[i]);
	printf("\n");
	
	return Test_KernelNand_Ioctl(NF_IOCTL_TEST, (unsigned long)cmd);
}

void nf_test_print_menu(void)
{
	int i = 0;
	
	for(i=0; i<SUPPORT_NF_TEST_CMD_NUM; i++)
	{
		if(nf_cmd[i].head.cmd != 0)
			printf("%03d - %s(para num:%d)\n", nf_cmd[i].head.cmd,nf_cmd[i].hint,nf_cmd[i].head.para_num);
	}
	return;
}
int cmd_wait(void)
{
	char ch;

	return -1;
}
void nf_test_do_menu(void)
{
	char cmd_select;
	int cmd = 0;
	while(1)
	{
		nf_test_print_menu();
		cmd = cmd_wait();
		if( cmd < 0)
			break;
		nf_test_do_cmd(nf_cmd + cmd);
	}
}
#endif


int main(int argc, char *argv[])
{
	int test_type = 0;
	int i = 0, cnt = 0;

	if(argc<2)
		test_type = 0xff;
	else
		test_type = atoi(argv[1]);

	switch(test_type)
	{
	/*
		case 'm':
			if( argc > 2 )
				cnt = atol(argv[2]);
			else
				cnt = 2;
			for(i=0; i < cnt;i++)
				Test_MultiNormalFile(0x03);
			break;
		case 'r':
			Test_singleHugeFile_Read();
			break;
		case 'c':
			//while(1)
			Test_MultiNormalFile(0x02);
			break;
		case 'C':
			Test_EnableKernelDebug();	
			break;
		case 'e'://erease all
			Test_KernelNand_Ioctl(NF_IOCTL_EREASEALL, 0);

			break;
		//case 'E':
		//	Test_KernelNand_Ioctl(0x40002, 0);
		//	break;
		//case 'd':
		//	Test_KernelNand_Ioctl(0x40003, 0);
		//	break;
		case 's':
			Test_singleHugeFile();
			break;
	*/
		
		case 22:
			//Test_KernelNand_Ioctl(0x40004, 3);
			if(argc<3)
				break;
			Test_KernelDebug_Set(atoi(argv[2]));
			break;
		case 14:
			if(argc<5)
			{
				printf("filename startaddr len [debug_option]\n");
				break;
			}
			if(argc>5)
				Test_DisplayFile(argv[2], atol(argv[3]), atol(argv[4]), atol(argv[5]));
			else
				Test_DisplayFile(argv[2], atol(argv[3]), atol(argv[4]), 0);
			break;
		case 11://random logic WRITE
		{
			if(argc<6)
			{
				printf("nf_test cmd PageAddr SectorAddr Length Pattern\n");
				break;
			}
			memset((unsigned char *)nf_cmd[NF_TEST_CMD_PW_SWRITE].para, 0, 4*8);
			nf_cmd[NF_TEST_CMD_PW_SWRITE].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_PW_SWRITE].para[1] = atol(argv[3]);
			nf_cmd[NF_TEST_CMD_PW_SWRITE].para[2] = atol(argv[4]);
			nf_cmd[NF_TEST_CMD_PW_SWRITE].para[3] = atol(argv[5]);
			
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_PW_SWRITE);
		}
			break;
		case 12://random physical sector read
		{
			if(argc<5)
			{
				printf("nf_test cmd PageAddr SectorAddr Length\n");
				break;
			}
			memset((unsigned char *)nf_cmd[NF_TEST_CMD_PR_SREAD].para, 0, 4*8);
			nf_cmd[NF_TEST_CMD_PR_SREAD].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_PR_SREAD].para[1] = atol(argv[3]);
			nf_cmd[NF_TEST_CMD_PR_SREAD].para[2] = atol(argv[4]);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_PR_SREAD);
		}
			break;
		
		case 8://physical page read
		{
			if(argc<4)
			{
				printf("nf_test cmd Addr Length\n");
				break;
			}
			memset((unsigned char *)nf_cmd[NF_TEST_CMD_PR_PREAD].para, 0, 4*8);
			nf_cmd[NF_TEST_CMD_PR_PREAD].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_PR_PREAD].para[1] = atol(argv[3]);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_PR_PREAD);
		}
			break;
		case 9://random logic WRITE
		{
			if(argc<5)
			{
				printf("nf_test cmd SectorAddr Length Pattern\n");
				break;
			}
			memset((unsigned char *)nf_cmd[NF_TEST_CMD_LW_SWRITE].para, 0, 4*8);
			nf_cmd[NF_TEST_CMD_LW_SWRITE].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_LW_SWRITE].para[1] = atol(argv[3]);
			nf_cmd[NF_TEST_CMD_LW_SWRITE].para[2] = atol(argv[4]);
			
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_LW_SWRITE);
		}
			break;
		case 10://random logic read
		{
			if(argc<4)
			{
				printf("nf_test cmd SectorAddr Length\n");
				break;
			}
			memset((unsigned char *)nf_cmd[NF_TEST_CMD_LR_SREAD].para, 0, 4*8);
			nf_cmd[NF_TEST_CMD_LR_SREAD].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_LR_SREAD].para[1] = atol(argv[3]);
			
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_LR_SREAD);
		}
			break;
		case 20://set nand invalidate
		{
			memset(nf_cmd[NF_TEST_CMD_INVALIDATE].para, 0, 32);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_INVALIDATE);
		}
			break;
		case 21://fdisk
		{
			memset(nf_cmd[NF_TEST_CMD_FDISK].para, 0, 32);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_FDISK);
		}
			break;
		case 23://CACHE FLUSH
		{
			memset(nf_cmd[NF_TEST_CMD_CACHEFLUSH].para, 0, 32);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_CACHEFLUSH);
		}
			break;
		case 24://NF_TEST_CMD_FILLDISK
		{
			if(argc<4)
			{
				printf("nf_test cmd SectorAddr Length\n");
				break;
			}
			memset(nf_cmd[NF_TEST_CMD_FILLDISK].para, 0, 32);
			nf_cmd[NF_TEST_CMD_FILLDISK].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_FILLDISK].para[1] = atol(argv[3]);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_FILLDISK);
		}
			break;
		case 25://¹Ì¶¨Ð´Âß¼­¿é
		{
			if(argc<4)
			{
				printf("nf_test cmd SatrtSectorAddr writeNum\n");
				break;
			}
			memset(nf_cmd[NF_TEST_CMD_WRITE_FIXEDLOG].para, 0, 32);
			nf_cmd[NF_TEST_CMD_WRITE_FIXEDLOG].para[0]= atol(argv[2]);
			nf_cmd[NF_TEST_CMD_WRITE_FIXEDLOG].para[1]= atol(argv[3]);
			nf_test_do_cmd(nf_cmd+NF_TEST_CMD_WRITE_FIXEDLOG);
		}
			break;
		case 26:
			if(argc<4)
			{
				printf("nf_test 26 RW_Func RW_Flag\n");
				break;
			}
			Test_NF_PHYS_PartitionWrite(atol(argv[2]), atol(argv[3]));
			break;
		case 27:
			if(argc<3)
			{
				printf("nf_test 27 debug_Flag(OCT)\n");
				break;
			}
			Test_EnableKernelDebug(atoi(argv[2]));
			break;
		default:
			nf_test_print_menu();
			break;
	}
	return 0;
}

