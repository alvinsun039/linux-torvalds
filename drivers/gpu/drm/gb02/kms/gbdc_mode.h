/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#ifndef GB02MAC2680
#define GB02MAC2680

#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
// #include <drm/drm_encoder.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_helper.h>
#else
#include <drm/display/drm_dp.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
#include <drm/drm_dp_mst_helper.h>
#else
#include <drm/display/drm_dp_mst_helper.h>
#endif
#include <drm/drm_fixed.h>
#include <drm/drm_crtc_helper.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

struct gbdc_bo;

/* drm_mode->private_flags */
#define GB02MAC2681 (1<<31)
#define GB02MAC2682 0
#define GB02MAC2683  (0x3ff<< GB02MAC2682)//3ff
#define GB02MAC2684  10
#define GB02MAC2685 (0x3ff<< GB02MAC2684) //ffc00
#define GB02MAC2686(x) ((x) & GB02MAC2683)
#define GB02MAC2687(x) (((x) << GB02MAC2684) & GB02MAC2685)
#define GB02MAC2688(x) (((x) & GB02MAC2685) >> GB02MAC2684)
#define GB02MAC2689(x)  GB02MAC2686(x)
#define GB02MAC2690  (1<<1)

/* Flag to use the scanline counter instead of the pixel counter */
#define GB02MAC2691  (1<<2)


#define GB02MAC2692 		7
#define GB02MAC2693			6
#define GB02MAC2694 		7
#define GB02MAC2695	 	6

enum gbdc_rmx_type {
	RMX_OFF,
	RMX_FULL,
	RMX_CENTER,
	RMX_ASPECT
};

enum gbdc_tv_std {
	TV_STD_NTSC,
	TV_STD_PAL,
	TV_STD_PAL_M,
	TV_STD_PAL_60,
	TV_STD_NTSC_J,
	TV_STD_SCART_PAL,
	TV_STD_SECAM,
	TV_STD_PAL_CN,
	TV_STD_PAL_N,
};

enum gbdc_underscan_type {
	UNDERSCAN_OFF,
	UNDERSCAN_ON,
	UNDERSCAN_AUTO,
};

enum gbdc_hpd_id {
	GBDC_HPD_1 = 0,
	GBDC_HPD_2,
	GBDC_HPD_3,
	GBDC_HPD_4,
	GBDC_HPD_5,
	GBDC_HPD_6,
	GBDC_HPD_NONE = 0xff,
};

enum gbdc_output_csc {
	GBDC_OUTPUT_CSC_BYPASS = 0,
	GBDC_OUTPUT_CSC_TVRGB = 1,
	GBDC_OUTPUT_CSC_YCBCR601 = 2,
	GBDC_OUTPUT_CSC_YCBCR709 = 3,
};

#define GB02MAC2696 16

/* gbdc gpio-based i2c
 * 1. "mask" reg and bits
 *    grabs the gpio pins for software use
 *    0=not held  1=held
 * 2. "a" reg and bits
 *    output pin value
 *    0=low 1=high
 * 3. "en" reg and bits
 *    sets the pin direction
 *    0=input 1=output
 * 4. "y" reg and bits
 *    input pin value
 *    0=low 1=high
 */
struct GB02STR205 {
	bool valid;
	/* id used by atom */
	uint8_t i2c_id;
	/* id used by atom */
	enum gbdc_hpd_id hpd;
	/* can be used with hw i2c engine */
	bool hw_capable;
	/* uses multi-media i2c engine */
	bool mm_i2c;
	/* regs and bits */
	uint32_t mask_clk_reg;
	uint32_t mask_data_reg;
	uint32_t a_clk_reg;
	uint32_t a_data_reg;
	uint32_t en_clk_reg;
	uint32_t en_data_reg;
	uint32_t y_clk_reg;
	uint32_t y_data_reg;
	uint32_t mask_clk_mask;
	uint32_t mask_data_mask;
	uint32_t a_clk_mask;
	uint32_t a_data_mask;
	uint32_t en_clk_mask;
	uint32_t en_data_mask;
	uint32_t y_clk_mask;
	uint32_t y_data_mask;
};

struct GB02STR206 {
	uint32_t freq;
	uint32_t value;
};

#define GB02MAC2697 16

/* pll flags */
#define GB02MAC2698        (1 << 0)
#define GB02MAC2699      (1 << 1)
#define GB02MAC2700          (1 << 2)
#define GB02MAC2701               (1 << 3)
#define GB02MAC2702   (1 << 4)
#define GB02MAC2703  (1 << 5)
#define GB02MAC2704    (1 << 6)
#define GB02MAC2705   (1 << 7)
#define GB02MAC2706  (1 << 8)
#define GB02MAC2707 (1 << 9)
#define GB02MAC2708      (1 << 10)
#define GB02MAC2709 (1 << 11)
#define GB02MAC2710         (1 << 12)
#define GB02MAC2711               (1 << 13)
#define GB02MAC2712 (1 << 14)

struct GB02STR207 {
	/* reference frequency */
	uint32_t reference_freq;

	/* fixed dividers */
	uint32_t reference_div;
	uint32_t post_div;

	/* pll in/out limits */
	uint32_t pll_in_min;
	uint32_t pll_in_max;
	uint32_t pll_out_min;
	uint32_t pll_out_max;
	uint32_t lcd_pll_out_min;
	uint32_t lcd_pll_out_max;
	uint32_t best_vco;

	/* divider limits */
	uint32_t min_ref_div;
	uint32_t max_ref_div;
	uint32_t min_post_div;
	uint32_t max_post_div;
	uint32_t min_feedback_div;
	uint32_t max_feedback_div;
	uint32_t min_frac_feedback_div;
	uint32_t max_frac_feedback_div;

	/* flags for the current clock */
	uint32_t flags;

	/* pll id */
	uint32_t id;
};

/* mostly for macs, but really any system without connector tables */
enum gbdc_connector_table {
	CT_NONE = 0,
	CT_GENERIC,
	CT_IBOOK,
	CT_POWERBOOK_EXTERNAL,
	CT_POWERBOOK_INTERNAL,
	CT_POWERBOOK_VGA,
	CT_MINI_EXTERNAL,
	CT_MINI_INTERNAL,
	CT_IMAC_G5_ISIGHT,
	CT_EMAC,
	CT_RN50_POWER,
	CT_MAC_X800,
	CT_MAC_G5_9600,
	CT_SAM440EP,
	CT_MAC_G4_SILVER
};

enum gbdc_dvo_chip {
	DVO_SIL164,
	DVO_SIL1178,
};

struct gbdc_fbdev;

struct GB02STR208 {
	bool enabled;
	int offset;
	bool last_buffer_filled_status;
	int id;
};

struct GB02STR209 {
#if 0
	struct atom_context *atom_context;
	struct card_info *atom_card_info;
	enum gbdc_connector_table connector_table;
#endif
	bool mode_config_initialized;
	struct gbdc_crtc *crtcs[GB02MAC2693];
#if 0
	struct GB02STR208 *afmt[GB02MAC2694];
#endif
	/* DVI-I properties */
	struct drm_property *coherent_mode_property;
	/* DAC enable load detect */
	struct drm_property *load_detect_property;
	/* TV standard */
	struct drm_property *tv_std_property;
	/* legacy TMDS PLL detect */
	struct drm_property *tmds_pll_property;
	/* underscan */
	struct drm_property *underscan_property;
	struct drm_property *underscan_hborder_property;
	struct drm_property *underscan_vborder_property;
	/* audio */
	struct drm_property *audio_property;
	/* FMT dithering */
	struct drm_property *dither_property;
	/* Output CSC */
	struct drm_property *output_csc_property;
	/* hardcoded DFP edid from BIOS */
	struct edid *bios_hardcoded_edid;
	int bios_hardcoded_edid_size;
#if 0
	/* pointer to fbdev info structure */
	struct gbdc_fbdev *rfbdev;
	/* firmware flags */
	u16 firmware_flags;
	/* pointer to backlight encoder */
	struct GB02STR156 *bl_encoder;
#endif
	/* bitmask for active encoder frontends */
	uint32_t active_encoders;
};

#define GB02MAC2713 0xFF

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE) || defined(CONFIG_BACKLIGHT_CLASS_DEVICE_MODULE)

struct GB02STR210 {
	struct GB02STR156 *encoder;
	uint8_t negative;
};

#endif

#define GB02MAC2714 32
#define GB02MAC2715 32

/* need to store these as reading
   back code tables is excessive */
struct GB02STR211 {
	uint32_t tv_uv_adr;
	uint32_t timing_cntl;
	uint32_t hrestart;
	uint32_t vrestart;
	uint32_t frestart;
	uint16_t h_code_timing[GB02MAC2714];
	uint16_t v_code_timing[GB02MAC2715];
};

struct GB02STR212 {
	uint16_t percentage;
	uint16_t percentage_divider;
	uint8_t type;
	uint16_t step;
	uint8_t delay;
	uint8_t range;
	uint8_t refdiv;
	/* asic_ss */
	uint16_t rate;
	uint16_t amount;
};

enum gbdc_flip_status {
	GBDC_FLIP_NONE,
	GBDC_FLIP_PENDING,
	GBDC_FLIP_SUBMITTED
};

struct GB02STR213 {
	/* legacy primary dac */
	uint32_t ps2_pdac_adj;
};

struct GB02STR214 {
	/* legacy lvds */
	uint16_t panel_vcc_delay;
	uint8_t panel_pwr_delay;
	uint8_t panel_digon_delay;
	uint8_t panel_blon_delay;
	uint16_t panel_ref_divider;
	uint8_t panel_post_divider;
	uint16_t panel_fb_divider;
	bool use_bios_dividers;
	uint32_t lvds_gen_cntl;
	/* panel mode */
	struct drm_display_mode native_mode;
	struct backlight_device *bl_dev;
	int dpms_mode;
	uint8_t backlight_level;
};

struct GB02STR215 {
	/* legacy tv dac */
	uint32_t ps2_tvdac_adj;
	uint32_t ntsc_tvdac_adj;
	uint32_t pal_tvdac_adj;

	int h_pos;
	int v_pos;
	int h_size;
	int supported_tv_stds;
	bool tv_on;
	enum gbdc_tv_std tv_std;
	struct GB02STR211 tv;
};

struct GB02STR216 {
	/* legacy int tmds */
	struct GB02STR206 tmds_pll[4];
};

/* spread spectrum */
struct GB02STR217 {
	bool linkb;
	/* atom dig */
	bool coherent_mode;
	int dig_encoder;	/* -1 disabled, 0 DIGA, 1 DIGB, etc. */
	/* atom lvds/edp */
	uint32_t lcd_misc;
	uint16_t panel_pwr_delay;
	uint32_t lcd_ss_id;
	/* panel mode */
	struct drm_display_mode native_mode;
	struct backlight_device *bl_dev;
	int dpms_mode;
	uint8_t backlight_level;
	int panel_mode;
	struct GB02STR208 *afmt;
	struct r600_audio_pin *pin;
	int active_mst_links;
};

struct GB02STR218 {
	enum gbdc_tv_std tv_std;
};

struct GB02STR219 {
	int crtc;
	struct GB02STR156 *primary;
	struct gbdc_connector *connector;
	struct drm_dp_mst_port *port;
	int pbn;
	int fe;
	bool fe_from_be;
	bool enc_active;
};
#if 0
struct GB02STR156 {
	struct drm_encoder base;
	uint32_t encoder_enum;
	uint32_t encoder_id;
	uint32_t devices;
	uint32_t active_device;
	uint32_t flags;
	uint32_t pixel_clock;
	enum gbdc_rmx_type rmx_type;
	enum gbdc_underscan_type underscan_type;
	uint32_t underscan_hborder;
	uint32_t underscan_vborder;
	struct drm_display_mode native_mode;
	void *enc_priv;
	int audio_polling_active;
	bool is_ext_encoder;
	u16 caps;
	struct gbdc_audio_funcs *audio;
	enum gbdc_output_csc output_csc;
	bool can_mst;
	uint32_t offset;
	bool is_mst_encoder;
	/* front end for this mst encoder */
};
#endif
struct gbdc_connector_atom_dig {
	uint32_t igp_lane_info;
	/* displayport */
	u8 dpcd[DP_RECEIVER_CAP_SIZE];
	u8 dp_sink_type;
	int dp_clock;
	int dp_lane_count;
	bool edp_on;
	bool is_mst;
};

struct GB02STR220 {
	bool valid;
	u8 id;
	u32 reg;
	u32 mask;
	u32 shift;
};

struct GB02STR221 {
	enum gbdc_hpd_id hpd;
	u8 plugged_state;
	struct GB02STR220 gpio;
};

struct GB02STR222 {
	u32 router_id;
	struct GB02STR205 i2c_info;
	u8 i2c_addr;
	/* i2c mux */
	bool ddc_valid;
	u8 ddc_mux_type;
	u8 ddc_mux_control_pin;
	u8 ddc_mux_state;
	/* clock/data mux */
	bool cd_valid;
	u8 cd_mux_type;
	u8 cd_mux_control_pin;
	u8 cd_mux_state;
};

enum gbdc_connector_audio {
	GBDC_AUDIO_DISABLE = 0,
	GBDC_AUDIO_ENABLE = 1,
	GBDC_AUDIO_AUTO = 2
};

enum gbdc_connector_dither {
	GBDC_FMT_DITHER_DISABLE = 0,
	GBDC_FMT_DITHER_ENABLE = 1,
};

struct GB02STR223 {
	uint16_t fe;
	uint16_t slots;
};
#if 0
struct gbdc_connector {
	struct drm_connector base;
	uint32_t connector_id;
	uint32_t devices;
	struct gbdc_i2c_chan *ddc_bus;
	/* some systems have an hdmi and vga port with a shared ddc line */
	bool shared_ddc;
	bool use_digital;
	/* we need to mind the EDID between detect
	   and get modes due to analog/digital/tvencoder */
	struct edid *edid;
	void *con_priv;
	bool dac_load_detect;
	bool detected_by_load;	/* if the connection status was determined by load */
	bool detected_hpd_without_ddc;	/* if an HPD signal was detected on DVI, but ddc probing failed */
	uint16_t connector_object_id;
	struct GB02STR221 hpd;
	struct GB02STR222 router;
	struct gbdc_i2c_chan *router_bus;
	enum gbdc_connector_audio audio;
	enum gbdc_connector_dither dither;
	int pixelclock_for_modeset;
	bool is_mst_connector;
	struct gbdc_connector *mst_port;
	struct drm_dp_mst_port *port;
	struct drm_dp_mst_topology_mgr mst_mgr;

	struct GB02STR156 *mst_encoder;
	struct GB02STR223 cur_stream_attribs[6];
	int enabled_attribs;
};
#endif
#if 0
struct GB02STR159 {
	struct drm_framebuffer base;
	struct drm_gem_object *obj;
};
#endif
#define ENCODER_MODE_IS_DP(em) (((em) == ATOM_ENCODER_MODE_DP) || \
				((em) == ATOM_ENCODER_MODE_DP_MST))

struct GB02STR224 {
	u32 post_div;
	union {
		struct {
#ifdef __BIG_ENDIAN
			u32 reserved:6;
			u32 whole_fb_div:12;
			u32 frac_fb_div:14;
#else
			u32 frac_fb_div:14;
			u32 whole_fb_div:12;
			u32 reserved:6;
#endif
		};
		u32 fb_div;
	};
	u32 ref_div;
	bool enable_post_div;
	bool enable_dithen;
	u32 vco_mode;
	u32 real_clock;
	/* added for CI */
	u32 post_divider;
	u32 flags;
};

struct GB02STR227 {
	union {
		struct {
#ifdef __BIG_ENDIAN
			u32 reserved:8;
			u32 clkfrac:12;
			u32 clkf:12;
#else
			u32 clkf:12;
			u32 clkfrac:12;
			u32 reserved:8;
#endif
		};
		u32 fb_div;
	};
	u32 post_div;
	u32 bwcntl;
	u32 dll_speed;
	u32 vco_mode;
	u32 yclk_sel;
	u32 qdr;
	u32 half_rate;
};

#define GB02MAC2728  0x50
#define GB02MAC2730  0x40
#define GB02MAC2732  0x30
#define GB02MAC2733   0x20
#define GB02MAC2734  0x10
#define GB02MAC2735   0xb0
#define GB02MAC2736   0xf0

struct GB02STR230 {
	u8 mem_vendor;
	u8 mem_type;
};

#define GB02MAC2737 16

struct GB02STR231 {
	u8 num_entries;
	u8 rsv[3];
	u32 mclk[GB02MAC2737];
};

#define GB02MAC2738 32
#define GB02MAC2739 20

struct GB02STR232 {
	u32 mclk_max;
	u32 mc_data[GB02MAC2738];
};

struct GB02STR233 {
	u16 s1;
	u8 pre_reg_data;
};

struct GB02STR234 {
	u8 last;
	u8 num_entries;
	struct GB02STR232
	    mc_reg_table_entry[GB02MAC2739];
	struct GB02STR233
	    mc_reg_address[GB02MAC2738];
};

#define GB02MAC2741 32

struct GB02STR235 {
	u16 value;
	u32 smio_low;
};

struct GB02STR236 {
	u32 count;
	u32 mask_low;
	u32 phase_delay;
	struct GB02STR235 entries[GB02MAC2741];
};

/* Driver internal use only flags of gbdc_get_crtc_scanoutpos() */
#define GB02MAC2746        (1 << 0)
#define GB02MAC2748    (1 << 1)
#define GB02MAC2750     (1 << 2)
#define GB02MAC2752 		(1 << 30)
#define GB02MAC2753	(1 << 31)

#endif
