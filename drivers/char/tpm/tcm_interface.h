/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TCM_INTREFACE_H__
#define __TCM_INTREFACE_H__

#include <linux/types.h>

#define MAX_BUFFER_SIZE			2048
/*
 * Basic Data Types
 */
#ifndef BYTE
#define BYTE uint8_t
#endif
typedef uint8_t  BYTE;
typedef uint8_t TCM_BOOL;
typedef uint16_t UINT16;

#ifndef UINT32
#define UINT32 uint32_t
#endif
//typedef uint32_t UINT32;
//typedef uint64_t UINT64;

#ifndef UINT64
#define UINT64 uint64_t
#endif

#ifndef BOOL
#define BOOL BYTE
#endif
//typedef BYTE     BOOL;

/*
 * New
 */
#define TCM_NONCE_SIZE                 32
#define TCM_HASH_SIZE                  32
#define TCM_AUTHDATA_SIZE              32

#define TCM_U16_SIZE                   2
#define TCM_U32_SIZE                   4

//#define TCM_PARAMSIZE_OFFSET           TCM_U16_SIZE
#define TCM_RETURN_OFFSET	(TCM_U16_SIZE + TCM_U32_SIZE)
#define TCM_DATA_OFFSET	(TCM_RETURN_OFFSET + TCM_U32_SIZE)

/*
 * TCM Helper Data Types
 */
typedef uint8_t	TCM_AUTH_DATA_USAGE;
typedef uint8_t	TCM_PAYLOAD_TYPE;
typedef uint8_t	TCM_ENTITY_TYPE;
typedef uint16_t	TCM_TAG;
typedef uint16_t	TCM_STRUCTURE_TAG;
typedef uint16_t	TCM_KEY_USAGE;
typedef uint16_t	TCM_ENC_SCHEME;
typedef uint16_t	TCM_SIG_SCHEME;
typedef uint32_t	TCM_COMMAND_CODE;
typedef uint32_t	TCM_RESULT;
typedef uint32_t	TCM_AUTHHANDLE;
typedef uint32_t	TCM_KEY_HANDLE;
typedef uint32_t	TCM_HANDLE;
typedef uint32_t	TCM_KEY_FLAGS;
typedef uint32_t	TCM_ALGORITHM_ID;
typedef uint32_t	TCM_PCRINDEX;

/*
 * Structure Tags
 * are defined together with the dedicated structures.
 */

/*
 * TCM_AUTH_DATA_USAGE
 */
#define TCM_AUTH_NEVER	0x00
#define TCM_AUTH_ALWAYS	0x01
#define TCM_AUTH_PRIV_USE_ONLY	0x03

/*
 * TCM_PAYLOAD_TYPE
 * This specifies the type of payload in various messages.
 */
#define TCM_PT_SYM	0x00//�Գ���Կ����
#define TCM_PT_ASYM	0x01//�ǶԳ���Կ����
#define TCM_PT_BIND	0x02//������������
#define TCM_PT_SEAL	0x05//��װ��������
#define TCM_PT_SYM_MIGRATE	0x08//�Գ�Ǩ������
#define TCM_PT_ASYM_MIGRATE	0x09//�ǶԳ�Ǩ������
		/* 0x10 - 0xFF ����*/

/*
 * TCM_ENTITY_TYPE
 * This specifies the types of entity and ADIP encryption schemes
 * that are supported by the TCM.
 */
#define TCM_ET_KEYHANDLE	0x01//��Կ���
#define TCM_ET_OWNER	0x02//0x40000001 TCM������
#define TCM_ET_DATA	0x03//����
#define TCM_ET_SMK	0x04//0x40000000 SMK
#define TCM_ET_KEY	0x05//��Կ
#define TCM_ET_REVOKE	0x06//0x40000002	�ɳ�����Կ
#define TCM_ET_KEYXOR	0x10
#define TCM_ET_KEYSMS4	0x11
#define TCM_ET_NONE	0x12//��ȨЭ����ʵ�崴��
#define TCM_ET_AUTHDATA_ID	0x13
#define TCM_ET_AUTHDATA	0x14//��Ȩ����

/*
 * TCM_SESSION_TYPE
 */
#define TCM_ST_INVALID	0x00	//��Ч�Ự
#define TCM_ST_AP	0x01	//AP�Ự

/*
 * TCM_TAG,Command Tags
 * Indicate
 */
#define TCM_TAG_RQU_COMMAND	0x00C1
#define TCM_TAG_RQU_AUTH1_COMMAND	0x00C2
#define TCM_TAG_RQU_AUTH2_COMMAND	0x00C3
#define TCM_TAG_RSP_COMMAND	0x00C4
#define TCM_TAG_RSP_AUTH1_COMMAND	0x00C5
#define TCM_TAG_RSP_AUTH2_COMMAND	0x00C6

/*
 * TCM_STRUCTURE_TAG
 */
#define TCM_TAG_PERMANENT_DATA	0x0022
#define TCM_TAG_STANY_DATA	0x0024
#define TCM_TAG_SIGNINFO	0x0005
#define TCM_TAG_PCR_INFO	0x0006
#define TCM_TAG_STORED_DATA	0x0016
#define TCM_TAG_KEY	0x0015
#define TCM_TAG_QUOTE_INFO	0x0036

/*
 * TCM_KEY_USAGE
 */
#define TCM_ECCKEY_SIGNING	0x0010
#define TCM_ECCKEY_STORAGE	0x0011
#define TCM_ECCKEY_IDENTITY	0x0012
#define TCM_ECCKEY_BIND	0x0014
#define TCM_ECCKEY_MIGRATE	0x0016
#define TCM_ECCKEY_PEK	0x0017
#define TCM_SMS4KEY_STORAGE	0x0018
#define TCM_SMS4KEY_BIND	0x0019
#define TCM_SMS4KEY_MIGRATE	0x001A

/*
 * TCM_ENC_SCHEME
 */
#define TCM_ES_ECC	0x0006
#define TCM_ES_ECCNONE	0x0004
#define TCM_ES_SMS4_CBC	0x0008
#define TCM_ES_SMS4_ECB	0x000A

/*
 * TCM_SIG_SCHEME
 */


#define TCM_SS_ECCNONE	0x0001
#define TCM_SS_ECC	0x0005

// TCM_NV_ATTRIBUTES values
#define TCM_NV_PER_READ_STCLEAR  (1UL << 31)
#define TCM_NV_PER_AUTH_READ     (1UL << 18)
#define TCM_NV_PER_OWNER_READ    (1UL << 17)
#define TCM_NV_PER_PPREAD        (1UL << 16)
#define TCM_NV_PER_GLOBALLOCK    (1UL << 15)
#define TCM_NV_PER_WRITE_STCLEAR (1UL << 14)
#define TCM_NV_PER_WRITEDEFINE   (1UL << 13)
#define TCM_NV_PER_WRITEALL      (1UL << 12)
#define TCM_NV_PER_AUTHWRITE     (1UL << 2)
#define TCM_NV_PER_OWNERWRITE    (1UL << 1)
#define TCM_NV_PER_PPWRITE       (1UL << 0)
#define TCM_NV_INDEX_LOCK        0xFFFFFFFF

/*
 * TCM_COMMAND_CODE     Ordinals
 * The command ordinals provide
 * TCM_COMMAND_CODE
 */
#define TCM_PROTECTED_ORDINAL	0x00008000
#define TCM_ORD_APCreate	0x000080BF
#define TCM_ORD_APTerminate	0x000080C0
#define TCM_ORD_Startup	0x00008099
#define TCM_ORD_PhysicalEnable	0x0000806F
#define TCM_ORD_PhysicalSetDeactivated	0x00008072
#define TCM_ORD_ForceClear	0x0000805D
#define TCM_ORD_OwnerClear	0x0000805B
#define TCM_ORD_TakeOwnership	0x0000800D
#define TCM_ORD_GetCapability	0x00008065
#define TCM_ORD_ReadPubek	0x0000807C
#define TCM_ORD_CreateWrapKey	0x0000801F
#define TCM_ORD_LoadKey	0x000080EF
#define TCM_ORD_EvictKey	0x00008022
#define TCM_ORD_GetPubKey	0x00008021
#define TCM_ORD_SMS4Encrypt	0x000080C5
#define TCM_ORD_SMS4Decrypt	0x000080C6
//#define TCM_ORD_ECCEncrypt
#define TCM_ORD_ECCDecrypt	0x000080EE
#define TCM_ORD_Sign	0x0000803C
#define TCM_ORD_Extend	0x00008014
#define TCM_ORD_PcrRead	0x00008015
#define TCM_ORD_SCHStart	0x000080EA
#define TCM_ORD_SCHUpdate	0x000080EB
#define TCM_ORD_SCHComplete	0x000080EC
#define TCM_ORD_SCHCompleteExtend	0x000080ED
#define TCM_ORD_FlushSpecific	0x000080BA
#define TCM_ORD_PcrRead	0x00008015
#define TCM_ORD_Extend	0x00008014
#define TCM_ORD_GetRandom	0x00008046
#define TCM_ORD_PcrReset	0x000080C8

//wrong definitions below, cannot used in hardware tcm
//related functions may be not defined in tcm

//#define TCM_ORD_CreateWrapKey	31
//#define TCM_ORD_LoadKey	239
//#define TCM_ORD_EvictKey	230
#define TCM_ORD_GetKeyHandle	231
//#define TCM_ORD_GetPubKey	33
//#define TCM_ORD_WrapKey	189
//#define TCM_ORD_CertifyKey	50
//#define TCM_ORD_SMS4Encrypt	197
//#define TCM_ORD_SMS4Decrypt	198
//#define TCM_ORD_ECCDecrypt	238
#define TCM_ORD_ECCEncrypt	133
//#define TCM_ORD_Sign	60
#define TCM_ORD_Verify	134
//#define TCM_ORD_Extend	20
//#define TCM_ORD_PCRRead	21
//#define TCM_ORD_Quote	22
//#define TCM_ORD_PCR_Reset	200
//#define TCM_ORD_Seal	23
//#define TCM_ORD_Unseal	24
//#define TCM_ORD_SCHStart	234
//#define TCM_ORD_SCHUpdate	235
//#define TCM_ORD_SCHComplete	236
//#define TCM_ORD_SCHCompleteExtend	237
//#define TCM_ORD_GetRandom	70

// TCM_PROTOCOL_ID protocol id values
#define TCM_PID_OIAP	0x0001// The OIAP protocol.
#define TCM_PID_OSAP	0x0002// The OSAP protocol.
#define TCM_PID_ADIP	0x0003// The ADIP protocol.
#define TCM_PID_ADCP	0X0004// The ADCP protocol.
#define TCM_PID_OWNER	0X0005// The protocol for taking ownership of a TCM.
#define TCM_PID_DSAP	0x0006// The DSAP protocol
#define TCM_PID_TRANSPORT	0x0007// The transport protocol
#define TCM_PID_AP	0x0008

// TCM 1.1 Capability Tags
#define TCPA_CAP_VERSION  0x00000006

//TCM_CAPABILITY_AREA for TCM_GetCapability (TCM 1.2)
#define TCM_CAP_ORD  0x00000001
#define TCM_CAP_ALG  0x00000002
#define TCM_CAP_PID  0x00000003
#define TCM_CAP_FLAG  0x00000004
#define TCM_CAP_PROPERTY  0x00000005
#define TCM_CAP_VERSION  0x00000006
#define TCM_CAP_KEY_HANDLE  0x00000007
#define TCM_CAP_CHECK_LOADED  0x00000008
#define TCM_CAP_SYM_MODE  0x00000009
#define TCM_CAP_KEY_STATUS  0x0000000C
#define TCM_CAP_NV_LIST  0x0000000D
#define TCM_CAP_MFR  0x00000010
#define TCM_CAP_NV_INDEX  0x00000011
#define TCM_CAP_TRANS_ALG  0x00000012
#define TCM_CAP_HANDLE  0x00000014
#define TCM_CAP_TRANS_ES  0x00000015
#define TCM_CAP_AUTH_ENCRYPT  0x00000017
#define TCM_CAP_SELECT_SIZE  0x00000018
#define TCM_CAP_VERSION_VAL  0x0000001A

//TCM_CAP_PROPERTY Subcap Values for TCM_GetCapability
#define TCM_CAP_PROP_PCR  0x00000101//
#define TCM_CAP_PROP_DIR  0x00000102//
#define TCM_CAP_PROP_MANUFACTURER  0x00000103//
#define TCM_CAP_PROP_KEYS  0x00000104//
#define TCM_CAP_PROP_MIN_COUNTER  0x00000107//
#define TCM_CAP_PROP_AUTHSESS  0x0000010A//
#define TCM_CAP_PROP_TRANSESS  0x0000010B//
#define TCM_CAP_PROP_COUNTERS  0x0000010C//
#define TCM_CAP_PROP_MAX_AUTHSESS  0x0000010D//
#define TCM_CAP_PROP_MAX_TRANSESS  0x0000010E//
#define TCM_CAP_PROP_MAX_COUNTERS  0x0000010F//
#define TCM_CAP_PROP_MAX_KEYS  0x00000110//
#define TCM_CAP_PROP_OWNER  0x00000111//
#define TCM_CAP_PROP_CONTEXT  0x00000112//
#define TCM_CAP_PROP_MAX_CONTEXT  0x00000113 //
#define TCM_CAP_PROP_FAMILYROWS  0x00000114 //
#define TCM_CAP_PROP_TIS_TIMEOUT  0x00000115 //
#define TCM_CAP_PROP_STARTUP_EFFECT  0x00000116 //
#define TCM_CAP_PROP_DELEGATE_ROW  0x00000117 //
#define TCM_CAP_PROP_DAA_MAX  0x00000119 //
#define CAP_PROP_SESSION_DAA  0x0000011A //
#define TCM_CAP_PROP_CONTEXT_DIST  0x0000011B //
#define TCM_CAP_PROP_DAA_uintERRUPT  0x0000011C //
#define TCM_CAP_PROP_SESSIONS  0X0000011D //
#define TCM_CAP_PROP_MAX_SESSIONS  0x0000011E //
#define TCM_CAP_PROP_CMK_RESTRICTION  0x0000011F //
#define TCM_CAP_PROP_DURATION  0x00000120 //
#define TCM_CAP_PROP_ACTIVE_COUNTER  0x00000122 //
#define TCM_CAP_PROP_MAX_NV_AVAILABLE  0x00000123 //
#define TCM_CAP_PROP_INPUT_BUFFER  0x00000124 //

#define TCM_CAP_FLAG_PERMANENT  0x00000108
#define TCM_CAP_FLAG_VOLATILE  0x00000109

// TCM Resource types
#define TCM_RT_KEY  0x00000001
#define TCM_RT_AUTH  0x00000002
#define TCM_RT_HASH  0X00000003
#define TCM_RT_TRANS  0x00000004
#define TCM_RT_CONTEXT  0x00000005
#define TCM_RT_COUNTER  0x00000006
#define TCM_RT_DELEGATE  0x00000007
#define TCM_RT_DAA_TCM  0x00000008
#define TCM_RT_DAA_V0  0x00000009
#define TCM_RT_DAA_V1  0x0000000A

// TCM error code constant
#define TCM_BASE  0x0
#define TCM_SUCCESS TCM_BASE
#define TCM_E_AUTHFAIL  0x00000001
#define TCM_E_BADINDEX  0x00000002
#define TCM_E_BAD_PARAMETER  0x00000003
#define TCM_E_AUDITFAILURE  0x00000004
#define TCM_E_CLEAR_DISABLED  0x00000005
#define TCM_E_DEACTIVATED  0x00000006
#define TCM_E_DISABLED  0x00000007
#define TCM_E_DISABLED_CMD  0x00000008
#define TCM_E_FAIL  0x00000009
#define TCM_E_BAD_ORDINAL  0x0000000a
#define TCM_E_INSTALL_DISABLED  0x0000000b
#define TCM_E_INVALID_KEYHANDLE  0x0000000c
#define TCM_E_KEYNOTFOUND  0x0000000d
#define TCM_E_INAPPROPRIATE_ENC  0x0000000e
#define TCM_E_MIGRATEFAIL  0x0000000f
#define TCM_E_INVALID_PCR_INFO  0x00000010
#define TCM_E_NOSPACE  0x00000011
#define TCM_E_NOSRK  0x00000012
#define TCM_E_NOTSEALED_BLOB  0x00000013
#define TCM_E_OWNER_SET  0x00000014
#define TCM_E_RESOURCES  0x00000015
#define TCM_E_SHORTRANDOM  0x00000016
#define TCM_E_SIZE  0x00000017
#define TCM_E_WRONGPCRVAL  0x00000018
#define TCM_E_BAD_PARAM_SIZE  0x00000019
#define TCM_E_SHA_THREAD  0x0000001a
#define TCM_E_SHA_ERROR  0x0000001b
#define TCM_E_FAILEDSELFTEST  0x0000001c
#define TCM_E_AUTH2FAIL  0x0000001d
#define TCM_E_BADTAG  0x0000001e
#define TCM_E_IOERROR  0x0000001f
#define TCM_E_ENCRYPT_ERROR  0x00000020
#define TCM_E_DECRYPT_ERROR  0x00000021
#define TCM_E_INVALID_AUTHHANDLE  0x00000022
#define TCM_E_NO_ENDORSEMENT  0x00000023
#define TCM_E_INVALID_KEYUSAGE  0x00000024
#define TCM_E_WRONG_ENTITYTYPE  0x00000025
#define TCM_E_INVALID_POSTINIT  0x00000026
#define TCM_E_INAPPROPRIATE_SIG  0x00000027
#define TCM_E_BAD_KEY_PROPERTY  0x00000028
#define TCM_E_BAD_MIGRATION  0x00000029
#define TCM_E_BAD_SCHEME  0x0000002a
#define TCM_E_BAD_DATASIZE  0x0000002b
#define TCM_E_BAD_MODE  0x0000002c
#define TCM_E_BAD_PRESENCE  0x0000002d
#define TCM_E_BAD_VERSION  0x0000002e
#define TCM_E_NO_WRAP_TRANSPORT  0x0000002f
#define TCM_E_AUDITFAIL_UNSUCCESSFUL  0x00000030
#define TCM_E_AUDITFAIL_SUCCESSFUL  0x00000031
#define TCM_E_NOTRESETABLE  0x00000032
#define TCM_E_NOTLOCAL  0x00000033
#define TCM_E_BAD_TYPE  0x00000034
#define TCM_E_INVALID_RESOURCE  0x00000035
#define TCM_E_NOTFIPS  0x00000036
#define TCM_E_INVALID_FAMILY  0x00000037
#define TCM_E_NO_NV_PERMISSION  0x00000038
#define TCM_E_REQUIRES_SIGN  0x00000039
#define TCM_E_KEY_NOTSUPPORTED  0x0000003a
#define TCM_E_AUTH_CONFLICT  0x0000003b
#define TCM_E_AREA_LOCKED  0x0000003c
#define TCM_E_BAD_LOCALITY  0x0000003d
#define TCM_E_READ_ONLY  0x0000003e
#define TCM_E_PER_NOWRITE  0x0000003f
#define TCM_E_FAMILYCOUNT  0x00000040
#define TCM_E_WRITE_LOCKED  0x00000041
#define TCM_E_BAD_ATTRIBUTES  0x00000042
#define TCM_E_INVALID_STRUCTURE  0x00000043
#define TCM_E_KEY_OWNER_CONTROL  0x00000044
#define TCM_E_BAD_COUNTER  0x00000045
#define TCM_E_NOT_FULLWRITE  0x00000046
#define TCM_E_CONTEXT_GAP  0x00000047
#define TCM_E_MAXNVWRITES  0x00000048
#define TCM_E_NOOPERATOR  0x00000049
#define TCM_E_RESOURCEMISSING  0x0000004a
#define TCM_E_DELEGATE_LOCK  0x0000004b
#define TCM_E_DELEGATE_FAMILY  0x0000004c
#define TCM_E_DELEGATE_ADMIN  0x0000004d
#define TCM_E_TRANSPORT_NOTEXCLUSIVE  0x0000004e
#define TCM_E_OWNER_CONTROL  0x0000004f
#define TCM_E_DAA_RESOURCES  0x00000050
#define TCM_E_DAA_INPUT_DATA0  0x00000051
#define TCM_E_DAA_INPUT_DATA1  0x00000052
#define TCM_E_DAA_ISSUER_SETTINGS  0x00000053
#define TCM_E_DAA_TCM_SETTINGS  0x00000054
#define TCM_E_DAA_STAGE  0x00000055
#define TCM_E_DAA_ISSUER_VALIDITY  0x00000056
#define TCM_E_DAA_WRONG_W  0x00000057
#define TCM_E_BAD_HANDLE  0x00000058
#define TCM_E_BAD_DELEGATE  0x00000059
#define TCM_E_BADCONTEXT  0x0000005a
#define TCM_E_TOOMANYCONTEXTS  0x0000005b
#define TCM_E_MA_TICKET_SIGNATURE  0x0000005c
#define TCM_E_MA_DESTINATION  0x0000005d
#define TCM_E_MA_SOURCE  0x0000005e
#define TCM_E_MA_AUTHORITY  0x0000005f
#define TCM_E_PERMANENTEK  0x00000061
#define TCM_E_BAD_SIGNATURE  0x00000062
#define TCM_E_NOCONTEXTSPACE  0x00000063
#define TDDL_E_ALREADY_OPENED  0x00000081
#define TDDL_E_ALREADY_CLOSED  0x00000082
#define TDDL_E_INSUFFICIENT_BUFFER  0x00000083
#define TDDL_E_COMMAND_COMPLETED  0x00000084
#define TDDL_E_COMMAND_ABORTED  0x00000085
#define TDDL_E_IOERROR  0x00000087
#define TDDL_E_BADTAG  0x00000088
#define TDDL_E_COMPONENT_NOT_FOUND  0x00000089
#define TCM_E_RETRY  0x00000800
#define TCM_E_NEEDS_SELFTEST  0x00000801
#define TCM_E_DOING_SELFTEST  0x00000802
#define TCM_E_DEFEND_LOCK_RUNNING  0x00000803


#define TCM_NON_FATAL  0x00000800

/*
 * TCM_KEY_HANDLE Reserved Key Handles
 * T
 */
#define TCM_KH_SMK	0x40000000
#define TCM_KH_OWNER	0x40000001
#define TCM_KH_REVOKE	0x40000002
#define TCM_KH_TRANSPORT	0x40000003
#define TCM_KH_OPERATOR	0x40000004
#define TCM_KH_EK	0x40000006

/*
 * TCM_KEY_FLAGS
 */
#define TCM_KEY_FLAG_MIGRATABLE	0x00000002
#define TCM_KEY_FLAG_VOLATILE	0x00000004
#define TCM_KEY_FLAG_PCR_IGNORE	0x00000008

/*
 * TCM_ALGORITHM_ID
 * This table defines the types
 */
#define TCM_ALG_KDF	0x00000007
#define TCM_ALG_XOR	0x0000000A
#define TCM_ALG_ECC	0x0000000B
#define TCM_ALG_SMS4	0x0000000C
#define TCM_ALG_SCH	0x0000000D
#define TCM_ALG_HMAC	0x0000000E



#define TCM_MEMORY_ALIGNMENT_MANDATORY 1



#define ERR_MASK 0x80000000
#define ERR_DUMMY 0x80001000
#define ERR_HMAC_FAIL 0x80001001
#define ERR_NULL_ARG 0x80001002
#define ERR_BAD_ARG 0x80001003
#define ERR_CRYPT_ERR 0x80001004
#define ERR_IO 0x80001005
#define ERR_MEM_ERR 0x80001006
#define ERR_BAD_FILE 0x80001007
#define ERR_BAD_DATA 0x80001008
#define ERR_BAD_SIZE 0x80001009
#define ERR_BUFFER 0x8000100a
#define ERR_STRUCTURE 0x8000100b
#define ERR_NOT_FOUND 0x8000100c
#define ERR_ENV_VARIABLE 0x8000100d
#define ERR_NO_TRANSPORT 0x8000100e
#define ERR_BADRESPONSETAG 0x8000100f
#define ERR_SIGNATURE 0x80001010
#define ERR_PCR_LIST_NOT_IMA 0x80001011
#define ERR_CHECKSUM 0x80001012
#define ERR_BAD_RESP 0x80001013
#define ERR_BAD_SESSION_TYPE 0x80001014
#define ERR_BAD_FILE_CLOSE 0x80001015
#define ERR_BAD_FILE_WRITE 0x80001016
#define ERR_BAD_FILE_READ 0x80001017

#define ERR_LAST 0x80001018

#define TCM_MAX_BUFF_SIZE 4096
#define TCM_HASH_SIZE 32
#define TCM_NONCE_SIZE 32

#define TCM_U16_SIZE 2
#define TCM_U32_SIZE 4

#define TCM_PARAMSIZE_OFFSET TCM_U16_SIZE
#define TCM_RETURN_OFFSET (TCM_U16_SIZE + TCM_U32_SIZE)
#define TCM_DATA_OFFSET (TCM_RETURN_OFFSET + TCM_U32_SIZE)

/*
 * Number of PCRs of the TCM (must be a multiple of eight)
 */
#define TCM_NUM_PCR 24

/*
 * TCM_PCR_SELECTION
 * Provides a standard method
 * Note: An error is reported
 */
struct TCM_PCR_SELECTION {
	UINT16 sizeOfSelect;
	BYTE pcrSelect[TCM_NUM_PCR/8];
};
#define sizeof_TCM_PCR_SELECTION(s) (2 + s.sizeOfSelect)


struct tcm_buffer {
	uint32_t size;
	uint32_t used;
	uint32_t flags;
	unsigned char *buffer;
};

enum {
	BUFFER_FLAG_ON_STACK = 1,
};

#define STORE32(buffer, offset, value)	store32(buffer, offset, value)
#define STORE16(buffer, offset, value)	store16(buffer, offset, value)

#define STORE32N(buffer, offset, value)	memcpy(&buffer[offset], &value, 4)
#define STORE16N(buffer, offset, value)	memcpy(&buffer[offset], &value, 2)

#define LOAD32(buffer, offset)	load32(buffer, offset)
#define LOAD16(buffer, offset)	load16(buffer, offset)
#define LOAD32N(buffer, offset)	load32N(buffer, offset)
#define LOAD16N(buffer, offset)	load16N(buffer, offset)

static inline void store32(unsigned char *const buffer,
		int offset, uint32_t value)
{
	int i;

	for (i = 3; i >= 0; i--) {
		buffer[offset+i] = (value & 0xff);
		value >>= 8;
	}
}


static inline void store16(unsigned char *const buffer,
		int offset, uint16_t value)
{
	int i;

	for (i = 1; i >= 0; i--) {
		buffer[offset+i] = (value & 0xff);
		value >>= 8;
	}
}

static inline uint32_t load32(const unsigned char *buffer, int offset)
{
	int i;
	uint32_t res = 0;

	for (i = 0; i <= 3; i++) {
		res <<= 8;
		res |= buffer[offset+i];
	}
	return res;
}


static inline uint16_t load16(const unsigned char *buffer, int offset)
{
	int i;
	uint16_t res = 0;

	for (i = 0; i <= 1; i++) {
		res <<= 8;
		res |= buffer[offset+i];
	}
	return res;
}


static inline uint32_t load32N(const unsigned char *buffer, int offset)
{
	uint32_t res;

	memcpy(&res, &buffer[offset], sizeof(res));
	return res;
}

static inline uint16_t load16N(const unsigned char *buffer, int offset)
{
	uint16_t res;

	memcpy(&res, &buffer[offset], sizeof(res));
	return res;
}

#define TCM_CURRENT_TICKS_SIZE \
	(sizeof(TCM_STRUCTURE_TAG)+2*TCM_U32_SIZE+TCM_U16_SIZE+TCM_NONCE_SIZE)



uint32_t tcm_get_random(int line, UINT32 len, BYTE *data);
int tcm_hash_sm3(int line, uint8_t *in_data, uint32_t indata_len,
		uint8_t *out_data, uint32_t *out_datalen);
int tcm_pcr_read(int line, UINT32 PcrIndex, BYTE *PcrValue);
int tcm_pcr_extend(int line, UINT32 PcrIndex, BYTE *PcrValue);
int tcm_pcr_reset(int line, UINT32 PcrIndex);

#endif
