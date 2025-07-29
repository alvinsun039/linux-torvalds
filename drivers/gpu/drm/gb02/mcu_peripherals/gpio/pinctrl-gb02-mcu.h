/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PINCTRL_GB02_MCU_H
#define __PINCTRL_GB02_MCU_H

#define GB02MAC523 0x00
#define GB02MAC524 0x04
#define GB02MAC525 0x08
#define GB02MAC526 0x0C
#define GB02MAC527 0x10
#define GB02MAC528 0x14
#define GB02MAC529 0x18
#define GB02MAC530 0x1C
#define GB02MAC531 0x20
#define GB02MAC532 0x24
#define GB02MAC534 0x28
#define GB02MAC535 0x2C
#define GB02MAC536 0x30
#define GB02MAC537 0x40

#define GB02MAC538 8
#define GB02MAC539 6
#define GB02MAC540 16
#define GB02MAC541 0
#define GB02MAC542 1

#define GB02MAC543 2

#define GB02MAC544 'P'
#define PINCTRL_MCU_CMD_GPIO40 _IOW(GB02MAC544, 1u, int)
#define PINCTRL_MCU_CMD_GPIO41 _IOW(GB02MAC544, 2u, int)
#define PINCTRL_MCU_CMD_GPIO42 _IOW(GB02MAC544, 3u, int)
#define PINCTRL_MCU_CMD_GPIO43 _IOW(GB02MAC544, 4u, int)

#define GB02MAC545
#define GB02MAC546 (40)
#define GB02MAC547 (43)

#ifdef CONFIG_MCU_GPIO
int GB02FUNC366(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info);
void GB02FUNC371(struct GB02STR72 *peri_info);
void GB02FUNC292(unsigned int gpio, bool enable);
int GB02FUNC312(unsigned int gpio, int value);
int GB02FUNC310(unsigned int gpio);
int GB02FUNC301(unsigned int gpio, bool direction);
int GB02FUNC304(unsigned int gpio);
int GB02FUNC295(unsigned int gpio);
void GB02FUNC300(unsigned int gpio);
bool GB02FUNC398(unsigned int gpio);
#else
static inline int GB02FUNC366(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	return 0;
}

static inline void GB02FUNC371(struct GB02STR72 *peri_info)
{
	return;
}

static inline void GB02FUNC292(unsigned int gpio, bool enable)
{
	return;
}

static inline int GB02FUNC312(unsigned int gpio, int value)
{
	return 0;
}

static inline int GB02FUNC310(unsigned int gpio)
{
	return 0;
}

static inline int GB02FUNC301(unsigned int gpio, bool direction)
{
	return 0;
}

static inline int GB02FUNC304(unsigned int gpio)
{
	return 0;
}

static inline int GB02FUNC295(unsigned int gpio)
{
	return 0;
}

static inline void GB02FUNC300(unsigned int gpio)
{
	return;
}

static inline bool GB02FUNC398(unsigned int gpio)
{
	return false;
}
#endif
#endif /* __PINCTRL_GB02_MCU_H */
