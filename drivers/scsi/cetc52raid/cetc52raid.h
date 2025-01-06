/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * Copyright 2000-2020 Broadcom Inc. All rights reserved.
 *
 * Copyright (C) 2024 Cetc52 Tech Inc.
 *
 */

#ifndef CETC52RAID_H
#define CETC52RAID_H

typedef u8 U8;
typedef __le16 U16;
typedef __le32 U32;
typedef __le64 U64 __aligned(4);

#define CETC52RAID_IOC_STATE_RESET          (0x00000000)
#define CETC52RAID_IOC_STATE_READY          (0x10000000)
#define CETC52RAID_IOC_STATE_OPERATIONAL    (0x20000000)
#define CETC52RAID_IOC_STATE_FAULT          (0x40000000)
#define CETC52RAID_IOC_STATE_COREDUMP       (0x50000000)
#define CETC52RAID_IOC_STATE_MASK           (0xF0000000)

struct Cetc52raidSysInterfaceRegs_t {
	U32 Doorbell;
	U32 WriteSequence;
	U32 HostDiagnostic;
	U32 Reserved1;
	U32 DiagRWData;
	U32 DiagRWAddressLow;
	U32 DiagRWAddressHigh;
	U32 Reserved2[5];
	U32 HostInterruptStatus;
	U32 HostInterruptMask;
	U32 DCRData;
	U32 DCRAddress;
	U32 Reserved3[2];
	U32 ReplyFreeHostIndex;
	U32 Reserved4[8];
	U32 ReplyPostHostIndex;
	U32 Reserved5;
	U32 HCBSize;
	U32 HCBAddressLow;
	U32 HCBAddressHigh;
	U32 Reserved6[12];
	U32 Scratchpad[4];
	U32 RequestDescriptorPostLow;
	U32 RequestDescriptorPostHigh;
	U32 AtomicRequestDescriptorPost;
	U32 IocLogBufPosition;
	U32 HostLogBufPosition;
	U32 Reserved7[11];
};

#define CETC52RAID_DOORBELL_USED                (0x08000000)
#define CETC52RAID_DOORBELL_DATA_MASK           (0x0000FFFF)
#define CETC52RAID_DOORBELL_FUNCTION_SHIFT      (24)
#define CETC52RAID_DOORBELL_ADD_DWORDS_SHIFT    (16)

#define CETC52RAID_DIAG_RESET_ADAPTER           (0x00000004)

#define CETC52RAID_HIS_SYS2IOC_DB_STATUS        (0x80000000)
#define CETC52RAID_HIS_IOC2SYS_DB_STATUS        (0x00000001)

#define CETC52RAID_RPHI_MSIX_INDEX_SHIFT        (24)

#define CETC52RAID_REQ_DESCRIPT_FLAGS_SCSI_IO           (0x00)
#define CETC52RAID_REQ_DESCRIPT_FLAGS_HIGH_PRIORITY     (0x06)
#define CETC52RAID_REQ_DESCRIPT_FLAGS_DEFAULT_TYPE      (0x08)
#define CETC52RAID_REQ_DESCRIPT_FLAGS_FAST_PATH_SCSI_IO (0x0C)

struct CETC52RAID_DEFAULT_REQUEST_DESCRIPTOR {
	U8 RequestFlags;
	U8 MSIxIndex;
	U16 SMID;
	U16 LMID;
	U16 DescriptorTypeDependent;
};

struct CETC52RAID_HIGH_PRIORITY_REQUEST_DESCRIPTOR {
	U8 RequestFlags;
	U8 MSIxIndex;
	U16 SMID;
	U16 LMID;
	U16 Reserved1;
};

struct CETC52RAID_SCSI_IO_REQUEST_DESCRIPTOR {
	U8 RequestFlags;
	U8 MSIxIndex;
	U16 SMID;
	U16 LMID;
	U16 DevHandle;
};

typedef
struct CETC52RAID_SCSI_IO_REQUEST_DESCRIPTOR
	CETC52RAID_FP_SCSI_IO_REQUEST_DESCRIPTOR;

union Cetc52raidReqDescUnion_t {
	struct CETC52RAID_DEFAULT_REQUEST_DESCRIPTOR Default;
	struct CETC52RAID_HIGH_PRIORITY_REQUEST_DESCRIPTOR HighPriority;
	struct CETC52RAID_SCSI_IO_REQUEST_DESCRIPTOR SCSIIO;
	CETC52RAID_FP_SCSI_IO_REQUEST_DESCRIPTOR FastPathSCSIIO;
	U64 Words;
};

struct Cetc52raidAtomicReqDesc_t {
	U8 RequestFlags;
	U8 MSIxIndex;
	U16 SMID;
};

#define CETC52RAID_RPY_DESCRIPT_FLAGS_TYPE_MASK                 (0x0F)
#define CETC52RAID_RPY_DESCRIPT_FLAGS_SCSI_IO_SUCCESS           (0x00)
#define CETC52RAID_RPY_DESCRIPT_FLAGS_ADDRESS_REPLY             (0x01)
#define CETC52RAID_RPY_DESCRIPT_FLAGS_FAST_PATH_SCSI_IO_SUCCESS (0x06)
#define CETC52RAID_RPY_DESCRIPT_FLAGS_UNUSED                    (0x0F)

struct Cetc52raidDefaultRepDesc_t {
	U8 ReplyFlags;
	U8 MSIxIndex;
	U16 DescriptorTypeDependent1;
	U32 DescriptorTypeDependent2;
};

struct CETC52RAID_ADDRESS_REPLY_DESCRIPTOR {
	U8 ReplyFlags;
	U8 MSIxIndex;
	U16 SMID;
	U32 ReplyFrameAddress;
};

struct CETC52RAID_SCSI_IO_SUCCESS_REPLY_DESCRIPTOR {
	U8 ReplyFlags;
	U8 MSIxIndex;
	U16 SMID;
	U16 TaskTag;
	U16 Reserved1;
};

typedef
struct CETC52RAID_SCSI_IO_SUCCESS_REPLY_DESCRIPTOR
	CETC52RAID_FP_SCSI_IO_SUCCESS_REPLY_DESCRIPTOR;

union Cetc52raidRepDescUnion_t {
	struct Cetc52raidDefaultRepDesc_t Default;
	struct CETC52RAID_ADDRESS_REPLY_DESCRIPTOR AddressReply;
	struct CETC52RAID_SCSI_IO_SUCCESS_REPLY_DESCRIPTOR SCSIIOSuccess;
	CETC52RAID_FP_SCSI_IO_SUCCESS_REPLY_DESCRIPTOR FastPathSCSIIOSuccess;
	U64 Words;
};

#define CETC52RAID_FUNC_SCSI_IO_REQUEST             (0x00)
#define CETC52RAID_FUNC_SCSI_TASK_MGMT              (0x01)
#define CETC52RAID_FUNC_IOC_INIT                    (0x02)
#define CETC52RAID_FUNC_IOC_FACTS                   (0x03)
#define CETC52RAID_FUNC_CONFIG                      (0x04)
#define CETC52RAID_FUNC_PORT_FACTS                  (0x05)
#define CETC52RAID_FUNC_PORT_ENABLE                 (0x06)
#define CETC52RAID_FUNC_EVENT_NOTIFICATION          (0x07)
#define CETC52RAID_FUNC_EVENT_ACK                   (0x08)
#define CETC52RAID_FUNC_FW_DOWNLOAD                 (0x09)
#define CETC52RAID_FUNC_FW_UPLOAD                   (0x12)
#define CETC52RAID_FUNC_RAID_ACTION                 (0x15)
#define CETC52RAID_FUNC_RAID_SCSI_IO_PASSTHROUGH    (0x16)
#define CETC52RAID_FUNC_SCSI_ENCLOSURE_PROCESSOR    (0x18)
#define CETC52RAID_FUNC_SMP_PASSTHROUGH             (0x1A)
#define CETC52RAID_FUNC_SAS_IO_UNIT_CONTROL         (0x1B)
#define CETC52RAID_FUNC_IO_UNIT_CONTROL             (0x1B)
#define CETC52RAID_FUNC_SATA_PASSTHROUGH            (0x1C)
#define CETC52RAID_FUNC_IOC_MESSAGE_UNIT_RESET      (0x40)
#define CETC52RAID_FUNC_HANDSHAKE                   (0x42)
#define CETC52RAID_FUNC_LOG_INIT                    (0x57)

#define CETC52RAID_IOCSTATUS_MASK                   (0x7FFF)
#define CETC52RAID_IOCSTATUS_SUCCESS                (0x0000)
#define CETC52RAID_IOCSTATUS_INVALID_FUNCTION       (0x0001)
#define CETC52RAID_IOCSTATUS_BUSY                   (0x0002)
#define CETC52RAID_IOCSTATUS_INVALID_SGL            (0x0003)
#define CETC52RAID_IOCSTATUS_INTERNAL_ERROR         (0x0004)
#define CETC52RAID_IOCSTATUS_INVALID_VPID           (0x0005)
#define CETC52RAID_IOCSTATUS_INSUFFICIENT_RESOURCES (0x0006)
#define CETC52RAID_IOCSTATUS_INVALID_FIELD          (0x0007)
#define CETC52RAID_IOCSTATUS_INVALID_STATE          (0x0008)
#define CETC52RAID_IOCSTATUS_OP_STATE_NOT_SUPPORTED (0x0009)
#define CETC52RAID_IOCSTATUS_INSUFFICIENT_POWER     (0x000A)

#define CETC52RAID_IOCSTATUS_CONFIG_INVALID_ACTION  (0x0020)
#define CETC52RAID_IOCSTATUS_CONFIG_INVALID_TYPE    (0x0021)
#define CETC52RAID_IOCSTATUS_CONFIG_INVALID_PAGE    (0x0022)
#define CETC52RAID_IOCSTATUS_CONFIG_INVALID_DATA    (0x0023)
#define CETC52RAID_IOCSTATUS_CONFIG_NO_DEFAULTS     (0x0024)
#define CETC52RAID_IOCSTATUS_CONFIG_CANT_COMMIT     (0x0025)

#define CETC52RAID_IOCSTATUS_SCSI_RECOVERED_ERROR   (0x0040)
#define CETC52RAID_IOCSTATUS_SCSI_INVALID_DEVHANDLE (0x0042)
#define CETC52RAID_IOCSTATUS_SCSI_DEVICE_NOT_THERE  (0x0043)
#define CETC52RAID_IOCSTATUS_SCSI_DATA_OVERRUN      (0x0044)
#define CETC52RAID_IOCSTATUS_SCSI_DATA_UNDERRUN     (0x0045)
#define CETC52RAID_IOCSTATUS_SCSI_IO_DATA_ERROR     (0x0046)
#define CETC52RAID_IOCSTATUS_SCSI_PROTOCOL_ERROR    (0x0047)
#define CETC52RAID_IOCSTATUS_SCSI_TASK_TERMINATED   (0x0048)
#define CETC52RAID_IOCSTATUS_SCSI_RESIDUAL_MISMATCH (0x0049)
#define CETC52RAID_IOCSTATUS_SCSI_TASK_MGMT_FAILED  (0x004A)
#define CETC52RAID_IOCSTATUS_SCSI_IOC_TERMINATED    (0x004B)
#define CETC52RAID_IOCSTATUS_SCSI_EXT_TERMINATED    (0x004C)

#define CETC52RAID_IOCSTATUS_EEDP_GUARD_ERROR       (0x004D)
#define CETC52RAID_IOCSTATUS_EEDP_REF_TAG_ERROR     (0x004E)
#define CETC52RAID_IOCSTATUS_EEDP_APP_TAG_ERROR     (0x004F)

#define CETC52RAID_IOCSTATUS_TARGET_INVALID_IO_INDEX      (0x0062)
#define CETC52RAID_IOCSTATUS_TARGET_ABORTED               (0x0063)
#define CETC52RAID_IOCSTATUS_TARGET_NO_CONN_RETRYABLE     (0x0064)
#define CETC52RAID_IOCSTATUS_TARGET_NO_CONNECTION         (0x0065)
#define CETC52RAID_IOCSTATUS_TARGET_XFER_COUNT_MISMATCH   (0x006A)
#define CETC52RAID_IOCSTATUS_TARGET_DATA_OFFSET_ERROR     (0x006D)
#define CETC52RAID_IOCSTATUS_TARGET_TOO_MUCH_WRITE_DATA   (0x006E)
#define CETC52RAID_IOCSTATUS_TARGET_IU_TOO_SHORT          (0x006F)
#define CETC52RAID_IOCSTATUS_TARGET_ACK_NAK_TIMEOUT       (0x0070)
#define CETC52RAID_IOCSTATUS_TARGET_NAK_RECEIVED          (0x0071)

#define CETC52RAID_IOCSTATUS_SAS_SMP_REQUEST_FAILED       (0x0090)
#define CETC52RAID_IOCSTATUS_SAS_SMP_DATA_OVERRUN         (0x0091)
#define CETC52RAID_IOCSTATUS_FLAG_LOG_INFO_AVAILABLE      (0x8000)

struct Cetc52raidReqHeader_t {
	U16 FunctionDependent1;
	U8 ChainOffset;
	U8 Function;
	U16 FunctionDependent2;
	U8 FunctionDependent3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
};

struct Cetc52raidDefaultRep_t {
	U16 FunctionDependent1;
	U8 MsgLength;
	U8 Function;
	U16 FunctionDependent2;
	U8 FunctionDependent3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U16 FunctionDependent5;
	U16 IOCStatus;
	U32 IOCLogInfo;
};

struct CETC52RAID_VERSION_STRUCT {
	U8 Dev;
	U8 Unit;
	U8 Minor;
	U8 Major;
};

union CETC52RAID_VERSION_UNION {
	struct CETC52RAID_VERSION_STRUCT Struct;
	U32 Word;
};

struct Cetc52SGESimple32_t {
	U32 FlagsLength;
	U32 Address;
};

struct Cetc52SGESimple64_t {
	U32 FlagsLength;
	U64 Address;
};

struct CETC52RAID_SGE_SIMPLE_UNION {
	U32 FlagsLength;
	union {
		U32 Address32;
		U64 Address64;
	} u;
};

struct CETC52RAID_SGE_CHAIN_UNION {
	U16 Length;
	U8 NextChainOffset;
	U8 Flags;
	union {
		U32 Address32;
		U64 Address64;
	} u;
};

#define CETC52RAID_SGE_FLAGS_LAST_ELEMENT             (0x80)
#define CETC52RAID_SGE_FLAGS_END_OF_BUFFER            (0x40)
#define CETC52RAID_SGE_FLAGS_END_OF_LIST              (0x01)
#define CETC52RAID_SGE_FLAGS_SHIFT                    (24)
#define CETC52RAID_SGE_FLAGS_SIMPLE_ELEMENT           (0x10)
#define CETC52RAID_SGE_FLAGS_SYSTEM_ADDRESS           (0x00)
#define CETC52RAID_SGE_FLAGS_HOST_TO_IOC              (0x04)
#define CETC52RAID_SGE_FLAGS_32_BIT_ADDRESSING        (0x00)
#define CETC52RAID_SGE_FLAGS_64_BIT_ADDRESSING        (0x02)

struct CETC52RAID_IEEE_SGE_SIMPLE32 {
	U32 Address;
	U32 FlagsLength;
};

struct CETC52RAID_IEEE_SGE_SIMPLE64 {
	U64 Address;
	U32 Length;
	U16 Reserved1;
	U8 Reserved2;
	U8 Flags;
};

union CETC52RAID_IEEE_SGE_SIMPLE_UNION {
	struct CETC52RAID_IEEE_SGE_SIMPLE32 Simple32;
	struct CETC52RAID_IEEE_SGE_SIMPLE64 Simple64;
};

union CETC52RAID_IEEE_SGE_CHAIN_UNION {
	struct CETC52RAID_IEEE_SGE_SIMPLE32 Chain32;
	struct CETC52RAID_IEEE_SGE_SIMPLE64 Chain64;
};

struct CETC52RAID_IEEE_SGE_CHAIN64 {
	U64 Address;
	U32 Length;
	U16 Reserved1;
	U8 NextChainOffset;
	U8 Flags;
};

union CETC52RAID_IEEE_SGE_IO_UNION {
	struct CETC52RAID_IEEE_SGE_SIMPLE64 IeeeSimple;
	struct CETC52RAID_IEEE_SGE_CHAIN64 IeeeChain;
};

#define CETC52RAID_IEEE_SGE_FLAGS_END_OF_LIST       (0x40)
#define CETC52RAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT    (0x00)
#define CETC52RAID_IEEE_SGE_FLAGS_CHAIN_ELEMENT     (0x80)
#define CETC52RAID_IEEE_SGE_FLAGS_SYSTEM_ADDR       (0x00)

union CETC52RAID_SIMPLE_SGE_UNION {
	struct CETC52RAID_SGE_SIMPLE_UNION Cetc52Simple;
	union CETC52RAID_IEEE_SGE_SIMPLE_UNION IeeeSimple;
};

union CETC52RAID_SGE_IO_UNION {
	struct CETC52RAID_SGE_SIMPLE_UNION Cetc52Simple;
	struct CETC52RAID_SGE_CHAIN_UNION Cetc52Chain;
	union CETC52RAID_IEEE_SGE_SIMPLE_UNION IeeeSimple;
	union CETC52RAID_IEEE_SGE_CHAIN_UNION IeeeChain;
};

struct CETC52RAID_CONFIG_PAGE_HEADER {
	U8 PageVersion;
	U8 PageLength;
	U8 PageNumber;
	U8 PageType;
};

struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER {
	U8 PageVersion;
	U8 Reserved1;
	U8 PageNumber;
	U8 PageType;
	U16 ExtPageLength;
	U8 ExtPageType;
	U8 Reserved2;
};

#define CETC52RAID_CONFIG_PAGETYPE_IO_UNIT                (0x00)
#define CETC52RAID_CONFIG_PAGETYPE_IOC                    (0x01)
#define CETC52RAID_CONFIG_PAGETYPE_BIOS                   (0x02)
#define CETC52RAID_CONFIG_PAGETYPE_RAID_VOLUME            (0x08)
#define CETC52RAID_CONFIG_PAGETYPE_MANUFACTURING          (0x09)
#define CETC52RAID_CONFIG_PAGETYPE_RAID_PHYSDISK          (0x0A)
#define CETC52RAID_CONFIG_PAGETYPE_EXTENDED               (0x0F)
#define CETC52RAID_CONFIG_PAGETYPE_MASK                   (0x0F)
#define CETC52RAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT         (0x10)
#define CETC52RAID_CONFIG_EXTPAGETYPE_SAS_EXPANDER        (0x11)
#define CETC52RAID_CONFIG_EXTPAGETYPE_SAS_DEVICE          (0x12)
#define CETC52RAID_CONFIG_EXTPAGETYPE_SAS_PHY             (0x13)
#define CETC52RAID_CONFIG_EXTPAGETYPE_LOG                 (0x14)
#define CETC52RAID_CONFIG_EXTPAGETYPE_ENCLOSURE           (0x15)
#define CETC52RAID_CONFIG_EXTPAGETYPE_RAID_CONFIG         (0x16)
#define CETC52RAID_CONFIG_EXTPAGETYPE_DRIVER_MAPPING      (0x17)
#define CETC52RAID_CONFIG_EXTPAGETYPE_SAS_PORT            (0x18)
#define CETC52RAID_CONFIG_EXTPAGETYPE_EXT_MANUFACTURING   (0x1A)

#define CETC52RAID_RAID_VOLUME_PGAD_FORM_GET_NEXT_HANDLE  (0x00000000)
#define CETC52RAID_RAID_VOLUME_PGAD_FORM_HANDLE           (0x10000000)

#define CETC52RAID_PHYSDISK_PGAD_FORM_GET_NEXT_PHYSDISKNUM    (0x00000000)
#define CETC52RAID_PHYSDISK_PGAD_FORM_PHYSDISKNUM             (0x10000000)

#define CETC52RAID_SAS_EXPAND_PGAD_FORM_GET_NEXT_HNDL     (0x00000000)
#define CETC52RAID_SAS_EXPAND_PGAD_FORM_HNDL_PHY_NUM      (0x10000000)
#define CETC52RAID_SAS_EXPAND_PGAD_FORM_HNDL              (0x20000000)
#define CETC52RAID_SAS_EXPAND_PGAD_PHYNUM_SHIFT           (16)
#define CETC52RAID_SAS_DEVICE_PGAD_FORM_GET_NEXT_HANDLE   (0x00000000)
#define CETC52RAID_SAS_DEVICE_PGAD_FORM_HANDLE            (0x20000000)
#define CETC52RAID_SAS_PHY_PGAD_FORM_PHY_NUMBER           (0x00000000)
#define CETC52RAID_SAS_ENCLOS_PGAD_FORM_GET_NEXT_HANDLE   (0x00000000)
#define CETC52RAID_SAS_ENCLOS_PGAD_FORM_HANDLE            (0x10000000)
#define CETC52RAID_RAID_PGAD_FORM_GET_NEXT_CONFIGNUM      (0x00000000)

struct Cetc52raidCfgReq_t {
	U8 Action;
	U8 SGLFlags;
	U8 ChainOffset;
	U8 Function;
	U16 ExtPageLength;
	U8 ExtPageType;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U8 Reserved2;
	U8 ProxyVF_ID;
	U16 Reserved4;
	U32 Reserved3;
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 PageAddress;
	union CETC52RAID_SGE_IO_UNION PageBufferSGE;
};

#define CETC52RAID_CONFIG_ACTION_PAGE_HEADER              (0x00)
#define CETC52RAID_CONFIG_ACTION_PAGE_READ_CURRENT        (0x01)
#define CETC52RAID_CONFIG_ACTION_PAGE_WRITE_CURRENT       (0x02)
#define CETC52RAID_CONFIG_ACTION_PAGE_WRITE_NVRAM         (0x04)

struct Cetc52raidCfgRep_t {
	U8 Action;
	U8 SGLFlags;
	U8 MsgLength;
	U8 Function;
	U16 ExtPageLength;
	U8 ExtPageType;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U16 Reserved2;
	U16 IOCStatus;
	U32 IOCLogInfo;
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
};

struct Cetc52raidManP0_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U8 ChipName[16];
	U8 ChipRevision[8];
	U8 BoardName[16];
	U8 BoardAssembly[16];
	U8 BoardTracerNumber[16];
};

struct CETC52RAID_MANPAGE7_CONNECTOR_INFO {
	U32 Pinout;
	U8 Connector[16];
	U8 Location;
	U8 ReceptacleID;
	U16 Slot;
	U16 Slotx2;
	U16 Slotx4;
};

struct Cetc52raidIOUnitP0_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U64 UniqueValue;
	union CETC52RAID_VERSION_UNION NvdataVersionDefault;
	union CETC52RAID_VERSION_UNION NvdataVersionPersistent;
};

struct Cetc52raidIOUnitP1_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 Flags;
};

#define CETC52RAID_IOUNITPAGE1_NATIVE_COMMAND_Q_DISABLE       (0x00000100)
#define CETC52RAID_IOUNITPAGE1_DISABLE_TASK_SET_FULL_HANDLING (0x00000020)

struct CETC52RAID_IOUNIT8_SENSOR {
	U16 Flags;
	U16 Reserved1;
	U16 Threshold[4];
	U32 Reserved2;
	U32 Reserved3;
	U32 Reserved4;
};

struct Cetc52raidIOUnitP8_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 Reserved1;
	U32 Reserved2;
	U8 NumSensors;
	U8 PollingInterval;
	U16 Reserved3;
	struct CETC52RAID_IOUNIT8_SENSOR Sensor[];
};

struct Cetc52raidIOCP1_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 Flags;
	U32 CoalescingTimeout;
	U8 CoalescingDepth;
	U8 PCISlotNum;
	U8 PCIBusNum;
	U8 PCIDomainSegment;
	U32 Reserved1;
	U32 ProductSpecific;
};

struct Cetc52raidIOCP8_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U8 NumDevsPerEnclosure;
	U8 Reserved1;
	U16 Reserved2;
	U16 MaxPersistentEntries;
	U16 MaxNumPhysicalMappedIDs;
	U16 Flags;
	U16 Reserved3;
	U16 IRVolumeMappingFlags;
	U16 Reserved4;
	U32 Reserved5;
};

#define CETC52RAID_IOCPAGE8_IRFLAGS_MASK_VOLUME_MAPPING_MODE  (0x00000003)
#define CETC52RAID_IOCPAGE8_IRFLAGS_LOW_VOLUME_MAPPING        (0x00000000)

struct CETC52RAID_BOOT_DEVICE_ADAPTER_ORDER {
	U32 Reserved1;
	U32 Reserved2;
	U32 Reserved3;
	U32 Reserved4;
	U32 Reserved5;
	U32 Reserved6;
};

struct CETC52RAID_BOOT_DEVICE_SAS_WWID {
	U64 SASAddress;
	U8 LUN[8];
	U32 Reserved1;
	U32 Reserved2;
};

struct CETC52RAID_BOOT_DEVICE_ENCLOSURE_SLOT {
	U64 EnclosureLogicalID;
	U32 Reserved1;
	U32 Reserved2;
	U16 SlotNumber;
	U16 Reserved3;
	U32 Reserved4;
};

struct CETC52RAID_BOOT_DEVICE_DEVICE_NAME {
	U64 DeviceName;
	U8 LUN[8];
	U32 Reserved1;
	U32 Reserved2;
};

union CETC52RAID_BIOSPAGE2_BOOT_DEVICE {
	struct CETC52RAID_BOOT_DEVICE_ADAPTER_ORDER AdapterOrder;
	struct CETC52RAID_BOOT_DEVICE_SAS_WWID SasWwid;
	struct CETC52RAID_BOOT_DEVICE_ENCLOSURE_SLOT EnclosureSlot;
	struct CETC52RAID_BOOT_DEVICE_DEVICE_NAME DeviceName;
};

struct Cetc52raidBiosP2_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 Reserved1;
	U32 Reserved2;
	U32 Reserved3;
	U32 Reserved4;
	U32 Reserved5;
	U32 Reserved6;
	U8 ReqBootDeviceForm;
	U8 Reserved7;
	U16 Reserved8;
	union CETC52RAID_BIOSPAGE2_BOOT_DEVICE RequestedBootDevice;
	U8 ReqAltBootDeviceForm;
	U8 Reserved9;
	U16 Reserved10;
	union CETC52RAID_BIOSPAGE2_BOOT_DEVICE RequestedAltBootDevice;
	U8 CurrentBootDeviceForm;
	U8 Reserved11;
	U16 Reserved12;
	union CETC52RAID_BIOSPAGE2_BOOT_DEVICE CurrentBootDevice;
};

#define CETC52RAID_BIOSPAGE2_FORM_MASK                        (0x0F)
#define CETC52RAID_BIOSPAGE2_FORM_NO_DEVICE_SPECIFIED         (0x00)
#define CETC52RAID_BIOSPAGE2_FORM_SAS_WWID                    (0x05)
#define CETC52RAID_BIOSPAGE2_FORM_ENCLOSURE_SLOT              (0x06)
#define CETC52RAID_BIOSPAGE2_FORM_DEVICE_NAME                 (0x07)

struct CETC52RAID_ADAPTER_INFO {
	U8 PciBusNumber;
	U8 PciDeviceAndFunctionNumber;
	U16 AdapterFlags;
};

struct CETC52RAID_ADAPTER_ORDER_AUX {
	U64 WWID;
	U32 Reserved1;
	U32 Reserved2;
};

struct Cetc52raidBiosP3_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U32 GlobalFlags;
	U32 BiosVersion;
	struct CETC52RAID_ADAPTER_INFO AdapterOrder[4];
	U32 Reserved1;
	struct CETC52RAID_ADAPTER_ORDER_AUX AdapterOrderAux[4];
};

struct CETC52RAID_RAIDVOL0_PHYS_DISK {
	U8 RAIDSetNum;
	U8 PhysDiskMap;
	U8 PhysDiskNum;
	U8 Reserved;
};

struct CETC52RAID_RAIDVOL0_SETTINGS {
	U16 Settings;
	U8 HotSparePool;
	U8 Reserved;
};

struct Cetc52raidRaidVolP0_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U16 DevHandle;
	U8 VolumeState;
	U8 VolumeType;
	U32 VolumeStatusFlags;
	struct CETC52RAID_RAIDVOL0_SETTINGS VolumeSettings;
	U64 MaxLBA;
	U32 StripeSize;
	U16 BlockSize;
	U16 Reserved1;
	U8 SupportedPhysDisks;
	U8 ResyncRate;
	U16 DataScrubDuration;
	U8 NumPhysDisks;
	U8 Reserved2;
	U8 Reserved3;
	U8 InactiveStatus;
	struct CETC52RAID_RAIDVOL0_PHYS_DISK PhysDisk[];
};

#define CETC52RAID_RAID_VOL_STATE_MISSING                         (0x00)
#define CETC52RAID_RAID_VOL_STATE_FAILED                          (0x01)
#define CETC52RAID_RAID_VOL_STATE_INITIALIZING                    (0x02)
#define CETC52RAID_RAID_VOL_STATE_ONLINE                          (0x03)
#define CETC52RAID_RAID_VOL_STATE_DEGRADED                        (0x04)
#define CETC52RAID_RAID_VOL_STATE_OPTIMAL                         (0x05)
#define CETC52RAID_RAID_VOL_TYPE_RAID0                            (0x00)
#define CETC52RAID_RAID_VOL_TYPE_RAID1E                           (0x01)
#define CETC52RAID_RAID_VOL_TYPE_RAID1                            (0x02)
#define CETC52RAID_RAID_VOL_TYPE_RAID10                           (0x05)
#define CETC52RAID_RAID_VOL_TYPE_UNKNOWN                          (0xFF)

#define CETC52RAID_RAIDVOL0_STATUS_FLAG_RESYNC_IN_PROGRESS        (0x00010000)

struct Cetc52raidRaidVolP1_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U16 DevHandle;
	U16 Reserved0;
	U8 GUID[24];
	U8 Name[16];
	U64 WWID;
	U32 Reserved1;
	U32 Reserved2;
};

struct CETC52RAID_RAIDPHYSDISK0_SETTINGS {
	U16 Reserved1;
	U8 HotSparePool;
	U8 Reserved2;
};

struct CETC52RAID_RAIDPHYSDISK0_INQUIRY_DATA {
	U8 VendorID[8];
	U8 ProductID[16];
	U8 ProductRevLevel[4];
	U8 SerialNum[32];
};

struct Cetc52raidRaidPDP0_t {
	struct CETC52RAID_CONFIG_PAGE_HEADER Header;
	U16 DevHandle;
	U8 Reserved1;
	U8 PhysDiskNum;
	struct CETC52RAID_RAIDPHYSDISK0_SETTINGS PhysDiskSettings;
	U32 Reserved2;
	struct CETC52RAID_RAIDPHYSDISK0_INQUIRY_DATA InquiryData;
	U32 Reserved3;
	U8 PhysDiskState;
	U8 OfflineReason;
	U8 IncompatibleReason;
	U8 PhysDiskAttributes;
	U32 PhysDiskStatusFlags;
	U64 DeviceMaxLBA;
	U64 HostMaxLBA;
	U64 CoercedMaxLBA;
	U16 BlockSize;
	U16 Reserved5;
	U32 Reserved6;
};

#define CETC52RAID_RAID_PD_STATE_NOT_CONFIGURED               (0x00)
#define CETC52RAID_RAID_PD_STATE_NOT_COMPATIBLE               (0x01)
#define CETC52RAID_RAID_PD_STATE_OFFLINE                      (0x02)
#define CETC52RAID_RAID_PD_STATE_ONLINE                       (0x03)
#define CETC52RAID_RAID_PD_STATE_HOT_SPARE                    (0x04)
#define CETC52RAID_RAID_PD_STATE_DEGRADED                     (0x05)
#define CETC52RAID_RAID_PD_STATE_REBUILDING                   (0x06)
#define CETC52RAID_RAID_PD_STATE_OPTIMAL                      (0x07)

#define CETC52RAID_SAS_NEG_LINK_RATE_MASK_PHYSICAL            (0x0F)
#define CETC52RAID_SAS_NEG_LINK_RATE_UNKNOWN_LINK_RATE        (0x00)
#define CETC52RAID_SAS_NEG_LINK_RATE_PHY_DISABLED             (0x01)
#define CETC52RAID_SAS_NEG_LINK_RATE_NEGOTIATION_FAILED       (0x02)
#define CETC52RAID_SAS_NEG_LINK_RATE_SATA_OOB_COMPLETE        (0x03)
#define CETC52RAID_SAS_NEG_LINK_RATE_PORT_SELECTOR            (0x04)
#define CETC52RAID_SAS_NEG_LINK_RATE_SMP_RESET_IN_PROGRESS    (0x05)
#define CETC52RAID_SAS_NEG_LINK_RATE_1_5                      (0x08)
#define CETC52RAID_SAS_NEG_LINK_RATE_3_0                      (0x09)
#define CETC52RAID_SAS_NEG_LINK_RATE_6_0                      (0x0A)
#define CETC52RAID_SAS_NEG_LINK_RATE_12_0                     (0x0B)

#define CETC52RAID_SAS_PHYINFO_VIRTUAL_PHY                    (0x00001000)

#define CETC52RAID_SAS_PRATE_MIN_RATE_MASK                    (0x0F)
#define CETC52RAID_SAS_HWRATE_MIN_RATE_MASK                   (0x0F)

struct CETC52RAID_SAS_IO_UNIT0_PHY_DATA {
	U8 Port;
	U8 PortFlags;
	U8 PhyFlags;
	U8 NegotiatedLinkRate;
	U32 ControllerPhyDeviceInfo;
	U16 AttachedDevHandle;
	U16 ControllerDevHandle;
	U32 DiscoveryStatus;
	U32 Reserved;
};

struct Cetc52raidSasIOUnitP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U32 Reserved1;
	U8 NumPhys;
	U8 Reserved2;
	U16 Reserved3;
	struct CETC52RAID_SAS_IO_UNIT0_PHY_DATA PhyData[];
};

#define CETC52RAID_SASIOUNIT0_PORTFLAGS_DISCOVERY_IN_PROGRESS (0x08)
#define CETC52RAID_SASIOUNIT0_PORTFLAGS_AUTO_PORT_CONFIG      (0x01)
#define CETC52RAID_SASIOUNIT0_PHYFLAGS_ZONING_ENABLED         (0x10)
#define CETC52RAID_SASIOUNIT0_PHYFLAGS_PHY_DISABLED           (0x08)

struct CETC52RAID_SAS_IO_UNIT1_PHY_DATA {
	U8 Port;
	U8 PortFlags;
	U8 PhyFlags;
	U8 MaxMinLinkRate;
	U32 ControllerPhyDeviceInfo;
	U16 MaxTargetPortConnectTime;
	U16 Reserved1;
};

struct Cetc52raidSasIOUnitP1_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U16 ControlFlags;
	U16 SASNarrowMaxQueueDepth;
	U16 AdditionalControlFlags;
	U16 SASWideMaxQueueDepth;
	U8 NumPhys;
	U8 SATAMaxQDepth;
	U8 ReportDeviceMissingDelay;
	U8 IODeviceMissingDelay;
	struct CETC52RAID_SAS_IO_UNIT1_PHY_DATA PhyData[];
};

#define CETC52RAID_SASIOUNIT1_REPORT_MISSING_TIMEOUT_MASK (0x7F)
#define CETC52RAID_SASIOUNIT1_REPORT_MISSING_UNIT_16      (0x80)
#define CETC52RAID_SASIOUNIT1_PHYFLAGS_ZONING_ENABLE      (0x10)
#define CETC52RAID_SASIOUNIT1_PHYFLAGS_PHY_DISABLE        (0x08)

struct Cetc52raidExpanderP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U8 PhysicalPort;
	U8 ReportGenLength;
	U16 EnclosureHandle;
	U64 SASAddress;
	U32 DiscoveryStatus;
	U16 DevHandle;
	U16 ParentDevHandle;
	U16 ExpanderChangeCount;
	U16 ExpanderRouteIndexes;
	U8 NumPhys;
	U8 SASLevel;
	U16 Flags;
	U16 STPBusInactivityTimeLimit;
	U16 STPMaxConnectTimeLimit;
	U16 STP_SMP_NexusLossTime;
	U16 MaxNumRoutedSasAddresses;
	U64 ActiveZoneManagerSASAddress;
	U16 ZoneLockInactivityLimit;
	U16 Reserved1;
	U8 TimeToReducedFunc;
	U8 InitialTimeToReducedFunc;
	U8 MaxReducedFuncTime;
	U8 Reserved2;
};

struct Cetc52raidExpanderP1_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U8 PhysicalPort;
	U8 Reserved1;
	U16 Reserved2;
	U8 NumPhys;
	U8 Phy;
	U16 NumTableEntriesProgrammed;
	U8 ProgrammedLinkRate;
	U8 HwLinkRate;
	U16 AttachedDevHandle;
	U32 PhyInfo;
	U32 AttachedDeviceInfo;
	U16 ExpanderDevHandle;
	U8 ChangeCount;
	U8 NegotiatedLinkRate;
	U8 PhyIdentifier;
	U8 AttachedPhyIdentifier;
	U8 Reserved3;
	U8 DiscoveryInfo;
	U32 AttachedPhyInfo;
	U8 ZoneGroup;
	U8 SelfConfigStatus;
	U16 Reserved4;
};

struct Cetc52raidSasDevP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U16 Slot;
	U16 EnclosureHandle;
	U64 SASAddress;
	U16 ParentDevHandle;
	U8 PhyNum;
	U8 AccessStatus;
	U16 DevHandle;
	U8 AttachedPhyIdentifier;
	U8 ZoneGroup;
	U32 DeviceInfo;
	U16 Flags;
	U8 PhysicalPort;
	U8 MaxPortConnections;
	U64 DeviceName;
	U8 PortGroups;
	U8 DmaGroup;
	U8 ControlGroup;
	U8 EnclosureLevel;
	U8 ConnectorName[4];
	U32 Reserved3;
};

#define CETC52RAID_SAS_DEVICE0_ASTATUS_NO_ERRORS                  (0x00)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SATA_INIT_FAILED           (0x01)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SATA_CAPABILITY_FAILED     (0x02)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SATA_AFFILIATION_CONFLICT  (0x03)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SATA_NEEDS_INITIALIZATION  (0x04)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_ROUTE_NOT_ADDRESSABLE      (0x05)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SMP_ERROR_NOT_ADDRESSABLE  (0x06)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_DEVICE_BLOCKED             (0x07)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_UNKNOWN                (0x10)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_AFFILIATION_CONFLICT   (0x11)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_DIAG                   (0x12)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_IDENTIFICATION         (0x13)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_CHECK_POWER            (0x14)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_PIO_SN                 (0x15)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_MDMA_SN                (0x16)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_UDMA_SN                (0x17)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_ZONING_VIOLATION       (0x18)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_NOT_ADDRESSABLE        (0x19)
#define CETC52RAID_SAS_DEVICE0_ASTATUS_SIF_MAX                    (0x1F)
#define CETC52RAID_SAS_DEVICE0_FLAGS_FAST_PATH_CAPABLE            (0x2000)
#define CETC52RAID_SAS_DEVICE0_FLAGS_SATA_ASYNCHRONOUS_NOTIFY     (0x0400)
#define CETC52RAID_SAS_DEVICE0_FLAGS_SATA_SW_PRESERVE             (0x0200)
#define CETC52RAID_SAS_DEVICE0_FLAGS_SATA_SMART_SUPPORTED         (0x0040)
#define CETC52RAID_SAS_DEVICE0_FLAGS_SATA_NCQ_SUPPORTED           (0x0020)
#define CETC52RAID_SAS_DEVICE0_FLAGS_SATA_FUA_SUPPORTED           (0x0010)
#define CETC52RAID_SAS_DEVICE0_FLAGS_ENCL_LEVEL_VALID             (0x0002)
#define CETC52RAID_SAS_DEVICE0_FLAGS_DEVICE_PRESENT               (0x0001)

struct Cetc52raidSasPhyP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U16 OwnerDevHandle;
	U16 Reserved1;
	U16 AttachedDevHandle;
	U8 AttachedPhyIdentifier;
	U8 Reserved2;
	U32 AttachedPhyInfo;
	U8 ProgrammedLinkRate;
	U8 HwLinkRate;
	U8 ChangeCount;
	U8 Flags;
	U32 PhyInfo;
	U8 NegotiatedLinkRate;
	U8 Reserved3;
	U16 Reserved4;
};

struct Cetc52raidSasPhyP1_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U32 Reserved1;
	U32 InvalidDwordCount;
	U32 RunningDisparityErrorCount;
	U32 LossDwordSynchCount;
	U32 PhyResetProblemCount;
};

struct Cetc52raidSasEncP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U32 Reserved1;
	U64 EnclosureLogicalID;
	U16 Flags;
	U16 EnclosureHandle;
	U16 NumSlots;
	U16 StartSlot;
	U8 ChassisSlot;
	U8 EnclosureLevel;
	U16 SEPDevHandle;
	U8 OEMRD;
	U8 Reserved1a;
	U16 Reserved2;
	U32 Reserved3;
};

#define CETC52RAID_SAS_ENCLS0_FLAGS_CHASSIS_SLOT_VALID    (0x0020)

struct CETC52RAID_RAIDCONFIG0_CONFIG_ELEMENT {
	U16 ElementFlags;
	U16 VolDevHandle;
	U8 HotSparePool;
	U8 PhysDiskNum;
	U16 PhysDiskDevHandle;
};

#define CETC52RAID_RAIDCONFIG0_EFLAGS_MASK_ELEMENT_TYPE       (0x000F)
#define CETC52RAID_RAIDCONFIG0_EFLAGS_VOL_PHYS_DISK_ELEMENT   (0x0001)
#define CETC52RAID_RAIDCONFIG0_EFLAGS_HOT_SPARE_ELEMENT       (0x0002)
#define CETC52RAID_RAIDCONFIG0_EFLAGS_OCE_ELEMENT             (0x0003)

struct Cetc52raidRaidCfgP0_t {
	struct CETC52RAID_CONFIG_EXTENDED_PAGE_HEADER Header;
	U8 NumHotSpares;
	U8 NumPhysDisks;
	U8 NumVolumes;
	U8 ConfigNum;
	U32 Flags;
	U8 ConfigGUID[24];
	U32 Reserved1;
	U8 NumElements;
	U8 Reserved2;
	U16 Reserved3;
	struct CETC52RAID_RAIDCONFIG0_CONFIG_ELEMENT ConfigElement[];
};

struct Cetc52raidFWImgHeader_t {
	U32 Signature;
	U32 Signature0;
	U32 Signature1;
	U32 Signature2;
	union CETC52RAID_VERSION_UNION CETC52Version;
	union CETC52RAID_VERSION_UNION FWVersion;
	union CETC52RAID_VERSION_UNION NVDATAVersion;
	union CETC52RAID_VERSION_UNION PackageVersion;
	U16 VendorID;
	U16 ProductID;
	U16 ProtocolFlags;
	U16 Reserved26;
	U32 IOCCapabilities;
	U32 ImageSize;
	U32 NextImageHeaderOffset;
	U32 Checksum;
	U32 Reserved38;
	U32 Reserved3C;
	U32 Reserved40;
	U32 Reserved44;
	U32 Reserved48;
	U32 Reserved4C;
	U32 Reserved50;
	U32 Reserved54;
	U32 Reserved58;
	U32 Reserved5C;
	U32 BootFlags;
	U32 FirmwareVersionNameWhat;
	U8 FirmwareVersionName[32];
	U32 VendorNameWhat;
	U8 VendorName[32];
	U32 PackageNameWhat;
	U8 PackageName[32];
	U32 ReservedD0;
	U32 ReservedD4;
	U32 ReservedD8;
	U32 ReservedDC;
	U32 ReservedE0;
	U32 ReservedE4;
	U32 ReservedE8;
	U32 ReservedEC;
	U32 ReservedF0;
	U32 ReservedF4;
	U32 ReservedF8;
	U32 ReservedFC;
};

struct CETC52RAID_HASH_EXCLUSION_FORMAT {
	U32 Offset;
	U32 Size;
};

struct Cetc52raidComptImgHeader_t {
	U32 Signature0;
	U32 LoadAddress;
	U32 DataSize;
	U32 StartAddress;
	U32 Signature1;
	U32 FlashOffset;
	U32 FlashSize;
	U32 VersionStringOffset;
	U32 BuildDateStringOffset;
	U32 BuildTimeStringOffset;
	U32 EnvironmentVariableOffset;
	U32 ApplicationSpecific;
	U32 Signature2;
	U32 HeaderSize;
	U32 Crc;
	U8 NotFlashImage;
	U8 Compressed;
	U16 Reserved3E;
	U32 SecondaryFlashOffset;
	U32 Reserved44;
	U32 Reserved48;
	union CETC52RAID_VERSION_UNION RMCInterfaceVersion;
	union CETC52RAID_VERSION_UNION Reserved50;
	union CETC52RAID_VERSION_UNION FWVersion;
	union CETC52RAID_VERSION_UNION NvdataVersion;
	struct CETC52RAID_HASH_EXCLUSION_FORMAT HashExclusion[4];
	U32 NextImageHeaderOffset;
	U32 Reserved80[32];
};

struct CETC52RAID_SCSI_IO_CDB_EEDP32 {
	U8 CDB[20];
	__be32 PrimaryReferenceTag;
	U16 PrimaryApplicationTag;
	U16 PrimaryApplicationTagMask;
	U32 TransferLength;
};

union CETC52_SCSI_IO_CDB_UNION {
	U8 CDB32[32];
	struct CETC52RAID_SCSI_IO_CDB_EEDP32 EEDP32;
	struct CETC52RAID_SGE_SIMPLE_UNION SGE;
};

struct Cetc52SCSIIOReq_t {
	U16 DevHandle;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved1;
	U8 Reserved2;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U32 SenseBufferLowAddress;
	U16 SGLFlags;
	U8 SenseBufferLength;
	U8 Reserved4;
	U8 SGLOffset0;
	U8 SGLOffset1;
	U8 SGLOffset2;
	U8 SGLOffset3;
	U32 SkipCount;
	U32 DataLength;
	U32 BidirectionalDataLength;
	U16 IoFlags;
	U16 EEDPFlags;
	U32 EEDPBlockSize;
	U32 SecondaryReferenceTag;
	U16 SecondaryApplicationTag;
	U16 ApplicationTagTranslationMask;
	U8 LUN[8];
	U32 Control;
	union CETC52_SCSI_IO_CDB_UNION CDB;
	union CETC52RAID_SGE_IO_UNION SGL;
};

#define CETC52RAID_SCSIIO_MSGFLAGS_SYSTEM_SENSE_ADDR      (0x00)

#define CETC52RAID_SCSIIO_CONTROL_ADDCDBLEN_SHIFT     (26)
#define CETC52RAID_SCSIIO_CONTROL_NODATATRANSFER      (0x00000000)
#define CETC52RAID_SCSIIO_CONTROL_WRITE               (0x01000000)
#define CETC52RAID_SCSIIO_CONTROL_READ                (0x02000000)
#define CETC52RAID_SCSIIO_CONTROL_BIDIRECTIONAL       (0x03000000)
#define CETC52RAID_SCSIIO_CONTROL_CMDPRI_SHIFT        (11)
#define CETC52RAID_SCSIIO_CONTROL_SIMPLEQ             (0x00000000)
#define CETC52RAID_SCSIIO_CONTROL_ORDEREDQ            (0x00000200)
#define CETC52RAID_SCSIIO_CONTROL_TLR_ON              (0x00000040)

union CETC52RAID_SCSI_IO_CDB_UNION {
	U8 CDB32[32];
	struct CETC52RAID_SCSI_IO_CDB_EEDP32 EEDP32;
	struct CETC52RAID_IEEE_SGE_SIMPLE64 SGE;
};

struct Cetc52raidSCSIIOReq_t {
	U16 DevHandle;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved1;
	U8 Reserved2;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U32 SenseBufferLowAddress;
	U8 DMAFlags;
	U8 Reserved5;
	U8 SenseBufferLength;
	U8 Reserved4;
	U8 SGLOffset0;
	U8 SGLOffset1;
	U8 SGLOffset2;
	U8 SGLOffset3;
	U32 SkipCount;
	U32 DataLength;
	U32 BidirectionalDataLength;
	U16 IoFlags;
	U16 EEDPFlags;
	U16 EEDPBlockSize;
	U16 Reserved6;
	U32 SecondaryReferenceTag;
	U16 SecondaryApplicationTag;
	U16 ApplicationTagTranslationMask;
	U8 LUN[8];
	U32 Control;
	union CETC52RAID_SCSI_IO_CDB_UNION CDB;
	union CETC52RAID_IEEE_SGE_IO_UNION SGL;
};

struct Cetc52raidSCSIIORep_t {
	U16 DevHandle;
	U8 MsgLength;
	U8 Function;
	U16 Reserved1;
	U8 Reserved2;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U8 SCSIStatus;
	U8 SCSIState;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U32 TransferCount;
	U32 SenseCount;
	U32 ResponseInfo;
	U16 TaskTag;
	U16 SCSIStatusQualifier;
	U32 BidirectionalTransferCount;
	U32 EEDPErrorOffset;
	U16 EEDPObservedAppTag;
	U16 EEDPObservedGuard;
	U32 EEDPObservedRefTag;
};

#define CETC52RAID_SCSI_STATUS_GOOD                   (0x00)
#define CETC52RAID_SCSI_STATUS_CHECK_CONDITION        (0x02)
#define CETC52RAID_SCSI_STATUS_CONDITION_MET          (0x04)
#define CETC52RAID_SCSI_STATUS_BUSY                   (0x08)
#define CETC52RAID_SCSI_STATUS_INTERMEDIATE           (0x10)
#define CETC52RAID_SCSI_STATUS_INTERMEDIATE_CONDMET   (0x14)
#define CETC52RAID_SCSI_STATUS_RESERVATION_CONFLICT   (0x18)
#define CETC52RAID_SCSI_STATUS_COMMAND_TERMINATED     (0x22)
#define CETC52RAID_SCSI_STATUS_TASK_SET_FULL          (0x28)
#define CETC52RAID_SCSI_STATUS_ACA_ACTIVE             (0x30)
#define CETC52RAID_SCSI_STATUS_TASK_ABORTED           (0x40)
#define CETC52RAID_SCSI_STATE_RESPONSE_INFO_VALID     (0x10)
#define CETC52RAID_SCSI_STATE_TERMINATED              (0x08)
#define CETC52RAID_SCSI_STATE_NO_SCSI_STATUS          (0x04)
#define CETC52RAID_SCSI_STATE_AUTOSENSE_FAILED        (0x02)
#define CETC52RAID_SCSI_STATE_AUTOSENSE_VALID         (0x01)

struct Cetc52raidSCSITmgReq_t {
	U16 DevHandle;
	U8 ChainOffset;
	U8 Function;
	U8 Reserved1;
	U8 TaskType;
	U8 Reserved2;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U8 LUN[8];
	U32 Reserved4[7];
	U16 TaskMID;
	U16 Reserved5;
};

#define CETC52RAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK           (0x01)
#define CETC52RAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET        (0x02)
#define CETC52RAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET         (0x03)
#define CETC52RAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET   (0x05)
#define CETC52RAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK           (0x07)
#define CETC52RAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET           (0x00)

struct Cetc52raidSCSITmgRep_t {
	U16 DevHandle;
	U8 MsgLength;
	U8 Function;
	U8 ResponseCode;
	U8 TaskType;
	U8 Reserved1;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U16 Reserved3;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U32 TerminationCount;
	U32 ResponseInfo;
};

#define CETC52RAID_SCSITASKMGMT_RSP_TM_COMPLETE               (0x00)
#define CETC52RAID_SCSITASKMGMT_RSP_INVALID_FRAME             (0x02)
#define CETC52RAID_SCSITASKMGMT_RSP_TM_NOT_SUPPORTED          (0x04)
#define CETC52RAID_SCSITASKMGMT_RSP_TM_FAILED                 (0x05)
#define CETC52RAID_SCSITASKMGMT_RSP_TM_SUCCEEDED              (0x08)
#define CETC52RAID_SCSITASKMGMT_RSP_TM_INVALID_LUN            (0x09)
#define CETC52RAID_SCSITASKMGMT_RSP_IO_QUEUED_ON_IOC          (0x80)

struct Cetc52raidSepReq_t {
	U16 DevHandle;
	U8 ChainOffset;
	U8 Function;
	U8 Action;
	U8 Flags;
	U8 Reserved1;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U32 SlotStatus;
	U32 Reserved3;
	U32 Reserved4;
	U32 Reserved5;
	U16 Slot;
	U16 EnclosureHandle;
};

#define CETC52RAID_SEP_REQ_ACTION_WRITE_STATUS                (0x00)
#define CETC52RAID_SEP_REQ_FLAGS_DEVHANDLE_ADDRESS            (0x00)
#define CETC52RAID_SEP_REQ_FLAGS_ENCLOSURE_SLOT_ADDRESS       (0x01)
#define CETC52RAID_SEP_REQ_SLOTSTATUS_PREDICTED_FAULT         (0x00000040)

struct Cetc52raidSepRep_t {
	U16 DevHandle;
	U8 MsgLength;
	U8 Function;
	U8 Action;
	U8 Flags;
	U8 Reserved1;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U16 Reserved3;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U32 SlotStatus;
	U32 Reserved4;
	U16 Slot;
	U16 EnclosureHandle;
};

struct Cetc52raidIOCInitReq_t {
	U8 WhoInit;
	U8 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 MsgVersion;
	U16 HeaderVersion;
	U32 Reserved5;
	U16 ConfigurationFlags;
	U8 HostPageSize;
	U8 HostMSIxVectors;
	U16 Reserved8;
	U16 SystemRequestFrameSize;
	U16 ReplyDescriptorPostQueueDepth;
	U16 ReplyFreeQueueDepth;
	U32 SenseBufferAddressHigh;
	U32 SystemReplyAddressHigh;
	U64 SystemRequestFrameBaseAddress;
	U64 ReplyDescriptorPostQueueAddress;
	U64 ReplyFreeQueueAddress;
	U64 TimeStamp;
};

#define CETC52RAID_WHOINIT_HOST_DRIVER                (0x04)
#define CETC52RAID_IOCINIT_MSGFLAG_RDPQ_ARRAY_MODE    (0x01)

struct Cetc52raidIOCInitRDPQArrayEntry {
	U64 RDPQBaseAddress;
	U32 Reserved1;
	U32 Reserved2;
};

struct Cetc52raidIOCInitRep_t {
	U8 WhoInit;
	U8 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 Reserved5;
	U16 IOCStatus;
	U32 IOCLogInfo;
};

struct Cetc52raidIOCLogReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U64 BufAddr;
	U32 BufSize;
};

struct Cetc52raidIOCLogRep_t {
	U16 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 Reserved5;
	U16 IOCStatus;
	U32 IOCLogInfo;
};

struct Cetc52raidIOCFactsReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
};

struct Cetc52raidIOCFactsRep_t {
	U16 MsgVersion;
	U8 MsgLength;
	U8 Function;
	U16 HeaderVersion;
	U8 IOCNumber;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U16 IOCExceptions;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U8 MaxChainDepth;
	U8 WhoInit;
	U8 NumberOfPorts;
	U8 MaxMSIxVectors;
	U16 RequestCredit;
	U16 ProductID;
	U32 IOCCapabilities;
	union CETC52RAID_VERSION_UNION FWVersion;
	U16 IOCRequestFrameSize;
	U16 IOCMaxChainSegmentSize;
	U16 MaxInitiators;
	U16 MaxTargets;
	U16 MaxSasExpanders;
	U16 MaxEnclosures;
	U16 ProtocolFlags;
	U16 HighPriorityCredit;
	U16 MaxReplyDescriptorPostQueueDepth;
	U8 ReplyFrameSize;
	U8 MaxVolumes;
	U16 MaxDevHandle;
	U16 MaxPersistentEntries;
	U16 MinDevHandle;
	U8 CurrentHostPageSize;
	U8 Reserved4;
	U8 SGEModifierMask;
	U8 SGEModifierValue;
	U8 SGEModifierShift;
	U8 Reserved5;
};

#define CETC52RAID_IOCFACTS_CAPABILITY_ATOMIC_REQ               (0x00080000)
#define CETC52RAID_IOCFACTS_CAPABILITY_RDPQ_ARRAY_CAPABLE       (0x00040000)
#define CETC52RAID_IOCFACTS_CAPABILITY_MSI_X_INDEX              (0x00008000)
#define CETC52RAID_IOCFACTS_CAPABILITY_EVENT_REPLAY             (0x00002000)
#define CETC52RAID_IOCFACTS_CAPABILITY_INTEGRATED_RAID          (0x00001000)
#define CETC52RAID_IOCFACTS_CAPABILITY_TLR                      (0x00000800)
#define CETC52RAID_IOCFACTS_CAPABILITY_MULTICAST                (0x00000100)
#define CETC52RAID_IOCFACTS_CAPABILITY_BIDIRECTIONAL_TARGET     (0x00000080)
#define CETC52RAID_IOCFACTS_CAPABILITY_EEDP                     (0x00000040)
#define CETC52RAID_IOCFACTS_CAPABILITY_TASK_SET_FULL_HANDLING   (0x00000004)
#define CETC52RAID_IOCFACTS_PROTOCOL_SCSI_INITIATOR             (0x0002)
#define CETC52RAID_IOCFACTS_PROTOCOL_SCSI_TARGET                (0x0001)

struct Cetc52raidPortFactsReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 PortNumber;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
};

struct Cetc52raidPortFactsRep_t {
	U16 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 Reserved2;
	U8 PortNumber;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U16 Reserved4;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U8 Reserved5;
	U8 PortType;
	U16 Reserved6;
	U16 MaxPostedCmdBuffers;
	U16 Reserved7;
};

struct Cetc52raidPortEnableReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U8 Reserved2;
	U8 PortFlags;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
};

struct Cetc52raidPortEnableRep_t {
	U16 Reserved1;
	U8 MsgLength;
	U8 Function;
	U8 Reserved2;
	U8 PortFlags;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 Reserved5;
	U16 IOCStatus;
	U32 IOCLogInfo;
};

#define CETC52RAID_EVENT_NOTIFY_EVENTMASK_WORDS           (4)
struct Cetc52raidEventNotificationReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U32 Reserved5;
	U32 Reserved6;
	U32 EventMasks[CETC52RAID_EVENT_NOTIFY_EVENTMASK_WORDS];
	U16 SASBroadcastPrimitiveMasks;
	U16 SASNotifyPrimitiveMasks;
	U32 Reserved8;
};

struct Cetc52raidEventNotificationRep_t {
	U16 EventDataLength;
	U8 MsgLength;
	U8 Function;
	U16 Reserved1;
	U8 AckRequired;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U16 Reserved3;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U16 Event;
	U16 Reserved4;
	U32 EventContext;
	U32 EventData[];
};

#define CETC52RAID_EVENT_NOTIFICATION_ACK_REQUIRED        (0x01)
#define CETC52RAID_EVENT_LOG_DATA                         (0x0001)
#define CETC52RAID_EVENT_STATE_CHANGE                     (0x0002)
#define CETC52RAID_EVENT_HARD_RESET_RECEIVED              (0x0005)
#define CETC52RAID_EVENT_EVENT_CHANGE                     (0x000A)
#define CETC52RAID_EVENT_SAS_DEVICE_STATUS_CHANGE         (0x000F)
#define CETC52RAID_EVENT_IR_OPERATION_STATUS              (0x0014)
#define CETC52RAID_EVENT_SAS_DISCOVERY                    (0x0016)
#define CETC52RAID_EVENT_SAS_BROADCAST_PRIMITIVE          (0x0017)
#define CETC52RAID_EVENT_SAS_INIT_DEVICE_STATUS_CHANGE    (0x0018)
#define CETC52RAID_EVENT_SAS_INIT_TABLE_OVERFLOW          (0x0019)
#define CETC52RAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST         (0x001C)
#define CETC52RAID_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE    (0x001D)
#define CETC52RAID_EVENT_IR_VOLUME                        (0x001E)
#define CETC52RAID_EVENT_IR_PHYSICAL_DISK                 (0x001F)
#define CETC52RAID_EVENT_IR_CONFIGURATION_CHANGE_LIST     (0x0020)
#define CETC52RAID_EVENT_LOG_ENTRY_ADDED                  (0x0021)
#define CETC52RAID_EVENT_SAS_QUIESCE                      (0x0025)
#define CETC52RAID_EVENT_TEMP_THRESHOLD                   (0x0027)
#define CETC52RAID_EVENT_SAS_DEVICE_DISCOVERY_ERROR       (0x0035)

struct Cetc52raidEventDataSasDeviceStatusChange_t {
	U16 TaskTag;
	U8 ReasonCode;
	U8 PhysicalPort;
	U8 ASC;
	U8 ASCQ;
	U16 DevHandle;
	U32 Reserved2;
	U64 SASAddress;
	U8 LUN[8];
};

#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_SMART_DATA                           (0x05)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_UNSUPPORTED                          (0x07)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_INTERNAL_DEVICE_RESET                (0x08)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_TASK_ABORT_INTERNAL                  (0x09)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_ABORT_TASK_SET_INTERNAL              (0x0A)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_CLEAR_TASK_SET_INTERNAL              (0x0B)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_QUERY_TASK_INTERNAL                  (0x0C)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_ASYNC_NOTIFICATION                   (0x0D)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_CMP_INTERNAL_DEV_RESET               (0x0E)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_CMP_TASK_ABORT_INTERNAL              (0x0F)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_SATA_INIT_FAILURE                    (0x10)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_EXPANDER_REDUCED_FUNCTIONALITY       (0x11)
#define CETC52RAID_EVENT_SAS_DEV_STAT_RC_CMP_EXPANDER_REDUCED_FUNCTIONALITY   (0x12)

struct Cetc52raidEventDataIrOpStatus_t {
	U16 VolDevHandle;
	U16 Reserved1;
	U8 RAIDOperation;
	U8 PercentComplete;
	U16 Reserved2;
	U32 ElapsedSeconds;
};

#define CETC52RAID_EVENT_IR_RAIDOP_RESYNC                     (0x00)
#define CETC52RAID_EVENT_IR_RAIDOP_ONLINE_CAP_EXPANSION       (0x01)
#define CETC52RAID_EVENT_IR_RAIDOP_CONSISTENCY_CHECK          (0x02)
#define CETC52RAID_EVENT_IR_RAIDOP_BACKGROUND_INIT            (0x03)
#define CETC52RAID_EVENT_IR_RAIDOP_MAKE_DATA_CONSISTENT       (0x04)

struct Cetc52raidEventDataIrVol_t {
	U16 VolDevHandle;
	U8 ReasonCode;
	U8 Reserved1;
	U32 NewValue;
	U32 PreviousValue;
};

#define CETC52RAID_EVENT_IR_VOLUME_RC_STATE_CHANGED           (0x03)
struct Cetc52raidEventDataIrPhyDisk_t {
	U16 Reserved1;
	U8 ReasonCode;
	U8 PhysDiskNum;
	U16 PhysDiskDevHandle;
	U16 Reserved2;
	U16 Slot;
	U16 EnclosureHandle;
	U32 NewValue;
	U32 PreviousValue;
};

#define CETC52RAID_EVENT_IR_PHYSDISK_RC_STATE_CHANGED         (0x03)

struct Cetc52raidEventIrCfgEle_t {
	U16 ElementFlags;
	U16 VolDevHandle;
	U8 ReasonCode;
	U8 PhysDiskNum;
	U16 PhysDiskDevHandle;
};

#define CETC52RAID_EVENT_IR_CHANGE_EFLAGS_ELEMENT_TYPE_MASK   (0x000F)
#define CETC52RAID_EVENT_IR_CHANGE_EFLAGS_VOLUME_ELEMENT      (0x0000)
#define CETC52RAID_EVENT_IR_CHANGE_EFLAGS_VOLPHYSDISK_ELEMENT (0x0001)
#define CETC52RAID_EVENT_IR_CHANGE_EFLAGS_HOTSPARE_ELEMENT    (0x0002)
#define CETC52RAID_EVENT_IR_CHANGE_RC_ADDED                   (0x01)
#define CETC52RAID_EVENT_IR_CHANGE_RC_REMOVED                 (0x02)
#define CETC52RAID_EVENT_IR_CHANGE_RC_NO_CHANGE               (0x03)
#define CETC52RAID_EVENT_IR_CHANGE_RC_HIDE                    (0x04)
#define CETC52RAID_EVENT_IR_CHANGE_RC_UNHIDE                  (0x05)
#define CETC52RAID_EVENT_IR_CHANGE_RC_VOLUME_CREATED          (0x06)
#define CETC52RAID_EVENT_IR_CHANGE_RC_VOLUME_DELETED          (0x07)
#define CETC52RAID_EVENT_IR_CHANGE_RC_PD_CREATED              (0x08)
#define CETC52RAID_EVENT_IR_CHANGE_RC_PD_DELETED              (0x09)

struct Cetc52raidEventDataIrCfgChangeList_t {
	U8 NumElements;
	U8 Reserved1;
	U8 Reserved2;
	U8 ConfigNum;
	U32 Flags;
	struct Cetc52raidEventIrCfgEle_t ConfigElement[];
};

#define CETC52RAID_EVENT_IR_CHANGE_FLAGS_FOREIGN_CONFIG   (0x00000001)
struct Cetc52raidEventDataSasDiscovery_t {
	U8 Flags;
	U8 ReasonCode;
	U8 PhysicalPort;
	U8 Reserved1;
	U32 DiscoveryStatus;
};

#define CETC52RAID_EVENT_SAS_DISC_RC_STARTED                      (0x01)

struct Cetc52raidEventDataSasBroadcastPrimitive_t {
	U8 PhyNum;
	U8 Port;
	U8 PortWidth;
	U8 Primitive;
};

#define CETC52RAID_EVENT_PRIMITIVE_ASYNCHRONOUS_EVENT             (0x04)

struct CETC52RAID_EVENT_SAS_TOPO_PHY_ENTRY {
	U16 AttachedDevHandle;
	U8 LinkRate;
	U8 PhyStatus;
};

struct Cetc52raidEventDataSasTopoChangeList_t {
	U16 EnclosureHandle;
	U16 ExpanderDevHandle;
	U8 NumPhys;
	U8 Reserved1;
	U16 Reserved2;
	U8 NumEntries;
	U8 StartPhyNum;
	U8 ExpStatus;
	U8 PhysicalPort;
	struct CETC52RAID_EVENT_SAS_TOPO_PHY_ENTRY PHY[];
};

#define CETC52RAID_EVENT_SAS_TOPO_ES_ADDED                        (0x01)
#define CETC52RAID_EVENT_SAS_TOPO_ES_NOT_RESPONDING               (0x02)
#define CETC52RAID_EVENT_SAS_TOPO_ES_RESPONDING                   (0x03)
#define CETC52RAID_EVENT_SAS_TOPO_ES_DELAY_NOT_RESPONDING         (0x04)
#define CETC52RAID_EVENT_SAS_TOPO_PHYSTATUS_VACANT                (0x80)
#define CETC52RAID_EVENT_SAS_TOPO_RC_MASK                         (0x0F)
#define CETC52RAID_EVENT_SAS_TOPO_RC_TARG_ADDED                   (0x01)
#define CETC52RAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING          (0x02)
#define CETC52RAID_EVENT_SAS_TOPO_RC_PHY_CHANGED                  (0x03)
#define CETC52RAID_EVENT_SAS_TOPO_RC_NO_CHANGE                    (0x04)
#define CETC52RAID_EVENT_SAS_TOPO_RC_DELAY_NOT_RESPONDING         (0x05)

struct Cetc52raidEventDataSasEnclDevStatusChange_t {
	U16 EnclosureHandle;
	U8 ReasonCode;
	U8 PhysicalPort;
	U64 EnclosureLogicalID;
	U16 NumSlots;
	U16 StartSlot;
	U32 PhyBits;
};

#define CETC52RAID_EVENT_SAS_ENCL_RC_ADDED                (0x01)
#define CETC52RAID_EVENT_SAS_ENCL_RC_NOT_RESPONDING       (0x02)

struct Cetc52raidEventDataSasDeviceDiscoveryError_t {
	U16 DevHandle;
	U8 ReasonCode;
	U8 PhysicalPort;
	U32 Reserved1[2];
	U64 SASAddress;
	U32 Reserved2[2];
};

#define CETC52RAID_EVENT_SAS_DISC_ERR_SMP_FAILED         (0x01)
#define CETC52RAID_EVENT_SAS_DISC_ERR_SMP_TIMEOUT        (0x02)

struct Cetc52raidEventAckReq_t {
	U16 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 Event;
	U16 Reserved5;
	U32 EventContext;
};

struct Cetc52raidFWUploadReq_t {
	U8 ImageType;
	U8 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U32 Reserved5;
	U32 Reserved6;
	U32 Reserved7;
	U32 ImageOffset;
	U32 ImageSize;
	union CETC52RAID_IEEE_SGE_IO_UNION SGL;
};

struct Cetc52raidFWUploadRep_t {
	U8 ImageType;
	U8 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 Reserved2;
	U8 Reserved3;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved4;
	U16 Reserved5;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U32 ActualImageSize;
};

struct Cetc52raidIoUnitControlReq_t {
	U8 Operation;
	U8 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 DevHandle;
	U8 IOCParameter;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U16 Reserved4;
	U8 PhyNum;
	U8 PrimFlags;
	U32 Primitive;
	U8 LookupMethod;
	U8 Reserved5;
	U16 SlotNumber;
	U64 LookupAddress;
	U32 IOCParameterValue;
	U32 IOCParameterValue2;
	U32 Reserved8;
};

#define CETC52RAID_CTRL_OP_REMOVE_DEVICE (0x0D)

struct Cetc52raidIoUnitControlRep_t {
	U8 Operation;
	U8 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 DevHandle;
	U8 IOCParameter;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U16 Reserved4;
	U16 IOCStatus;
	U32 IOCLogInfo;
};

struct CETC52RAID_RAID_ACTION_RATE_DATA {
	U8 RateToChange;
	U8 RateOrMode;
	U16 DataScrubDuration;
};

struct CETC52RAID_RAID_ACTION_START_RAID_FUNCTION {
	U8 RAIDFunction;
	U8 Flags;
	U16 Reserved1;
};

struct CETC52RAID_RAID_ACTION_STOP_RAID_FUNCTION {
	U8 RAIDFunction;
	U8 Flags;
	U16 Reserved1;
};

struct CETC52RAID_RAID_ACTION_HOT_SPARE {
	U8 HotSparePool;
	U8 Reserved1;
	U16 DevHandle;
};

struct CETC52RAID_RAID_ACTION_FW_UPDATE_MODE {
	U8 Flags;
	U8 DeviceFirmwareUpdateModeTimeout;
	U16 Reserved1;
};

union CETC52RAID_RAID_ACTION_DATA {
	U32 Word;
	struct CETC52RAID_RAID_ACTION_RATE_DATA Rates;
	struct CETC52RAID_RAID_ACTION_START_RAID_FUNCTION StartRaidFunction;
	struct CETC52RAID_RAID_ACTION_STOP_RAID_FUNCTION StopRaidFunction;
	struct CETC52RAID_RAID_ACTION_HOT_SPARE HotSpare;
	struct CETC52RAID_RAID_ACTION_FW_UPDATE_MODE FwUpdateMode;
};

struct Cetc52raidRaidActionReq_t {
	U8 Action;
	U8 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 VolDevHandle;
	U8 PhysDiskNum;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U32 Reserved3;
	union CETC52RAID_RAID_ACTION_DATA ActionDataWord;
	struct CETC52RAID_SGE_SIMPLE_UNION ActionDataSGE;
};

struct CETC52RAID_RAID_VOL_INDICATOR {
	U64 TotalBlocks;
	U64 BlocksRemaining;
	U32 Flags;
	U32 ElapsedSeconds;
};

struct CETC52RAID_RAID_COMPATIBILITY_RESULT_STRUCT {
	U8 State;
	U8 Reserved1;
	U16 Reserved2;
	U32 GenericAttributes;
	U32 OEMSpecificAttributes;
	U32 Reserved3;
	U32 Reserved4;
};

union CETC52RAID_RAID_ACTION_REPLY_DATA {
	U32 Word[6];
	struct CETC52RAID_RAID_VOL_INDICATOR RaidVolumeIndicator;
	U16 VolDevHandle;
	U8 VolumeState;
	U8 PhysDiskNum;
	struct CETC52RAID_RAID_COMPATIBILITY_RESULT_STRUCT RaidCompatibilityResult;
};

struct Cetc52raidRaidActionRep_t {
	U8 Action;
	U8 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 VolDevHandle;
	U8 PhysDiskNum;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved2;
	U16 Reserved3;
	U16 IOCStatus;
	U32 IOCLogInfo;
	union CETC52RAID_RAID_ACTION_REPLY_DATA ActionData;
};

#define CETC52RAID_SAS_DEVICE_INFO_SEP                (0x00004000)
#define CETC52RAID_SAS_DEVICE_INFO_ATAPI_DEVICE       (0x00002000)
#define CETC52RAID_SAS_DEVICE_INFO_SSP_TARGET         (0x00000400)
#define CETC52RAID_SAS_DEVICE_INFO_STP_TARGET         (0x00000200)
#define CETC52RAID_SAS_DEVICE_INFO_SMP_TARGET         (0x00000100)
#define CETC52RAID_SAS_DEVICE_INFO_SATA_DEVICE        (0x00000080)
#define CETC52RAID_SAS_DEVICE_INFO_SSP_INITIATOR      (0x00000040)
#define CETC52RAID_SAS_DEVICE_INFO_STP_INITIATOR      (0x00000020)
#define CETC52RAID_SAS_DEVICE_INFO_SMP_INITIATOR      (0x00000010)
#define CETC52RAID_SAS_DEVICE_INFO_SATA_HOST          (0x00000008)
#define CETC52RAID_SAS_DEVICE_INFO_MASK_DEVICE_TYPE   (0x00000007)
#define CETC52RAID_SAS_DEVICE_INFO_NO_DEVICE          (0x00000000)
#define CETC52RAID_SAS_DEVICE_INFO_END_DEVICE         (0x00000001)
#define CETC52RAID_SAS_DEVICE_INFO_EDGE_EXPANDER      (0x00000002)
#define CETC52RAID_SAS_DEVICE_INFO_FANOUT_EXPANDER    (0x00000003)

struct Cetc52raidSmpPassthroughReq_t {
	U8 PassthroughFlags;
	U8 PhysicalPort;
	U8 ChainOffset;
	U8 Function;
	U16 RequestDataLength;
	U8 SGLFlags;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U32 Reserved2;
	U64 SASAddress;
	U32 Reserved3;
	U32 Reserved4;
	union CETC52RAID_SIMPLE_SGE_UNION SGL;
};

struct Cetc52raidSmpPassthroughRep_t {
	U8 PassthroughFlags;
	U8 PhysicalPort;
	U8 MsgLength;
	U8 Function;
	U16 ResponseDataLength;
	U8 SGLFlags;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved1;
	U8 Reserved2;
	U8 SASStatus;
	U16 IOCStatus;
	U32 IOCLogInfo;
	U32 Reserved3;
	U8 ResponseData[4];
};

struct Cetc52raidSasIoUnitControlReq_t {
	U8 Operation;
	U8 Reserved1;
	U8 ChainOffset;
	U8 Function;
	U16 DevHandle;
	U8 IOCParameter;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U16 Reserved4;
	U8 PhyNum;
	U8 PrimFlags;
	U32 Primitive;
	U8 LookupMethod;
	U8 Reserved5;
	U16 SlotNumber;
	U64 LookupAddress;
	U32 IOCParameterValue;
	U32 Reserved7;
	U32 Reserved8;
};

#define CETC52RAID_SAS_OP_PHY_LINK_RESET              (0x06)
#define CETC52RAID_SAS_OP_PHY_HARD_RESET              (0x07)
#define CETC52RAID_SAS_OP_REMOVE_DEVICE               (0x0D)
struct Cetc52raidSasIoUnitControlRep_t {
	U8 Operation;
	U8 Reserved1;
	U8 MsgLength;
	U8 Function;
	U16 DevHandle;
	U8 IOCParameter;
	U8 MsgFlags;
	U8 VP_ID;
	U8 VF_ID;
	U16 Reserved3;
	U16 Reserved4;
	U16 IOCStatus;
	U32 IOCLogInfo;
};
#endif