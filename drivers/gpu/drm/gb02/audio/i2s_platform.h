#ifndef	__I2S_PLATFORM_H__
#define	__I2S_PLATFORM_H__
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include "gb02_fpga_v2vdma.h"
#include "local.h"

#ifndef	GB02MAC296
#define	GB02MAC907 0
#define GB02MAC908  2
#else
#define GB02MAC908  0
#define	GB02MAC907	2
#endif

#define GB02MAC909 2
#define GB02MAC910 3

#define GB02MAC911 149
#define GB02MAC912 99
#define GB02MAC913 49
#define GB02MAC914 24
#define GB02MAC915	12

//#define AUDIO_SOUND_SWITCH_DIFF_A
//#define DEBUG_SAVE_FILE

/* 18M/6 = 3M */
#define GB02MAC916 0x300000
#define GB02MAC917 0x10000
typedef struct {
	unsigned long bus_address;
	unsigned int size;
	unsigned int audio_id;
	unsigned long translation_offset;
} memalloc_params;

struct GB02STR83 {
	u64	hdwr_pa;
	void	*hdwr_va;
	u64	hdwr_len;

	u64	ddrwr_pa;
	void	*ddrwr_va;
	u64	ddrwr_len;
	u64	base_switch;
	u64	pll_va;
};

struct GB02STR84 {
	int	pcm_channel;
	int	audio_sample;
	int	pcm_width;
};

struct GB02STR85 {
	unsigned char *in_buf;
	unsigned char *out_buf;
};

enum DP_CONNECT_STATUS {
	DP_DISCONNECT = 0,
	DP_CONNECT,
	DP_STATUS_MAX,
};

enum DP_SUPPORT_AUDIO_STATUS {
	DP_DISSUPPORT_AUDIO = 0,
	DP_SUPPORT_AUDIO,
	DP_STATUS_AUDIO_MAX,
};

enum AUDIO_RESTART_STATUS {
	AUDIO_DISNEED_RESTART = 0,
	AUDIO_NEED_RESTART,
	AUDIO_RESTART_MAX,
};

enum AUDIO_SUSPEND_RESTORE_STATUS {
	AUDIO_SUSPEND_DISRESTORE = 0,
	AUDIO_SUSPEND_RESTORE,
	AUDIO_SUSPEND_RESTORE_MAX,
};

enum AUDIO_NEED_MUTE {
        AUDIO_SET_UNMUTE = 0,
        AUDIO_SET_MUTE,
        AUDIO_SET_MUTE_MAX,
};

enum AUDIO_JACK_INIT {
        AUDIO_JACK_UNINIT = 0,
        AUDIO_JACK_INIT,
        AUDIO_JACK_MAX,
};

struct GB02STR86 {
	struct pci_dev *pdev;
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct snd_jack *jack[GB02MAC309];
	u64	aoddr_pbase;	//audio phy base
	u64	vphy_base;	//vram phy base
	int	abuffer_size;	
	struct GB02STR42 dma_info;
	struct GB02STR96 i2s_info;
	u64	base_switch;
	u64	pll_va;
	u64	dp_va;
	void	*ddrwr_va;
	int     pcm_bytes[GB02MAC309];
	int     suspend_play_flag[GB02MAC309];
	int	dp_connect_status[GB02MAC309];	// 1 connect; 0 disconnect
	int	dp_edid_support_audio[GB02MAC309];	// 1 support; 0 dissupport
	int	audio_reset_parameters[GB02MAC309]; // 1 need; 0 unneed
	unsigned int g_sample_cnt[GB02MAC309];
	int	jack_init_flag[GB02MAC309];
#ifdef AUDIO_SOUND_SWITCH_DIFF_A
	int	jack_stop_pcm[GB02MAC309];
#endif
	struct GB02STR84	GB02STR84[GB02MAC309];
	struct GB02STR85 audio_data_info[GB02MAC309];
	struct snd_pcm_substream *playback_substream[GB02MAC309];
#ifdef DEBUG_SAVE_FILE
	struct file *fp;
#endif
};

struct GB02STR86 *GB02FUNC505(void);
void GB02FUNC508(void);
irqreturn_t GB02FUNC561(int irq, void *arg);
void GB02FUNC514(int chid, int sample_rate, int ch_count,
	int sample_dep, u64 pll_va, u64 base_switch, u64 dp_va);
void GB02FUNC547(int dp_num, int pcm_channel,
	int audio_sample, int pcm_width, int hotplug_status);
void GB02FUNC554(int dp_num, int audio_support_status);
void GB02FUNC556(int dp_num, int restart_val);
int GB02FUNC560(int dp_num);
int GB02FUNC532(struct GB02STR86 *i2s_cinfo, int idx);
#endif
