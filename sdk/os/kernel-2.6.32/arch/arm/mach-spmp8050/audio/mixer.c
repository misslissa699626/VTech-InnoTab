#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/delay.h>
//#include <linux/soundcard.h>

#include <asm/io.h>
#include <asm/system.h>         /* cli(), *_flags */
#include <asm/uaccess.h>        /* copy_*_user */

#include <mach/hardware.h>
#include <mach/audio/audio_util.h>
#include <mach/audio/soundcard.h>

MODULE_LICENSE("GPL");

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef HAL_READ_UINT32
#define HAL_READ_UINT32( _register_, _value_ ) \
        ((_value_) = *((volatile unsigned int *)(_register_)))
#endif

#ifndef HAL_WRITE_UINT32
#define HAL_WRITE_UINT32( _register_, _value_ ) \
        (*((volatile unsigned int *)(_register_)) = (_value_))
#endif

#if 1
#define DEBUG0(fmt,args...) printk(fmt,##args)
#else
#define DEBUG0(fmt,args...)
#endif

#if 0
#define DEBUG1(fmt,args...) printk(fmt,##args)
#else
#define DEBUG1(fmt,args...)
#endif

#define DEVICE_NAME "mixer"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static int mixer_major = 0;
static int mixer_minor = 0;
static int mixer_nr_devs = 1;

struct mixer_dev_s {
	struct semaphore sem;
	struct cdev cdev;
}mixer_dev_t;

struct mixer_dev_s *mixer_devices;
static struct class *mixer_class;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                            F U N C T I O N                             *
 **************************************************************************/
static unsigned int
vol_to_level(
	unsigned int vol
)
{
  if (vol > 100){
    vol = 100;
  }
  return (((vol * 0x1F) + 100 - 1)/100);
}

static unsigned int
level_to_vol(
	unsigned int lvl
)
{
  return (((lvl * 100) + 0x1F - 1)/0x1F);
}

static int
mixer_ioctl_headphone_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  long l_lvl, r_lvl;
  int result;

  if (dev == NULL)
    return -EIO;

  audio_HDPHN_volget(&l_lvl, &r_lvl);
  //DEBUG0("[%s] r_lvl:%lx, l_lvl:%lx\n", __FUNCTION__, r_lvl, l_lvl);

  result = (level_to_vol(r_lvl) & 0xFF) << 8;
  result += (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_headphone_write(
	struct file *filp,
	unsigned int vol
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int l_lvl = vol_to_level(vol & 0xFF);
  unsigned int r_lvl = vol_to_level(vol >> 8);
  int result;

  if (dev == NULL)
    return -EIO;

  //DEBUG0("[%s] r_lvl:%x, l_lvl:%x\n", __FUNCTION__, r_lvl, l_lvl);
  audio_HDPHN_volset((long)l_lvl, (long)r_lvl);

  result = (level_to_vol(r_lvl) & 0xFF) << 8;
  result += (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_pcm_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  long l_lvl, r_lvl;
  int result;

  if (dev == NULL)
    return -EIO;

  audio_WAVE_volget(&l_lvl, &r_lvl);

  result = (level_to_vol(r_lvl) & 0xFF) << 8;
  result += (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_pcm_write(
	struct file *filp,
	unsigned int vol
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int l_lvl = vol_to_level(vol & 0xFF);
  unsigned int r_lvl = vol_to_level(vol >> 8);
  int result;

  if (dev == NULL)
    return -EIO;

  audio_WAVE_volset((long)l_lvl, (long)r_lvl);

  result = (level_to_vol(r_lvl) & 0xFF) << 8;
  result += (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_mic_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  long l_lvl;
  int result;

  //DEBUG0("[%s]\n", __FUNCTION__);
  if (dev == NULL)
    return -EIO;

  //DEBUG0("[%s]\n", __FUNCTION__);
  audio_MIC_volget(&l_lvl);

  result = (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_mic_write(
	struct file *filp,
	unsigned int vol
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int l_lvl = vol_to_level(vol & 0xFF);
  int result;

  //DEBUG0("[%s]\n", __FUNCTION__);
  if (dev == NULL)
    return -EIO;

  //DEBUG0("[%s]\n", __FUNCTION__);
  audio_MIC_volset((long)l_lvl);

  result = (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_line_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  long l_lvl, r_lvl;
  int result;

  if (dev == NULL)
    return -EIO;

  audio_LINEIN_volget(&l_lvl, &r_lvl);

  result = (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_line_write(
	struct file *filp,
	unsigned int vol
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int l_lvl = vol_to_level(vol & 0xFF);
  unsigned int r_lvl = vol_to_level(vol >> 8);
  int result;

  if (dev == NULL)
    return -EIO;

  audio_LINEIN_volset((long)l_lvl, (long)r_lvl);

  result = (level_to_vol(r_lvl) & 0xFF) << 8;
  result += (level_to_vol(l_lvl) & 0xFF);

  return result;
}

static int
mixer_ioctl_mute_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  long l_mute, r_mute;
  int result;

  if (dev == NULL)
    return -EIO;

  audio_WAVE_muteget(&l_mute, &r_mute);

  result = (l_mute & 0x01) | ((r_mute & 0x01) << 1);

  return result;
}

static int
mixer_ioctl_mute_write(
	struct file *filp,
	unsigned int mute
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int l_mute = mute & 0x01;
  unsigned int r_mute = (mute >> 1) & 0x01;

  if (dev == NULL)
    return -EIO;

  audio_WAVE_muteset((long)l_mute, (long)r_mute);
  audio_LINEOUT_muteset((long)l_mute, (long)r_mute);

  return 0;
}

static int
mixer_ioctl_outsrc_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int outsrc;

  if (dev == NULL)
	  return -EIO;

  outsrc = audio_HPINS_get();

  switch (outsrc) {
  case 0:
	  return SOUND_MIXER_PCM;
  case 1:
	  return SOUND_MIXER_MIC;
  case 2:
	  return SOUND_MIXER_LINE;
  }

  return 0;
}

static int
mixer_ioctl_outsrc_write(
	struct file *filp,
	int outsrc
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  switch (outsrc) {
  case SOUND_MIXER_MIC:
    audio_HPINS_set(1);
    return SOUND_MIXER_MIC;

  case SOUND_MIXER_LINE:
	audio_HPINS_set(2);
	return SOUND_MIXER_LINE;

  default:
    audio_HPINS_set(0);
    return SOUND_MIXER_PCM;
  }
}

static int
mixer_ioctl_recsrc_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;
  unsigned int adins;

  if (dev == NULL)
	  return -EIO;

  adins = audio_Adins_get();

  switch (adins) {
  case 0x00:
	  return SOUND_MASK_MIC;
  case 0x01:
	  return SOUND_MASK_LINE;
  }

  return 0;
}

static int
mixer_ioctl_recsrc_write(
	struct file *filp,
	int mask
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  switch (mask) {
  case SOUND_MASK_LINE:
    audio_LINEIN_ctrl(1);
    return SOUND_MASK_LINE;

  default:
    audio_MIC_ctrl(1);
    return SOUND_MASK_MIC;

  }
}

static int
mixer_ioctl_devmask_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  return SOUND_MASK_PCM | SOUND_MASK_LINE | SOUND_MASK_MIC;
}

static int
mixer_ioctl_caps_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  return SOUND_CAP_EXCL_INPUT;
}

static int
mixer_ioctl_recmask_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  return SOUND_MASK_LINE | SOUND_MASK_MIC;
}

static int
mixer_ioctl_stereodevs_read(
	struct file *filp
)
{
  struct mixer_dev_s *dev = (struct mixer_dev_s *)filp->private_data;

  if (dev == NULL)
    return -EIO;

  return SOUND_MASK_PCM | SOUND_MASK_LINE;
}

/*
 * The open() implementation
 */
static int
mixer_open(
	struct inode *inode,
	struct file *filp
)
{
  struct mixer_dev_s *dev;

  dev = container_of(inode->i_cdev, struct mixer_dev_s, cdev);
  filp->private_data = dev; /* for other methods */

  //DEBUG0("[%s]\n", __FUNCTION__);
  return 0;
}


/*
 * The release() implementation
 */
static int
mixer_release(
	struct inode *inode,
	struct file *filp
)
{
  return 0;
}


/*
 * The ioctl() implementation
 */
int mixer_ioctl(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
  void __user *argp = (void __user *)arg;
  int __user *p = argp;
  int tmp;

  switch (cmd) {

  case SOUND_MIXER_READ_VOLUME:
    tmp = mixer_ioctl_headphone_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_PCM:
    tmp = mixer_ioctl_pcm_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_LINE:
    tmp = mixer_ioctl_line_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_MIC:
    tmp = mixer_ioctl_mic_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_RECSRC:
    tmp = mixer_ioctl_recsrc_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_DEVMASK:
    tmp = mixer_ioctl_devmask_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_RECMASK:
    tmp = mixer_ioctl_recmask_read(filp);
    return put_user(tmp, p);

  case SOUND_MIXER_READ_STEREODEVS:
    tmp = mixer_ioctl_stereodevs_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_CAPS:
    tmp = mixer_ioctl_caps_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_VOLUME:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_headphone_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_PCM:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_pcm_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_LINE:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_line_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_MIC:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_mic_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_RECSRC:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_recsrc_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_READ_MUTE:
	tmp = mixer_ioctl_mute_read(filp);
	if (tmp < 0)
	  return tmp;
	return put_user(tmp, p);

  case SOUND_MIXER_WRITE_MUTE:
	if (get_user(tmp,p))
	  return -EFAULT;
	return mixer_ioctl_mute_write(filp, tmp);

  case SOUND_MIXER_READ_OUTSRC:
    tmp = mixer_ioctl_outsrc_read(filp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  case SOUND_MIXER_WRITE_OUTSRC:
    if (get_user(tmp,p))
      return -EFAULT;
    tmp = mixer_ioctl_outsrc_write(filp, tmp);
    if (tmp < 0)
      return tmp;
    return put_user(tmp, p);

  default:
	  DEBUG0("[%s] Unknown IO control.\n", __FUNCTION__);
	  return 0;
  }
}

static const struct file_operations dev_fops =
{
	.owner = THIS_MODULE,
	.ioctl = mixer_ioctl,
	.open = mixer_open,
	.release = mixer_release,
};


static void
mixer_setup_cdev(
	struct mixer_dev_s *dev,
	int index
)
{
	int err, devno = MKDEV(mixer_major, mixer_minor + index);

	cdev_init(&dev->cdev, &dev_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &dev_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		DEBUG0("mknod /dev/%s%d failed\n", DEVICE_NAME, index);
	}

	device_create(mixer_class, NULL, MKDEV(mixer_major, mixer_minor + index), NULL, DEVICE_NAME);
}


static void
mixer_module_exit(
	void
)
{
	int i;
	dev_t devno = MKDEV(mixer_major, mixer_minor);

	if (mixer_devices) {
		for (i = 0; i < mixer_nr_devs; i++) {
			device_destroy(mixer_class,MKDEV(mixer_major, mixer_minor + i));
			cdev_del(&mixer_devices[i].cdev);
		}
		kfree(mixer_devices);
	}
	unregister_chrdev_region(devno, mixer_nr_devs);
	class_destroy(mixer_class);

}


static int
mixer_module_init(
	void
)
{
	int result, i;
	dev_t dev = 0;


	result = alloc_chrdev_region(&dev, mixer_minor, mixer_nr_devs, DEVICE_NAME);
	mixer_major = MAJOR(dev);
	if (result < 0) {
		DEBUG0("allocate device region failed\n");
		return result;
	}

	mixer_devices = kmalloc(mixer_nr_devs * sizeof(mixer_dev_t), GFP_KERNEL);
	if (!mixer_devices) {
		DEBUG0("memory allocate failed\n");
		result = -ENOMEM;
		goto fail;
	}
	memset(mixer_devices, 0, mixer_nr_devs * sizeof(mixer_dev_t));

	mixer_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mixer_class)) {
		return -EFAULT;
	}

	for (i = 0; i < mixer_nr_devs; i++) {
		init_MUTEX(&mixer_devices[i].sem);
		mixer_setup_cdev(&mixer_devices[i], i);
	}

	return 0;

  fail:
	mixer_module_exit();
	return result;
}


module_init(mixer_module_init);
module_exit(mixer_module_exit);

MODULE_DESCRIPTION("MIXER module");
MODULE_AUTHOR("gabriel.liao");
