#ifndef __GB_SN_INFO_H
#define __GB_SN_INFO_H
#include "mem_chip.h"
#include "param_def.h"

#define GB02MAC2090 0x80000

#define GB02MAC2092 0
#define SN_DEFAULT "XT10121020512401010001"

#ifdef MEM_CHIP_DBG
#define GB02MAC2096 (27+10)
#else
#define GB02MAC2096 (32)
#endif

#ifdef MEM_CHIP_DBG
typedef struct {
	enum MemChipVendor		vendor;
	enum MemChipBitWidth 	bit_width;
	enum MemChipChnl 		channel;
	enum MemChipDensity 	density;
	enum MemChipDies		dies;
}mem_chip_t;
#endif

typedef  struct {
	bool snvaild;
	uint8_t vendor;
	uint8_t factory;
	uint8_t series;
	uint8_t interface;
	uint8_t gpu_type;
	uint8_t gpu_batch;
	uint8_t bin_level;
	date_info_t date_time;
	uint16_t lot;
	char sn_code[GB02MAC2096];
	uint8_t check_passed;
} serial_number_info_t;

bool GB02FUNC1568(void);
int GB02FUNC1569(void);
int GB02FUNC1570(void);
int GB02FUNC1571(void);
int GB02FUNC1572(void);
int GB02FUNC1574(void);
int GB02FUNC1575(void);
int GB02FUNC1576(void);
date_info_t * gb_get_date_form_sn(void);
int GB02FUNC1577(void);
serial_number_info_t * gb_get_sn_info_t(void);

char* GB02FUNC1580(serial_number_info_t *p_info);
int GB02FUNC1586(char *sn, serial_number_info_t *p_info);
void GB02FUNC1584(serial_number_info_t *p_info);

void GB02FUNC1598(struct pci_dev *pdev);

#endif