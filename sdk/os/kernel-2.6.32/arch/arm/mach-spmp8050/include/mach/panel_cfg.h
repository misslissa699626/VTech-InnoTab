#define gPanelBacklightActiveLow 0  //temporay~~

void Panel_ctrl_on(void);
void Panel_ctrl_off(void);
void Panel_ctrl_WR(int cmd_bit, int cmd, int data_bit, int dat, int dummy_bit);
void Panel_ctrl_backlight_en(int aOnOff);
