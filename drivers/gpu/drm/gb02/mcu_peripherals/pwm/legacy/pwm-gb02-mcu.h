/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PWM_GB02_MCU_H
#define __PWM_GB02_MCU_H
#include <linux/pwm.h>
#include "gpu/gb_device.h"
#include "common/gb-peripherals-common.h"
//#undef GPIO_PWM
/* timer */
#define GB02MAC1961 0x00   /* Control Register 1      */
#define GB02MAC1962 0x04   /* Control Register 2      */
#define GB02MAC1963 0x08  /* Slave mode control reg  */
#define GB02MAC1964 0x0C  /* DMA/interrupt register  */
#define GB02MAC1965 0x10	   /* Status register     */
#define GB02MAC1966 0x14   /* Event Generation Reg    */
#define GB02MAC1967 0x18 /* Capt/Comp 1 Mode Reg    */
#define GB02MAC1968 0x1C /* Capt/Comp 2 Mode Reg    */
#define GB02MAC1969 0x20  /* Capt/Comp Enable Reg    */
#define GB02MAC1970 0x24   /* Counter         */
#define GB02MAC1971 0x28   /* Prescaler               */
#define GB02MAC1972 0x2c   /* Auto-Reload Register    */
#define GB02MAC1973 0x34  /* Capt/Comp Register 1    */
#define GB02MAC1974 0x38  /* Capt/Comp Register 2    */
#define GB02MAC1975 0x3C  /* Capt/Comp Register 3    */
#define GB02MAC1976 0x40  /* Capt/Comp Register 4    */
#define GB02MAC1977 0x44  /* Break and Dead-Time Reg */
#define GB02MAC1978 0x48   /* DMA control register    */
#define GB02MAC1979 0x4C  /* DMA register for transfer */

#define GB02MAC1980 BIT(0)						/* Counter Enable      */
#define GB02MAC1981 BIT(4)						/* Counter Direction       */
#define GB02MAC1982 BIT(7)						/* Auto-reload Preload Ena */
#define GB02MAC1983 (BIT(4) | BIT(5) | BIT(6))	/* Master mode selection */
#define TIM_CR2_MMS2 GENMASK(23, 20)			/* Master mode selection 2 */
#define GB02MAC1984 (BIT(0) | BIT(1) | BIT(2)) /* Slave mode selection */
#define GB02MAC1985 (BIT(4) | BIT(5) | BIT(6))	/* Trigger selection */
#define GB02MAC1986 BIT(0)						/* Update interrupt    */
#define GB02MAC1987 BIT(8)						/* Update DMA request Enable */
#define GB02MAC1988 BIT(9)					/* CC1 DMA request Enable  */
#define GB02MAC1989 BIT(10)					/* CC2 DMA request Enable  */
#define GB02MAC1990 BIT(11)					/* CC3 DMA request Enable  */
#define GB02MAC1991 BIT(12)					/* CC4 DMA request Enable  */
#define GB02MAC1992 BIT(13)					/* COM DMA request Enable  */
#define GB02MAC1993 BIT(14)					/* Trigger DMA request Enable */
#define GB02MAC1994 BIT(0)						/* Update interrupt flag   */
#define GB02MAC1995 BIT(0)						/* Update Generation       */
#define GB02MAC1996 BIT(3)						/* Channel Preload Enable  */
#define GB02MAC1997 (BIT(6) | BIT(5))			/* Channel PWM Mode 1 */
#define GB02MAC1998 (BIT(0) | BIT(1))			/* Capture/compare 1 sel */
#define TIM_CCMR_IC1PSC GENMASK(3, 2)			/* Input capture 1 prescaler */
#define GB02MAC1999 (BIT(8) | BIT(9))			/* Capture/compare 2 sel */
#define TIM_CCMR_IC2PSC GENMASK(11, 10)			/* Input capture 2 prescaler */
#define GB02MAC2000 BIT(0)				/* IC1/IC3 selects TI1/TI3 */
#define GB02MAC2001 BIT(1)				/* IC1/IC3 selects TI2/TI4 */
#define GB02MAC2002 BIT(8)				/* IC2/IC4 selects TI2/TI4 */
#define GB02MAC2003 BIT(9)				/* IC2/IC4 selects TI1/TI3 */
#define GB02MAC2004 BIT(0)					/* Capt/Comp 1  out Ena    */
#define GB02MAC2005 BIT(1)					/* Capt/Comp 1  Polarity   */
#define GB02MAC2006 BIT(2)					/* Capt/Comp 1N out Ena    */
#define GB02MAC2007 BIT(3)					/* Capt/Comp 1N Polarity   */
#define GB02MAC2008 BIT(4)					/* Capt/Comp 2  out Ena    */
#define GB02MAC2009 BIT(5)					/* Capt/Comp 2  Polarity   */
#define GB02MAC2010 BIT(8)					/* Capt/Comp 3  out Ena    */
#define GB02MAC2011 BIT(9)					/* Capt/Comp 3  Polarity   */
#define GB02MAC2012 BIT(12)					/* Capt/Comp 4  out Ena    */
#define GB02MAC2013 BIT(13)					/* Capt/Comp 4  Polarity   */
#define GB02MAC2014 (BIT(0) | BIT(4) | BIT(8) | BIT(12))
#define GB02MAC2015 BIT(12) /* Break input enable      */
#define GB02MAC2016 BIT(13) /* Break input polarity    */
#define GB02MAC2017 BIT(14) /* Automatic Output Enable */
#define GB02MAC2018 BIT(15) /* Main Output Enable      */
#define GB02MAC2019 (BIT(16) | BIT(17) | BIT(18) | BIT(19))
#define GB02MAC2020 (BIT(20) | BIT(21) | BIT(22) | BIT(23))
#define GB02MAC2021 BIT(24)	   /* Break 2 input enable    */
#define GB02MAC2022 BIT(25)	   /* Break 2 input polarity  */
#define TIM_DCR_DBA GENMASK(4, 0)  /* DMA base addr */
#define TIM_DCR_DBL GENMASK(12, 8) /* DMA burst len */

#define GB02MAC2023 0xFFFF
#define GB02MAC2024 0x3
#define GB02MAC2025 4
#define GB02MAC2026 20
#define GB02MAC2027 4
#define GB02MAC2028 0xF
#define GB02MAC2029 16
#define GB02MAC2030 20

#define GB02MAC2031	0x3fc
#define GB02MAC2032 64
#define GB02_PWM_BL_DEV_NAME "gb02-pwm-backlight"
#define GB02MAC1767 0
#define GB02MAC2033 0

#define GB02_PWM_FAN_DEV_NAME "gb02-pwm-fan"
#ifdef GPIO_PWM
#define GB02MAC2034 0
#else
#define GB02MAC2034 1
#endif
#define GB02MAC2035 0

enum gb02_timers_dmas {
	GB02_TIMERS_DMA_CH1,
	GB02_TIMERS_DMA_CH2,
	GB02_TIMERS_DMA_CH3,
	GB02_TIMERS_DMA_CH4,
	GB02_TIMERS_DMA_UP,
	GB02_TIMERS_DMA_TRIG,
	GB02_TIMERS_DMA_COM,
	GB02_TIMERS_MAX_DMAS,
};

struct GB02STR167 {
	struct completion completion;
	phys_addr_t phys_base;
	struct mutex lock;
	struct dma_chan *chan;
	struct dma_chan *chans[GB02_TIMERS_MAX_DMAS];
};
struct GB02STR179 {
	struct pwm_chip chip;
	struct mutex lock; /* protect pwm config/enable */
	void __iomem *regs;
	u32 max_arr;
	bool have_complementary_output;
	u32 capture[4] ____cacheline_aligned; /* DMA'able buffer */
	void *driver_data;
	u32 ctrl_no;
	unsigned long clk_rate;
	struct device *dev;
	struct GB02STR167 dma; /* Only to be used by the parent */
};

struct GB02STR180 {
	u32 index;
	u32 level;
	u32 filter;
};

struct GB02STR181 {
	void __iomem *pci_mmio_bar0;
	void __iomem *pci_mmio_bar1;
	void __iomem *pci_mmio_bar4;
	struct device *dev;
	struct GB02STR179 pwm;
	void __iomem *res;		  /* resources found */
	struct resource *res_req; /* resources requested */
	void __iomem *sysctl_iofunc_base;
	struct backlight_device *bl_dev;
	struct GB02STR157			*GB02STR13;
};

struct GB02STR174 {
	struct pwm_chip chip;
	struct hrtimer timer;
	struct gpio_desc *gpiod;
	int gpio;
	unsigned int on_time;
	unsigned int off_time;
	struct mutex lock;
	bool pin_on;
	struct GB02STR157			*GB02STR13;
};

#ifdef CONFIG_MCU_PWM
int GB02FUNC1375(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC1378(struct GB02STR72 *peri_info);
#else
static inline int GB02FUNC1375(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1378(struct GB02STR72 *peri_info)
{
	return;
}
#endif

#ifdef CONFIG_GPIO_PWM
int GB02FUNC1412(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC1414(struct GB02STR72 *peri_info);
int GB02FUNC1415(struct GB02STR72 *peri_info);
#else
static inline int GB02FUNC1412(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1414(struct GB02STR72 *peri_info)
{
	return;
}
static inline int GB02FUNC1415(struct GB02STR72 *peri_info)
{
	return 0;
}
#endif

#if defined (CONFIG_GPIO_PWM) || defined(CONFIG_MCU_PWM)
int GB02FUNC1215(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info);
void GB02FUNC1219(struct GB02STR72 *peri_info);
int GB02FUNC1253(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info);
void GB02FUNC1262(struct GB02STR72 *peri_info);
int GB02FUNC1264(struct GB02STR72 *peri_info);
int GB02FUNC1268(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info);
int GB02FUNC1420(struct GB02STR72 *peri_info);
int GB02FUNC1422(struct GB02STR72 *peri_info);
#else
static inline int GB02FUNC1215(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1219(struct GB02STR72 *peri_info)
{
	return;
}
static inline int GB02FUNC1253(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline void GB02FUNC1262(struct GB02STR72 *peri_info)
{
	return;
}
static inline int GB02FUNC1264(struct GB02STR72 *peri_info)
{
	return 0;
}
static inline int GB02FUNC1268(struct GB02STR39 *gbdev, struct GB02STR72 *peri_info)
{
	return 0;
}
static inline int GB02FUNC1420(struct GB02STR72 *peri_info)
{
	return 0;
}
static inline int GB02FUNC1422(struct GB02STR72 *peri_info)
{
	return 0;
}
#endif
#endif
