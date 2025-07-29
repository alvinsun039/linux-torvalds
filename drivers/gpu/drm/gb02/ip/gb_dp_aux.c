/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#define DPTX_NO_DEBUG_REG

#include "gb_dp_dptx.h"

static int GB02FUNC316(struct dptx *dptx)
{
	u32 auxsts;
	u32 status;
	u32 auxm;
	u32 br;
	int count;

	count = 0;
	while (1) {
		auxsts = dptx_readl(dptx, GB02MAC2116);

		if (!(auxsts & GB02MAC2218))
			break;

		count++;
		if (count > 5000)
			return -ETIMEDOUT;
#ifdef GB02_PAL_DP
		msleep(1000);
#else
		udelay(1);
#endif
	};


	auxsts = dptx_readl(dptx, GB02MAC2116);

	status = (auxsts & DPTX_AUX_STS_STATUS_MASK) >>
		GB02MAC2211;

	auxm = (auxsts & DPTX_AUX_STS_AUXM_MASK) >>
		GB02MAC2217;
	br = (auxsts & DPTX_AUX_STS_BYTES_READ_MASK) >>
		GB02MAC2221;

	dptx_dbg_aux(dptx, "%s: 0x%08x: sts=%d, auxm=%d,\
		br=%d, replyrcvd=%d, replyerr=%d, timeout=%d, disconn=%d\n",
		     __func__, auxsts, status, auxm, br,
		     !!(auxsts & GB02MAC2218),
		     !!(auxsts & GB02MAC2220),
		     !!(auxsts & GB02MAC2219),
		     !!(auxsts & GB02MAC2222));

	switch (status) {
	case GB02MAC2212:
		dptx_dbg_aux(dptx, "%s: GB02MAC2212\n", __func__);
		break;
	case GB02MAC2213:
		dptx_dbg_aux(dptx, "%s: GB02MAC2213\n", __func__);
		break;
	case GB02MAC2214:
		dptx_dbg_aux(dptx, "%s: GB02MAC2214\n", __func__);
		break;
	case GB02MAC2215:
		dptx_dbg_aux(dptx, "%s: GB02MAC2215\n",
			     __func__);
		break;
	case GB02MAC2216:
		dptx_dbg_aux(dptx, "%s: GB02MAC2216\n",
			     __func__);
		break;
	default:
		dptx_err(dptx, "Invalid AUX status 0x%x\n", status);
		break;
	}

	dptx->aux.data[0] = dptx_readl(dptx, GB02MAC2117);
	dptx->aux.data[1] = dptx_readl(dptx, GB02MAC2118);
	dptx->aux.data[2] = dptx_readl(dptx, GB02MAC2119);
	dptx->aux.data[3] = dptx_readl(dptx, GB02MAC2120);
	dptx->aux.sts = auxsts;

	return 0;
}

static void GB02FUNC332(struct dptx *dptx)
{
	dptx_writel(dptx, GB02MAC2117, 0);
	dptx_writel(dptx, GB02MAC2118, 0);
	dptx_writel(dptx, GB02MAC2119, 0);
	dptx_writel(dptx, GB02MAC2120, 0);
}

static int GB02FUNC335(struct dptx *dptx, u8 *bytes, unsigned int len)
{
	unsigned int i;

	u32 *data = dptx->aux.data;

	for (i = 0; i < len; i++)
		bytes[i] = (data[i / 4] >> ((i % 4) * 8)) & 0xff;

	return len;
}

static int GB02FUNC337(struct dptx *dptx, u8 const *bytes,
			       unsigned int len)
{
	unsigned int i;
	u32 data[4];

	memset(data, 0, sizeof(u32) * 4);

	for (i = 0; i < len; i++)
		data[i / 4] |= (bytes[i] << ((i % 4) * 8));

	dptx_writel(dptx, GB02MAC2117, data[0]);
	dptx_writel(dptx, GB02MAC2118, data[1]);
	dptx_writel(dptx, GB02MAC2119, data[2]);
	dptx_writel(dptx, GB02MAC2120, data[3]);

	return len;
}

int GB02FUNC340(struct dptx *dptx,
		       bool rw,
		       bool i2c,
		       bool mot,
		       bool addr_only,
		       u32 addr,
		       u8 *bytes,
		       unsigned int len)
{
	int retval;
	int tries = 0;
	u32 auxcmd;
	u32 type;
	unsigned int status;
	unsigned int br;

#ifdef GB02_PAL_DP
	msleep(8000);
#endif
again:
	mdelay(1);
	tries++;
	if (tries > 5) {
		dptx_err(dptx, "AUX exceeded retries\n");
		return -EINVAL;
	}

	dptx_dbg_aux(dptx, "%s: addr=0x%08x, len=%d, try=%d\n",
		     __func__, addr, len, tries);

	if (WARN((len > 16) || (len == 0),
		 "AUX read/write len must be 1-16, len=%d\n", len))
		return -EINVAL;

	type = rw ? GB02MAC2207 : GB02MAC2206;

	if (!i2c)
		type |= GB02MAC2210;

	if (i2c && mot)
		type |= GB02MAC2209;

	GB02FUNC332(dptx);

	if (!rw)
		GB02FUNC337(dptx, bytes, len);

	auxcmd = ((type << GB02MAC2205) |
		  (addr << GB02MAC2204) |
		  ((len - 1) << GB02MAC2202));

	if (addr_only)
		auxcmd |= GB02MAC2203;

	dptx_writel(dptx, GB02MAC2115, auxcmd);

	retval = GB02FUNC316(dptx);
	if (retval)
		return retval;

	if (retval == -ETIMEDOUT || (dptx->aux.sts & GB02MAC2219)) {
		dptx_warn(dptx, "AUX timed out\n");
		return retval;
	}

	if (retval == -ESHUTDOWN) {
		dptx_dbg(dptx, "AUX aborted on driver shutdown\n");
		return retval;
	}

	if (atomic_read(&dptx->aux.abort)) {
		dptx_dbg(dptx, "AUX aborted\n");
		return -ETIMEDOUT;
	}

	status = (dptx->aux.sts & DPTX_AUX_STS_STATUS_MASK) >>
		GB02MAC2211;

	br = (dptx->aux.sts & DPTX_AUX_STS_BYTES_READ_MASK) >>
		GB02MAC2221;

	switch (status) {
	case GB02MAC2212:
		dptx_dbg_aux(dptx, "AUX Success\n");
#if 1
		if (rw && br < len) {
			dptx_dbg_aux(dptx, "BR=0, Retry\n");
			//GB02FUNC1059(dptx, GB02MAC2169);
			goto again;
		}
#endif
		break;
	case GB02MAC2213:
	case GB02MAC2215:
		dptx_dbg(dptx, "AUX Nack\n");
		return -EINVAL;
	case GB02MAC2216:
	case GB02MAC2214:
		dptx_dbg(dptx, "AUX Defer\n");
		goto again;
	default:
		dptx_err(dptx, "AUX Status Invalid\n");
		GB02FUNC1059(dptx, GB02MAC2169);
		goto again;
	}

	if (rw)
		GB02FUNC335(dptx, bytes, len);

	return 0;
}

int GB02FUNC362(struct dptx *dptx,
		      bool rw,
		      bool i2c,
		      u32 addr,
		      u8 *bytes,
		      unsigned int len)
{
	int retval;
	unsigned int i;

	for (i = 0; i < len; ) {
		unsigned int curlen;

		curlen = min_t(unsigned int, len - i, 16);

		if (!i2c) {
			retval = GB02FUNC340(dptx, rw, i2c, true, false,
					     addr + i, &bytes[i], curlen);
		} else {
			retval = GB02FUNC340(dptx, rw, i2c, true, false,
				addr, &bytes[i], curlen);
		}
		if (retval)
			return retval;

		i += curlen;
	}

	return 0;
}

int GB02FUNC372(struct dptx *dptx,
			     u32 device_addr,
			     u8 *bytes,
			     u32 len)
{
	return GB02FUNC362(dptx, true, true,
				 device_addr, bytes, len);
}

int GB02FUNC375(struct dptx *dptx,
	unsigned int device_addr)
{
	u8 bytes[1];
	return GB02FUNC340(dptx, 0, true, false, true,
			     device_addr, &bytes[0], 1);
}

int GB02FUNC376(struct dptx *dptx,
			    u32 device_addr,
			    u8 *bytes,
			    u32 len)
{
	return GB02FUNC362(dptx, false, true,
				 device_addr, bytes, len);
}

int GB02FUNC378(struct dptx *dptx,
				u32 reg_addr,
				u8 *bytes,
				u32 len)
{
	return GB02FUNC362(dptx, true, false,
				 reg_addr, bytes, len);
}

int GB02FUNC380(struct dptx *dptx,
			       u32 reg_addr,
			       u8 *bytes,
			       u32 len)
{
	return GB02FUNC362(dptx, false, false,
				 reg_addr, bytes, len);
}

int GB02FUNC382(struct dptx *dptx, u32 addr, u8 *byte)
{
	return GB02FUNC378(dptx, addr, byte, 1);
}

int GB02FUNC385(struct dptx *dptx, u32 addr, u8 byte)
{
	return GB02FUNC380(dptx, addr, &byte, 1);
}
