#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/platform_device.h>
#include <asm/current.h>
#include <linux/sched.h>

#include <mach/irqs.h>
#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>
#include <mach/spu.h>
#include <mach/hal/hal_clock.h>

#include "SpuDrv.h"
#include "Audio.h"

//#define DEBUG_PRINT	printk
#define DEBUG_PRINT(...)

#define ERROR_PRINT	printk
//#define ERROR_PRINT(...)

//#define TRACE_LINE	{DEBUG_PRINT(KERN_WARNING "{TRACE_LINE %s:%d:%s()}\n", __FILE__, __LINE__, __FUNCTION__);}
#define TRACE_LINE	

#define OOPS	{*(int*)0 = 0;}

#define	SPU_MINOR				0
#define SPU_NR_DEVS				1
#define VIC_SPU					(19+32)

#define SPU_REGBASE		0x93000000
#define SPU_REG_START	0x9300BE00
#define SPU_REG_END		0x9300D000
#define SPU_REG_LEN		(SPU_REG_END - SPU_REG_START)

#define TOTAL_N_CHANNELS	32

int n_ch_total = TOTAL_N_CHANNELS;
int n_ch_midi = 16;
int n_stream = 4;

int wave_ch_start = 0;
int wave_ch_end = 0;
int stream_ch_start = 0;
int stream_ch_end = 0;
int midi_ch_start = 0;
int midi_ch_end = 0;


typedef struct spu_dev_s {
	struct cdev c_dev;
	int n_open;
	wait_queue_head_t spu_wait_queue;
	char *ioremap_request_ptr;
	char *ioremap_ptr;
	char *ioremap_dac_ptr;
	int ch_pid[TOTAL_N_CHANNELS];
} spu_dev_t;

int spu_major;
spu_dev_t *spu_device = 0;
struct class *spu_class;

int spu_fops_open(struct inode *inode, struct file *filp);
int spu_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg);
int spu_fops_release(struct inode *inode, struct file *filp);
ssize_t spu_fops_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t spu_fops_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
//int spu_fasync(int fd, struct file *filp, int mode);

int Audio_translateChToStreamHandle(int ch);		//defined in Audio.c
int Audio_translateStreamHandleToCh(int handle);	//defined in Audio.c

static int stub_Audio_allocStream(pid_t pid);
static void stub_Audio_freeStream(int arg);




static irqreturn_t isr_spu(int irq, void *dev_id);

struct file_operations spu_fops = {
	.owner = THIS_MODULE,
	//.poll = vppu_poll,
	.open = spu_fops_open,
	.ioctl = spu_fops_ioctl,
	.release = spu_fops_release,
	.read = spu_fops_read,
	.write = spu_fops_write,
//	.fasync = spu_fasync;
};

unsigned int spu_ioremap_ptr(void)
{
	return (unsigned int)(spu_device->ioremap_ptr);
}

unsigned int USER_VA_TO_KERNEL_VA(unsigned int uva)
{
	if (uva) {
		unsigned int pa = gp_user_va_to_pa((void*)uva);			/* user_addr to phy_addr */
		if (pa) {
 			unsigned int ka = (unsigned int)gp_chunk_va(pa);	/* phy_addr to kernel_addr */
			if (ka) {
				return ka;
			}
		}
		printk(KERN_WARNING "%s:%s(): ERROR! Address (0x%08X) does not in chunkmem!\n", __FILE__, __FUNCTION__, uva);
		OOPS
	}
	return 0;
}

int _init_ioremap(void)
{
	if (spu_device->ioremap_request_ptr == 0) {
		//request_mem_region
#if 0
		if (request_mem_region(SPU_REG_START, SPU_REG_LEN, "SPU") == 0) {
			return -1;
		}
#endif
		//ioremap
		spu_device->ioremap_request_ptr = (char*)ioremap_nocache(SPU_REG_START, SPU_REG_LEN);
		if (spu_device->ioremap_request_ptr == 0) {
			return -1;
		}
		spu_device->ioremap_ptr = spu_device->ioremap_request_ptr - 0xBE00;
		DEBUG_PRINT(KERN_WARNING "SPU: ioremap(0x%08X, 0x%08X) returns 0x%08X\n", SPU_REG_START, SPU_REG_LEN, (unsigned int)spu_device->ioremap_request_ptr);
		
		//Audio DAC is not initialized here, it should be done in dac-jack
	}
	return 0;
}

int _term_ioremap(void)
{
	if (spu_device->ioremap_request_ptr) {
		//iounmap
		iounmap(spu_device->ioremap_request_ptr);
		spu_device->ioremap_request_ptr = 0;
		spu_device->ioremap_ptr = 0;
#if 0
		//release_mem_region
		release_mem_region(SPU_REG_START, SPU_REG_LEN);
#endif

		//Audio DAC
		if (spu_device->ioremap_dac_ptr) {
			iounmap(spu_device->ioremap_dac_ptr);
			spu_device->ioremap_dac_ptr = 0;
		}
				
	}
	return 0;
}

int spu_fops_open(struct inode *inode, struct file *filp)
{
	spu_device->n_open++;
	filp->private_data = (void*)(current->pid);
	
	DEBUG_PRINT(KERN_WARNING "SPU open \n");
	return 0;
}

int spu_fops_release(struct inode *inode, struct file *filp)
{
	spu_device->n_open--;
	if (spu_device->n_open <= 0) {
		spu_device->n_open = 0;
	}

	{
		int i;
		pid_t *ch_pid = spu_device->ch_pid;
		pid_t pid = (pid_t)(filp->private_data);

		//WAVE channels
		for (i = wave_ch_start; i < wave_ch_end; i++) {
			if (ch_pid[i] == pid) {
				Spu_stopWave(i);
				ch_pid[i] = 0;
			}
		}

		//MIDI channels
		if (ch_pid[midi_ch_start] == pid) {
			Audio_cleanupMidi();
			for (i = midi_ch_start; i < midi_ch_end; i++) {
				ch_pid[i] = 0;
			}
		}

		//STREAM channels
		//DEBUG_PRINT(KERN_WARNING "%s():%d: stop stream channels of pid=%d\n", __FUNCTION__, __LINE__, pid);
		for (i = stream_ch_start; i < stream_ch_end; i += 2) {
			if (ch_pid[i] == pid) {
				int handle = Audio_translateChToStreamHandle(i);
				DEBUG_PRINT(KERN_WARNING "Audio_translateChToStreamHandle(%d) returns 0x%08X\n", i, handle);
				if (handle >= 0) {
					Audio_closeStream(handle);
					/*while (Audio_getStreamPlayStatus(handle) == AUDIO_PLAY_BUSY) {
						set_current_state(TASK_INTERRUPTIBLE);
						schedule_timeout(HZ * 100 / 1000);
					}*/
					Audio_freeStream(handle);
				}
				ch_pid[i] = 0;
				ch_pid[i + 1] = 0;
			}
		}
		
	}

	/* Success */
	DEBUG_PRINT(KERN_WARNING "SPU release \n");
	return 0;
}






static void spu_device_release(struct device *dev)                       
{                                                                           
	DIAG_INFO("remove vppu device ok\n");                                      
}                                                                           

static struct resource spu_resources[] = {
	[0] = {
		.start  = SPU_REG_START,
		.end	= SPU_REG_END - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device spu_platform_device = {                             
	.name	= "spu",                                                         
	.id	= 0,                                                                  
	.dev	= {                                                                 
		.release = spu_device_release,                                       
	},                                                                        
	.num_resources  = ARRAY_SIZE(spu_resources),
	.resource       = spu_resources,
};   

static struct platform_driver spu_platform_driver = {                                                                             
	//.suspend = spu_suspend,                                                
	//.resume = spu_resume,                                                  
	.driver	= {                                                               
		.owner	= THIS_MODULE,                                                  
		.name	= "spu"                                                        
	}                                                                     
};                                                                          



void __exit spu_module_exit(void)
{
	Audio_cleanup();
	free_irq(VIC_SPU, (void*)spu_device);
	_term_ioremap();

	{
		dev_t devno = MKDEV(spu_major, SPU_MINOR);
		cdev_del(&(spu_device->c_dev));
		kfree(&spu_device);
		unregister_chrdev_region(devno, SPU_NR_DEVS);
	
		platform_device_unregister(&spu_platform_device);
		platform_driver_unregister(&spu_platform_driver);
	}
	DEBUG_PRINT(KERN_WARNING "SPU module exit \n");
}

int __init spu_module_init(void)
{
	int result;
	dev_t dev;
	int devno;

	result = alloc_chrdev_region(&dev, SPU_MINOR, 1, "SPU");
	if (result < 0) {
		ERROR_PRINT(KERN_WARNING "SPU: can't get major \n");
		return result;
	}
	spu_major = MAJOR(dev);
	spu_class = class_create(THIS_MODULE, "SPU");
	
	spu_device = kmalloc(sizeof(spu_dev_t), GFP_KERNEL);
	if (!spu_device) {
		ERROR_PRINT(KERN_WARNING "SPU: kmalloc failed \n");
		result = -ENOMEM;
		goto fail;
	}
	memset(spu_device, 0, sizeof(spu_dev_t));
	
	devno = MKDEV(spu_major, SPU_MINOR);
	cdev_init(&(spu_device->c_dev), &spu_fops);
	spu_device->c_dev.owner = THIS_MODULE;
	spu_device->c_dev.ops = &spu_fops;
	result = cdev_add(&(spu_device->c_dev), devno, 1);
	device_create(spu_class, NULL, devno, NULL, "spu%d", 0);
	
	if (result)
		ERROR_PRINT(KERN_WARNING "Error adding spu");
  
	init_waitqueue_head(&spu_device->spu_wait_queue);
  
	platform_device_register(&spu_platform_device);
	result = platform_driver_register(&spu_platform_driver);

	if (spu_device->ioremap_ptr == 0) {
		if (_init_ioremap() < 0) {
			_term_ioremap();
			ERROR_PRINT(KERN_WARNING "SPU: failed in _init_ioremap()\n");
			result = -1;
			goto fail;
		}
		if (request_irq(VIC_SPU, isr_spu, 0, "SPU", (void*)spu_device) != 0) {
			_term_ioremap();
			ERROR_PRINT(KERN_WARNING "SPU: failed to install IRQ handler\n");
			result = -1;
			goto fail;
		}
	}





	///////////////////////////////////////////////////////////////////////
	// Begin of enable SPU clock, refer to hal_spu.c gpHalSpuClkEnable()
	///////////////////////////////////////////////////////////////////////
	gpHalScuClkEnable(SCU_A_PERI_SPU, SCU_A, 1);

	//gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaSysSel), 0x02000080);
	{ 
		#define P_SCUA_SYS_SEL		0x930070E0
		unsigned int *p = (unsigned int*)ioremap_nocache(P_SCUA_SYS_SEL, 4);
		if (!p) {
			ERROR_PRINT(KERN_WARNING "VPPU: failed to set P_SCUC_PERI_CLKEN.22 = 1\n");
		} else {
			unsigned int n = ioread32(p);
			n |= 0x02000080;
			iowrite32(n, p);
			iounmap(p);				
		}
	}
	
	//gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuPostWavCtrl), 0x100);
	#define P_SPU_CtrPW		0x9300BE94
	iowrite32(0x100, spu_device->ioremap_ptr + (P_SPU_CtrPW - SPU_REGBASE));
	
	//vicsiu: below shall be handled by dac-jack
	//gpHalSPU_RegWrite((unsigned int)&(piisReg->iisCtrl), 0x104421);
	//gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacHdpHn), 0xFFFFFFFF, 0x18f);
	//gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacCtrl), 0xFFFFFFFF, 0xd03);
	//gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacPwrCtrl), 0xFFFFFFFF, 0x03);
	///////////////////////////////////////////////////////////////////////
	// End of enable SPU clock, refer to hal_spu.c gpHalSpuClkEnable()
	///////////////////////////////////////////////////////////////////////







	wave_ch_start = 0;
	wave_ch_end = wave_ch_start + n_ch_total - n_ch_midi - n_stream * 2;
	stream_ch_start = wave_ch_end;
	stream_ch_end = stream_ch_start + n_stream * 2;
	midi_ch_start = stream_ch_end;
	midi_ch_end = midi_ch_start + n_ch_midi;

	Audio_init(n_ch_total, n_ch_midi, n_stream);

	DEBUG_PRINT(KERN_WARNING "SPU module init\n");
	return result;

fail:
	ERROR_PRINT(KERN_WARNING "SPU module init failed \n");
	kfree(spu_device);
	unregister_chrdev_region(dev, SPU_NR_DEVS);
	return result;
}


static int check_pid_ch(pid_t pid, int ch)
{
	if (ch >= 0 && ch < TOTAL_N_CHANNELS) {
		if (spu_device->ch_pid[ch] == pid) {
			return 1;
		}
		DEBUG_PRINT(KERN_WARNING "failed in check_pid_ch(%d, %d)\n", pid, ch);
	}
	return 0;
}

static int check_pid_midi(pid_t pid)
{
	if (n_ch_midi > 0) {
		if (spu_device->ch_pid[midi_ch_start] == pid) {
			return 1;
		}
		DEBUG_PRINT(KERN_WARNING "failed in check_pid_midi(%d)\n", pid);
	}
	return 0;
}

static int stub_Audio_allocStream(pid_t pid)
{
	int ret = Audio_allocStream();
	int i = Audio_translateStreamHandleToCh(ret);
	DEBUG_PRINT(KERN_WARNING "%s: handle=0x%08X -> i=%d\n", __FUNCTION__, ret, i);
	spu_device->ch_pid[i] = pid;
	spu_device->ch_pid[i+1] = pid;
	return ret;
}

static void stub_Audio_freeStream(int arg)
{
	int i = Audio_translateStreamHandleToCh(arg);
	DEBUG_PRINT(KERN_WARNING "%s: handle=0x%08X -> i=%d\n", __FUNCTION__, arg, i);
	spu_device->ch_pid[i] = 0;
	spu_device->ch_pid[i+1] = 0;
	
	Audio_freeStream(arg);
}


int spu_fops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long cmd_arg)
{
	int ret = 0;
	int arg = (int)cmd_arg;
	pid_t pid = (pid_t)(filp->private_data);
	
	DEBUG_PRINT(KERN_WARNING "%s(cmd=%X)\n", __FUNCTION__, cmd);
	
	switch (cmd) {
// 	case SPU_IOC_Audio_init: 
// 		{
// 			unsigned int args[3];
// 			copy_from_user(args, (const void __user*)arg, sizeof(args));
// 			Audio_init(args[0], args[1], (int)args[2]);
// 		}
// 		break;
// 	case SPU_IOC_Audio_cleanup:
// 		Audio_cleanup(); 
// 		break;
	case SPU_IOC_Audio_getMasterVolume:
		ret = Audio_getMasterVolume();
		break;
	case SPU_IOC_Audio_setMasterVolume:
		Audio_setMasterVolume(arg);
		break;
	case SPU_IOC_Audio_setWave:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			args[1] = USER_VA_TO_KERNEL_VA(args[1]);
			if (check_pid_ch(pid, args[0])) {
				Audio_setWave(args[0], (unsigned char*)args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_setDrumNote:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_setDrumNote(args[0], args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_setMelodyNote:
		{
			int args[4];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_setMelodyNote(args[0], args[1], args[2], args[3]);
			}
		}
		break;
	case SPU_IOC_Audio_releaseMelodyNote:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_releaseMelodyNote(args[0], args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_resetWave:
		if (check_pid_ch(pid, arg)) {
			Audio_resetWave(arg);
		}
		break;
	case SPU_IOC_Audio_pauseWave:
		if (check_pid_ch(pid, arg)) {
			Audio_pauseWave(arg);
		}
		break;
	case SPU_IOC_Audio_resumeWave:
		if (check_pid_ch(pid, arg)) {
			Audio_resumeWave(arg);
		}
		break;
	case SPU_IOC_Audio_getWaveStatus:
		if (check_pid_ch(pid, arg)) {
			ret = Audio_getWaveStatus(arg);
		}
		break;
	case SPU_IOC_Audio_isWaveLoopEnable:
		if (check_pid_ch(pid, arg)) {
			ret = Audio_isWaveLoopEnable(arg);
		}
		break;
	case SPU_IOC_Audio_enableWaveLoop:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_enableWaveLoop(args[0], args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_getWaveVolume:
		if (check_pid_ch(pid, arg)) {
			ret = Audio_getWaveVolume(arg);
		}
		break;
	case SPU_IOC_Audio_setWaveVolume:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_setWaveVolume(args[0], args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_getWavePan:
		if (check_pid_ch(pid, arg)) {
			ret = Audio_getWavePan(arg);
		}
		break;
	case SPU_IOC_Audio_setWavePan:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_setWavePan(args[0], args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_getWavePlaybackSpeed:
		if (check_pid_ch(pid, arg)) {
			ret = Audio_getWavePlaybackSpeed(arg);
		}
		break;
	case SPU_IOC_Audio_setWavePlaybackSpeed:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			if (check_pid_ch(pid, args[0])) {
				Audio_setWavePlaybackSpeed(args[0], (unsigned short)args[1]);
			}
		}
		break;
	case SPU_IOC_Audio_initMidi:
		if (check_pid_midi(pid)) {
			int args[3];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			args[0] = USER_VA_TO_KERNEL_VA(args[0]);
			args[1] = USER_VA_TO_KERNEL_VA(args[1]);

			Audio_initMidi((unsigned char*)args[0], (unsigned char*)args[1], (unsigned)args[2]);
		}
		break;
	case SPU_IOC_Audio_cleanupMidi:
		if (check_pid_midi(pid)) {
			Audio_cleanupMidi();
		}
		break;
	case SPU_IOC_Audio_setMidi:
		if (check_pid_midi(pid)) {
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			args[0] = USER_VA_TO_KERNEL_VA(args[0]);
			Audio_setMidi((unsigned char*)args[0], (unsigned)args[1]);
		}
		break;
	case SPU_IOC_Audio_setMidiStream:
		if (check_pid_midi(pid)) {
			arg = USER_VA_TO_KERNEL_VA(arg);
			Audio_setMidiStream((AudioMidiStream*)arg);
		}
		break;
	case SPU_IOC_Audio_resetMidi:
		if (check_pid_midi(pid)) {
			Audio_resetMidi();
		}
		break;
	case SPU_IOC_Audio_pauseMidi:
		if (check_pid_midi(pid)) {
			Audio_pauseMidi();
		}
		break;
	case SPU_IOC_Audio_resumeMidi:
		if (check_pid_midi(pid)) {
			Audio_resumeMidi();
		}
		break;
	case SPU_IOC_Audio_getMidiStatus:
		if (check_pid_midi(pid)) {
			ret = Audio_getMidiStatus();
		}
		break;
	case SPU_IOC_Audio_isMidiLoopEnable:
		if (check_pid_midi(pid)) {
			ret = Audio_isMidiLoopEnable();
		}
		break;
	case SPU_IOC_Audio_enableMidiLoop:
		if (check_pid_midi(pid)) {
			Audio_enableMidiLoop(arg);
		}
		break;
	case SPU_IOC_Audio_getMidiVolume:
		if (check_pid_midi(pid)) {
			ret = Audio_getMidiVolume();
		}
		break;
	case SPU_IOC_Audio_setMidiVolume:
		if (check_pid_midi(pid)) {
			Audio_setMidiVolume(arg);
		}
		break;
	case SPU_IOC_Audio_getMidiGlobalPan:
		if (check_pid_midi(pid)) {
			ret = Audio_getMidiGlobalPan();
		}
		break;
	case SPU_IOC_Audio_setMidiGlobalPan:
		if (check_pid_midi(pid)) {
			Audio_setMidiGlobalPan(arg);
		}
		break;
	case SPU_IOC_Audio_enableMidiGlobalPan:
		if (check_pid_midi(pid)) {
			Audio_enableMidiGlobalPan(arg);
		}
		break;
	case SPU_IOC_Audio_getMidiInstrumentVolume:
		if (check_pid_midi(pid)) {
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			ret = Audio_getMidiInstrumentVolume(args[0], args[1]);
		}
		break;
	case SPU_IOC_Audio_setMidiInstrumentVolume:
		if (check_pid_midi(pid)) {
			int args[3];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			Audio_setMidiInstrumentVolume(args[0], args[1], args[2]);
		}
		break;
	case SPU_IOC_Audio_getMidiCurrentPlayTime:
		if (check_pid_midi(pid)) {
			ret = Audio_getMidiCurrentPlayTime();
		}
		break;
	case SPU_IOC_Audio_getMidiPlaybackSpeed:
		if (check_pid_midi(pid)) {
			ret = Audio_getMidiPlaybackSpeed();
		}
		break;
	case SPU_IOC_Audio_setMidiPlaybackSpeed:
		if (check_pid_midi(pid)) {
			Audio_setMidiPlaybackSpeed((unsigned short)arg);
		}
		break;
	case SPU_IOC_Audio_fadeMidi:
		if (check_pid_midi(pid)) {
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			Audio_fadeMidi(args[0], args[1]);
		}
	case SPU_IOC_Audio_isMidiFade:
		if (check_pid_midi(pid)) {
			ret = Audio_isMidiFade();
		}
		break;
	case SPU_IOC_Audio_setMidiLyricEventHandler:
		//TODO!!!!
		break;
	case SPU_IOC_Audio_handleBeatCounter:
		//TODO!!!!
		break;
	case SPU_IOC_Audio_allocStream:
		ret = stub_Audio_allocStream(pid);
		break;
	case SPU_IOC_Audio_freeStream:
		stub_Audio_freeStream(arg);
		break;
	case SPU_IOC_Audio_openStream:
		{
			int args[1 + sizeof(AudioStreamInputParam) / sizeof(int)];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			ret = Audio_openStream(args[0], (AudioStreamInputParam*)&args[1]);
			DEBUG_PRINT(KERN_WARNING "Audio_openStream(...) returns 0x%08X\n", ret);
		}
		break;
	case SPU_IOC_Audio_closeStream:
		Audio_closeStream(arg);
		break;
	case SPU_IOC_Audio_resetStream:
		ret = Audio_resetStream(arg);
		break;
	case SPU_IOC_Audio_pauseStream:
		ret = Audio_pauseStream(arg);
		break;
	case SPU_IOC_Audio_resumeStream:
		ret = Audio_resumeStream(arg);
		break;
	case SPU_IOC_Audio_getStreamVolume:
		ret = Audio_resumeStream(arg);
		break;
	case SPU_IOC_Audio_setStreamVolume:
		{
			int args[2];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			ret = Audio_setStreamVolume(args[0], args[1]);
		}
		break;
	case SPU_IOC_Audio_getStreamPlayStatus:
		ret = Audio_getStreamPlayStatus(arg);
		break;
	case SPU_IOC_Audio_getStreamPlayPos:
		ret = Audio_getStreamPlayPos(arg);
		break;
	case SPU_IOC_Audio_isStreamFull:
		ret = Audio_isStreamFull(arg);
		break;
	case SPU_IOC_Audio_isStreamEmpty:
		ret = Audio_isStreamEmpty(arg);
		break;
	case SPU_IOC_Audio_fillStreamData:
		{
			int args[3];
			copy_from_user(args, (const void __user*)arg, sizeof(args));
			args[1] = USER_VA_TO_KERNEL_VA(args[1]);
			ret = Audio_fillStreamData(args[0], (unsigned short*)args[1], args[2]);
		}
		break;
	case SPU_IOC_Audio_enableStreamStereo:
		Audio_enableStreamStereo((int)arg);
		break;
	case SPU_IOC_Audio_getStreamFreeSize:
		ret = Audio_getStreamFreeSize(arg);
		break;
	case SPU_IOC_Audio_acquireWaveChannels:
		/*{
			int i;
			printk(KERN_WARNING "SPU_IOC_Audio_acquireWaveChannels pid=%d; ch_pid[]=\n", pid);
			printk(KERN_WARNING "ch_pid[0..15]=");
			for (i = 0; i < 16; i++) {
				printk( "%d, ", spu_device->ch_pid[i]);
			}
			printk("\n");
			printk(KERN_WARNING "ch_pid[16..31]=");
			for (i = 16; i < 32; i++) {
				printk( "%d, ", spu_device->ch_pid[i]);
			}
			printk("\n");
		}*/

		ret = -1;
		if (wave_ch_end - wave_ch_start > 0) {
			pid_t occupied_pid = spu_device->ch_pid[wave_ch_start];
			if (occupied_pid == 0) {
				int i;
				for (i = wave_ch_start; i < wave_ch_end; i++) {
					spu_device->ch_pid[i] = pid;
				}
				ret = 0;
				
			} else if (occupied_pid == pid) {
				ret = 0;
			}
		}
		break;
	case SPU_IOC_Audio_acquireMidiChannels:
		ret = -1;
		if (midi_ch_end - midi_ch_start > 0) {
			pid_t occupied_pid = spu_device->ch_pid[midi_ch_start];
			if (occupied_pid == 0) {
				int i;
				for (i = midi_ch_start; i < midi_ch_end; i++) {
					spu_device->ch_pid[i] = pid;
				}
				ret = 0;
				
			} else if (occupied_pid == pid) {
				ret = 0;
			}
		}
		break;

	case SPU_IOC_Debug_getChannelAllocation:
		{
			copy_to_user((void __user*)arg, spu_device->ch_pid, sizeof(spu_device->ch_pid));
		}
		break;

#if 0
	case 0x9999:	//for testing only
		{
			unsigned int *array = (unsigned int*)USER_VA_TO_KERNEL_VA(arg);
			int i;
			printk(KERN_WARNING "%s:%d:%s(0x9999)\n", __FILE__, __LINE__, __FUNCTION__);
			printk(KERN_WARNING "arg=%08X, array=%08X\n", arg, (unsigned int)array);
			for (i = 0; i < 128; i++) {
				if ((i % 8) == 0)
					printk(KERN_WARNING "%08X: ", (unsigned int)&array[i]);
				printk("%08X ", array[i]);
				if ((i % 8) == 7)
					printk("\n");
			}
		}
		break;
#endif

	default:
		ret = -ENOIOCTLCMD;
		break;                      
	}

	return ret;
}

static void put_reg(unsigned int offset, unsigned int value)
{
	iowrite32(value, spu_device->ioremap_ptr + offset);
}

static unsigned int get_reg(unsigned int offset)
{
	return ioread32(spu_device->ioremap_ptr + offset);
}

ssize_t spu_fops_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	if (*offp < SPU_REG_START - SPU_REGBASE || *offp >= SPU_REG_END - SPU_REGBASE) {
		return 0;
	}
	if ((*offp & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to read
	if (*offp + count > SPU_REG_END - SPU_REGBASE) {
		count = SPU_REG_END - SPU_REGBASE - *offp;
	}

	if (count > 0) {
		size_t cnt = 0;
		while (cnt < count) {
			unsigned int n = 0;
			n = get_reg(*offp);
			copy_to_user(buff, &n, 4);
			buff += 4;
			*offp += 4;
			cnt += 4;
		}
	}
	return count;
}

ssize_t spu_fops_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	if (*offp < SPU_REG_START - SPU_REGBASE || *offp >= SPU_REG_END - SPU_REGBASE) {
		return 0;
	}
	if ((*offp & 3) != 0) {
		return 0;
	}
	count &= ~3;	//4-byte per unit to read
	if (*offp + count > SPU_REG_END - SPU_REGBASE) {
		count = SPU_REG_END - SPU_REGBASE - *offp;
	}

	if (count > 0) {
		size_t cnt = 0;
		while (cnt < count) {
			unsigned int n;
			copy_from_user(&n, buff, 4);
			put_reg(*offp, n);		
			buff += 4;
			*offp += 4;
			cnt += 4;
		}
	}
	return count;
}

void Spu_FiqIsr(void);		//defined in SpuDrv.c
void Spu_BeatCntIsr(void);	//defined in SpuDrv.c

static irqreturn_t isr_spu(int irq, void *dev_id)
{
	spu_dev_t *dev = (spu_dev_t*)dev_id;
	//DEBUG_PRINT(KERN_WARNING "%s(%d, 0x%08X)\n", __FUNCTION__, irq, (unsigned int)dev_id);
	if (dev) {
		unsigned int fiq_en = (get_reg(0xBE88) << 16) | get_reg(0xBE08);	//P_SPU_CtrChFiqEn and P_SPU_CtrChFiqEn_H
		unsigned int fiq_st = (get_reg(0xBE8C) << 16) | get_reg(0xBE0C);	//P_SPU_CtrChFiqSts and P_SPU_CtrChFiqSts_H
		
		unsigned int beatcnt = get_reg(0xBE14);
		
		if (fiq_en & fiq_st) {
			Spu_FiqIsr();
		}

		if ((beatcnt & 0xc000) == 0xc000) {
			Spu_BeatCntIsr();
		}
	}
	return IRQ_HANDLED;
}


module_init(spu_module_init);
module_exit(spu_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("VTech");
MODULE_DESCRIPTION("VTech SPU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

