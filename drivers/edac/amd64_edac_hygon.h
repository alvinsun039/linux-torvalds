/* SPDX-License-Identifier: GPL-2.0-only*/
#ifndef _AMD64_EDAC_HYGON_H_
#define _AMD64_EDAC_HYGON_H_

static int df_indirect_read_instance(u16 node, u8 func, u16 reg, u8 instance_id, u32 *lo);
static int df_indirect_read_broadcast(u16 node, u8 func, u16 reg, u32 *lo);
static int fixup_node_id(int node_id, struct mce *m);
static inline void error_address_to_page_and_offset(u64 error_address, struct err_info *err);
static void __log_ecc_error(struct mem_ctl_info *mci, struct err_info *err, u8 ecc_type);
static int __df_indirect_read(u16 node, u8 func, u16 reg, u8 instance_id, u32 *lo);
static u32 get_umc_base_f18h_m4h(u16 node, u8 channel);
static inline u32 get_umc_reg(struct amd64_pvt *pvt, u32 reg);

struct addr_ctx_hygon {
	u64 ret_addr;
	u32 tmp;
	u16 nid;
	u8 inst_id;
};

static int umc_normaddr_to_sysaddr_hygon(u64 norm_addr, u16 nid, u8 umc, u64 *sys_addr)
{
	u64 dram_base_addr, dram_limit_addr, dram_hole_base;

	u8 die_id_shift, socket_id_shift;
	u16 die_id_mask, socket_id_mask;
	u8 intlv_num_dies, intlv_num_chan, intlv_num_sockets;
	u8 intlv_addr_sel, intlv_addr_bit;
	u8 num_intlv_bits, hashed_bit;
	u8 lgcy_mmio_hole_en, base = 0;
	u8 cs_mask, cs_id = 0;
	bool hash_enabled = false;

	struct addr_ctx_hygon ctx;

	memset(&ctx, 0, sizeof(ctx));

	/* Start from the normalized address */
	ctx.ret_addr = norm_addr;

	ctx.nid = nid;
	ctx.inst_id = umc;

	/* Read DramOffset, check if base 1 is used. */
	if ((hygon_f18h_m4h() || hygon_f18h_m10h()) &&
	    df_indirect_read_instance(nid, 0, 0x214, umc, &ctx.tmp))
		goto out_err;
	else if (df_indirect_read_instance(nid, 0, 0x1B4, umc, &ctx.tmp))
		goto out_err;

	/* Remove HiAddrOffset from normalized address, if enabled: */
	if (ctx.tmp & BIT(0)) {
		u64 hi_addr_offset = (ctx.tmp & GENMASK_ULL(31, 20)) << 8;

		if (norm_addr >= hi_addr_offset) {
			ctx.ret_addr -= hi_addr_offset;
			base = 1;
		}
	}

	/* Read D18F0x110 (DramBaseAddress). */
	if (df_indirect_read_instance(nid, 0, 0x110 + (8 * base), umc, &ctx.tmp))
		goto out_err;

	/* Check if address range is valid. */
	if (!(ctx.tmp & BIT(0))) {
		pr_err("%s: Invalid DramBaseAddress range: 0x%x.\n", __func__, ctx.tmp);
		goto out_err;
	}

	if (hygon_f18h_m4h() || hygon_f18h_m10h())
		intlv_num_sockets = (ctx.tmp >> 2) & 0x3;
	lgcy_mmio_hole_en = ctx.tmp & BIT(1);
	intlv_num_chan	  = (ctx.tmp >> 4) & 0xF;
	intlv_addr_sel	  = (ctx.tmp >> 8) & 0x7;
	dram_base_addr	  = (ctx.tmp & GENMASK_ULL(31, 12)) << 16;

	/* {0, 1, 2, 3} map to address bits {8, 9, 10, 11} respectively */
	if (intlv_addr_sel > 3) {
		pr_err("%s: Invalid interleave address select %d.\n", __func__, intlv_addr_sel);
		goto out_err;
	}

	/* Read D18F0x114 (DramLimitAddress). */
	if (df_indirect_read_instance(nid, 0, 0x114 + (8 * base), umc, &ctx.tmp))
		goto out_err;

	if (!hygon_f18h_m4h() && !hygon_f18h_m10h())
		intlv_num_sockets = (ctx.tmp >> 8) & 0x1;
	intlv_num_dies	  = (ctx.tmp >> 10) & 0x3;
	dram_limit_addr	  = ((ctx.tmp & GENMASK_ULL(31, 12)) << 16) | GENMASK_ULL(27, 0);

	intlv_addr_bit = intlv_addr_sel + 8;

	if ((hygon_f18h_m4h() && boot_cpu_data.x86_model >= 0x6) ||
	    hygon_f18h_m10h()) {
		if (df_indirect_read_instance(nid, 0, 0x60, umc, &ctx.tmp))
			goto out_err;
		intlv_num_dies = ctx.tmp & 0x3;
	}

	/* Re-use intlv_num_chan by setting it equal to log2(#channels) */
	switch (intlv_num_chan) {
	case 0:
		intlv_num_chan = 0;
		break;
	case 1:
		intlv_num_chan = 1;
		break;
	case 3:
		intlv_num_chan = 2;
		break;
	case 5:
		intlv_num_chan = 3;
		break;
	case 7:
		intlv_num_chan = 4;
		break;
	case 8:
		intlv_num_chan = 1;
		hash_enabled = true;
		break;
	default:
		if (hygon_f18h_m4h() && boot_cpu_data.x86_model == 0x4 &&
		    intlv_num_chan == 2)
			break;
		pr_err("%s: Invalid number of interleaved channels %d.\n", __func__, intlv_num_chan);
		goto out_err;
	}

	num_intlv_bits = intlv_num_chan;

	if (intlv_num_dies > 2) {
		pr_err("%s: Invalid number of interleaved nodes/dies %d.\n", __func__, intlv_num_dies);
		goto out_err;
	}

	num_intlv_bits += intlv_num_dies;

	/* Add a bit if sockets are interleaved. */
	num_intlv_bits += intlv_num_sockets;

	/* Assert num_intlv_bits in the correct range. */
	if ((hygon_f18h_m4h() && num_intlv_bits > 7) ||
	    (!hygon_f18h_m4h() && num_intlv_bits > 4)) {
		pr_err("%s: Invalid interleave bits %d.\n", __func__, num_intlv_bits);
		goto out_err;
	}

	if (num_intlv_bits > 0) {
		u64 temp_addr_x, temp_addr_i, temp_addr_y;
		u8 die_id_bit, sock_id_bit;
		u16 cs_fabric_id;

		/*
		 * Read FabricBlockInstanceInformation3_CS[BlockFabricID].
		 * This is the fabric id for this coherent slave. Use
		 * umc/channel# as instance id of the coherent slave
		 * for FICAA.
		 */
		if (df_indirect_read_instance(nid, 0, 0x50, umc, &ctx.tmp))
			goto out_err;

		if (hygon_f18h_m4h() || hygon_f18h_m10h())
			cs_fabric_id = (ctx.tmp >> 8) & 0x7FF;
		else
			cs_fabric_id = (ctx.tmp >> 8) & 0xFF;
		die_id_bit   = 0;

		/* If interleaved over more than 1 channel: */
		if (intlv_num_chan) {
			die_id_bit = intlv_num_chan;
			cs_mask	   = (1 << die_id_bit) - 1;
			cs_id	   = cs_fabric_id & cs_mask;
		}

		sock_id_bit = die_id_bit;

		/* Read D18F1x208 (SystemFabricIdMask). */
		if (intlv_num_dies || intlv_num_sockets)
			if (df_indirect_read_broadcast(nid, 1, 0x208, &ctx.tmp))
				goto out_err;

		/* If interleaved over more than 1 die. */
		if (intlv_num_dies) {
			sock_id_bit  = die_id_bit + intlv_num_dies;
			if (hygon_f18h_m4h()) {
				die_id_shift = (ctx.tmp >> 12) & 0xF;
				die_id_mask  = ctx.tmp & 0x7FF;
				cs_id |= (((cs_fabric_id & die_id_mask) >> die_id_shift) - 4) <<
						die_id_bit;
			} else {
				die_id_shift = (ctx.tmp >> 24) & 0xF;
				die_id_mask  = (ctx.tmp >> 8) & 0xFF;
				cs_id |= ((cs_fabric_id & die_id_mask) >> die_id_shift) <<
						die_id_bit;
			}
		}

		/* If interleaved over more than 1 socket. */
		if (intlv_num_sockets) {
			socket_id_shift	= (ctx.tmp >> 28) & 0xF;
			if (hygon_f18h_m4h())
				socket_id_mask	= (ctx.tmp >> 16) & 0x7FF;
			else
				socket_id_mask	= (ctx.tmp >> 16) & 0xFF;

			cs_id |= ((cs_fabric_id & socket_id_mask) >> socket_id_shift) << sock_id_bit;
		}

		/*
		 * The pre-interleaved address consists of XXXXXXIIIYYYYY
		 * where III is the ID for this CS, and XXXXXXYYYYY are the
		 * address bits from the post-interleaved address.
		 * "num_intlv_bits" has been calculated to tell us how many "I"
		 * bits there are. "intlv_addr_bit" tells us how many "Y" bits
		 * there are (where "I" starts).
		 */
		temp_addr_y = ctx.ret_addr & GENMASK_ULL(intlv_addr_bit - 1, 0);
		temp_addr_i = (cs_id << intlv_addr_bit);
		temp_addr_x = (ctx.ret_addr & GENMASK_ULL(63, intlv_addr_bit)) << num_intlv_bits;
		ctx.ret_addr    = temp_addr_x | temp_addr_i | temp_addr_y;
	}

	/* Add dram base address */
	ctx.ret_addr += dram_base_addr;

	/* If legacy MMIO hole enabled */
	if (lgcy_mmio_hole_en) {
		if (df_indirect_read_broadcast(nid, 0, 0x104, &ctx.tmp))
			goto out_err;

		dram_hole_base = ctx.tmp & GENMASK(31, 24);
		if (ctx.ret_addr >= dram_hole_base)
			ctx.ret_addr += (BIT_ULL(32) - dram_hole_base);
	}

	if (hash_enabled) {
		/* Save some parentheses and grab ls-bit at the end. */
		hashed_bit =	(ctx.ret_addr >> 12) ^
				(ctx.ret_addr >> 18) ^
				(ctx.ret_addr >> 21) ^
				(ctx.ret_addr >> 30) ^
				cs_id;

		hashed_bit &= BIT(0);

		if (hashed_bit != ((ctx.ret_addr >> intlv_addr_bit) & BIT(0)))
			ctx.ret_addr ^= BIT(intlv_addr_bit);
	}

	/* Is calculated system address is above DRAM limit address? */
	if (ctx.ret_addr > dram_limit_addr)
		goto out_err;

	*sys_addr = ctx.ret_addr;
	return 0;

out_err:
	return -EINVAL;
}

static void decode_umc_error_hygon(int node_id, struct mce *m)
{
	u8 ecc_type = (m->status >> 45) & 0x3;
	struct mem_ctl_info *mci;
	struct amd64_pvt *pvt;
	struct err_info err;
	u64 sys_addr;
	u8 umc;

	node_id = fixup_node_id(node_id, m);

	mci = edac_mc_find(node_id);
	if (!mci)
		return;

	pvt = mci->pvt_info;

	memset(&err, 0, sizeof(err));

	if (m->status & MCI_STATUS_DEFERRED)
		ecc_type = 3;

	if (!(m->status & MCI_STATUS_SYNDV)) {
		err.err_code = ERR_SYND;
		goto log_error;
	}

	if (ecc_type == 2) {
		u8 length = (m->synd >> 18) & 0x3f;

	if (length)
		err.syndrome = (m->synd >> 32) & GENMASK(length - 1, 0);
	else
		err.err_code = ERR_CHANNEL;
	}

	pvt->ops->get_err_info(m, &err);

	if ((hygon_f18h_m4h() && boot_cpu_data.x86_model >= 0x6) || hygon_f18h_m10h())
		umc = (err.channel << 1) + ((m->ipid & BIT(13)) >> 13);
	else
		umc = err.channel;

	if (hygon_f18h_m4h() || hygon_f18h_m10h()) {
		if (umc_normaddr_to_sysaddr_hygon(m->addr, pvt->mc_node_id, umc, &sys_addr)) {
			err.err_code = ERR_NORM_ADDR;
			goto log_error;
		}
	}

	error_address_to_page_and_offset(sys_addr, &err);

log_error:
	 __log_ecc_error(mci, &err, ecc_type);
}

static bool hygon_umc_channel_enabled(struct amd64_pvt *pvt, int channel)
{
	u32 enable;

	if (hygon_f18h_m10h()) {
		__df_indirect_read(pvt->mc_node_id, 1, 0x32c, 0xc, &enable);
		if ((enable & BIT(channel)))
			return true;
		return false;
	}

	return true;
}

#define for_each_chip_select_hygon(i, dct, pvt) \
	for (i = 0; i < pvt->csels[dct].b_cnt; i++)

#define chip_select_base_hygon(i, dct, pvt) \
	pvt->csels[dct].csbases[i]

#define for_each_chip_select_mask_hygon(i, dct, pvt) \
	for (i = 0; i < pvt->csels[dct].m_cnt; i++)

#define for_each_umc_hygon(i) \
	for (i = 0; i < pvt->max_mcs; i++)

static void umc_read_base_mask_hygon(struct amd64_pvt *pvt)
{
	u32 umc_base_reg, umc_base_reg_sec;
	u32 umc_mask_reg, umc_mask_reg_sec;
	u32 base_reg, base_reg_sec;
	u32 mask_reg, mask_reg_sec;
	u32 *base, *base_sec;
	u32 *mask, *mask_sec;
	u32 umc_base;
	int cs, umc;

	for_each_umc_hygon(umc) {
		if (!hygon_umc_channel_enabled(pvt, umc))
			continue;

		if (hygon_f18h_m4h())
			umc_base = get_umc_base_f18h_m4h(pvt->mc_node_id, umc);
		else
			umc_base = get_umc_base(umc);

		umc_base_reg = umc_base + UMCCH_BASE_ADDR;
		umc_base_reg_sec = umc_base + UMCCH_BASE_ADDR_SEC;

		for_each_chip_select_hygon(cs, umc, pvt) {
			base = &pvt->csels[umc].csbases[cs];
			base_sec = &pvt->csels[umc].csbases_sec[cs];

			base_reg = umc_base_reg + (cs * 4);
			base_reg_sec = umc_base_reg_sec + (cs * 4);

			if (!amd_smn_read(pvt->mc_node_id, base_reg, base))
				edac_dbg(0, "  DCSB%d[%d]=0x%08x reg: 0x%x\n", umc, cs, *base, base_reg);

			if (!amd_smn_read(pvt->mc_node_id, base_reg_sec, base_sec))
				edac_dbg(0, "    DCSB_SEC%d[%d]=0x%08x reg: 0x%x\n", umc, cs, *base_sec, base_reg_sec);
		}

		umc_mask_reg = umc_base + UMCCH_ADDR_MASK;
		umc_mask_reg_sec = umc_base + get_umc_reg(pvt, UMCCH_ADDR_MASK_SEC);

		for_each_chip_select_mask_hygon(cs, umc, pvt) {
			mask = &pvt->csels[umc].csmasks[cs];
			mask_sec = &pvt->csels[umc].csmasks_sec[cs];

			mask_reg = umc_mask_reg + (cs * 4);
			mask_reg_sec = umc_mask_reg_sec + (cs * 4);

			if (!amd_smn_read(pvt->mc_node_id, mask_reg, mask))
				edac_dbg(0, "  DCSM%d[%d]=0x%08x reg: 0x%x\n", umc, cs, *mask, mask_reg);

			if (!amd_smn_read(pvt->mc_node_id, mask_reg_sec, mask_sec))
				edac_dbg(0, "    DCSM_SEC%d[%d]=0x%08x reg: 0x%x\n", umc, cs, *mask_sec, mask_reg_sec);
			}
	}
}

/*
 * Retrieve the hardware registers of the memory controller.
 */
static void umc_read_mc_regs_hygon(struct amd64_pvt *pvt)
{
	u8 nid = pvt->mc_node_id;
	struct amd64_umc *umc;
	u32 i, umc_base;

	/* Read registers from each UMC */
	for_each_umc_hygon(i) {
		if (!hygon_umc_channel_enabled(pvt, i))
			continue;

		if (hygon_f18h_m4h())
			umc_base = get_umc_base_f18h_m4h(pvt->mc_node_id, i);
		else
			umc_base = get_umc_base(i);

		umc = &pvt->umc[i];

		amd_smn_read(nid, umc_base + get_umc_reg(pvt, UMCCH_DIMM_CFG), &umc->dimm_cfg);
		amd_smn_read(nid, umc_base + UMCCH_UMC_CFG, &umc->umc_cfg);
		amd_smn_read(nid, umc_base + UMCCH_SDP_CTRL, &umc->sdp_ctrl);
		amd_smn_read(nid, umc_base + UMCCH_ECC_CTRL, &umc->ecc_ctrl);
		amd_smn_read(nid, umc_base + UMCCH_UMC_CAP_HI, &umc->umc_cap_hi);
	}
}

static void umc_determine_memory_type_hygon(struct amd64_pvt *pvt)
{
	struct amd64_umc *umc;
	u32 i;

	for_each_umc_hygon(i) {
		umc = &pvt->umc[i];

		if (!(umc->sdp_ctrl & UMC_SDP_INIT)) {
			umc->dram_type = MEM_EMPTY;
			continue;
		}

		/*
		 * Check if the system supports the "DDR Type" field in UMC Config
		 * and has DDR5 DIMMs in use.
		 */
		if ((pvt->flags.zn_regs_v2 ||
			hygon_f18h_m4h() ||
			hygon_f18h_m10h()) &&
			((umc->umc_cfg & GENMASK(2, 0)) == 0x1)) {
			if (umc->dimm_cfg & BIT(5))
				umc->dram_type = MEM_LRDDR5;
			else if (umc->dimm_cfg & BIT(4))
				umc->dram_type = MEM_RDDR5;
			else
				umc->dram_type = MEM_DDR5;
		} else {
			if (umc->dimm_cfg & BIT(5))
				umc->dram_type = MEM_LRDDR4;
			else if (umc->dimm_cfg & BIT(4))
				umc->dram_type = MEM_RDDR4;
			else
				umc->dram_type = MEM_DDR4;
		}

		edac_dbg(1, "  UMC%d DIMM type: %s\n", i, edac_mem_types[umc->dram_type]);
	}
}
#endif
