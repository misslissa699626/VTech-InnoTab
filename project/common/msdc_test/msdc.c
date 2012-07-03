#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mach/typedef.h>
#include <sys/ioctl.h>
#include <mach/gp_usb.h>

typedef struct mount_s{
	char mount_name[128];
	char dev_name[128];
}mount_t;

enum{
	MSDC_DETECT = 0,
	MSDC_CONNECT,
	MSDC_CONFIG,
	MSDC_CONFIG_FAIL,
	MSDC_IN,
	MSDC_REMOVE,
	USB_POWER_ON,
};


int usb_fd;
//char mount_name[128];
//char dev_name[128];
int dev_count = 0;
struct mount_s *path;

int gp_usb_detect(void)
{
	int tmp;

	ioctl(usb_fd, USBDEVFS_VBUS_HIGH, &tmp);
	//printf("HOST VBS [%d]\n",tmp);
	return tmp;
}

int umount_dev()
{
	char buffer[256];
	char file[256] = {0};
	FILE*   fp=NULL;
	char* ptr = NULL;
	int i;

	for(i = 0; i < dev_count; i++){
		sprintf(buffer,"mount | grep -w \"%s\"", path[i].mount_name);
		fp = popen(buffer, "r");
		if( NULL == fp ) {
			printf("mount name error = %s\n", path[i].mount_name);
			continue;
		}

		fread( buffer, sizeof(unsigned char), sizeof(buffer), fp);
		pclose(fp);
		ptr = strstr(buffer, " on ");
		if( NULL == ptr ) {
			printf("can't find dev = %s\n", path[i].mount_name);
			continue;
		}

		memset(path[i].dev_name, 0, sizeof(path[i].dev_name));
		strncpy(path[i].dev_name, buffer, ptr-buffer);

		sprintf(buffer,"umount %s", path[i].mount_name);
		system(buffer);
		
		if(i == 0){
			sprintf(file, "%s", path[i].dev_name);
		}
		else{
			sprintf(file, "%s,%s", file,path[i].dev_name);
		}
	}

	sprintf(buffer,"modprobe g_spmp_storage file=%s stall=0 removable=1", file);
	system(buffer);

	return 0;
}

void mount_dev()
{
	char buffer[128];
	int i;

	for(i = 0; i < dev_count; i++){
		sprintf(buffer,"mount -t vfat -o utf8 %s %s", path[i].dev_name, path[i].mount_name);
		system(buffer);
	}

	system("modprobe -r g_spmp_storage");

}

void main(int argc, char *argv[])
{
	int tmp;
	int count = 0;
	unsigned int delay_time = 1000000;
	int mode = 0;
	int vbs;
	int i;

	if(argc < 2){
		printf("please input dev\n");
		printf("example:	./msdc /media/nanda /media/sdcard\n\n");
		return;
	}

	dev_count = argc -1;

	usb_fd = open("/dev/usb_device",O_RDWR);

	if(usb_fd < 0){
		printf("can't open usb node\n");
		return;
	}

	path = malloc(sizeof(mount_t)*dev_count);
	for(i = 0; i< dev_count; i++){
		sprintf(path[i].mount_name, "%s", argv[i+1]);
	}
	system("modprobe -r g_spmp_storage");

	while(1){
		vbs = gp_usb_detect();
		if(!vbs){
			if((mode > MSDC_CONNECT)&&(mode != USB_POWER_ON)){
				mount_dev();
			}
			mode = MSDC_DETECT;
		}

		switch(mode){
			case MSDC_DETECT:
				delay_time = 1000000;
				if(vbs){
					mode++;
					delay_time = 10000;
				}
				break;

			case MSDC_CONNECT:
				if(umount_dev()){
					exit(0);
				}

				tmp = 1;
				ioctl(usb_fd, USBDEVFS_SW_CONNECT_SET, &tmp);
				mode++;
				break;

			case MSDC_CONFIG:
				ioctl(usb_fd, USBDEVFS_HOST_CONFIGED, &tmp);
	
				if(tmp){
					mode = MSDC_IN;
					printf("MSDC IN\n");
					count = 0;
				}
				else{
					count ++;

					if(count > 100){
						count = 0;
						mode = MSDC_CONFIG_FAIL;
					}
				}
				break;

			case MSDC_CONFIG_FAIL:
				mount_dev();
				mode = USB_POWER_ON;
				delay_time = 1000000;
				tmp = 0;
				ioctl(usb_fd, USBDEVFS_SW_CONNECT_SET, &tmp);
				printf("usb power on\n");
				break;
			
			case MSDC_IN:
				//printf("MSDC IN\n");
				ioctl(usb_fd, USBDEVFS_HOST_SAFTY_REMOVED, &tmp);
				if(tmp){
					mount_dev();
					mode = MSDC_REMOVE;
				}

				ioctl(usb_fd, USBDEVFS_HOST_CONFIGED, &tmp);
				if(!tmp){
					printf("error: MSDC IN but config is zero\n");
					count ++;
					if(count >4){
						mount_dev();
						mode = MSDC_REMOVE;
					}
				}
				else{
					count = 0;
				}
				delay_time = 1000000;
				break;

			case MSDC_REMOVE:
				tmp = 0;
				ioctl(usb_fd, USBDEVFS_SW_CONNECT_SET, &tmp);
				delay_time = 1000000;
				//printf("MSDC IN\n");
				break;

			case USB_POWER_ON:
				delay_time = 1000000;		
				//printf("usb power on\n");
				break;
				
		}

		usleep(delay_time);
	}

	close(usb_fd);
	free(path);

}




