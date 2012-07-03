void audio_Power_ctrl(int On);

unsigned int audio_Adins_get(void);

unsigned int audio_HPINS_get(void);
void audio_HPINS_set(unsigned int hpins);

void audio_WAVE_ctrl(unsigned int enable);
void audio_WAVE_volset(long l_vol,long r_vol);
void audio_WAVE_volget(long *l_vol,long *r_vol);
long audio_WAVE_volmax(void);
void audio_WAVE_muteget(long *l_mute,long *r_mute);
void audio_WAVE_muteset(long l_mute,long r_mute);
/**********************************************************
*  LINEOUT Section
***********************************************************/
void audio_LINEOUT_volset(long l_vol,long r_vol);
void audio_LINEOUT_volget(long *l_vol,long *r_vol);
long audio_LINEOUT_volmax(void);
void audio_LINEOUT_muteget(long *l_mute,long *r_mute);
void audio_LINEOUT_muteset(long l_mute,long r_mute);
/**********************************************************
*  LINEIN Section
***********************************************************/
void audio_LINEIN_ctrl(unsigned int enable);
void audio_LINEIN_volset(long l_vol,long r_vol);
void audio_LINEIN_volget(long *l_vol,long *r_vol);
long audio_LINEIN_volmax(void);
/**********************************************************
*  MIC Section
***********************************************************/
void audio_MIC_ctrl(unsigned int enable);
void audio_MIC_volset(long l_vol);
void audio_MIC_volget(long *l_vol);
long audio_MIC_volmax(void);

int audio_FREQ_set(unsigned int a_AudSampleRate, int play);
/**********************************************************
*  Headphone Section
***********************************************************/
void audio_HDPHN_volset(long l_vol,long r_vol);
void audio_HDPHN_volget(long *l_vol,long *r_vol);
