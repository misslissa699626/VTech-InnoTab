void audio2_Power_ctrl(int On);

unsigned int audio2_Adins_get(void);

unsigned int audio2_HPINS_get(void);
void audio2_HPINS_set(unsigned int hpins);

void audio2_WAVE_ctrl(unsigned int enable);
void audio2_WAVE_volset(long l_vol,long r_vol);
void audio2_WAVE_volget(long *l_vol,long *r_vol);
long audio2_WAVE_volmax(void);
void audio2_WAVE_muteget(long *l_mute,long *r_mute);
void audio2_WAVE_muteset(long l_mute,long r_mute);
int audio2_WAVE_ismute(void);
/**********************************************************
*  LINEOUT Section
***********************************************************/
void audio2_LINEOUT_volset(long l_vol,long r_vol);
void audio2_LINEOUT_volget(long *l_vol,long *r_vol);
long audio2_LINEOUT_volmax(void);
void audio2_LINEOUT_muteget(long *l_mute,long *r_mute);
void audio2_LINEOUT_muteset(long l_mute,long r_mute);
/**********************************************************
*  LINEIN Section
***********************************************************/
void audio2_LINEIN_ctrl(unsigned int enable);
void audio2_LINEIN_volset(long l_vol,long r_vol);
void audio2_LINEIN_volget(long *l_vol,long *r_vol);
long audio2_LINEIN_volmax(void);
/**********************************************************
*  MIC Section
***********************************************************/
void audio2_MIC_ctrl(unsigned int enable);
void audio2_MIC_volset(long l_vol);
void audio2_MIC_volget(long *l_vol);
long audio2_MIC_volmax(void);

int audio2_FREQ_set(unsigned int a_AudSampleRate, int play);
/**********************************************************
*  Headphone Section
***********************************************************/
void audio2_HDPHN_volset(long l_vol,long r_vol);
void audio2_HDPHN_volget(long *l_vol,long *r_vol);

void audio2_suspend(void);
void audio2_resume(void);
