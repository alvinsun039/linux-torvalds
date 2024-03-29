// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "tcm_interface.h"
#include "ax99100_spi.h"

static unsigned int logflag;

int TSS_buildbuff(char *format, struct tcm_buffer *tb, ...)
{
	unsigned char *totpos;
	va_list argp;
	char *p;
	unsigned int totlen;
	unsigned char *o;
	unsigned long l;
	unsigned short s;
	unsigned char c;
	unsigned long len;
	uint16_t len16;
	unsigned char byte = 0;
	unsigned char hexflag;
	unsigned char *ptr;
	unsigned char *buffer = tb->buffer;
	unsigned int start = tb->used;
	int dummy;

	va_start(argp, tb);
	totpos = 0;
	totlen = tb->used;
	o = &buffer[totlen];
	hexflag = 0;
	p = format;
	while (*p != '\0') {
		switch (*p) {
		case ' ':
		break;
		case 'L':
		case 'X':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 4 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			l = (unsigned long)va_arg(argp, unsigned long);
			STORE32(o, 0, l);
			if (*p == 'X')
				va_arg(argp, unsigned long);
			o += 4;
			totlen += TCM_U32_SIZE;
		break;
		case 'S':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 2 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			s = (unsigned short)va_arg(argp, int);
			STORE16(o, 0, s);
			o += TCM_U16_SIZE;
			totlen += TCM_U16_SIZE;
		break;
		case 'l':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 4 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			l = (unsigned long)va_arg(argp, unsigned long);
			STORE32N(o, 0, l);
			o += TCM_U32_SIZE;
			totlen += TCM_U32_SIZE;
		break;
		case 's':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 2 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			s = (unsigned short)va_arg(argp, int);
			STORE16N(o, 0, s);
			o += TCM_U16_SIZE;
			totlen += TCM_U16_SIZE;
		break;
		case 'o':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 1 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			c = (unsigned char)va_arg(argp, int);
			*(o) = c;
			o += 1;
			totlen += 1;
		break;
		case '@':
		case '*':
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len = (int)va_arg(argp, int);
			if (totlen + 4 + len >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			STORE32(o, 0, len);
			o += TCM_U32_SIZE;
			if (len > 0)
				memcpy(o, ptr, len);
			o += len;
			totlen += len + TCM_U32_SIZE;
		break;
		case '&':
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len16 = (uint16_t)va_arg(argp, int);
			if (totlen + 2 + len16 >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len16 > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			STORE16(o, 0, len16);
			o += TCM_U16_SIZE;
			if (len16 > 0)
				memcpy(o, ptr, len16);
			o += len16;
			totlen += len16 + TCM_U16_SIZE;
		break;
		case '%':
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len = (int)va_arg(argp, int);
			if (totlen + len >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			if (len > 0)
				memcpy(o, ptr, len);
			o += len;
			totlen += len;
		break;
		case 'T':
			if (hexflag)
				return ERR_BAD_ARG;
			if (totlen + 4 >= tb->size)
				return ERR_BUFFER;
			byte = 0;
			totpos = o;
			o += TCM_U32_SIZE;
			totlen += TCM_U32_SIZE;
		break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (totlen + 1 >= tb->size)
				return ERR_BUFFER;
			byte = byte << 4;
			byte = byte |  ((*p - '0') & 0x0F);
			if (hexflag) {
				*o = byte;
				++o;
				hexflag = 0;
				totlen += 1;
			} else {
				++hexflag;
			}
		break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (totlen + 1 >= tb->size)
				return ERR_BUFFER;
			byte = byte << 4;
			byte = byte |  (((*p - 'A') & 0x0F) + 0x0A);
			if (hexflag) {
				*o = byte;
				++o;
				hexflag = 0;
				totlen += 1;
			} else {
				++hexflag;
			}
		break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if (totlen + 1 >= tb->size)
				return ERR_BUFFER;
			byte = byte << 4;
			byte = byte |  (((*p - 'a') & 0x0F) + 0x0A);
			if (hexflag) {
				*o = byte;
				++o;
				hexflag = 0;
				totlen += 1;
			} else {
				++hexflag;
			}
		break;
		case '^':
	/* the size indicator is only 16 bits long */
	/* parameters: address of length indicator,
	 *maximum number of bytes
	 *address of buffer
	 */
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len16 = (uint16_t)va_arg(argp, int);
			dummy = va_arg(argp, int);
			if (totlen + len16 >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len16 > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			STORE16(o, 0, len16);
			o += TCM_U16_SIZE;
			if (len16 > 0)
				memcpy(o, ptr, len16);
			o += len16;
			totlen += TCM_U16_SIZE + len16;
		break;
		case '!':
	/* the size indicator is 32 bytes long */
	/* parameters: address of length indicator,
	 *maximum number of bytes
	 *address of buffer
	 */
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len = va_arg(argp, int);
			dummy = va_arg(argp, int);
			if (totlen + len >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			STORE32(o, 0, len);
			o += TCM_U32_SIZE;
			if (len > 0)
				memcpy(o, ptr, len);
			o += len;
			totlen += TCM_U32_SIZE + len;
		break;
		case '#':
	/* reverse write the buffer (good for 'exponent') */
	/* the size indicator is 32 bytes long */
	/* parameters: address of length indicator,
	 *maximum number of bytes
	 *address of buffer
	 */
			if (hexflag)
				return ERR_BAD_ARG;
			byte = 0;
			len = va_arg(argp, int);
			dummy = va_arg(argp, int);
			if (totlen + len >= tb->size)
				return ERR_BUFFER;
			ptr = (unsigned char *)va_arg(argp, unsigned char *);
			if (len > 0 && ptr == NULL)
				return ERR_NULL_ARG;
			STORE32(o, 0, len);
			o += TCM_U32_SIZE;
			totlen += TCM_U32_SIZE + len;
			while (len > 0) {
				*o = ptr[len-1];
				o++;
				len--;
			}
		break;
		default:
			return ERR_BAD_ARG;
		}
		++p;
	}
	if (totpos != 0)
		STORE32(totpos, 0, totlen);
	va_end(argp);
	tb->used = totlen;
	return totlen-start;
}

void TCM_SPI_Open(int line)
{
	init_spi_func(line);
}

void TCM_SPI_Close(int line)
{
	close_spi_func(line);
}

uint32_t TCM_SPI_Transmit(int line, u8 *in, uint32_t insize,
		u8 *out, uint32_t *outsize)
{
	u8 *buffer;

	buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buffer || buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	memcpy(buffer, in, insize);
	if (ax99100_send(line, buffer, insize) < 0) {
		pr_err("%s fail\n", __func__);
		return -1;
	}
	*outsize = ax99100_recv(line, buffer, insize);
	memcpy(out, buffer, *outsize);
	kfree(buffer);

	return 0;
}

static uint32_t TCM_Transmit_Internal(int line, struct tcm_buffer *tb,
		const char *msg, int allowTransport)
{
	uint32_t rc = 0;
	u8 *indata;
	u8 *outdata;
	int readlen = 0;
	int i = 0;

	outdata = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!outdata || outdata == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	indata = tb->buffer;
	if (logflag == 1) {
		pr_err(" ==================================\n ");
		pr_err(" *****TCM transmit input command %s*****\n ", msg);
		pr_err(" input length: %d\n", tb->used);
		for (i = 0; i < tb->used; i++) {
			pr_err("%02x ", indata[i]);
		if (i > 0 && (i + 1) % 16 == 0)
			pr_err("\n");
		}
		pr_err("\n");
	}

	TCM_SPI_Transmit(line, indata, tb->used, outdata, &readlen);
	memcpy(tb->buffer, outdata, readlen);
	tb->used = readlen;

	if (logflag == 1) {
		pr_err(" *****TCM transmit output*****\n ");
		pr_err(" output length: %d\n", readlen);
		for (i = 0; i < readlen; i++) {
			pr_err("%02x ", outdata[i]);
			if (i > 0 && (i + 1) % 16 == 0)
				pr_err("\n");
		}
		pr_err("\n");
		pr_err(" ==================================\n ");
	}

	kfree(outdata);

	return rc;
}

uint32_t TCM_Transmit(int line, struct tcm_buffer *tb, const char *msg)
{
	return TCM_Transmit_Internal(line, tb, msg, 1);
}

uint32_t tcm_get_random(int line, UINT32 len, u8 *data)
{
	uint32_t ret;
	uint32_t ordinal = TCM_ORD_GetRandom;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	ret = TSS_buildbuff("00 C1 T L L", &tcmdata, ordinal, len);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "GetRandom");
	if (ret != 0)
		goto ERR;

	memcpy(data, &tcmdata.buffer[TCM_DATA_OFFSET+TCM_U32_SIZE], len);
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return len;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}
EXPORT_SYMBOL_GPL(tcm_get_random);

/***Call Hardware(TCM) HASH(SM3) algrithm***/
uint32_t tcm_buffer_load32(const struct tcm_buffer *tb,
		uint32_t off, uint32_t *val)
{
	if (off+3 >= tb->used)
		return ERR_BUFFER;

	*val = LOAD32(tb->buffer, off);
	return 0;
}

uint32_t TCM_SCHStart(int line, uint32_t *max)
{
	uint32_t ret = 0;
	uint32_t ordinal = TCM_ORD_SCHStart;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	ret = TSS_buildbuff("00 C1 T L", &tcmdata, ordinal);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "SCHStart");
	if (ret != 0)
		goto ERR;

	STORE32(tcmdata.buffer, 6, ret);
	if (ret == 0)
		tcm_buffer_load32(&tcmdata, TCM_DATA_OFFSET, max);
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}

uint32_t TCM_SCHUpdate(int line, unsigned char *data, uint32_t len)
{
	uint32_t ret = 0;
	uint32_t ordinal = TCM_ORD_SCHUpdate;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	ret = TSS_buildbuff("00 C1 T L L %", &tcmdata, ordinal, len, len, data);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "SCHUpdate");
	if (ret != 0)
		goto ERR;

	STORE32(tcmdata.buffer, 6, ret);
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;

	return ret;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}

uint32_t TCM_SCHComplete(int line, unsigned char *data,
		uint32_t len, unsigned char *digest)
{
	uint32_t ret = 0;
	uint32_t ordinal = TCM_ORD_SCHComplete;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	len = 0;
	ret = TSS_buildbuff("00 C1 T L L %", &tcmdata, ordinal, len, len, data);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "SCHComplete");
	if (ret != 0)
		goto ERR;

	STORE32(tcmdata.buffer, 6, ret);
	if (ret == 0)
		memcpy(digest, &tcmdata.buffer[TCM_DATA_OFFSET], 32);

	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}

int tcm_hash_sm3(int line, uint8_t *in_data, uint32_t indata_len,
		uint8_t *out_data, uint32_t *out_datalen)
{

	uint32_t ret = 0;
	uint32_t i = 0;
	uint32_t updateMax = 0;

	ret = TCM_SCHStart(line, &updateMax);
	if (ret != 0)
		return ret;
	if (indata_len < updateMax) {
		ret = TCM_SCHUpdate(line, in_data, indata_len);
		if (ret != 0)
			return ret;
		*out_datalen = 32;
		ret = TCM_SCHComplete(line, in_data, indata_len, out_data);
		if (ret != 0)
			return ret;
	} else {
		for (i = 0; i < (indata_len/updateMax); i++) {
			ret = TCM_SCHUpdate(line, in_data+updateMax*i,
					updateMax);
			if (ret != 0)
				return ret;
		}
		ret = TCM_SCHUpdate(line, in_data+updateMax*i,
				indata_len-updateMax*i);
		if (ret != 0)
			return ret;
		*out_datalen = 32;
		ret = TCM_SCHComplete(line, in_data+updateMax*i,
				indata_len-updateMax*i, out_data);
		if (ret != 0)
			return ret;
	}
	return 0;
}

/***pcr function***/
uint32_t TCM_PcrRead(int line, uint32_t pcrIndex, unsigned char *pcrvalue)
{
	uint32_t ret;
	uint32_t ordinal = TCM_ORD_PcrRead;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	if (pcrvalue == NULL) {
		ret = ERR_NULL_ARG;
		goto ERR;
	}
	ret = TSS_buildbuff("00 C1 T L L", &tcmdata, ordinal, pcrIndex);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "PCRRead");
	if (ret != 0)
		goto ERR;
	memcpy(pcrvalue, &tcmdata.buffer[TCM_DATA_OFFSET], TCM_HASH_SIZE);
	return 0;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}

uint32_t TCM_PcrReset(int line, uint32_t pcrIndex)
{
	uint32_t ret;
	uint32_t ordinal = TCM_ORD_PcrReset;
	struct TCM_PCR_SELECTION pcrselection;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	pcrselection.sizeOfSelect = 0x0003;
	pcrselection.pcrSelect[0] = 0x01;
	pcrselection.pcrSelect[1] = 0x01;
	pcrselection.pcrSelect[2] = 0x01;
	if (pcrIndex < 8) {
		pcrselection.pcrSelect[0] <<= pcrIndex;
		pcrselection.pcrSelect[1] = 0x00;
		pcrselection.pcrSelect[2] = 0x00;
	} else if ((pcrIndex > 7) && (pcrIndex < 16)) {
		pcrselection.pcrSelect[0] = 0x00;
		pcrselection.pcrSelect[1] <<= (pcrIndex - 8);
		pcrselection.pcrSelect[2] = 0x00;
	} else if ((pcrIndex > 15) && (pcrIndex < 24)) {
		pcrselection.pcrSelect[0] = 0x00;
		pcrselection.pcrSelect[1] = 0x00;
		pcrselection.pcrSelect[2] <<= (pcrIndex - 16);
	} else {
		ret = ERR_PCR_LIST_NOT_IMA;
		goto ERR;
	}

	ret = TSS_buildbuff("00 C1 T L S o o o", &tcmdata, ordinal,
		pcrselection.sizeOfSelect, pcrselection.pcrSelect[0],
		pcrselection.pcrSelect[1], pcrselection.pcrSelect[2]);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "PCRReset");
	if (ret != 0)
		goto ERR;
	return 0;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}

uint32_t TCM_PcrExtend(int line, uint32_t pcrIndex, unsigned char *pcrvalue)
{
	uint32_t ret;
	uint32_t ordinal = TCM_ORD_Extend;
	struct tcm_buffer tcmdata;

	tcmdata.size = TCM_MAX_BUFF_SIZE;
	tcmdata.used = 0;
	tcmdata.flags = BUFFER_FLAG_ON_STACK;
	tcmdata.buffer = kzalloc(TCM_MAX_BUFF_SIZE, GFP_KERNEL);
	if (!tcmdata.buffer || tcmdata.buffer == NULL) {
		pr_err("%s kzalloc fail!\n", __func__);
		return false;
	}

	if (pcrvalue == NULL) {
		ret = ERR_NULL_ARG;
		goto ERR;
	}
	ret = TSS_buildbuff("00 C1 T L L %", &tcmdata, ordinal,
			pcrIndex, TCM_HASH_SIZE, pcrvalue);
	if ((ret & ERR_MASK) != 0)
		goto ERR;
	ret = TCM_Transmit(line, &tcmdata, "PcrExtend");
	if (ret != 0)
		goto ERR;
	memcpy(pcrvalue, &tcmdata.buffer[TCM_DATA_OFFSET], TCM_HASH_SIZE);
	return 0;
ERR:
	kfree(tcmdata.buffer);
	tcmdata.buffer = NULL;
	return ret;
}


int tcm_pcr_read(int line, UINT32 PcrIndex, u8 *PcrValue)
{
	int ret = 0;
	uint8_t pcrreadvalue[TCM_HASH_SIZE] = {0};

	memcpy(pcrreadvalue, PcrValue, TCM_HASH_SIZE);
	ret = TCM_PcrRead(line, PcrIndex, pcrreadvalue);
	if (ret != 0)
		return ret;
	memcpy(PcrValue, pcrreadvalue, TCM_HASH_SIZE);
	return ret;
}

int tcm_pcr_extend(int line, UINT32 PcrIndex, u8 *PcrValue)
{
	int ret = 0;
	uint8_t pcrsetvalue[TCM_HASH_SIZE] = {0};

	memcpy(pcrsetvalue, PcrValue, TCM_HASH_SIZE);
	ret = TCM_PcrExtend(line, PcrIndex, pcrsetvalue);
	if (ret != 0)
		return ret;
	memcpy(PcrValue, pcrsetvalue, TCM_HASH_SIZE);
	return ret;
}

int tcm_pcr_reset(int line, UINT32 PcrIndex)
{
	return TCM_PcrReset(line, PcrIndex);
}
