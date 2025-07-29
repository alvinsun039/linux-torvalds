// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <linux/bitfield.h>
#include <linux/dmaengine.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include "pwm-gb02-mcu.h"
#include "common/gb-peripherals-common.h"

#ifdef PWM_DEBUG
#define GB02MAC1942(arg...)		gb_printf(KERN_INFO, "["PLATFORM_PWM_DEVICE_NAME"] " arg)
#else
#define GB02MAC1942(arg...)
#endif

#define GB02MAC1943 'W'
#define PWM_MCU_CMD_1 _IOW(GB02MAC1943, 1u, int)

#define GB02MAC1944 8
#define GB02MAC1945  0xFF
#define GB02MAC1946 2

static struct GB02STR181 *gb02_pwm_infos[2];

static inline void GB02FUNC1280(unsigned int val,
				struct GB02STR179 *pwm, unsigned int reg)
{
	writel(val, pwm->regs + reg);
}

static inline unsigned long GB02FUNC1281(struct GB02STR179 *pwm,
		unsigned int reg)
{
	return readl(pwm->regs + reg);
}

static inline void GB02FUNC1282(unsigned int dat,
				struct GB02STR179 *pwm, unsigned int reg, unsigned int mask)
{
	unsigned long val = 0;

	val = GB02FUNC1281(pwm, reg);
	val &= ~mask;
	val |= dat;

	GB02FUNC1280(val, pwm, reg);
}

static inline struct GB02STR179 *GB02FUNC1462(struct pwm_chip *chip)
{
	return container_of(chip, struct GB02STR179, chip);
}

static u32 GB02FUNC1289(struct GB02STR179 *pwm)
{
	u32 ccer;

	ccer = GB02FUNC1281(pwm, GB02MAC1969);
	return ccer & GB02MAC2014;
}

static void GB02FUNC1290(struct GB02STR179 *pwm, int ch, u32 value)
{
	switch (ch) {
	case 0:
		GB02FUNC1280(value, pwm, GB02MAC1973);
		break;
	case 1:
		GB02FUNC1280(value, pwm, GB02MAC1974);
		break;
	case 2:
		GB02FUNC1280(value, pwm, GB02MAC1975);
		break;
	case 3:
		GB02FUNC1280(value, pwm, GB02MAC1976);
		break;
	default:
		break;
	}
}

#define GB02MAC1948 (GB02MAC2005 | GB02MAC2009)
#define GB02MAC1949 (GB02MAC2004 | GB02MAC2008)
#define GB02MAC1950 (GB02MAC2011 | GB02MAC2013)
#define GB02MAC1951 (GB02MAC2010 | GB02MAC2012)
/* DIER register DMA enable bits */
static const u32 gb02_timers_dier_dmaen[GB02_TIMERS_MAX_DMAS] = {
	GB02MAC1988,
	GB02MAC1989,
	GB02MAC1990,
	GB02MAC1991,
	GB02MAC1986,
	GB02MAC1993,
	GB02MAC1992
};

static void GB02FUNC1298(void *p)
{
	struct GB02STR167 *dma = p;
	struct dma_tx_state state;
	enum dma_status status;

	status = dmaengine_tx_status(dma->chan, dma->chan->cookie, &state);
	if (status == DMA_COMPLETE)
		complete(&dma->completion);
}

/**
 * GB02FUNC1301 - Read from timers registers using DMA.
 *
 * Read from GB02 timers registers using DMA on a single event.
 * @dev: reference to GB02STR167 MFD device
 * @buf: DMA'able destination buffer
 * @id: gb02_timers_dmas event identifier (ch[1..4], up, trig or com)
 * @reg: registers start offset for DMA to read from (like CCRx for capture)
 * @num_reg: number of registers to read upon each DMA request, starting @reg.
 * @bursts: number of bursts to read (e.g. like two for pwm period capture)
 * @tmo_ms: timeout (milliseconds)
 */
static int GB02FUNC1301(struct GB02STR179 *priv, u32 *buf,
				enum gb02_timers_dmas id, u32 reg,
				unsigned int num_reg, unsigned int bursts,
				unsigned long tmo_ms)
{
	unsigned long timeout = msecs_to_jiffies(tmo_ms);
	struct GB02STR167 *dma = &priv->dma;
	struct device *dev = priv->dev;
	size_t len = num_reg * bursts * sizeof(u32);
	struct dma_async_tx_descriptor *desc;
	struct dma_slave_config config;
	dma_cookie_t cookie;
	dma_addr_t dma_buf;
	u32 dbl, dba;
	long err;
	int ret;

	/* Sanity check */
	if (id < GB02_TIMERS_DMA_CH1 || id >= GB02_TIMERS_MAX_DMAS)
		return -EINVAL;

	if (!num_reg || !bursts || reg > GB02MAC2031 ||
		(reg + num_reg * sizeof(u32)) > GB02MAC2031)
		return -EINVAL;

	if (!dma->chans[id])
		return -ENODEV;
	mutex_lock(&dma->lock);

	/* Select DMA channel in use */
	dma->chan = dma->chans[id];
	dma_buf = dma_map_single(dev, buf, len, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dma_buf)) {
		ret = -ENOMEM;
		goto unlock;
	}

	/* Prepare DMA read from timer registers, using DMA burst mode */
	memset(&config, 0, sizeof(config));
	config.src_addr = (dma_addr_t)dma->phys_base + GB02MAC1979;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	ret = dmaengine_slave_config(dma->chan, &config);
	if (ret)
		goto unmap;

	desc = dmaengine_prep_slave_single(dma->chan, dma_buf, len,
									   DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT);
	if (!desc) {
		ret = -EBUSY;
		goto unmap;
	}

	desc->callback = GB02FUNC1298;
	desc->callback_param = dma;
	cookie = dmaengine_submit(desc);
	ret = dma_submit_error(cookie);
	if (ret)
		goto dma_term;

	reinit_completion(&dma->completion);
	dma_async_issue_pending(dma->chan);

	/* Setup and enable timer DMA burst mode */
	dbl = FIELD_PREP(TIM_DCR_DBL, bursts - 1);
	dba = FIELD_PREP(TIM_DCR_DBA, reg >> 2);
	GB02FUNC1280(dbl | dba, priv, GB02MAC1978);

	/* Clear pending flags before enabling DMA request */
	GB02FUNC1280(0, priv, GB02MAC1965);

	GB02FUNC1282(gb02_timers_dier_dmaen[id], priv, GB02MAC1964, gb02_timers_dier_dmaen[id]);

	err = wait_for_completion_interruptible_timeout(&dma->completion,
			timeout);
	if (err == 0)
		ret = -ETIMEDOUT;
	else if (err < 0)
		ret = err;

	GB02FUNC1282(0, priv, GB02MAC1964, gb02_timers_dier_dmaen[id]);
	GB02FUNC1280(0, priv, GB02MAC1965);

	GB02FUNC1280(0, priv, GB02MAC1978);
dma_term:
	dmaengine_terminate_all(dma->chan);
unmap:
	dma_unmap_single(dev, dma_buf, len, DMA_FROM_DEVICE);
unlock:
	dma->chan = NULL;
	mutex_unlock(&dma->lock);

	return ret;
}

static void GB02FUNC1309(struct GB02STR179 *priv)
{
	int i;
	char name[4];
	struct GB02STR167 *dma = &priv->dma;
	struct device *dev = priv->dev;

	init_completion(&dma->completion);
	mutex_init(&dma->lock);
	dma->phys_base = (phys_addr_t)priv->regs;
	/* Optional DMA support: get valid DMA channel(s) or NULL */
	for (i = GB02_TIMERS_DMA_CH1; i <= GB02_TIMERS_DMA_CH4; i++) {
		snprintf(name, ARRAY_SIZE(name), "ch%1d", i + 1);
		dma->chans[i] = dma_request_slave_channel(dev, name);
	}
	dma->chans[GB02_TIMERS_DMA_UP] =
		dma_request_slave_channel(dev, "up");
	dma->chans[GB02_TIMERS_DMA_TRIG] =
		dma_request_slave_channel(dev, "trig");
	dma->chans[GB02_TIMERS_DMA_COM] =
		dma_request_slave_channel(dev, "com");
}

static void GB02FUNC1314(struct GB02STR179 *priv)
{
	int i;
	struct GB02STR167 *dma = &priv->dma;

	for (i = GB02_TIMERS_DMA_CH1; i < GB02_TIMERS_MAX_DMAS; i++)
		if (dma->chans[i])
			dma_release_channel(dma->chans[i]);
}

/*
 * Capture using PWM input mode:
 *							  ___		  ___
 * TI[1, 2, 3 or 4]: ........._|   |________|
 *							 ^0  ^1	   ^2
 *							  .   .		.
 *							  .   .		XXXXX
 *							  .   .   XXXXX	 |
 *							  .  XXXXX	 .	|
 *							XXXXX .		.	|
 * COUNTER:		______XXXXX  .   .		.	|_XXX
 *				 start^	   .   .		.		^stop
 *					  .	   .   .		.
 *					  v	   v   .		v
 *								  v
 * CCR1/CCR3:	   tx..........t0...........t2
 * CCR2/CCR4:	   tx..............t1.........
 *
 * DMA burst transfer:		  |			|
 *							  v			v
 * DMA buffer:				  { t0, tx }   { t2, t1 }
 * DMA done:								 ^
 *
 * 0: IC1/3 snapchot on rising edge: counter value -> CCR1/CCR3
 *	+ DMA transfer CCR[1/3] & CCR[2/4] values (t0, tx: doesn't care)
 * 1: IC2/4 snapchot on falling edge: counter value -> CCR2/CCR4
 * 2: IC1/3 snapchot on rising edge: counter value -> CCR1/CCR3
 *	+ DMA transfer CCR[1/3] & CCR[2/4] values (t2, t1)
 *
 * DMA done, compute:
 * - Period	 = t2 - t0
 * - Duty cycle = t1 - t0
 */

static int GB02FUNC1315(struct GB02STR179 *priv, struct pwm_device *pwm,
								unsigned long tmo_ms, u32 *raw_prd,
								u32 *raw_dty)
{
	//struct device *parent = priv->chip.dev->parent;
	enum gb02_timers_dmas dma_id;
	u32 ccen, ccr;
	int ret;

	/* Ensure registers have been updated, enable counter and capture */
	GB02FUNC1282(GB02MAC1995, priv, GB02MAC1966, GB02MAC1995);
	GB02FUNC1282(GB02MAC1980, priv, GB02MAC1961, GB02MAC1980);

	/* Use cc1 or cc3 DMA resp for PWM input channels 1 & 2 or 3 & 4 */
	dma_id = pwm->hwpwm < 2 ? GB02_TIMERS_DMA_CH1 : GB02_TIMERS_DMA_CH3;
	ccen = pwm->hwpwm < 2 ? GB02MAC1949 : GB02MAC1951;
	ccr = pwm->hwpwm < 2 ? GB02MAC1973 : GB02MAC1975;
	GB02FUNC1282(ccen, priv, GB02MAC1969, ccen);

	/*
	 * Timer DMA burst mode. Request 2 registers, 2 bursts, to get both
	 * CCR1 & CCR2 (or CCR3 & CCR4) on each capture event.
	 * We'll get two capture snapchots: { CCR1, CCR2 }, { CCR1, CCR2 }
	 * or { CCR3, CCR4 }, { CCR3, CCR4 }
	 */
	ret = GB02FUNC1301(priv, priv->capture, dma_id, ccr, 2,
									 2, tmo_ms);
	if (ret)
		goto stop;

	/* Period: t2 - t0 (take care of counter overflow) */
	if (priv->capture[0] <= priv->capture[2])
		*raw_prd = priv->capture[2] - priv->capture[0];
	else
		*raw_prd = priv->max_arr - priv->capture[0] + priv->capture[2];

	/* Duty cycle capture requires at least two capture units */
	if (pwm->chip->npwm < 2)
		*raw_dty = 0;
	else if (priv->capture[0] <= priv->capture[3])
		*raw_dty = priv->capture[3] - priv->capture[0];
	else
		*raw_dty = priv->max_arr - priv->capture[0] + priv->capture[3];

	if (*raw_dty > *raw_prd) {
		/*
		 * Race beetween PWM input and DMA: it may happen
		 * falling edge triggers new capture on TI2/4 before DMA
		 * had a chance to read CCR2/4. It means capture[1]
		 * contains period + duty_cycle. So, subtract period.
		 */
		*raw_dty -= *raw_prd;
	}

stop:
	GB02FUNC1282(0, priv, GB02MAC1969, ccen);
	GB02FUNC1282(0, priv, GB02MAC1961, GB02MAC1980);

	return ret;
}

static int GB02FUNC1320(struct pwm_chip *chip, struct pwm_device *pwm,
							struct pwm_capture *result, unsigned long tmo_ms)
{
	struct GB02STR179 *priv = GB02FUNC1462(chip);
	unsigned long long prd, div, dty;
	unsigned long rate;
	unsigned int psc = 0, icpsc, scale;
	u32 raw_prd = 0, raw_dty = 0;
	int ret = 0;

	mutex_lock(&priv->lock);

	if (GB02FUNC1289(priv)) {
		ret = -EBUSY;
		goto unlock;
	}

	rate = priv->clk_rate;

	/* prescaler: fit timeout window provided by upper layer */
	div = (unsigned long long)rate * (unsigned long long)tmo_ms;
	do_div(div, MSEC_PER_SEC);
	prd = div;
	while ((div > priv->max_arr) && (psc < GB02MAC2023)) {
		psc++;
		div = prd;
		do_div(div, psc + 1);
	}
	GB02FUNC1280(priv->max_arr, priv, GB02MAC1972);
	GB02FUNC1280(psc, priv, GB02MAC1971);

	/* Map TI1 or TI2 PWM input to IC1 & IC2 (or TI3/4 to IC3 & IC4) */
	GB02FUNC1282(
		GB02MAC1998 | GB02MAC1999, priv, pwm->hwpwm & 0x1 ?
		GB02MAC2001 | GB02MAC2002 :
		GB02MAC2000 | GB02MAC2003,
		pwm->hwpwm < 2 ? GB02MAC1967 : GB02MAC1968);

	/* Capture period on IC1/3 rising edge, duty cycle on IC2/4 falling. */
	GB02FUNC1282(
		pwm->hwpwm < 2 ?
		GB02MAC2009 : GB02MAC2013, priv, GB02MAC1969, pwm->hwpwm < 2 ?
		GB02MAC1948 : GB02MAC1950);

	ret = GB02FUNC1315(priv, pwm, tmo_ms, &raw_prd, &raw_dty);
	if (ret)
		goto stop;

	/*
	 * Got a capture. Try to improve accuracy at high rates:
	 * - decrease counter clock prescaler, scale up to max rate.
	 * - use input prescaler, capture once every /2 /4 or /8 edges.
	 */
	if (raw_prd) {
		u32 max_arr = priv->max_arr - 0x1000; /* arbitrary margin */

		scale = max_arr / min(max_arr, raw_prd);
	} else {
		scale = priv->max_arr; /* bellow resolution, use max scale */
	}

	if (psc && scale > 1) {
		/* 2nd measure with new scale */
		psc /= scale;

		GB02FUNC1280(psc, priv, GB02MAC1971);
		ret = GB02FUNC1315(priv, pwm, tmo_ms, &raw_prd,
								   &raw_dty);
		if (ret)
			goto stop;
	}

	/* Compute intermediate period not to exceed timeout at low rates */
	prd = (unsigned long long)raw_prd * (psc + 1) * NSEC_PER_SEC;
	do_div(prd, rate);

	for (icpsc = 0; icpsc < GB02MAC2024 ; icpsc++) {
		/* input prescaler: also keep arbitrary margin */
		if (raw_prd >= (priv->max_arr - 0x1000) >> (icpsc + 1))
			break;
		if (prd >= (tmo_ms * NSEC_PER_MSEC) >> (icpsc + 2))
			break;
	}

	if (!icpsc)
		goto done;

	/* Last chance to improve period accuracy, using input prescaler */
	GB02FUNC1282(FIELD_PREP(TIM_CCMR_IC1PSC, icpsc) |
						FIELD_PREP(TIM_CCMR_IC2PSC, icpsc), priv,
						pwm->hwpwm < 2 ? GB02MAC1967 : GB02MAC1968,
						TIM_CCMR_IC1PSC | TIM_CCMR_IC2PSC
					   );

	ret = GB02FUNC1315(priv, pwm, tmo_ms, &raw_prd, &raw_dty);
	if (ret)
		goto stop;

	if (raw_dty >= (raw_prd >> icpsc)) {
		/*
		 * We may fall here using input prescaler, when input
		 * capture starts on high side (before falling edge).
		 * Example with icpsc to capture on each 4 events:
		 *
		 *	   start   1st capture					 2nd capture
		 *		 v	 v							   v
		 *		 ___   _____   _____   _____   _____   ____
		 * TI1..4	 |__|	|__|	|__|	|__|	|__|
		 *			v  v	.  .	.  .	.	   v  v
		 * icpsc1/3:  .  0	.  1	.  2	.  3	.  0
		 * icpsc2/4:  0	   1	   2	   3	   0
		 *			v  v							v  v
		 * CCR1/3  ......t0..............................t2
		 * CCR2/4  ..t1..............................t1'...
		 *			   .							.  .
		 * Capture0:	 .<----------------------------->.
		 * Capture1:	 .<-------------------------->.  .
		 *			   .							.  .
		 * Period:	   .<------>					.  .
		 * Low side:								  .<>.
		 *
		 * Result:
		 * - Period = Capture0 / icpsc
		 * - Duty = Period - Low side = Period - (Capture0 - Capture1)
		 */
		raw_dty = (raw_prd >> icpsc) - (raw_prd - raw_dty);
	}

done:
	prd = (unsigned long long)raw_prd * (psc + 1) * NSEC_PER_SEC;
	result->period = DIV_ROUND_UP_ULL(prd, rate << icpsc);
	dty = (unsigned long long)raw_dty * (psc + 1) * NSEC_PER_SEC;
	result->duty_cycle = DIV_ROUND_UP_ULL(dty, rate);
stop:
	GB02FUNC1280(0, priv, GB02MAC1969);
	GB02FUNC1280(0, priv, pwm->hwpwm < 2 ? GB02MAC1967 : GB02MAC1968);
	GB02FUNC1280(0, priv, GB02MAC1971);
unlock:
	mutex_unlock(&priv->lock);

	return ret;
}

static int GB02FUNC1328(struct GB02STR179 *priv, int ch,
						   int duty_ns, int period_ns)
{
	unsigned long long prd, div, dty;
	unsigned int prescaler = 0;
	u32 ccmr, mask, shift;

	/* Period and prescaler values depends on clock rate */
	div = (unsigned long long)(priv->clk_rate) * period_ns;

	do_div(div, NSEC_PER_SEC);
	prd = div;

	while (div > priv->max_arr) {
		prescaler++;
		div = prd;
		do_div(div, prescaler + 1);
	}

	prd = div;

	if (prescaler > GB02MAC2023)
		return -EINVAL;

	/*
	 * All channels share the same prescaler and counter so when two
	 * channels are active at the same time we can't change them
	 */
	if (GB02FUNC1289(priv) & ~(1 << ch * 4)) {
		u32 psc, arr;

		psc = GB02FUNC1281(priv, GB02MAC1971);
		arr = GB02FUNC1281(priv, GB02MAC1972);

		if ((psc != prescaler) || (arr != prd - 1))
			return -EBUSY;
	}

	GB02FUNC1280(prescaler, priv, GB02MAC1971);
	GB02FUNC1280(prd - 1, priv, GB02MAC1972);
	GB02FUNC1282(GB02MAC1982, priv, GB02MAC1961, GB02MAC1982);

	/* Calculate the duty cycles */
	dty = prd * duty_ns;
	do_div(dty, period_ns);

	GB02FUNC1290(priv, ch, dty);

	/* Configure output mode */
	shift = (ch & 0x1) * GB02MAC1944;
	ccmr = (GB02MAC1996 | GB02MAC1997) << shift;
	mask = GB02MAC1945 << shift;

	if (ch < 2)
		GB02FUNC1282(ccmr, priv, GB02MAC1967, mask);
	else
		GB02FUNC1282(ccmr, priv, GB02MAC1968, mask);

	GB02FUNC1282(GB02MAC2018 | GB02MAC2017, priv,
						GB02MAC1977, GB02MAC2018 | GB02MAC2017);

	return 0;
}

static int GB02FUNC1333(struct GB02STR179 *priv, int ch,
								 enum pwm_polarity polarity)
{
	u32 mask;

	mask = GB02MAC2005 << (ch * 4);
	if (priv->have_complementary_output)
		mask |= GB02MAC2007 << (ch * 4);

	GB02FUNC1282(polarity == PWM_POLARITY_NORMAL ? 0 : mask,
						priv, GB02MAC1969, mask);

	return 0;
}

static int GB02FUNC1335(struct GB02STR179 *priv, int ch)
{
	u32 mask;

	/* Enable channel */
	mask = GB02MAC2004 << (ch * 4);
	if (priv->have_complementary_output)
		mask |= GB02MAC2006 << (ch * 4);

	GB02FUNC1282(mask, priv, GB02MAC1969, mask);

	/* Make sure that registers are updated */
	GB02FUNC1282(GB02MAC1995, priv, GB02MAC1966, GB02MAC1995);

	/* Enable controller */
	GB02FUNC1282(GB02MAC1980, priv, GB02MAC1961, GB02MAC1980);

	return 0;
}

static void GB02FUNC1338(struct GB02STR179 *priv, int ch)
{
	u32 mask;

	/* Disable channel */
	mask = GB02MAC2004 << (ch * 4);
	if (priv->have_complementary_output)
		mask |= GB02MAC2006 << (ch * 4);

	GB02FUNC1282(0, priv, GB02MAC1969, mask);

	/* When all channels are disabled, we can disable the controller */
	if (!GB02FUNC1289(priv))
		GB02FUNC1282(0, priv, GB02MAC1961, GB02MAC1980);
}

static int GB02FUNC1344(struct pwm_chip *chip, struct pwm_device *pwm,
						  const struct pwm_state *state)
{
	bool enabled;
	struct GB02STR179 *priv = GB02FUNC1462(chip);
	int ret;

	enabled = pwm->state.enabled;

	if (enabled && !state->enabled) {
		GB02FUNC1338(priv, pwm->hwpwm);
		return 0;
	}

	if (state->polarity != pwm->state.polarity)
		GB02FUNC1333(priv, pwm->hwpwm, state->polarity);

	ret = GB02FUNC1328(priv, pwm->hwpwm,
						  state->duty_cycle, state->period);
	if (ret)
		return ret;

	if (!enabled && state->enabled)
		ret = GB02FUNC1335(priv, pwm->hwpwm);

	return ret;
}
#if KERNEL_VERSION(5, 3, 18) <= LINUX_VERSION_CODE
static int gb02_pwm_apply_locked(struct pwm_chip *chip, struct pwm_device *pwm,
								 const struct pwm_state *state)
#else
static int gb02_pwm_apply_locked(struct pwm_chip *chip, struct pwm_device *pwm,
								 struct pwm_state *state)
#endif
{
	struct GB02STR179 *priv = GB02FUNC1462(chip);
	int ret;

	/* protect common prescaler for all active channels */
	mutex_lock(&priv->lock);
	GB02MAC1942("%s %d %d %d %d\n", __func__, chip->base, chip->npwm, pwm->hwpwm, pwm->pwm);
	ret = GB02FUNC1344(chip, pwm, state);
	mutex_unlock(&priv->lock);

	return ret;
}

static void GB02FUNC1490(struct pwm_chip *chip,
							   struct pwm_device *pwm,
							   struct pwm_state *state)
{
	struct GB02STR179 *priv = GB02FUNC1462(chip);

	mutex_lock(&priv->lock);

	//xxxxxx;
	GB02MAC1942("%s %d %llx %llx %d\n", __func__, state->polarity, state->period, state->duty_cycle, state->enabled);
	mutex_unlock(&priv->lock);
}

static const struct pwm_ops gb02_pwm_ops = {
	.owner = THIS_MODULE,
	.get_state = GB02FUNC1490,
	.apply = gb02_pwm_apply_locked,
	.capture = IS_ENABLED(CONFIG_DMA_ENGINE) ? GB02FUNC1320 : NULL,
};
static void GB02FUNC1350(struct GB02STR179 *priv)
{
	u32 ccer;

	/*
	 * If complementary bit doesn't exist writing 1 will have no
	 * effect so we can detect it.
	 */
	GB02FUNC1282(GB02MAC2006, priv, GB02MAC1969,
						GB02MAC2006);
	ccer = GB02FUNC1281(priv, GB02MAC1969);
	GB02FUNC1282(0, priv, GB02MAC1969, GB02MAC2006);

	priv->have_complementary_output = (ccer != 0);
}

static int GB02FUNC1493(struct GB02STR179 *priv)
{
	u32 ccer;
	int npwm = 0;

	/*
	 * If channels enable bits don't exist writing 1 will have no
	 * effect so we can detect and count them.
	 */
	GB02FUNC1282(GB02MAC2014, priv, GB02MAC1969,
						GB02MAC2014);
	ccer = GB02FUNC1281(priv, GB02MAC1969);
	GB02FUNC1282(0, priv, GB02MAC1969, GB02MAC2014);

	if (ccer & GB02MAC2004)
		npwm++;

	if (ccer & GB02MAC2008)
		npwm++;

	if (ccer & GB02MAC2010)
		npwm++;

	if (ccer & GB02MAC2012)
		npwm++;

	return npwm;
}

#ifdef PWM_MISCDEVICE_DEBUG
static int GB02FUNC251(struct inode *inode, struct file *filp)
{
	filp->private_data = gb02_pwm_infos;
	return 0;
}

static int GB02FUNC253(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static long GB02FUNC1356(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	GB02MAC1942("enter ioctrl!\n");

	switch (cmd) {
	default:
		break;
	}

	return ret;
}

static const struct file_operations mmap_fops = {
	.owner = THIS_MODULE,
	.open = GB02FUNC251,
	.release = GB02FUNC253,
	.unlocked_ioctl = GB02FUNC1356,
};
static struct miscdevice gb02_pwm_misc = {
	.fops	   = &mmap_fops,
	.name	   = "GB02STR179",
	.minor	  = MISC_DYNAMIC_MINOR,
};
#endif

static void GB02FUNC1358(struct GB02STR181 *GB02STR181)
{
	GB02FUNC528(GB02MAC740, GB02STR181->pci_mmio_bar4, GB02MAC734);
}

static void GB02FUNC1285(struct GB02STR181 *GB02STR181)
{
	unsigned int val;

	val = GB02FUNC530(GB02STR181->sysctl_iofunc_base, GB02MAC851);
	val &= (~BIT(10));
	val &= (~BIT(9));
	GB02MAC1942("pwm set mux %x\n", val);
	GB02FUNC528(val, GB02STR181->sysctl_iofunc_base, GB02MAC851);
}

static unsigned long GB02FUNC1360(struct GB02STR181 *GB02STR181)
{
	unsigned long periph_freq_div, periph_freq;

	periph_freq_div = GB02FUNC530(GB02STR181->sysctl_iofunc_base, GB02MAC850)
					  & 0x7;
	periph_freq = GB02MAC757 * 1000 / (periph_freq_div + 1);

	return periph_freq;
}

static u32 GB02FUNC1501(struct GB02STR179 *priv)
{
	u32 val = 0;

	/*
	 * Only the available bits will be written so when readback
	 * we get the maximum value of auto reload register
	 */
	GB02FUNC1280(~0L, priv, GB02MAC1972);
	val = GB02FUNC1281(priv, GB02MAC1972);
	GB02FUNC1280(0x0, priv, GB02MAC1972);

	return val;
}

static int GB02FUNC1361(struct GB02STR181 *GB02STR181, u32 pwm_ctrl_no)
{
	struct GB02STR179 *priv = &(GB02STR181->pwm);
	int ret = 0;

	mutex_init(&priv->lock);
	priv->regs = GB02STR181->res;
	if (priv->regs == NULL) {
		dev_err(GB02STR181->dev, "invalid address\n");
		ret = -EINVAL;
		return ret;
	}
	priv->ctrl_no = pwm_ctrl_no;
	priv->max_arr = GB02FUNC1501(priv);
	priv->clk_rate = GB02FUNC1360(GB02STR181);
	GB02MAC1942(" pwm_ctrl:%x max_arr %x clk_rate %lx\n", priv->ctrl_no, priv->max_arr, priv->clk_rate);
	GB02FUNC1350(priv);

	priv->chip.base = -1;
	priv->chip.dev = GB02STR181->dev;
	priv->chip.ops = &gb02_pwm_ops;
	priv->chip.npwm = GB02FUNC1493(priv);

	ret = pwmchip_add(&priv->chip);
	if (ret < 0)
		return ret;

	priv->driver_data = GB02STR181;

	dev_dbg(GB02STR181->dev, "registers %p (PWM%d)\n",
			priv->regs, priv->ctrl_no);

	return 0;
}

static int GB02FUNC1506(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR181 *GB02STR181;
	struct GB02STR70 *pcie_info;
	void __iomem *pcie_mmio_bar;
	int id, ret = 0;

	GB02MAC1942("Start to init GB02 MCU's PWM%d:\n", device_id);
	pcie_info = gb_dev->gb_pcie;
	id = device_id;
	GB02STR181 = devm_kzalloc(gb_dev->dev, sizeof(struct GB02STR181), GFP_KERNEL);
	if (GB02STR181 == NULL)
		return -ENOMEM;
	GB02STR181->dev = gb_dev->dev;
	if (id > GB02MAC760) {
		dev_err(GB02STR181->dev, "invalid device num=%d\n", id);
		ret = -ENOENT;
		goto err_return;
	}
	GB02STR181->pwm.dev = GB02STR181->dev;

	/* PCIE's BAR0 */
	pcie_mmio_bar = gb_dev->gb_pcie->pci_bars[GB02MAC718].mmio; //bar0
	if (!pcie_mmio_bar) {
		dev_err(GB02STR181->dev, "cannot get PCIE's BAR0 info\n");
		ret = -ENOMEM;
		goto err_return;
	} else {
		GB02STR181->pci_mmio_bar0 = pcie_mmio_bar;
		GB02MAC1942("%s:%d:BAR0 : %p", __func__, __LINE__, GB02STR181->pci_mmio_bar0);
	}
	GB02STR181->res = pcie_mmio_bar + GB02MAC748 + GB02MAC763 + id * GB02MAC766;

	/* PCIE's BAR1 */
	pcie_mmio_bar = gb_dev->gb_pcie->pci_bars[GB02MAC721].mmio; //bar1
	if (!pcie_mmio_bar) {
		dev_err(GB02STR181->dev, "cannot get PCIE's BAR1 info\n");
		ret = -ENOMEM;
		goto err_return;
	} else {
		GB02STR181->pci_mmio_bar1 = pcie_mmio_bar;
		GB02MAC1942("%s:%d:BAR1 : %p", __func__, __LINE__, GB02STR181->pci_mmio_bar1);
	}

	/* PCIE's BAR4 */
	pcie_mmio_bar = gb_dev->gb_pcie->pci_bars[GB02MAC726].mmio; //bar4
	if (!pcie_mmio_bar) {
		dev_err(GB02STR181->dev, "cannot get PCIE's BAR4 info\n");
		ret = -ENOMEM;
		goto err_return;
	} else {
		GB02STR181->pci_mmio_bar4 = pcie_mmio_bar;
		GB02MAC1942("%s:%d:BAR4 : %p", __func__, __LINE__, GB02STR181->pci_mmio_bar4);
	}

	/* MCU's SYSCTL */
	pcie_mmio_bar = gb_dev->gb_pcie->pci_bars[GB02MAC718].mmio; //bar0
	if (!pcie_mmio_bar) {
		dev_err(GB02STR181->dev, "cannot get MCU's SYSCTL info\n");
		ret = -ENOMEM;
		goto err_return;
	} else {
		GB02STR181->sysctl_iofunc_base = pcie_mmio_bar;
		GB02MAC1942("%s:%d:SYSCTL : %p", __func__, __LINE__, GB02STR181->sysctl_iofunc_base);
	}

	GB02FUNC1358(GB02STR181);
	GB02FUNC1285(GB02STR181);
	ret = GB02FUNC1361(GB02STR181, id);
	if (ret) {
		dev_err(GB02STR181->dev, "PWM init failed(%d)\n", ret);
		goto err_return;
	}

	peri_info->priv = GB02STR181;
	gb02_pwm_infos[id] = GB02STR181;
	GB02MAC1942("GB02 MCU's PWM initialized\n");
	if (id == GB02MAC2034)
		GB02FUNC1309(&GB02STR181->pwm);

	return ret;
err_return:
	return ret;
}

static int GB02FUNC1512(struct GB02STR72 *peri_info)
{
	struct GB02STR181 *GB02STR181 = (struct GB02STR181 *)peri_info->priv;
	struct GB02STR179 *priv = &(GB02STR181->pwm);
	unsigned int i = 0;
	u32 id = priv->ctrl_no;

	for (i = 0; i < priv->chip.npwm; i++)
		pwm_disable(&priv->chip.pwms[i]);

	pwmchip_remove(&priv->chip);
	if (id == GB02MAC2034)
		GB02FUNC1314(priv);

	gb02_pwm_infos[id] = NULL;
	return 0;
}

int GB02FUNC1375(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	int ret;
	int device_id = 0;
#ifdef PWM_MISCDEVICE_DEBUG
	static bool pwm_misc_inited;

	if (!pwm_misc_inited) {
		ret = misc_register(&gb02_pwm_misc);
		if (ret)
			dev_err(gb_dev->dev, "unable to register pwm misc device(%d)\n", ret);
		else
			pwm_misc_inited = true;
	}
#endif
	GB02MAC1942("gb02_pwm_drv_init\n");
	device_id = peri_info->mcu_peripherals_device_id;
	ret = GB02FUNC1506(gb_dev, peri_info, device_id);
	return ret;
}

void GB02FUNC1378(struct GB02STR72 *peri_info)
{
#ifdef PWM_MISCDEVICE_DEBUG
	misc_deregister(&gb02_pwm_misc);
#endif
	GB02FUNC1512(peri_info);
}
