/*
 * param_def.h
 *
 *  Created on: 2023年12月19日
 *      Author: gxich
 */

#ifndef GB02MAC2122
#define GB02MAC2122
#include "enum_macro.h"

#define VENDOR_(XX) \
	XX(VENDOR_SIETIUM, = 0x1)

#define FACTORY_(XX) \
	XX(FACTORY_SIETIUM_SHENZHEN, = 0x1) \
	XX(FACTORY_SIETIUM_XIAN, = 0x2) \
	XX(FACTORY_SIETIUM_YANTAI, = 0x3)

#define SERIES_(XX) \
	XX(SERIES_CHENQI1, = 0x1) \
	XX(SERIES_CHENQI2, = 0x2) \
	XX(SERIES_CHENQI3, = 0x3)

#define INTERFACE_(XX) \
	XX(INTERFACE_PCIE_HALF_L_HALF_H, = 0xA) \
	XX(INTERFACE_PCIE_HALF_L_FULL_H, = 0xB) \
	XX(INTERFACE_PCIE_FULL_L_HALF_H, = 0xC) \
	XX(INTERFACE_PCIE_FULL_L_FULL_H, = 0xD) \
	XX(INTERFACE_MXM_A, = 0x14) \
	XX(INTERFACE_MXM_B, = 0x15) \
	XX(INTERFACE_XMC, = 0x1D)

#define GPU_TYPE_(XX) \
	XX(GPU_TYPE_GENBU_01, =1) \
	XX(GPU_TYPE_GENBU_02, =2)

#define BIN_LEVEL_(XX) \
	XX(BIN_LEVEL_FULL_OK, =1) \
	XX(BIN_LEVEL_TPU_OK, =2) \
	XX(BIN_LEVEL_TPU_A, =3) \
	XX(BIN_LEVEL_TPU_B, =4) \
	XX(BIN_LEVEL_TPU_C, =5) \
	XX(BIN_LEVEL_TPU_D, =6)

DECL_ENUM(Vendor, VENDOR_)
DECL_ENUM(Factory, FACTORY_)
DECL_ENUM(Series, SERIES_)
DECL_ENUM(Interface, INTERFACE_)
DECL_ENUM(GpuType, GPU_TYPE_)
DECL_ENUM(BinLevel, BIN_LEVEL_)

typedef struct
{
	uint32_t year;
	uint8_t mon;
	uint8_t day;
	uint8_t week;
} xt_date_t;

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
}xt_time_t;

typedef struct {
	xt_date_t date;
	xt_time_t time;
}date_info_t;

#endif /* GB02MAC2122 */
