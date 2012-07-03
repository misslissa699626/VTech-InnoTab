/*
 * arch/arm/mach-spmp8000/include/mach/regs-rtc.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * RTC - System peripherals regsters.
 *
 */



#define SENSOR_BASE			IO3_ADDRESS(0x3000)

#define SENSOR_CR0	    (*(volatile unsigned *) (SENSOR_BASE + 0x00))
#define SENSOR_CR1	    (*(volatile unsigned *) (SENSOR_BASE + 0x04))
#define SENSOR_HSET	    (*(volatile unsigned *) (SENSOR_BASE + 0x08))
#define SENSOR_VSET	    (*(volatile unsigned *) (SENSOR_BASE + 0x0C))
#define SENSOR_LSTP	    (*(volatile unsigned *) (SENSOR_BASE + 0x1C))
#define SENSOR_FBADR0	(*(volatile unsigned *) (SENSOR_BASE + 0x20))
#define SENSOR_FBADR1	(*(volatile unsigned *) (SENSOR_BASE + 0x24))
#define SENSOR_FBADR2	(*(volatile unsigned *) (SENSOR_BASE + 0x28))
#define SENSOR_HOLD	    (*(volatile unsigned *) (SENSOR_BASE + 0x2C))
#define SENSOR_IRQEN    (*(volatile unsigned *) (SENSOR_BASE + 0x78))
#define SENSOR_IRQSTS   (*(volatile unsigned *) (SENSOR_BASE + 0x7C))

#define SENSOR_CR0_OFST	     0x00
#define SENSOR_CR1_OFST	     0x04
#define SENSOR_HSET_OFST	 0x08 
#define SENSOR_VSET_OFST	 0x0C
#define SENSOR_LSTP_OFST	 0x1C
#define SENSOR_FBADR0_OFST   0x20
#define SENSOR_FBADR1_OFST   0x24
#define SENSOR_FBADR2_OFST   0x28
#define SENSOR_HOLD_OFST	 0x2C 
#define SENSOR_IRQEN_OFST    0x78
#define SENSOR_IRQSTS_OFST   0x7C


// CSI CR0
#define SENSOR_CSIEN        0x0001

#define SENSOR_CCIR601      0x0000  //0 is CCIR601
#define SENSOR_CCIR656      0x0002  //0 is CCIR601

#define SENSOR_HSYNC_HIGH   0x0004  // data valid when high/low
#define SENSOR_VSYNC_HIGH   0x0008  // data valid when high/low
#define SENSOR_FIELDHODD    0x0010  // field is odd when high/low
#define SENSOR_INTERLACE    0x0020

#define SENSOR_YUVIN_RGB    0x0000
#define SENSOR_YUVIN_YUV    0x0100

#define SENSOR_YUVOUT_RGB   0x0000
#define SENSOR_YUVOUT_YUV   0x0200

#define SENSOR_INSEQ_MASK   0x0C00
#define SENSOR_INSEQ_YUYV   0x0000
#define SENSOR_INSEQ_UYVY   0x0400
#define SENSOR_INSEQ_YVYU   0x0800
#define SENSOR_INSEQ_VYUY   0x0C00

#define SENSOR_UV_INVERSE   0x1000

#define SENSOR_RGB565_IN   0x0000
#define SENSOR_RGB555_IN   0x8000

#define SENSOR_MP4_OUT_NORMAL   0x00000
#define SENSOR_MP4_OUT_MP4      0x80000

#define SENSOR_FRAME_END_MODE_MASK  (0x3 << 20)

//#define SENSOR_EVERY_FIELD_END_MODE (0x0 << 20)
#define SENSOR_ODD_FIELD_END_MODE   (0x1 << 20)
#define SENSOR_EVEN_FIELD_END_MODE  (0x2 << 20)
#define SENSOR_EVERY_FIELD_END_MODE (0x3 << 20)

#define SENSOR_SAMPLE_EDGE_POS   (0x0 << 22)
#define SENSOR_SAMPLE_EDGE_NEG   (0x1 << 22)

#define SENSOR_BUF_SEL_MASK      (0x3 << 24)
#define SENSOR_BUF_SEL_ADR0      (0x0 << 24)
#define SENSOR_BUF_SEL_ADR1      (0x1 << 24)
#define SENSOR_BUF_SEL_ADR2      (0x2 << 24)
#define SENSOR_BUF_SEL_ADR3      (0x3 << 24)

#define SENSOR_FRAME_NORMAL      (0x0 << 27)    
#define SENSOR_FRAME_DISABLE     (0x1 << 27)

#define SENSOR_CSI_UPDATE        (0x1 << 31)    

// CSI CR1
#define SENSOR_CSIRST            0x1
#define SENSOR_AHBMASTER_EN      0x4

//CSI_HSET
#define SENSOR_HSIZE_MASK   0xFFF
#define SENSOR_HSIZE_OFST   0  
#define SENSOR_HSTART_MASK  (0xFFF << 16)
#define SENSOR_HSTART_OFST  16
#define SENSOR_HDS_MASK     (0x7 << 28)
#define SENSOR_HDS_OFST     28

//CSI_VSET
#define SENSOR_VSIZE_MASK   0xFFF
#define SENSOR_VSIZE_OFST   0  
#define SENSOR_VSTART_MASK  (0xFFF << 16)
#define SENSOR_VSTART_OFST  16
#define SENSOR_VDS_MASK     (0x7 << 28)
#define SENSOR_VDS_OFST     28

//CSI_LSTP 
#define SENSOR_LINE_STEP_MASK    0xFFFF

//CSI_FBADR0
#define SENSOR_FBADR0_ADDR        0xFFFFFFE0
#define SENSOR_FBADR0_ADDR_OFST   5

//CSI_FBADR1
#define SENSOR_FBADR1_ADDR        0xFFFFFFE0
#define SENSOR_FBADR1_ADDR_OFST   5

//CSI_FBADR2
#define SENSOR_FBADR2_ADDR        0xFFFFFFE0
#define SENSOR_FBADR2_ADDR_OFST   5

//CSI_HOLD
#define SENSOR_CSI_HOLD      0x1

//CSI_IRQEN
#define SENSOR_IRQ_OF_EN        0x01
#define SENSOR_IRQ_HOLD_EN      0x02
#define SENSOR_IRQ_FRAMEEND_EN  0x04
#define SENSOR_IRQ_LASTLEN_EN   0x40
#define SENSOR_IRQ_CRUPDATE_EN  0x80

//CSI_IRQSTS
#define SENSOR_IRQ_OF_STS        0x01
#define SENSOR_IRQ_HOLD_STS      0x02
#define SENSOR_IRQ_FRAMEEND_STS  0x04
#define SENSOR_IRQ_LASTLEN_STS   0x40
#define SENSOR_IRQ_CRUPDATE_STS  0x80
