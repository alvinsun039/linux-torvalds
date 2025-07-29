// SPDX-License-Identifier: GPL-2.0-or-later
//
// GB02 SPI controller driver (master mode only)
//
// Based on code from SiFive SPI Driver
//

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>
#include "gpu/gb_device.h"
#include "spi-gb02-mcu.h"

#ifdef SPI_DEBUG
#define GB02MAC2554(arg...) gb_printf(KERN_INFO, "[" PLATFORM_SPI_DEVICE_NAME "] " arg)
#else
#define GB02MAC2554(arg...)
#endif

struct GB02STR187 {
	void __iomem *regs;				  /* virt. address of control registers */
	void __iomem *sysctl_iofunc_base; // bar0
	void __iomem *sysctl_cfg_base;	  // bar4
	struct spi_device *spidev;
	u32 version;
	u32 feature;
	unsigned long clkrate;	 /* bus clock */
	unsigned int fifo_depth; /* fifo depth in words */
	u32 cs_inactive;		 /* level of the CS pins when inactive */
	struct completion done;	 /* wake-up from interrupt */
};

static void GB02FUNC1694(struct GB02STR187 *spi, int offset, u32 value)
{
	GB02MAC2554("write offset 0x%x value 0x%08x\n", offset, value);
	iowrite32(value, spi->regs + offset);
}

static u32 GB02FUNC1695(struct GB02STR187 *spi, int offset)
{
	u32 tmp;

	tmp = ioread32(spi->regs + offset);
	GB02MAC2554("read offset 0x%x value 0x%08x\n", offset, tmp);
	return tmp;
}

static void GB02FUNC1697(struct GB02STR187 *spi)
{
	GB02FUNC528(GB02MAC740, spi->sysctl_cfg_base, GB02MAC734);
}

static unsigned long GB02FUNC1698(struct GB02STR187 *spi)
{
	unsigned long periph_freq_div, periph_freq;

	periph_freq_div = GB02FUNC530(spi->sysctl_iofunc_base, GB02MAC850) & 0x7;
	periph_freq = GB02MAC757 * 1000 / (periph_freq_div + 1);
	GB02MAC2554("------freq:%ld----------\n", periph_freq);
	return periph_freq;
}

static void GB02FUNC1700(struct GB02STR187 *spi)
{
	/* Watermark interrupts are disabled by default */
	GB02FUNC1694(spi, GB02MAC2585, 0);

	if ((spi->feature & GB02MAC2628) == GB02MAC2628) {
		/* Set spi cr reg: master mode, uDMA disabled, ddr disabled, cs output enable, hdsmode disabled */
		GB02FUNC1694(spi, GB02MAC2590, 0x11);
		/* Set FORCE register to 0x1, force enable, write protect disable */
		GB02FUNC1694(spi, GB02MAC2570, 0x1);
	}

	/* Default watermark FIFO threshold values */
	GB02FUNC1694(spi, GB02MAC2581, 1);
	GB02FUNC1694(spi, GB02MAC2582, 0);

	/* Set CS/SCK Delays and Inactive Time to defaults */
	GB02FUNC1694(spi, GB02MAC2576,
					   GB02MAC2600(1) |
					   GB02MAC2602(1));
	GB02FUNC1694(spi, GB02MAC2577,
					   GB02MAC2605(1) |
					   GB02MAC2607(0));

	/* Exit specialized memory-mapped SPI flash mode */
	GB02FUNC1694(spi, GB02MAC2583, 0);
}

static int
gb02_mcu_spi_prepare_message(struct spi_master *master, struct spi_message *msg)
{
	struct GB02STR187 *spi = spi_master_get_devdata(master);
	struct spi_device *device = msg->spi;

	/* Update the chip select polarity */
	if (device->mode & GB02MAC2398)
		spi->cs_inactive &= ~BIT(device->chip_select);
	else
		spi->cs_inactive |= BIT(device->chip_select);
	GB02MAC2554("spi @0x%p device mode 0x%x, cs_inactive 0x%x, chip_select 0x%x\n",
				   spi->regs, device->mode, spi->cs_inactive, device->chip_select);
	GB02FUNC1694(spi, GB02MAC2573, spi->cs_inactive);

	/* Select the correct device */
	if ((spi->feature & GB02MAC2628) == GB02MAC2628) {
		/* select device 0 using cs 1 for nuspi */
		GB02FUNC1694(spi, GB02MAC2572, device->chip_select + 1);
	} else {
		GB02FUNC1694(spi, GB02MAC2572, device->chip_select);
	}

	/* Set clock mode */
	GB02FUNC1694(spi, GB02MAC2567,
					   device->mode & GB02MAC2596);

	return 0;
}

static void GB02FUNC1702(struct spi_device *device, bool is_high)
{
	struct GB02STR187 *spi = spi_master_get_devdata(device->master);

	/* Reverse polarity is handled by SCMR/CPOL. Not inverted CS. */
	if (device->mode & GB02MAC2398)
		is_high = !is_high;

	GB02FUNC1694(spi, GB02MAC2574, is_high ? 0x03 : 0x02);
}

static int
gb02_mcu_spi_prep_transfer(struct GB02STR187 *spi, struct spi_device *device,
						   struct spi_transfer *t)
{
	u32 cr;
	unsigned long rate;
	unsigned int mode;

	rate = min_t(unsigned long, spi->clkrate >> 1, t->speed_hz);
	/* Calculate and program the clock rate */
	// cr =0x07;
	cr = DIV_ROUND_UP(spi->clkrate >> 1, rate) - 1;
	GB02MAC2554("----------cr:0x%x clkrate: %ld speed %d rate %ld-----------\n", cr, spi->clkrate, t->speed_hz, rate);

	cr = 0x07;
	cr &= GB02MAC2592;

	GB02FUNC1694(spi, GB02MAC2565, cr);

	mode = max_t(unsigned int, t->rx_nbits, t->tx_nbits);

	/* Set frame format */
	cr = GB02MAC2615(t->bits_per_word);
	switch (mode) {
	case SPI_NBITS_QUAD:
		cr |= GB02MAC2611;
		break;
	case SPI_NBITS_DUAL:
		cr |= GB02MAC2610;
		break;
	default:
		cr |= GB02MAC2609;
		break;
	}
	if (device->mode & GB02MAC2400)
		cr |= GB02MAC2613;
	if (!t->rx_buf)
		cr |= GB02MAC2614;
	GB02FUNC1694(spi, GB02MAC2578, cr);

	/* We will want to poll if the time we need to wait is
	 * less than the context switching time.
	 * Let's call that threshold 5us. The operation will take:
	 *	(8/mode) * fifo_depth / hz <= 5 * 10^-6
	 *	1600000 * fifo_depth <= hz * mode
	 */

	return 1600000 * spi->fifo_depth <= t->speed_hz * mode;
}

static void GB02FUNC1705(struct GB02STR187 *spi)
{
	u32 data;

	data = GB02FUNC1695(spi, GB02MAC2575);
	spi->version = data;
	if (data >= GB02MAC2591)
		spi->feature |= GB02MAC2628;
	else
		spi->feature &= ~GB02MAC2628;
}
#ifdef GB02_SPI_USE_IRQ
static irqreturn_t GB02FUNC1706(int irq, void *dev_id)
{
	struct GB02STR187 *spi = dev_id;
	u32 ip = GB02FUNC1695(spi, GB02MAC2586);

	if (ip & (GB02MAC2621 | GB02MAC2622)) {
		/* Disable interrupts until next transfer */
		GB02FUNC1694(spi, GB02MAC2585, 0);
		complete(&spi->done);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}
#endif
static void GB02FUNC1707(struct GB02STR187 *spi, u32 bit, int poll)
{
	GB02MAC2554("spi @0x%p wait poll %d , bits %x start\n",
				   spi->regs, poll, bit);
	if (poll) {
		u32 cr;
		u32 tmp = 10;

		do {
			cr = GB02FUNC1695(spi, GB02MAC2586);
			tmp--;
		} while ((!(cr & bit)) & tmp);
		/* wait for busy flag is clear */
		if ((spi->feature & GB02MAC2628) != 0) {
			while ((GB02FUNC1695(spi, GB02MAC2588) & GB02MAC2623) != 0)
				;
		}
	} else {
		reinit_completion(&spi->done);
		GB02FUNC1694(spi, GB02MAC2585, bit);
		wait_for_completion(&spi->done);
	}
	GB02MAC2554("spi @0x%p wait poll %d end\n",
				   spi->regs, poll);
}

static void GB02FUNC1710(struct GB02STR187 *spi, const u8 *tx_ptr)
{
	if ((spi->feature & GB02MAC2628) == 0) {
		WARN_ON_ONCE((GB02FUNC1695(spi, GB02MAC2579) & GB02MAC2618) != 0);
		GB02FUNC1694(spi, GB02MAC2579,
						   *tx_ptr & GB02MAC2617);
	} else {
		WARN_ON_ONCE((GB02FUNC1695(spi, GB02MAC2588) & GB02MAC2626) != 0);
		GB02FUNC1694(spi, GB02MAC2579,
						   *tx_ptr & GB02MAC2617);
	}
}

static void GB02FUNC1712(struct GB02STR187 *spi, u8 *rx_ptr)
{
	u32 data;

	if ((spi->feature & GB02MAC2628) == 0) {
		data = GB02FUNC1695(spi, GB02MAC2580);

		WARN_ON_ONCE((data & GB02MAC2620) != 0);
		*rx_ptr = data & GB02MAC2619;
	} else {
		u32 status = GB02FUNC1695(spi, GB02MAC2588);
		// TODO fix in newer IP, which can use spi_status rx empty flag
		WARN_ON_ONCE((status & GB02MAC2627) != 0);
		data = GB02FUNC1695(spi, GB02MAC2580);
		*rx_ptr = data & GB02MAC2619;
	}
}

static int
gb02_mcu_spi_transfer_one(struct spi_master *master, struct spi_device *device,
						  struct spi_transfer *t)
{
	struct GB02STR187 *spi = spi_master_get_devdata(master);
	int poll = gb02_mcu_spi_prep_transfer(spi, device, t);
	const u8 *tx_ptr = t->tx_buf;
	u8 *rx_ptr = t->rx_buf;
	u8 *rx_ptr2;
	unsigned int remaining_words = t->len;
	int j;

	GB02MAC2554("spi @0x%p %s %d, %d\n", spi->regs, __func__, remaining_words, spi->fifo_depth);
	GB02MAC2554("tx ");

	for (j = 0; j < remaining_words; j++)
		GB02MAC2554("0x%x ", tx_ptr[j]);
	GB02MAC2554("\n");
	while (remaining_words) {
		unsigned int n_words = min(remaining_words, spi->fifo_depth);
		unsigned int i;

		/* Enqueue n_words for transmission */
		for (i = 0; i < n_words; i++)
			GB02FUNC1710(spi, tx_ptr++);

		if (rx_ptr) {
			GB02MAC2554("rx_ptr ");
			/* Wait for transmission + reception to complete */
			GB02FUNC1694(spi, GB02MAC2582,
							   n_words - 1);
			GB02FUNC1707(spi, GB02MAC2622, poll);

			/* Read out all the data from the RX FIFO */
			GB02MAC2554("rx ");
			for (i = 0; i < n_words; i++) {
				rx_ptr2 = rx_ptr;
				GB02FUNC1712(spi, rx_ptr++);
				GB02MAC2554("0x%x ", rx_ptr2[0]);
			}
			GB02MAC2554("\n");
		} else {
			GB02MAC2554("else ");
			/* Wait for transmission to complete */
			GB02FUNC1707(spi, GB02MAC2621, poll);
		}

		remaining_words -= n_words;
	}
	GB02MAC2554("spi @0x%p %s end\n", spi->regs, __func__);

	return 0;
}
static struct spi_board_info board_info = {
	.modalias = "spidev",
	.bus_num = 0,
	.chip_select = 0,
	.max_speed_hz = 0,
};
/*TODO add spidev board_info in controler*/
static int GB02FUNC1717(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR187 *spi;
	int num_cs;
	u32 cs_bits;
	// u32 cs_bits, max_bits_per_word=8;//Only 8bit SPI words supported by the driver
	struct spi_master *master;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id, peri_base_bar_id;
	unsigned long clkrate;
	int id, ret = 0;

	pcie_info = gb_dev->gb_pcie;
	id = device_id;
	master = spi_alloc_master(gb_dev->dev, sizeof(struct GB02STR187));
	if (!master) {
		dev_err(gb_dev->dev, "out of memory\n");
		return -ENOMEM;
	}

	spi = spi_master_get_devdata(master);

	init_completion(&spi->done);
	peri_info->priv = spi;
	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	peri_base_bar_id = GB02FUNC477(pcie_info->GB02STR153);
	spi->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	spi->sysctl_cfg_base = gb_dev->gb_pcie->pci_bars[peri_base_bar_id].mmio; //bar4
	spi->regs = spi->sysctl_iofunc_base + GB02MAC748 + GB02MAC822; // get from platfrom_devdata
	GB02MAC2554("mapped at %p\n", spi->regs);
	GB02FUNC1697(spi);
	clkrate = GB02FUNC1698(spi);
	spi->clkrate = clkrate;
	spi->fifo_depth = GB02MAC2562;
	/* probe the number of CS lines */
	spi->cs_inactive = GB02FUNC1695(spi, GB02MAC2573);
	GB02FUNC1694(spi, GB02MAC2573, 0xffffffffU);
	cs_bits = GB02FUNC1695(spi, GB02MAC2573);
	GB02FUNC1694(spi, GB02MAC2573, spi->cs_inactive);
	if (!cs_bits) {
		dev_err(gb_dev->dev, "Could not auto probe CS lines\n");
		ret = -EINVAL;
		goto err_out;
	}

	num_cs = ilog2(cs_bits) + 1;
	if (num_cs > GB02MAC2561) {
		dev_err(gb_dev->dev, "Invalid number of spi slaves\n");
		ret = -EINVAL;
		goto err_out;
	}

	/* Define our master */
	master->bus_num = -1; // non static bus_num
	master->num_chipselect = num_cs;
	master->mode_bits = GB02MAC2386 | GB02MAC2388 | GB02MAC2398 | GB02MAC2400 | GB02MAC2410 | GB02MAC2412 | GB02MAC2414 | GB02MAC2416;
	/* TODO: add driver support for bits_per_word < 8
	 * we need to "left-align" the bits (unless GB02MAC2400)
	 */
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->flags = SPI_CONTROLLER_MUST_TX | SPI_MASTER_GPIO_SS;
	master->prepare_message = gb02_mcu_spi_prepare_message;
	master->set_cs = GB02FUNC1702;
	master->transfer_one = gb02_mcu_spi_transfer_one;

	/* probe gb02_mcu spi features */
	GB02FUNC1705(spi);
	/* Configure the SPI master hardware */
	GB02FUNC1700(spi);
	ret = devm_spi_register_master(gb_dev->dev, master);
	if (ret < 0) {
		dev_err(gb_dev->dev, "spi_register_master failed\n");
		goto err_out;
	}
	board_info.bus_num = master->bus_num;
	board_info.max_speed_hz = clkrate;
	spi->spidev = spi_new_device(master, &board_info);

	return 0;

err_out:
	dev_err(gb_dev->dev, "err_out failed\n");
	spi_master_put(master);

	return ret;
}

static int GB02FUNC1722(struct GB02STR72 *peri_info)
{
	struct GB02STR187 *spi = (struct GB02STR187 *)peri_info->priv;

	/* Disable all the interrupts just in case */
	GB02FUNC1694(spi, GB02MAC2585, 0);
	spi_unregister_device(spi->spidev);
	return 0;
}

int GB02FUNC1723(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;

	GB02MAC2554("gb02_pci_spi_init\n");

	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC1717(gb_dev, peri_info, device_id);
	return ret;
}

void GB02FUNC1725(struct GB02STR72 *peri_info)
{
	GB02FUNC1722(peri_info);
	GB02MAC2554("gb02_pci_spi_exit!\n");
}
