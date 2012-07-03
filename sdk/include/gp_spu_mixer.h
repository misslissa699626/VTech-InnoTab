#ifndef __GP_SPU_MIXER__
#define __GP_SPU_MIXER__

#define GP_SPU_LOOP_SIZE						(2*8*1024)//bytes
#define GP_SPU_QUEU_SIZE						6

typedef struct// gp_mixer
{	
	int  fd_mixer;
	unsigned int  hardware_ch;	
		
	char ch_name[32];					//mixer channle name , for AP break on user mode
	int sample_spec_type;
	int sample_spec_freq;
	
	int  fd_mem;		
	
	int  		iLoopQueuSize;		
	int  		iLoopBufLen;		
	
	int			underrun;
	
	int			wave_play_block;
	int			wave_write_block;
	int			wave_block_write_pos;
	
	unsigned int		l_wavedata_addr;	//left channel , must chuck memory
	unsigned int		r_wavedata_addr;	//right channel ,  must chuck memory

	unsigned int		l_wavedata_phy_addr;	//left channel , chuck memory physical adderss
	unsigned int		r_wavedata_phy_addr;	//right channel , chuck memory  physical adderss
}st_gp_mixer;

typedef struct gp_mixer_cmd
{	
	int  hardware_ch;	
	int  parameter;	
}st_gp_mixer_cmd;

//----------------------------------------------------------------------------------------------------------------
#define MIXER_CMD_SET_VOL							_IOW('D', 0x8003, st_gp_mixer_cmd)	//0x8303
#define MIXER_CMD_PAUSE								_IOW('D', 0x8004, st_gp_mixer_cmd)
#define MIXER_CMD_RESUME								_IOW('D', 0x8005, st_gp_mixer_cmd)
#define MIXER_CMD_STOP								_IOW('D', 0x8006, st_gp_mixer_cmd)

#define MIXER_NEW_CHANNEL							_IOW('D', 0x9000, st_gp_mixer)	//0x9000
#define MIXER_RELEASE_CHANNEL						_IOW('D', 0x9001, st_gp_mixer)	//0x9001
#define MIXER_GET_READ_POS_CHANNEL					_IOW('D', 0x9002, st_gp_mixer)	//0x9002
#define MIXER_SET_WRITE_POS_CHANNEL				_IOW('D', 0x9003, st_gp_mixer)	//0x9003

#define MIXER_GET_LOOP_BLOCK_CHANNEL				_IOW('D', 0x9004, int)	//0x9003

//channel format 
#define FORMAT_2CH_SS_S16		0	//SIGNED L16,R16,L16,R16...
#define FORMAT_2CH_SS_U16		1	//UNSIGNED L16,R16,L16,R16...
#define FORMAT_2CH_PS_S16		2	//SIGNED lbuffer: L16,L16,L16...  rbuffer:R16,R16,R16...
#define FORMAT_2CH_PS_U16		3	//UNSIGNED lbuffer: L16,L16,L16...  rbuffer:R16,R16,R16...
#define FORMAT_2CH_PM_S16		4	//SIGNED lbuffer: L16,L16,L16...  rbuffer:NULL
#define FORMAT_2CH_PM_U16		5	//UNSIGNED lbuffer: L16,L16,L16...  rbuffer:NULL


/*  gp_mixer */
extern st_gp_mixer *gp_channel_new(char *name, int sample_spec_type, int sample_spec_freq, int queu_size, int loop_len_nK);
extern int 	gp_channel_write(st_gp_mixer *mixer_ch, void *buffer_l, void *buffer_r,int iDataLen);
extern unsigned int gp_channel_get_latency(st_gp_mixer * mixer_ch);
extern unsigned int gp_channel_get_empty_size(st_gp_mixer * mixer_ch);
extern void gp_channel_close(st_gp_mixer *mixer_ch);
extern int gp_channel_dev_type();
extern int gp_channel_cmd(st_gp_mixer * mixer_ch, int cmd, int param);

#endif

