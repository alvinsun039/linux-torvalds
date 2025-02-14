/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This is the Fusion MPT base driver providing common API layer interface
 * for access to MPT (Message Passing Technology) firmware.
 *
 * Copyright (C) 2013-2021  LSI Corporation
 * Copyright (C) 2013-2021  Avago Technologies
 * Copyright (C) 2013-2021  Broadcom Inc.
 *  (mailto:MPT-FusionLinux.pdl@broadcom.com)
 *
 * Copyright (C) 2024 CETC 52nd Research Institute
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * NO WARRANTY
 * THE PROGRAM IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT
 * LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Each Recipient is
 * solely responsible for determining the appropriateness of using and
 * distributing the Program and assumes all risks associated with its
 * exercise of rights under this Agreement, including but not limited to
 * the risks and costs of program errors, damage to or loss of data,
 * programs or equipment, and unavailability or interruption of operations.

 * DISCLAIMER OF LIABILITY
 * NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
 * HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */

#ifndef CETC52RAID_FUNC_H_INCLUDED
#define CETC52RAID_FUNC_H_INCLUDED

#include "cetc52raid.h"
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_transport_sas.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_eh.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/irq_poll.h>

#ifndef fallthrough
#define fallthrough
#endif

#define SYS_LOG_BUF_SIZE	(0x20000)
#define MAX_UPD_PAYLOAD_SZ	(0x4000)

#define CETC52RAID_DRIVER_NAME		"Cetc52Raid"
#define CETC52RAID_AUTHOR		"CETC 52nd Research Institute"
#define CETC52RAID_DESCRIPTION		"CETC52 RAID Driver"
#define CETC52RAID_DRIVER_VERSION      "1.02.02.00"
#define CETC52RAID_MAJOR_VERSION	(1)
#define CETC52RAID_MINOR_VERSION	(02)
#define CETC52RAID_BUILD_VERSION	(02)
#define CETC52RAID_RELEASE_VERSION	(00)

#define CETC52RAID_VENDOR_ID		(0xCE52)
#define CETC52RAID_DEVICE_ID_1		(0x1000)
#define CETC52RAID_DEVICE_ID_2		(0x1001)

#define CETC52RAID_MAX_PHYS_SEGMENTS	SG_CHUNK_SIZE

#define CETC52RAID_MIN_PHYS_SEGMENTS	(16)
#define CETC52RAID_KDUMP_MIN_PHYS_SEGMENTS	(32)

#define CETC52RAID_MAX_SG_SEGMENTS	SG_MAX_SEGMENTS
#define CETC52RAID_MAX_PHYS_SEGMENTS_STRING "SG_CHUNK_SIZE"

#define CETC52RAID_SG_DEPTH	CETC52RAID_MAX_PHYS_SEGMENTS


#define CETC52RAID_CONFIG_PAGE_DEFAULT_TIMEOUT 15
#define CETC52RAID_CONFIG_COMMON_SGLFLAGS ((CETC52RAID_SGE_FLAGS_SIMPLE_ELEMENT | \
	CETC52RAID_SGE_FLAGS_LAST_ELEMENT | CETC52RAID_SGE_FLAGS_END_OF_BUFFER \
	| CETC52RAID_SGE_FLAGS_END_OF_LIST) << CETC52RAID_SGE_FLAGS_SHIFT)
#define CETC52RAID_CONFIG_COMMON_WRITE_SGLFLAGS ((CETC52RAID_SGE_FLAGS_SIMPLE_ELEMENT | \
	CETC52RAID_SGE_FLAGS_LAST_ELEMENT | CETC52RAID_SGE_FLAGS_END_OF_BUFFER \
	| CETC52RAID_SGE_FLAGS_END_OF_LIST | CETC52RAID_SGE_FLAGS_HOST_TO_IOC) \
	<< CETC52RAID_SGE_FLAGS_SHIFT)

#define CETC52RAID_SATA_QUEUE_DEPTH		(32)
#define CETC52RAID_SAS_QUEUE_DEPTH		(64)
#define CETC52RAID_RAID_QUEUE_DEPTH		(64)
#define CETC52RAID_KDUMP_SCSI_IO_DEPTH	(64)
#define CETC52RAID_RAID_MAX_SECTORS		(128)

#define CETC52RAID_NAME_LENGTH			(48)
#define CETC52RAID_DRIVER_NAME_LENGTH	(24)
#define CETC52RAID_STRING_LENGTH		(64)

#define CETC52RAID_FRAME_START_OFFSET	(256)
#define CETC52RAID_REPLY_FREE_POOL_SIZE	(512)
#define CETC52RAID_MAX_CALLBACKS		(32)
#define CETC52RAID_MAX_HBA_NUM_PHYS		(16)

#define CETC52RAID_INTERNAL_CMDS_COUNT				(10)
#define CETC52RAID_INTERNAL_SCSIIO_CMDS_COUNT		(3)
#define CETC52RAID_INTERNAL_SCSIIO_FOR_IOCTL		(1)
#define CETC52RAID_INTERNAL_SCSIIO_FOR_DISCOVERY	(2)

#define CETC52RAID_INVALID_DEVICE_HANDLE		(0xFFFF)
#define CETC52RAID_MAX_CHAIN_ELEMT_SZ			(16)
#define CETC52RAID_DEFAULT_NUM_FWCHAIN_ELEMTS	(8)
#define CETC52RAID_READL_RETRY_COUNT_OF_THIRTY	(30)
#define CETC52RAID_READL_RETRY_COUNT_OF_THREE	(3)

#define CETC52RAID_IOC_PRE_RESET_PHASE		(1)
#define CETC52RAID_IOC_AFTER_RESET_PHASE	(2)
#define CETC52RAID_IOC_DONE_RESET_PHASE		(3)

#define CETC52RAID_TARGET_FLAGS_RAID_COMPONENT	(0x01)
#define CETC52RAID_TARGET_FLAGS_VOLUME			(0x02)
#define CETC52RAID_TARGET_FASTPATH_IO			(0x08)

#define CETC52RAID_DEVICE_HIGH_IOPS_DEPTH		(8)
#define CETC52RAID_HIGH_IOPS_REPLY_QUEUES		(8)
#define CETC52RAID_HIGH_IOPS_BATCH_COUNT		(16)
#define CETC52RAID_GEN35_MAX_MSIX_QUEUES		(128)
#define CETC52RAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK	(16)

#define CETC52RAID_IFAULT_IOP_OVER_TEMP_THRESHOLD_EXCEEDED	(0x2810)

#ifndef DID_TRANSPORT_DISRUPTED
#define DID_TRANSPORT_DISRUPTED DID_BUS_BUSY
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX      (~0ULL)
#endif
#ifndef USHORT_MAX
#define USHORT_MAX      ((u16)(~0U))
#endif
#ifndef UINT_MAX
#define UINT_MAX        (~0U)
#endif

static inline void *cetc52raid_shost_private(struct Scsi_Host *shost)
{
	return (void *)shost->hostdata;
}

struct Cetc52raidManuP10_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U8 OEMIdentifier;
	U8 Reserved1;
	U16 Reserved2;
	U32 Reserved3;
	U32 GenericFlags0;
	U32 GenericFlags1;
	U32 Reserved4;
	U32 OEMSpecificFlags0;
	U32 OEMSpecificFlags1;
	U32 Reserved5[18];
};

struct Cetc52raidManuP11_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	__le32 Reserved1;
	u8 Reserved2;
	u8 EEDPTagMode;
	u8 Reserved3;
	u8 Reserved4;
	__le32 Reserved5[8];
	u16 AddlFlags2;
	u8 AddlFlags3;
	u8 Reserved6;
	__le32 Reserved7[7];
	u8 AbortTO;
	u8 NumPerDevEvents;
	u8 HostTraceBufferDecrementSizeKB;
	u8 HostTraceBufferFlags;
	u16 HostTraceBufferMaxSizeKB;
	u16 HostTraceBufferMinSizeKB;
	u8 CoreDumpTOSec;
	u8 TimeSyncInterval;
	u16 Reserved9;
	__le32 Reserved10;
};

struct CETC52RAID_TARGET {
	struct scsi_target *starget;
	u64 sas_address;
	struct cetc52raid_raid_device *raid_device;
	u16 handle;
	int num_luns;
	u32 flags;
	u8 deleted;
	u8 tm_busy;
	struct cetc52raid_hba_port *port;
	struct cetc52raid_sas_device *sas_dev;
};

#define CETC52RAID_DEVICE_FLAGS_INIT	(0x01)
#define CETC52RAID_DEVICE_TLR_ON		(0x02)

struct CETC52RAID_DEVICE {
	struct CETC52RAID_TARGET *sas_target;
	unsigned int lun;
	u32 flags;
	u8 configured_lun;
	u8 block;
	u8 deleted;
	u8 tlr_snoop_check;
	u8 ignore_delay_remove;
	u8 ncq_prio_enable;
	unsigned long ata_command_pending;
};

#define CETC52RAID_CMND_PENDING_BIT		(0)
#define CETC52RAID_CMD_NOT_USED			(0x8000)
#define CETC52RAID_CMD_COMPLETE			(0x0001)
#define CETC52RAID_CMD_PENDING			(0x0002)
#define CETC52RAID_CMD_REPLY_VALID		(0x0004)
#define CETC52RAID_CMD_RESET			(0x0008)
#define CETC52RAID_CMD_COMPLETE_ASYNC	(0x0010)

struct cetc52raid_internal_cmd {
	struct mutex mutex;
	struct completion done;
	void *reply;
	void *sense;
	u16 status;
	u16 smid;
};

struct cetc52raid_scsi_io_transfer {
	u16 handle;
	u8 is_raid;
	enum dma_data_direction dir;
	u32 data_length;
	dma_addr_t data_dma;
	u8 sense[SCSI_SENSE_BUFFERSIZE];
	u32 lun;
	u8 cdb_length;
	u8 cdb[32];
	u8 timeout;
	u8 VF_ID;
	u8 VP_ID;
	u8 valid_reply;
	u32 sense_length;
	u16 ioc_status;
	u8 scsi_state;
	u8 scsi_status;
	u32 log_info;
	u32 transfer_length;
};

struct cetc52raid_internal_qcmd {
	struct list_head list;
	void *request;
	void *reply;
	void *sense;
	u16 status;
	u16 smid;
	struct cetc52raid_scsi_io_transfer *transfer_packet;
};

#define CETC52RAID_WIDE_PORT_API		(1)
#define CETC52RAID_WIDE_PORT_API_PLUS	(1)

struct cetc52raid_sas_device {
	struct list_head list;
	struct scsi_target *starget;
	u64 sas_address;
	u64 device_name;
	u16 handle;
	u64 sas_address_parent;
	u16 enclosure_handle;
	u64 enclosure_logical_id;
	u16 volume_handle;
	u64 volume_wwid;
	u32 device_info;
	int id;
	int channel;
	u16 slot;
	u8 phy;
	u8 responding;
	u8 fast_path;
	u8 pfa_led_on;
	struct kref refcount;
	u8 *serial_number;
	u8 pend_sas_rphy_add;
	u8 enclosure_level;
	u8 chassis_slot;
	u8 is_chassis_slot_valid;
	u8 connector_name[5];
	u8 ssd_device;
	u8 supports_sata_smart;
	u8 port_type;
	struct cetc52raid_hba_port *port;
	struct sas_rphy *rphy;
};

static inline
void cetc52raid_sas_device_get(struct cetc52raid_sas_device *s)
{
	kref_get(&s->refcount);
}

static inline
void cetc52raid_sas_device_free(struct kref *r)
{
	kfree(container_of(r, struct cetc52raid_sas_device, refcount));
}

static inline
void cetc52raid_sas_device_put(struct cetc52raid_sas_device *s)
{
	kref_put(&s->refcount, cetc52raid_sas_device_free);
}

struct cetc52raid_raid_device {
	struct list_head list;
	struct scsi_target *starget;
	struct scsi_device *sdev;
	u64 wwid;
	u16 handle;
	u16 block_sz;
	int id;
	int channel;
	u8 volume_type;
	u8 num_pds;
	u8 responding;
	u8 percent_complete;
	u8 direct_io_enabled;
	u8 stripe_exponent;
	u8 block_exponent;
	u64 max_lba;
	u32 stripe_sz;
	u32 device_info;
	u16 pd_handle[8];
};

struct cetc52raid_boot_device {
	int channel;
	void *device;
};

struct cetc52raid_sas_port {
	struct list_head port_list;
	u8 num_phys;
	struct cetc52raid_hba_port *hba_port;
	struct sas_identify remote_identify;
	struct sas_rphy *rphy;
#if defined(CETC52RAID_WIDE_PORT_API)
	struct sas_port *port;
#endif
	struct list_head phy_list;
};

struct cetc52raid_sas_phy {
	struct list_head port_siblings;
	struct sas_identify identify;
	struct sas_identify remote_identify;
	struct sas_phy *phy;
	u8 phy_id;
	u16 handle;
	u16 attached_handle;
	u8 phy_belongs_to_port;
	u8 hba_vphy;
	struct cetc52raid_hba_port *port;
};

struct cetc52raid_raid_sas_node {
	struct list_head list;
	struct device *parent_dev;
	u8 num_phys;
	u64 sas_address;
	u16 handle;
	u64 sas_address_parent;
	u16 enclosure_handle;
	u64 enclosure_logical_id;
	u8 responding;
	u8 nr_phys_allocated;
	struct cetc52raid_hba_port *port;
	struct cetc52raid_sas_phy *phy;
	struct list_head sas_port_list;
	struct sas_rphy *rphy;
};

struct cetc52raid_enclosure_node {
	struct list_head list;
	struct Cetc52raidSasEncP0_t pg0;
};

enum reset_type {
	FORCE_BIG_HAMMER,
	SOFT_RESET,
};

struct cetc52raid_chain_tracker {
	void *chain_buffer;
	dma_addr_t chain_buffer_dma;
};

struct cetc52raid_chain_lookup {
	struct cetc52raid_chain_tracker *chains_per_smid;
	atomic_t chain_offset;
};

struct cetc52raid_scsiio_tracker {
	u16 smid;
	struct scsi_cmnd *scmd;
	u8 cb_idx;
	u8 direct_io;
	struct list_head chain_list;
	u16 msix_io;
};

struct cetc52raid_request_tracker {
	u16 smid;
	u8 cb_idx;
	struct list_head tracker_list;
};

struct cetc52raid_tr_list {
	struct list_head list;
	u16 handle;
	u16 state;
};

struct cetc52raid_sc_list {
	struct list_head list;
	u16 handle;
};

struct cetc52raid_event_ack_list {
	struct list_head list;
	U16 Event;
	U32 EventContext;
};

struct cetc52raid_adapter_reply_queue {
	struct CETC52RAID_ADAPTER *ioc;
	u8 msix_index;
	u32 reply_post_host_index;
	union Cetc52raidRepDescUnion_t *reply_post_free;
	char name[CETC52RAID_NAME_LENGTH];
	atomic_t busy;
	cpumask_var_t affinity_hint;
	u32 os_irq;
	struct irq_poll irqpoll;
	bool irq_poll_scheduled;
	bool irq_line_enable;
	bool is_blk_mq_poll_q;
	struct list_head list;
};

struct cetc52raid_blk_mq_poll_queue {
	atomic_t busy;
	atomic_t pause;
	struct cetc52raid_adapter_reply_queue *reply_q;
};

union cetc52raid_version_union {
	struct CETC52RAID_VERSION_STRUCT Struct;
	u32 Word;
};

typedef void (*CETC52RAID_ADD_SGE)(void *paddr, u32 flags_length,
				 dma_addr_t dma_addr);
typedef int (*CETC52RAID_BUILD_SG_SCMD)(struct CETC52RAID_ADAPTER *ioc,
				      struct scsi_cmnd *scmd, u16 smid);
typedef void (*CETC52RAID_BUILD_SG)(struct CETC52RAID_ADAPTER *ioc, void *psge,
				  dma_addr_t data_out_dma, size_t data_out_sz,
				  dma_addr_t data_in_dma, size_t data_in_sz);
typedef void (*CETC52RAID_BUILD_ZERO_LEN_SGE)(struct CETC52RAID_ADAPTER *ioc,
					    void *paddr);
typedef void (*PUT_SMID_IO_FP_HIP_TA)(struct CETC52RAID_ADAPTER *ioc, u16 smid,
				      u16 funcdep);
typedef void (*PUT_SMID_DEFAULT)(struct CETC52RAID_ADAPTER *ioc, u16 smid);
typedef u32(*BASE_READ_REG) (const void __iomem *addr,
			     u8 retry_count);
typedef u8(*GET_MSIX_INDEX) (struct CETC52RAID_ADAPTER *ioc,
			     struct scsi_cmnd *scmd);

struct cetc52raid_facts {
	u16 MsgVersion;
	u16 HeaderVersion;
	u8 IOCNumber;
	u8 VP_ID;
	u8 VF_ID;
	u16 IOCExceptions;
	u16 IOCStatus;
	u32 IOCLogInfo;
	u8 MaxChainDepth;
	u8 WhoInit;
	u8 NumberOfPorts;
	u8 MaxMSIxVectors;
	u16 RequestCredit;
	u16 ProductID;
	u32 IOCCapabilities;
	union cetc52raid_version_union FWVersion;
	u16 IOCRequestFrameSize;
	u16 IOCMaxChainSegmentSize;
	u16 MaxInitiators;
	u16 MaxTargets;
	u16 MaxSasExpanders;
	u16 MaxEnclosures;
	u16 ProtocolFlags;
	u16 HighPriorityCredit;
	u16 MaxReplyDescriptorPostQueueDepth;
	u8 ReplyFrameSize;
	u8 MaxVolumes;
	u16 MaxDevHandle;
	u16 MaxPersistentEntries;
	u16 MinDevHandle;
	u8 CurrentHostPageSize;
};

struct cetc52raid_port_facts {
	u8 PortNumber;
	u8 VP_ID;
	u8 VF_ID;
	u8 PortType;
	u16 MaxPostedCmdBuffers;
};

struct cetc52raid_reply_post_struct {
	union Cetc52raidRepDescUnion_t *reply_post_free;
	dma_addr_t reply_post_free_dma;
};

struct cetc52raid_virtual_phy {
	struct list_head list;
	u64 sas_address;
	u32 phy_mask;
	u8 flags;
};

#define CETC52RAID_VPHY_FLAG_DIRTY_PHY	(0x01)
struct cetc52raid_hba_port {
	struct list_head list;
	u64 sas_address;
	u32 phy_mask;
	u8 port_id;
	u8 flags;
	u32 vphys_mask;
	struct list_head vphys_list;
};

#define CETC52RAID_HBA_PORT_FLAG_DIRTY_PORT		(0x01)
#define CETC52RAID_HBA_PORT_FLAG_NEW_PORT		(0x02)
#define CETC52RAID_MULTIPATH_DISABLED_PORT_ID	(0xFF)

typedef void (*CETC52RAID_FLUSH_RUNNING_CMDS)(struct CETC52RAID_ADAPTER *
						ioc);

struct CETC52RAID_ADAPTER {
	struct list_head list;
	struct Scsi_Host *shost;
	u8 id;
	u8 IOCNumber;
	int cpu_count;
	char name[CETC52RAID_NAME_LENGTH];
	char driver_name[CETC52RAID_DRIVER_NAME_LENGTH];
	char tmp_string[CETC52RAID_STRING_LENGTH];
	struct pci_dev *pdev;
	struct Cetc52raidSysInterfaceRegs_t __iomem *chip;
	phys_addr_t chip_phys;
	int logging_level;
	int fwfault_debug;
	u8 ir_firmware;
	int bars;
	u8 mask_interrupts;
	struct mutex pci_access_mutex;
	char fault_reset_work_q_name[48];
	char hba_hot_unplug_work_q_name[48];
	struct workqueue_struct *fault_reset_work_q;
	struct workqueue_struct *hba_hot_unplug_work_q;
	struct delayed_work fault_reset_work;
	struct delayed_work hba_hot_unplug_work;
	struct workqueue_struct *smart_poll_work_q;
	struct delayed_work smart_poll_work;
	u8 adapter_over_temp;
	char firmware_event_name[48];
	struct workqueue_struct *firmware_event_thread;
	spinlock_t fw_event_lock;
	struct list_head fw_event_list;
	struct cetc52raid_fw_event_work *current_event;
	u8 fw_events_cleanup;
	int aen_event_read_flag;
	u8 broadcast_aen_busy;
	u16 broadcast_aen_pending;
	u8 shost_recovery;
	u8 got_task_abort_from_ioctl;
	u8 got_task_abort_from_sysfs;
	struct mutex reset_in_progress_mutex;
	struct mutex hostdiag_unlock_mutex;
	spinlock_t ioc_reset_in_progress_lock;
	spinlock_t hba_hot_unplug_lock;
	u8 ioc_link_reset_in_progress;
	int ioc_reset_status;
	u8 ignore_loginfos;
	u8 remove_host;
	u8 pci_error_recovery;
	u8 wait_for_discovery_to_complete;
	u8 is_driver_loading;
	u8 port_enable_failed;
	u8 start_scan;
	u16 start_scan_failed;
	u8 msix_enable;
	u8 *cpu_msix_table;
	resource_size_t **reply_post_host_index;
	u16 cpu_msix_table_sz;
	u32 ioc_reset_count;
	CETC52RAID_FLUSH_RUNNING_CMDS schedule_dead_ioc_flush_running_cmds;
	u32 non_operational_loop;
	u8 ioc_coredump_loop;
	u32 timestamp_update_count;
	u32 time_sync_interval;
	u8 multipath_on_hba;
	atomic64_t total_io_cnt;
	atomic64_t high_iops_outstanding;
	bool msix_load_balance;
	u16 thresh_hold;
	u8 high_iops_queues;
	u8 iopoll_q_start_index;
	u32 drv_internal_flags;
	u32 drv_support_bitmap;
	u32 dma_mask;
	bool enable_sdev_max_qd;
	bool use_32bit_dma;
	struct cetc52raid_blk_mq_poll_queue *blk_mq_poll_queues;
	u8 scsi_io_cb_idx;
	u8 tm_cb_idx;
	u8 transport_cb_idx;
	u8 scsih_cb_idx;
	u8 ctl_cb_idx;
	u8 ctl_tm_cb_idx;
	u8 base_cb_idx;
	u8 port_enable_cb_idx;
	u8 config_cb_idx;
	u8 tm_tr_cb_idx;
	u8 tm_tr_volume_cb_idx;
	u8 tm_tr_internal_cb_idx;
	u8 tm_sas_control_cb_idx;
	struct cetc52raid_internal_cmd base_cmds;
	struct cetc52raid_internal_cmd port_enable_cmds;
	struct cetc52raid_internal_cmd transport_cmds;
	struct cetc52raid_internal_cmd scsih_cmds;
	struct cetc52raid_internal_cmd tm_cmds;
	struct cetc52raid_internal_cmd ctl_cmds;
	struct cetc52raid_internal_cmd config_cmds;
	struct list_head scsih_q_intenal_cmds;
	spinlock_t scsih_q_internal_lock;
	CETC52RAID_ADD_SGE base_add_sg_single;
	CETC52RAID_BUILD_SG_SCMD build_sg_scmd;
	CETC52RAID_BUILD_SG build_sg;
	CETC52RAID_BUILD_ZERO_LEN_SGE build_zero_len_sge;
	u16 sge_size_ieee;
	CETC52RAID_BUILD_SG build_sg_mpi;
	CETC52RAID_BUILD_ZERO_LEN_SGE build_zero_len_sge_mpi;
	u32 event_type[CETC52RAID_EVENT_NOTIFY_EVENTMASK_WORDS];
	u32 event_context;
	void *event_log;
	u32 event_masks[CETC52RAID_EVENT_NOTIFY_EVENTMASK_WORDS];
	u8 disable_eedp_support;
	u8 tm_custom_handling;
	u16 max_shutdown_latency;
	u16 max_wideport_qd;
	u16 max_narrowport_qd;
	u8 max_sata_qd;
	struct cetc52raid_facts facts;
	struct cetc52raid_facts prev_fw_facts;
	struct cetc52raid_port_facts *pfacts;
	struct Cetc52raidManP0_t manu_pg0;
	struct Cetc52raidManuP10_t manu_pg10;
	struct Cetc52raidManuP11_t manu_pg11;
	struct Cetc52raidBiosP2_t bios_pg2;
	struct Cetc52raidBiosP3_t bios_pg3;
	struct Cetc52raidIOCP8_t ioc_pg8;
	struct Cetc52raidIOUnitP0_t iounit_pg0;
	struct Cetc52raidIOUnitP1_t iounit_pg1;
	struct Cetc52raidIOUnitP8_t iounit_pg8;
	struct Cetc52raidIOCP1_t ioc_pg1_copy;
	struct cetc52raid_boot_device req_boot_device;
	struct cetc52raid_boot_device req_alt_boot_device;
	struct cetc52raid_boot_device current_boot_device;
	struct cetc52raid_raid_sas_node sas_hba;
	struct list_head sas_expander_list;
	struct list_head enclosure_list;
	spinlock_t sas_node_lock;
	struct list_head sas_device_list;
	struct list_head sas_device_init_list;
	spinlock_t sas_device_lock;
	struct list_head pcie_device_list;
	struct list_head pcie_device_init_list;
	spinlock_t pcie_device_lock;
	struct list_head raid_device_list;
	spinlock_t raid_device_lock;
	u8 io_missing_delay;
	u16 device_missing_delay;
	int sas_id;
	int pcie_target_id;
	void *blocking_handles;
	void *pd_handles;
	u16 pd_handles_sz;
	void *pend_os_device_add;
	u16 pend_os_device_add_sz;
	u16 config_page_sz;
	void *config_page;
	dma_addr_t config_page_dma;
	void *config_vaddr;
	u16 hba_queue_depth;
	u16 sge_size;
	u16 scsiio_depth;
	u16 request_sz;
	u8 *request;
	dma_addr_t request_dma;
	u32 request_dma_sz;
	spinlock_t scsi_lookup_lock;
	int pending_io_count;
	wait_queue_head_t reset_wq;
	int pending_tm_count;
	u32 terminated_tm_count;
	wait_queue_head_t pending_tm_wq;
	u8 out_of_frames;
	wait_queue_head_t no_frames_tm_wq;
	u16 *io_queue_num;
	u32 page_size;
	struct cetc52raid_chain_lookup *chain_lookup;
	struct list_head free_chain_list;
	struct dma_pool *chain_dma_pool;
	u16 max_sges_in_main_message;
	u16 max_sges_in_chain_message;
	u16 chains_needed_per_io;
	u16 chain_segment_sz;
	u16 chains_per_prp_buffer;
	u16 hi_priority_smid;
	u8 *hi_priority;
	dma_addr_t hi_priority_dma;
	u16 hi_priority_depth;
	struct cetc52raid_request_tracker *hpr_lookup;
	struct list_head hpr_free_list;
	u16 internal_smid;
	u8 *internal;
	dma_addr_t internal_dma;
	u16 internal_depth;
	struct cetc52raid_request_tracker *internal_lookup;
	struct list_head internal_free_list;
	u8 *sense;
	dma_addr_t sense_dma;
	struct dma_pool *sense_dma_pool;
	u16 reply_sz;
	u8 *reply;
	dma_addr_t reply_dma;
	u32 reply_dma_max_address;
	u32 reply_dma_min_address;
	struct dma_pool *reply_dma_pool;
	u16 reply_free_queue_depth;
	__le32 *reply_free;
	dma_addr_t reply_free_dma;
	struct dma_pool *reply_free_dma_pool;
	u32 reply_free_host_index;
	u16 reply_post_queue_depth;
	struct cetc52raid_reply_post_struct *reply_post;
	struct dma_pool *reply_post_free_dma_pool;
	struct dma_pool *reply_post_free_array_dma_pool;
	struct Cetc52raidIOCInitRDPQArrayEntry *reply_post_free_array;
	dma_addr_t reply_post_free_array_dma;
	u8 reply_queue_count;
	struct list_head reply_queue_list;
	u8 rdpq_array_capable;
	u8 rdpq_array_enable;
	u8 rdpq_array_enable_assigned;
	u8 combined_reply_queue;
	u8 nc_reply_index_count;
	u8 smp_affinity_enable;
	resource_size_t **replyPostRegisterIndex;
	struct list_head delayed_tr_list;
	struct list_head delayed_tr_volume_list;
	struct list_head delayed_internal_tm_list;
	struct list_head delayed_sc_list;
	struct list_head delayed_event_ack_list;
	u32 ring_buffer_offset;
	u32 ring_buffer_sz;
	u8 reset_from_user;
	u8 hide_ir_msg;
	u8 warpdrive_msg;
	u8 mfg_pg10_hide_flag;
	u8 hide_drives;
	u8 atomic_desc_capable;
	BASE_READ_REG base_readl;
	PUT_SMID_IO_FP_HIP_TA put_smid_scsi_io;
	PUT_SMID_IO_FP_HIP_TA put_smid_fast_path;
	PUT_SMID_IO_FP_HIP_TA put_smid_hi_priority;
	PUT_SMID_DEFAULT put_smid_default;
	GET_MSIX_INDEX get_msix_index_for_smlio;
	void *device_remove_in_progress;
	u16 device_remove_in_progress_sz;
	u8 *tm_tr_retry;
	u32 tm_tr_retry_sz;
	u8 temp_sensors_count;
	struct list_head port_table_list;
	u8 *log_buffer;
	dma_addr_t log_buffer_dma;
	char pcie_log_work_q_name[48];
	struct workqueue_struct *pcie_log_work_q;
	struct delayed_work pcie_log_work;
	u32 open_pcie_trace;
};

#define CETC52RAID_DEBUG					(0x00000001)
#define CETC52RAID_DEBUG_MSG_FRAME			(0x00000002)
#define CETC52RAID_DEBUG_SG					(0x00000004)
#define CETC52RAID_DEBUG_EVENTS				(0x00000008)
#define CETC52RAID_DEBUG_EVENT_WORK_TASK	(0x00000010)
#define CETC52RAID_DEBUG_INIT				(0x00000020)
#define CETC52RAID_DEBUG_EXIT				(0x00000040)
#define CETC52RAID_DEBUG_FAIL				(0x00000080)
#define CETC52RAID_DEBUG_TM					(0x00000100)
#define CETC52RAID_DEBUG_REPLY				(0x00000200)
#define CETC52RAID_DEBUG_HANDSHAKE			(0x00000400)
#define CETC52RAID_DEBUG_CONFIG				(0x00000800)
#define CETC52RAID_DEBUG_DL					(0x00001000)
#define CETC52RAID_DEBUG_RESET				(0x00002000)
#define CETC52RAID_DEBUG_SCSI				(0x00004000)
#define CETC52RAID_DEBUG_IOCTL				(0x00008000)
#define CETC52RAID_DEBUG_CSMISAS			(0x00010000)
#define CETC52RAID_DEBUG_SAS				(0x00020000)
#define CETC52RAID_DEBUG_TRANSPORT			(0x00040000)
#define CETC52RAID_DEBUG_TASK_SET_FULL		(0x00080000)

#define CETC52RAID_CHECK_LOGGING(IOC, CMD, BITS)			\
{								\
	if (IOC->logging_level & BITS)				\
		CMD;						\
}

#define dprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG)
#define dsgprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_SG)
#define devtprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_EVENTS)
#define dewtprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_EVENT_WORK_TASK)
#define dinitprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_INIT)
#define dexitprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_EXIT)
#define dfailprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_FAIL)
#define dtmprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_TM)
#define dreplyprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_REPLY)
#define dhsprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_HANDSHAKE)
#define dcprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_CONFIG)
#define ddlprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_DL)
#define drsprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_RESET)
#define dsprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_SCSI)
#define dctlprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_IOCTL)
#define dcsmisasprintk(IOC, CMD)	\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_CSMISAS)
#define dsasprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_SAS)
#define dsastransport(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_SAS_WIDE)
#define dmfprintk(IOC, CMD)			\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_MSG_FRAME)
#define dtsfprintk(IOC, CMD)		\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_TASK_SET_FULL)
#define dtransportprintk(IOC, CMD)	\
	CETC52RAID_CHECK_LOGGING(IOC, CMD, CETC52RAID_DEBUG_TRANSPORT)

static inline void
cetc52raid_debug_dump_mf(void *mpi_request, int sz)
{
	int i;
	__le32 *mfp = (__le32 *) mpi_request;

	pr_info("mf:\n\t");
	for (i = 0; i < sz; i++) {
		if (i && ((i % 8) == 0))
			pr_info("\n\t");
		pr_info("%08x ", le32_to_cpu(mfp[i]));
	}
	pr_info("\n");
}

static inline void
cetc52raid_debug_dump_reply(void *mpi_request, int sz)
{
	int i;
	__le32 *mfp = (__le32 *) mpi_request;

	pr_info("reply:\n\t");
	for (i = 0; i < sz; i++) {
		if (i && ((i % 8) == 0))
			pr_info("\n\t");
		pr_info("%08x ", le32_to_cpu(mfp[i]));
	}
	pr_info("\n");
}

static inline void
cetc52raid_debug_dump_config(void *mpi_request, int sz)
{
	int i;
	__le32 *mfp = (__le32 *) mpi_request;

	pr_info("config:\n\t");
	for (i = 0; i < sz; i++) {
		if (i && ((i % 8) == 0))
			pr_info("\n\t");
		pr_info("%08x ", le32_to_cpu(mfp[i]));
	}
	pr_info("\n");
}

#define CETC52RAID_DRV_INTERNAL_BITMAP_BLK_MQ	(0x00000001)
#define CETC52RAID_DRV_INERNAL_FIRST_PE_ISSUED	(0x00000002)

typedef u8(*CETC52RAID_CALLBACK) (struct CETC52RAID_ADAPTER *ioc, u16 smid,
				u8 msix_index, u32 reply);

#define SCSIH_MAP_QUEUE(shost)	static void cetc52raid_scsihost_map_queues(shost)

extern struct list_head cetc52raid_ioc_list;
extern spinlock_t cetc52raid_gioc_lock;
void cetc52raid_base_start_watchdog(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_stop_watchdog(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_start_log_watchdog(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_stop_log_watchdog(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_trace_log_init(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_attach(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_detach(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_map_resources(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_free_resources(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_free_enclosure_list(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_hard_reset_handler(struct CETC52RAID_ADAPTER *ioc,
				       enum reset_type type);
void *cetc52raid_base_get_msg_frame(struct CETC52RAID_ADAPTER *ioc, u16 smid);
void *cetc52raid_base_get_sense_buffer(struct CETC52RAID_ADAPTER *ioc,
				       u16 smid);
__le32 cetc52raid_base_get_sense_buffer_dma(struct CETC52RAID_ADAPTER *ioc,
					    u16 smid);
__le64 cetc52raid_base_get_sense_buffer_dma_64(struct CETC52RAID_ADAPTER *ioc,
					       u16 smid);
void cetc52raid_base_sync_reply_irqs(struct CETC52RAID_ADAPTER *ioc, u8 poll);
u16 cetc52raid_base_get_smid_hpr(struct CETC52RAID_ADAPTER *ioc, u8 cb_idx);
u16 cetc52raid_base_get_smid_scsiio(struct CETC52RAID_ADAPTER *ioc, u8 cb_idx,
				    struct scsi_cmnd *scmd);
u16 cetc52raid_base_get_smid(struct CETC52RAID_ADAPTER *ioc, u8 cb_idx);
void cetc52raid_base_free_smid(struct CETC52RAID_ADAPTER *ioc, u16 smid);
void cetc52raid_base_initialize_callback_handler(void);
u8 cetc52raid_base_register_callback_handler(CETC52RAID_CALLBACK cb_func);
void cetc52raid_base_release_callback_handler(u8 cb_idx);
u8 cetc52raid_base_done(struct CETC52RAID_ADAPTER *ioc, u16 smid, u8 msix_index,
			u32 reply);
u8 cetc52raid_port_enable_done(struct CETC52RAID_ADAPTER *ioc, u16 smid,
			       u8 msix_index, u32 reply);
void *cetc52raid_base_get_reply_virt_addr(struct CETC52RAID_ADAPTER *ioc,
					  u32 phys_addr);
u32 cetc52raid_base_get_iocstate(struct CETC52RAID_ADAPTER *ioc, int cooked);
int cetc52raid_base_check_and_get_msix_vectors(struct pci_dev *pdev);
void cetc52raid_base_fault_info(struct CETC52RAID_ADAPTER *ioc, u16 fault_code);
#define cetc52raid_print_fault_code(ioc, fault_code) \
	do { \
		pr_err("%s fault info from func: %s\n", ioc->name, __func__); \
		cetc52raid_base_fault_info(ioc, fault_code); \
	} while (0)
void cetc52raid_base_coredump_info(struct CETC52RAID_ADAPTER *ioc,
				   u16 fault_code);
int cetc52raid_base_wait_for_coredump_completion(struct CETC52RAID_ADAPTER *ioc,
						 const char *caller);
int cetc52raid_base_sas_iounit_control(struct CETC52RAID_ADAPTER *ioc,
				       struct Cetc52raidSasIoUnitControlRep_t *
				       mpi_reply,
				       struct Cetc52raidSasIoUnitControlReq_t *
				       mpi_request);
int cetc52raid_base_scsi_enclosure_processor(struct CETC52RAID_ADAPTER *ioc,
					     struct Cetc52raidSepRep_t *mpi_reply,
					     struct Cetc52raidSepReq_t *mpi_request);
void cetc52raid_base_validate_event_type(struct CETC52RAID_ADAPTER *ioc,
					 u32 *event_type);
void cetc52raid_halt_firmware(struct CETC52RAID_ADAPTER *ioc, u8 set_fault);
struct cetc52raid_scsiio_tracker *cetc52raid_get_st_from_smid(
	struct CETC52RAID_ADAPTER *ioc, u16 smid);
void cetc52raid_base_clear_st(struct CETC52RAID_ADAPTER *ioc,
			      struct cetc52raid_scsiio_tracker *st);
struct cetc52raid_scsiio_tracker *cetc52raid_base_scsi_cmd_priv(
	struct scsi_cmnd *scmd);
int
cetc52raid_base_check_for_fault_and_issue_reset(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_port_enable(struct CETC52RAID_ADAPTER *ioc);
u8 cetc52raid_base_pci_device_is_unplugged(struct CETC52RAID_ADAPTER *ioc);
u8 cetc52raid_base_pci_device_is_available(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_free_irq(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_disable_msix(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_wait_for_commands_to_complete(struct CETC52RAID_ADAPTER *ioc);
u8 cetc52raid_base_check_cmd_timeout(struct CETC52RAID_ADAPTER *ioc,
				     u8 status, void *mpi_request, int sz);
#define cetc52raid_check_cmd_timeout(ioc, status, mpi_request, sz, issue_reset) \
	do { \
		pr_err("%s In func: %s\n", ioc->name, __func__); \
		issue_reset = cetc52raid_base_check_cmd_timeout(ioc, status, mpi_request, sz); \
	} while (0)
int cetc52raid_wait_for_ioc_to_operational(struct CETC52RAID_ADAPTER *ioc,
					   int wait_count);
void cetc52raid_base_start_hba_unplug_watchdog(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_stop_hba_unplug_watchdog(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_make_ioc_ready(struct CETC52RAID_ADAPTER *ioc,
				   enum reset_type type);
void cetc52raid_base_mask_interrupts(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_unmask_interrupts(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_blk_mq_poll(struct Scsi_Host *shost, unsigned int queue_num);
void cetc52raid_base_pause_mq_polling(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_base_resume_mq_polling(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_base_unlock_and_get_host_diagnostic(struct CETC52RAID_ADAPTER
						   *ioc, u32 *host_diagnostic);
void cetc52raid_base_lock_host_diagnostic(struct CETC52RAID_ADAPTER *ioc);
extern char driver_name[CETC52RAID_NAME_LENGTH];
struct scsi_cmnd *cetc52raid_scsihost_scsi_lookup_get(struct CETC52RAID_ADAPTER
						      *ioc, u16 smid);
u8 cetc52raid_scsihost_event_callback(struct CETC52RAID_ADAPTER *ioc,
				      u8 msix_index, u32 reply);
void cetc52raid_scsihost_reset_handler(struct CETC52RAID_ADAPTER *ioc,
				       int reset_phase);
int cetc52raid_scsihost_issue_tm(struct CETC52RAID_ADAPTER *ioc, u16 handle,
				 uint channel, uint id, uint lun, u8 type,
				 u16 smid_task, u8 timeout, u8 tr_method);
int cetc52raid_scsihost_issue_locked_tm(struct CETC52RAID_ADAPTER *ioc,
					u16 handle, uint channel, uint id,
					uint lun, u8 type, u16 smid_task,
					u8 timeout, u8 tr_method);
void cetc52raid_scsihost_set_tm_flag(struct CETC52RAID_ADAPTER *ioc,
				     u16 handle);
void cetc52raid_scsihost_clear_tm_flag(struct CETC52RAID_ADAPTER *ioc,
				       u16 handle);
void cetc52raid_expander_remove(
	struct CETC52RAID_ADAPTER *ioc, u64 sas_address,
	struct cetc52raid_hba_port *port);
void cetc52raid_device_remove_by_sas_address(struct CETC52RAID_ADAPTER *ioc,
					     u64 sas_address,
					     struct cetc52raid_hba_port *port);
u8 cetc52raid_check_for_pending_internal_cmds(struct CETC52RAID_ADAPTER *ioc,
					      u16 smid);
struct cetc52raid_hba_port *cetc52raid_get_port_by_id(
	struct CETC52RAID_ADAPTER *ioc, u8 port, u8 skip_dirty_flag);
struct cetc52raid_virtual_phy *cetc52raid_get_vphy_by_phy(
	struct CETC52RAID_ADAPTER *ioc, struct cetc52raid_hba_port *port, u32 phy);
struct cetc52raid_raid_sas_node *cetc52raid_scsihost_expander_find_by_handle(
	struct CETC52RAID_ADAPTER *ioc, u16 handle);
struct cetc52raid_raid_sas_node *cetc52raid_scsihost_expander_find_by_sas_address(
	struct CETC52RAID_ADAPTER *ioc,
	u64 sas_address,
	struct cetc52raid_hba_port *port);
struct cetc52raid_sas_device *__cetc52raid_get_sdev_by_addr_and_rphy(
	struct CETC52RAID_ADAPTER *ioc,
	u64 sas_address,
	struct sas_rphy *rphy);
struct cetc52raid_sas_device *cetc52raid_get_sdev_by_addr(
	struct CETC52RAID_ADAPTER *ioc,
	u64 sas_address,
	struct cetc52raid_hba_port *port);
struct cetc52raid_sas_device *cetc52raid_get_sdev_by_handle(
	struct CETC52RAID_ADAPTER *ioc, u16 handle);
void cetc52raid_scsihost_flush_running_cmds(struct CETC52RAID_ADAPTER *ioc);
void cetc52raid_port_enable_complete(struct CETC52RAID_ADAPTER *ioc);
struct cetc52raid_raid_device *cetc52raid_raid_device_find_by_handle(
	struct CETC52RAID_ADAPTER *ioc, u16 handle);
void cetc52raid_scsihost_sas_device_remove(struct CETC52RAID_ADAPTER *ioc,
				 struct cetc52raid_sas_device *sas_device);
void cetc52raid_scsihost_clear_outstanding_scsi_tm_commands(
	struct CETC52RAID_ADAPTER *ioc);
u32 cetc52raid_base_mod64(u64 dividend, u32 divisor);
void
cetc52raid__scsihost_change_queue_depth(struct scsi_device *sdev, int qdepth);
u8 cetc52raid_scsihost_ncq_prio_supp(struct scsi_device *sdev);
u8 cetc52raid_config_done(struct CETC52RAID_ADAPTER *ioc, u16 smid,
			  u8 msix_index, u32 reply);
int cetc52raid_config_get_number_hba_phys(struct CETC52RAID_ADAPTER *ioc,
					  u8 *num_phys);
int cetc52raid_config_get_manufacturing_pg0(struct CETC52RAID_ADAPTER *ioc,
					    struct Cetc52raidCfgRep_t *mpi_reply,
					    struct Cetc52raidManP0_t *
					    config_page);
int cetc52raid_config_get_manufacturing_pg10(struct CETC52RAID_ADAPTER *ioc,
					     struct Cetc52raidCfgRep_t *mpi_reply,
					     struct Cetc52raidManuP10_t
					     *config_page);
int cetc52raid_config_get_manufacturing_pg11(struct CETC52RAID_ADAPTER *ioc,
					     struct Cetc52raidCfgRep_t *mpi_reply,
					     struct Cetc52raidManuP11_t
					     *config_page);
int cetc52raid_config_set_manufacturing_pg11(struct CETC52RAID_ADAPTER *ioc,
					     struct Cetc52raidCfgRep_t *mpi_reply,
					     struct Cetc52raidManuP11_t
					     *config_page);
int cetc52raid_config_get_bios_pg2(struct CETC52RAID_ADAPTER *ioc,
				   struct Cetc52raidCfgRep_t *mpi_reply,
				   struct Cetc52raidBiosP2_t *config_page);
int cetc52raid_config_get_bios_pg3(struct CETC52RAID_ADAPTER *ioc,
				   struct Cetc52raidCfgRep_t *mpi_reply,
				   struct Cetc52raidBiosP3_t *config_page);
int cetc52raid_config_get_iounit_pg0(struct CETC52RAID_ADAPTER *ioc,
				     struct Cetc52raidCfgRep_t *mpi_reply,
				     struct Cetc52raidIOUnitP0_t *config_page);
int cetc52raid_config_get_sas_device_pg0(struct CETC52RAID_ADAPTER *ioc,
					 struct Cetc52raidCfgRep_t *mpi_reply,
					 struct Cetc52raidSasDevP0_t *config_page,
					 u32 form, u32 handle);
int cetc52raid_config_get_sas_iounit_pg0(struct CETC52RAID_ADAPTER *ioc,
					 struct Cetc52raidCfgRep_t *mpi_reply,
					 struct Cetc52raidSasIOUnitP0_t *config_page,
					 u16 sz);
int cetc52raid_config_get_iounit_pg1(struct CETC52RAID_ADAPTER *ioc,
				     struct Cetc52raidCfgRep_t *mpi_reply,
				     struct Cetc52raidIOUnitP1_t *config_page);
int cetc52raid_config_set_iounit_pg1(struct CETC52RAID_ADAPTER *ioc,
				     struct Cetc52raidCfgRep_t *mpi_reply,
				     struct Cetc52raidIOUnitP1_t *config_page);
int cetc52raid_config_get_iounit_pg8(struct CETC52RAID_ADAPTER *ioc,
				     struct Cetc52raidCfgRep_t *mpi_reply,
				     struct Cetc52raidIOUnitP8_t *config_page);
int cetc52raid_config_get_sas_iounit_pg1(struct CETC52RAID_ADAPTER *ioc,
					 struct Cetc52raidCfgRep_t *mpi_reply,
					 struct Cetc52raidSasIOUnitP1_t *config_page,
					 u16 sz);
int cetc52raid_config_set_sas_iounit_pg1(struct CETC52RAID_ADAPTER *ioc,
					 struct Cetc52raidCfgRep_t *mpi_reply,
					 struct Cetc52raidSasIOUnitP1_t *config_page,
					 u16 sz);
int cetc52raid_config_get_ioc_pg1(struct CETC52RAID_ADAPTER *ioc,
				  struct Cetc52raidCfgRep_t *mpi_reply,
				  struct Cetc52raidIOCP1_t *config_page);
int cetc52raid_config_set_ioc_pg1(struct CETC52RAID_ADAPTER *ioc,
				  struct Cetc52raidCfgRep_t *mpi_reply,
				  struct Cetc52raidIOCP1_t *config_page);
int cetc52raid_config_get_ioc_pg8(struct CETC52RAID_ADAPTER *ioc,
				  struct Cetc52raidCfgRep_t *mpi_reply,
				  struct Cetc52raidIOCP8_t *config_page);
int cetc52raid_config_get_expander_pg0(struct CETC52RAID_ADAPTER *ioc,
				       struct Cetc52raidCfgRep_t *mpi_reply,
				       struct Cetc52raidExpanderP0_t *config_page,
				       u32 form, u32 handle);
int cetc52raid_config_get_expander_pg1(struct CETC52RAID_ADAPTER *ioc,
				       struct Cetc52raidCfgRep_t *mpi_reply,
				       struct Cetc52raidExpanderP1_t *config_page,
				       u32 phy_number, u16 handle);
int cetc52raid_config_get_enclosure_pg0(struct CETC52RAID_ADAPTER *ioc,
					struct Cetc52raidCfgRep_t *mpi_reply,
					struct Cetc52raidSasEncP0_t *
					config_page, u32 form, u32 handle);
int cetc52raid_config_get_phy_pg0(struct CETC52RAID_ADAPTER *ioc,
				  struct Cetc52raidCfgRep_t *mpi_reply,
				  struct Cetc52raidSasPhyP0_t *config_page,
				  u32 phy_number);
int cetc52raid_config_get_phy_pg1(struct CETC52RAID_ADAPTER *ioc,
				  struct Cetc52raidCfgRep_t *mpi_reply,
				  struct Cetc52raidSasPhyP1_t *config_page,
				  u32 phy_number);
int cetc52raid_config_get_raid_volume_pg1(struct CETC52RAID_ADAPTER *ioc,
					  struct Cetc52raidCfgRep_t *mpi_reply,
					  struct Cetc52raidRaidVolP1_t *config_page,
					  u32 form, u32 handle);
int cetc52raid_config_get_number_pds(struct CETC52RAID_ADAPTER *ioc, u16 handle,
				     u8 *num_pds);
int cetc52raid_config_get_raid_volume_pg0(struct CETC52RAID_ADAPTER *ioc,
					  struct Cetc52raidCfgRep_t *mpi_reply,
					  struct Cetc52raidRaidVolP0_t *config_page,
					  u32 form, u32 handle, u16 sz);
int cetc52raid_config_get_phys_disk_pg0(struct CETC52RAID_ADAPTER *ioc,
					struct Cetc52raidCfgRep_t *mpi_reply,
					struct Cetc52raidRaidPDP0_t *
					config_page, u32 form,
					u32 form_specific);
int cetc52raid_config_get_volume_handle(struct CETC52RAID_ADAPTER *ioc,
					u16 pd_handle, u16 *volume_handle);
int cetc52raid_config_get_volume_wwid(struct CETC52RAID_ADAPTER *ioc,
				      u16 volume_handle, u64 *wwid);
extern const struct attribute_group *cetc52raid_host_groups[];
extern const struct attribute_group *cetc52raid_dev_groups[];
void cetc52raid_ctl_init(void);
void cetc52raid_ctl_exit(void);
u8 cetc52raid_ctl_done(struct CETC52RAID_ADAPTER *ioc, u16 smid, u8 msix_index,
		       u32 reply);
u8 cetc52raid_ctl_tm_done(struct CETC52RAID_ADAPTER *ioc, u16 smid,
			  u8 msix_index, u32 reply);
void cetc52raid_ctl_reset_handler(struct CETC52RAID_ADAPTER *ioc,
				  int reset_phase);
u8 cetc52raid_ctl_event_callback(struct CETC52RAID_ADAPTER *ioc, u8 msix_index,
				 u32 reply);
void cetc52raid_ctl_add_to_event_log(struct CETC52RAID_ADAPTER *ioc,
				     struct Cetc52raidEventNotificationRep_t *
				     mpi_reply);
void cetc52raid_ctl_clear_outstanding_ioctls(struct CETC52RAID_ADAPTER *ioc);
int cetc52raid_ctl_release(struct inode *inode, struct file *filep);
void ctl_init(void);
void ctl_exit(void);
u8 cetc52raid_transport_done(struct CETC52RAID_ADAPTER *ioc, u16 smid,
			     u8 msix_index, u32 reply);
struct cetc52raid_sas_port *cetc52raid_transport_port_add(
				struct CETC52RAID_ADAPTER *ioc,
				u16 handle, u64 sas_address,
				struct cetc52raid_hba_port *port);
void cetc52raid_transport_port_remove(struct CETC52RAID_ADAPTER *ioc,
				      u64 sas_address, u64 sas_address_parent,
				      struct cetc52raid_hba_port *port);
int cetc52raid_transport_add_host_phy(
			struct CETC52RAID_ADAPTER *ioc,
			struct cetc52raid_sas_phy *cetc52raid_phy,
			struct Cetc52raidSasPhyP0_t phy_pg0,
			struct device *parent_dev);
int cetc52raid_transport_add_expander_phy(struct CETC52RAID_ADAPTER *ioc,
					  struct cetc52raid_sas_phy *cetc52raid_phy,
					  struct Cetc52raidExpanderP1_t expander_pg1,
					  struct device *parent_dev);
void cetc52raid_transport_update_links(struct CETC52RAID_ADAPTER *ioc,
				       u64 sas_address, u16 handle,
				       u8 phy_number, u8 link_rate,
				       struct cetc52raid_hba_port *port);
extern struct sas_function_template cetc52raid_transport_functions;
extern struct scsi_transport_template *cetc52raid_transport_template;
void
cetc52raid_transport_del_phy_from_an_existing_port(struct CETC52RAID_ADAPTER
						   *ioc,
						   struct cetc52raid_raid_sas_node *sas_node,
						   struct cetc52raid_sas_phy
						   *cetc52raid_phy);
#if defined(CETC52RAID_WIDE_PORT_API)
void
cetc52raid_transport_add_phy_to_an_existing_port(
					struct CETC52RAID_ADAPTER *ioc,
					struct cetc52raid_raid_sas_node *sas_node,
					struct cetc52raid_sas_phy
					*cetc52raid_phy,
					u64 sas_address,
					struct cetc52raid_hba_port *port);
#endif
#endif
