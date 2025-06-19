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

#include <linux/delay.h>
#ifndef _GENBU_REGMAP_H_
#define _GENBU_REGMAP_H_


// Vertices process shader: CPU --> CP --> TPU.computer --> PTU --> DDR  Action: primitive tiling     TPU meaning shader core  PTU meaning tiler core
// fragment process shader: CPU --> CP --> TPU.fragment --> DDR		 Action: rasterizer
// Stage slot change to Job slot

/*
 * Begin Register Offsets
 */

#define GB02MAC1336        0x0000
#define GB02MAC1339(r)      (GB02MAC1336 + (r))

// GPU IRQ regs
#define GB02MAC1343                  0x020	/* (RO) GPU and revision identifier */
#define L2_FEATURES             0x024	/* (RO) Level 2 cache features */
#define GB02MAC1347            0x008   /* (RO) Fixed-function suspend buffer */
// 0x010 --- 0x01C	GPU reserved

// GPU Config&Control regs
#define PTU_FEATURES			0x02C	/* (RO) PTU (primitive tiling unit, tiler core)Polygon list feature */
#define MEM_FEATURES			0x03C	/* (RO) GPU Memory system feature */
#define MMU_FEATURES			0x040	/* (RO) MMU feature */
#define AS_PRESENT				0x044	/* (RO) Address space slots present */
#define GB02MAC1358				0x048	/* (RO) Stage slots present */

//GB2 add features, just reserved now, not be accomplished yet
#define GB02MAC1361			0x028	/* (RO) Power manager feature */
#define GB02MAC1364			0x030	/* (RO) Rasterizer feature */
#define GB02MAC1367			0x034	/* (RO) GPU Vram feature */
#define TPA_FEATURES			0x038	/* (RO) TPU (tile processing unit, shader core) array */
#define GB02MAC1371	0x050	/* (WO) GPU command feature */

#define GB02MAC1373         0x000	/* (RO) GPU IRQ Raw status */
#define GB02MAC1375           0x004	/* (WO) GPU IRQ clear */
#define GB02MAC1376            0x008	/* (RW) GPU IRQ mask */
#define GB02MAC1377          0x00C	/* (RO) GPU IRQ Raw & mask */


//Rename from TILER core to PTU
//Rename from SHADER core to TPU
//Rename from SS to JS
#define TILER_FEATURES			PTU_FEATURES
#define SS_PRESENT				GB02MAC1358

/* GB02MAC1373
 * GB02MAC1375
 * GB02MAC1376
 * GB02MAC1377 bitmap */
#define GB02MAC1380               (1 << 0)	/* A GPU Fault has occurred */
#define GB02MAC1381     (1 << 7)	/* More than one GPU Fault occurred. */
#define GB02MAC1383         (1 << 8)	/* Set when a reset has completed. Intended to use with SOFT_RESET
						   commands which may take time. */
#define GB02MAC1385    (1 << 9)	/* Set when a single core has finished powering up or down. */
#define GB02MAC1386       (1 << 10)	/* Set when all cores have finished powering up or down
						   and the power manager is idle. */

#define GB02MAC1387 (1 << 16)	/* Set when a performance count sample has completed. */
#define GB02MAC1388  (1 << 17)	/* Set when a cache clean operation has completed. */

#define GB02MAC1390 (GB02MAC1380 | GB02MAC1381 | GB02MAC1383 \
			| GB02MAC1386 | GB02MAC1387)


// Command excute status
#define GB02MAC1371	0x050	/* (WO) GPU command feature */
#define GB02MAC1392		0x054	/* (WO) GPU control command */
#define GB02MAC1393		0x058	/* (RO) GPU status */
#define GB02MAC1394	0x05C	/* (RO) Latest flush ID */

#define GB02MAC1395      (1 << 0)	/* Cores groups are l2 coherent */
#define GB02MAC1396               (1 << 8)	/* DBGEN wire status */
#define GB02MAC1398	0x060	/* (RO) GPU Fault status */
#define GB02MAC1399	0x064	/* (RO) GPU Fault address low 32bit*/
#define GB02MAC1401	0x068	/* (RO) GPU Fault address high 32bit*/

// total 8, GB02 used 1, others reseved
#define GB02MAC1402(c)	(0x01C * (c))
//#define GPU_COMMAND_FEATURES(c)	(GB02MAC1371 + GB02MAC1402(c))
//#define GB02MAC1403(c)		(GB02MAC1392 + GB02MAC1402(c))
//#define GB02MAC1404(c)		(GB02MAC1393 + GB02MAC1402(c))
//#define GPU_LATEST_FLUSH(c)	(GB02MAC1394 + GB02MAC1402(c))
//#define GB02MAC1406(c)	(GB02MAC1398 + GB02MAC1402(c))
//#define GB02MAC1407(c)	(GB02MAC1399 + GB02MAC1402(c))
//#define GB02MAC1408(c)	(GB02MAC1401 + GB02MAC1402(c))

#define GB02MAC1403		(GB02MAC1392 + GB02MAC1402(0))
#define GB02MAC1404		(GB02MAC1393 + GB02MAC1402(0))
#define GB02MAC1405		(GB02MAC1394 + GB02MAC1402(0))
#define GB02MAC1406		(GB02MAC1398 + GB02MAC1402(0))
#define GB02MAC1407	(GB02MAC1399 + GB02MAC1402(0))
#define GB02MAC1408	(GB02MAC1401 + GB02MAC1402(0))
// 0x130 --- 0x1FC 	GPU reserved

// Power manager
#define GB02MAC1409                 0x200	/* (WO) Power manager key register */
#define GB02MAC1410           0x204	/* (RW) Power manager override settings */
#define GB02MAC1411           0x208	/* (RW) Power manager override settings */


// Performance counter
#define PRFCNT_BASE_LO          0x220	/* (RW) Performance counter memory region base address, low word */
#define PRFCNT_BASE_HI          0x224	/* (RW) Performance counter memory region base address, high word */
#define GB02MAC1412           0x228	/* (RW) Performance counter configuration */
#define GB02MAC1413            0x22C	/* (RW) Performance counter enable flags for Stage Manager */
#define GB02MAC1414        	0x230	/* (RW) Performance counter enable flags for shader cores */
#define GB02MAC1415         	0x234	/* (RW) Performance counter enable flags for tiler */
#define PRFCNT_MMU_L2_EN_OLD    0x238	/* old l2 en, now no used */
#define GB02MAC1416        0x23C	/* (RW) Performance counter enable flags for MMU/L2 cache */
#define GB02MAC1417		0x18
#define GB02MAC1418		(800 << 20) /* GB2 GPU is 800MHz */

#define GB02MAC1421	GB02MAC1414
#define GB02MAC1423		GB02MAC1415

// Other regs
#define CYCLE_COUNT_LO          0x250	/* (RO) Cycle counter, low word */
#define CYCLE_COUNT_HI          0x254	/* (RO) Cycle counter, high word */
#define TIMESTAMP_LO            0x258	/* (RO) Global time stamp counter, low word */
#define TIMESTAMP_HI            0x25C	/* (RO) Global time stamp counter, high word */

#define GB02MAC1426		0x260	/* (RO) Maximum number of threads per core */
#define GB02MAC1427 0x264	/* (RO) Maximum workgroup size */
#define GB02MAC1429 0x268	/* (RO) Maximum threads waiting at a barrier */
#define THREAD_FEATURES         0x26C	/* (RO) Thread features */
#define GB02MAC1430      0x270	/* (RO) Support flags for indexed texture formats 0..31 */
#define GB02MAC1431      0x274	/* (RO) Support flags for indexed texture formats 32..63 */
#define GB02MAC1432      0x278	/* (RO) Support flags for indexed texture formats 64..95 */
#define GB02MAC1433(n) GB02MAC1339(GB02MAC1430 + ((n) << 2))

// total 32 jobJs0 --- > Js2 used, other reserved
#define GB02MAC1434		0x300	/* (RW) Slot support job type */
#define GB02MAC1435(slot)	(GB02MAC1434 + ((slot) << 2))
// 0x360 --- 0x3FC	GPU reserved

// User Define Regs
#define GB02MAC1436		0x400	/* (RO) Use for Read */
#define GB02MAC1437		0x4BC
#define GB02MAC1438(c)		(GB02MAC1436 + ((c) << 2))
// 0x4BC --- 0x4FC	GPU reserved

#define GB02MAC1439		0x500	/* (RW) Use for Write */
#define GB02MAC1440		0x5BC
#define GB02MAC1441(c)		(GB02MAC1439 + ((c) << 2))
// 0x5BC --- 0x5FC	GPU reserved

// Reserved
// 0x600 --- 0x7FC	GPU reserved


// Power Manager
#define GB02MAC1442		0x800
#define GB02MAC1443		0x804
#define GB02MAC1444		0x810
#define GB02MAC1445		0x814
#define GB02MAC1446		0x820
#define GB02MAC1448		0x824

#define GB02MAC1449		0x980
#define GB02MAC1450		0x984
#define GB02MAC1451		0x840
#define GB02MAC1452		0x844
#define GB02MAC1453		0x850
#define GB02MAC1454		0x854
#define GB02MAC1455			0x860
#define GB02MAC1456			0x864

#define GB02MAC1457		0x990
#define GB02MAC1458		0x994
#define GB02MAC1459		0x880
#define GB02MAC1460		0x884
#define GB02MAC1461		0x890
#define GB02MAC1462		0x894
#define GB02MAC1463			0x8A0
#define GB02MAC1464			0x8A4

#define GB02MAC1465		0x9A0
#define GB02MAC1466		0x9A4
#define GB02MAC1467		0x8C0
#define GB02MAC1468		0x8C4
#define GB02MAC1469		0x8D0
#define GB02MAC1470		0x8D4
#define GB02MAC1471		0x8E0
#define GB02MAC1472		0x8E4

#define GB02MAC1473		0x9B0
#define GB02MAC1474		0x9B4
#define GB02MAC1475		0x900
#define GB02MAC1476		0x904
#define GB02MAC1477		0x910
#define GB02MAC1478		0x914
#define GB02MAC1479		0x920
#define GB02MAC1480		0x924

#define GB02MAC1481		0x9C0
#define GB02MAC1482		0x9C4
#define GB02MAC1483	0x940
#define GB02MAC1484	0x944
#define GB02MAC1485	0x950
#define GB02MAC1486	0x954
#define GB02MAC1487		0x960
#define GB02MAC1488		0x964

#define COHERENCY_FEATURES  0x2C0	/* (RO) Coherency features present */
#define GB02MAC1489    0x2C4	/* (RW) Coherency enable */

#define GB02MAC1490		0x280	/* (RW) reserved */
#define GB02MAC1491		0x284	/* (RW) reserved */
#define GB02MAC1492		0x288	/* (RW) reserved */
#define GB02MAC1493		0x28C	/* (RW) limited AXI request from GPU */
#define GB02MAC1494		0x290	/* (RW) reserved */
#define GB02MAC1495		0x294	/* (RW) reserved */
#define REVIDR			0x298	/* (RW) reserved */

#define GB02MAC1496        0x2000
#define GB02MAC1497(r)      (GB02MAC1496 + (r))

#define GB02MAC1498         (GB02MAC1496 + 0x000) /* Raw interrupt status register */
#define GB02MAC1499           (GB02MAC1496 + 0x004) /* Interrupt clear register */
#define GB02MAC1500            (GB02MAC1496 + 0x008) /* Interrupt mask register */
#define GB02MAC1501          (GB02MAC1496 + 0x00C) /* Interrupt status register */
#define STAGE_IRQ_SS_STATE        (GB02MAC1496 + 0x010) /* status==active and _next == busy snapshot from last GB02MAC1499 */
#define STAGE_IRQ_THROTTLE        (GB02MAC1496 + 0x014) /* cycles to delay delivering an interrupt externally. The GB02MAC1501 is NOT affected by this, just the delivery of the interrupt.  */


// Slot0 ---> Slot2 Used, other reseved
#define GB02MAC1502               0x400 /* Configuration registers for stage slot 0 */
#define GB02MAC1503               0x480 /* Configuration registers for stage slot 1 */
#define GB02MAC1504               0x500 /* Configuration registers for stage slot 2 */

#define STAGE_SLOT_REG(n, r)      (GB02MAC1497(GB02MAC1502 + ((n) << 7)) + (r))
#define GB02MAC1505                         (GB02MAC1496 + GB02MAC1502)
#define GB02MAC1506(n)                   (GB02MAC1505 + ((n) * 0x80) + 0x00)
#define GB02MAC1507(n)                   (GB02MAC1505 + ((n) * 0x80) + 0x04)
#define GB02MAC1508(n)                   (GB02MAC1505 + ((n) * 0x80) + 0x08)
#define GB02MAC1509(n)                   (GB02MAC1505 + ((n) * 0x80) + 0x0c)
#define GB02MAC1510(n)               (GB02MAC1505 + ((n) * 0x80) + 0x10)
#define GB02MAC1511(n)               (GB02MAC1505 + ((n) * 0x80) + 0x14)
#define GB02MAC1512(n)                    (GB02MAC1505 + ((n) * 0x80) + 0x18)
#define GB02MAC1513(n)                 (GB02MAC1505 + ((n) * 0x80) + 0x1c)
#define GB02MAC1514(n)                   (GB02MAC1505 + ((n) * 0x80) + 0x20)
#define GB02MAC1515(n)                    (GB02MAC1505 + ((n) * 0x80) + 0x24)
#define GB02MAC1516(n)              (GB02MAC1505 + ((n) * 0x80) + 0x40)
#define GB02MAC1517(n)              (GB02MAC1505 + ((n) * 0x80) + 0x44)
#define GB02MAC1518(n)          (GB02MAC1505 + ((n) * 0x80) + 0x50)
#define GB02MAC1519(n)          (GB02MAC1505 + ((n) * 0x80) + 0x54)
#define GB02MAC1520(n)               (GB02MAC1505 + ((n) * 0x80) + 0x58)
#define GB02MAC1522(n)              (GB02MAC1505 + ((n) * 0x80) + 0x60)
#define GB02MAC1524(n)             (GB02MAC1505 + ((n) * 0x80) + 0x70)

#define GB02MAC1526        0x7C

/* End Register Offsets */




// Debug function Reg
#define GB02MAC1530		0xFE0
#define GB02MAC1532		0xFE4

/* End Register Offsets */



#define GB02MAC1536  0x1000
#define GB02MAC1538(r)              (GB02MAC1536 + (r))

#define GB02MAC1540         (GB02MAC1536 + 0x000)   /* (RW) Raw interrupt status register */
#define GB02MAC1542           (GB02MAC1536 + 0x004)   /* (WO) Interrupt clear register */
#define GB02MAC1544            (GB02MAC1536 + 0x008)   /* (RW) Interrupt mask register */
#define GB02MAC1546          (GB02MAC1536 + 0x00C)   /* (RO) Interrupt status register */

// AS0 --> AS7 Used, other reseved
#define GB02MAC1549                 0x800   /* Configuration registers for address space 0 */
#define GB02MAC1551                 0x840   /* Configuration registers for address space 1 */
#define GB02MAC1553                 0x880   /* Configuration registers for address space 2 */
#define GB02MAC1555                 0x8C0   /* Configuration registers for address space 3 */
#define GB02MAC1556                 0x900   /* Configuration registers for address space 4 */
#define GB02MAC1558                 0x940   /* Configuration registers for address space 5 */
#define GB02MAC1559                 0x980   /* Configuration registers for address space 6 */
#define GB02MAC1560                 0x9C0   /* Configuration registers for address space 7 */
#define GB02MAC1561                 0xA00   /* Configuration registers for address space 8 */
#define GB02MAC1562                 0xA40   /* Configuration registers for address space 9 */
#define GB02MAC1563                0xA80   /* Configuration registers for address space 10 */
#define GB02MAC1565                0xAC0   /* Configuration registers for address space 11 */
#define GB02MAC1567                0xB00   /* Configuration registers for address space 12 */
#define GB02MAC1569                0xB40   /* Configuration registers for address space 13 */
#define GB02MAC1571                0xB80   /* Configuration registers for address space 14 */
#define GB02MAC1573                0xBC0   /* Configuration registers for address space 15 */

#define MMU_AS_REG(n, r)        (GB02MAC1538(GB02MAC1549 + ((n) << 6)) + (r))
#define GB02MAC1576(as)                      (GB02MAC1538(GB02MAC1549) + ((as) << 6))
#define GB02MAC2809(as)              (GB02MAC1576(as) + 0x00) /* (RW) Translation Table Base Address for address space n, low word */
#define GB02MAC2810(as)              (GB02MAC1576(as) + 0x04) /* (RW) Translation Table Base Address for address space n, high word */
#define GB02MAC2811(as)               (GB02MAC1576(as) + 0x08) /* (RW) Memory attributes for address space n, low word. */
#define GB02MAC2812(as)               (GB02MAC1576(as) + 0x0C) /* (RW) Memory attributes for address space n, high word. */
#define GB02MAC2813(as)              (GB02MAC1576(as) + 0x10) /* (RW) Lock region address for address space n, low word */
#define GB02MAC2814(as)              (GB02MAC1576(as) + 0x14) /* (RW) Lock region address for address space n, high word */
#define GB02MAC1580(as)                  (GB02MAC1576(as) + 0x18) /* (WO) MMU command register for address space n */
#define GB02MAC1582(as)              (GB02MAC1576(as) + 0x1C) /* (RO) MMU fault status register for address space n */
#define GB02MAC2815(as)          (GB02MAC1576(as) + 0x20) /* (RO) Fault Address for address space n, low word */
#define GB02MAC2816(as)          (GB02MAC1576(as) + 0x24) /* (RO) Fault Address for address space n, high word */
#define GB02MAC1585(as)                   (GB02MAC1576(as) + 0x28) /* (RO) Status flags for address space n */
#define GB02MAC2817(as)              (GB02MAC1576(as) + 0x30) /* (RW) Translation table configuration for address space n, low word */
#define GB02MAC2818(as)              (GB02MAC1576(as) + 0x34) /* (RW) Translation table configuration for address space n, high word */
#define GB02MAC2819(as)            (GB02MAC1576(as) + 0x38) /* (RO) Secondary fault address for address space n, low word */
#define GB02MAC2820(as)            (GB02MAC1576(as) + 0x3C) /* (RO) Secondary fault address for address space n, high word */


/* End Register Offsets */

#define GB02MAC1589            GB02MAC1434	/* (RO) Features of stage slot 0 */
#define GB02MAC1591(n)      GB02MAC1339(GB02MAC1589 + ((n) << 2))

#define SHADER_PRESENT_LO       GB02MAC1442	/* (RO) Shader core present bitmap, low word */
#define SHADER_PRESENT_HI       GB02MAC1443	/* (RO) Shader core present bitmap, high word */
#define TILER_PRESENT_LO        GB02MAC1444	/* (RO) Tiler core present bitmap, low word */
#define TILER_PRESENT_HI        GB02MAC1445	/* (RO) Tiler core present bitmap, high word */
#define STACK_PRESENT_LO        GB02MAC1449   /* (RO) Core stack present bitmap, low word */
#define STACK_PRESENT_HI        GB02MAC1450   /* (RO) Core stack present bitmap, high word */

#define SHADER_READY_LO         GB02MAC1451	/* (RO) Shader core ready bitmap, low word */
#define SHADER_READY_HI         GB02MAC1452	/* (RO) Shader core ready bitmap, high word */
#define TILER_READY_LO          GB02MAC1453	/* (RO) Tiler core ready bitmap, low word */
#define TILER_READY_HI          GB02MAC1454	/* (RO) Tiler core ready bitmap, high word */
#define STACK_READY_LO          GB02MAC1457   /* (RO) Core stack ready bitmap, low word */
#define STACK_READY_HI          GB02MAC1458   /* (RO) Core stack ready bitmap, high word */

#define SHADER_PWRON_LO         GB02MAC1459	/* (WO) Shader core power on bitmap, low word */
#define SHADER_PWRON_HI         GB02MAC1460	/* (WO) Shader core power on bitmap, high word */
#define TILER_PWRON_LO          GB02MAC1461	/* (WO) Tiler core power on bitmap, low word */
#define TILER_PWRON_HI          GB02MAC1462	/* (WO) Tiler core power on bitmap, high word */
#define STACK_PWRON_LO          GB02MAC1465   /* (RO) Core stack power on bitmap, low word */
#define STACK_PWRON_HI          GB02MAC1466   /* (RO) Core stack power on bitmap, high word */

#define SHADER_PWROFF_LO        GB02MAC1467	/* (WO) Shader core power off bitmap, low word */
#define SHADER_PWROFF_HI        GB02MAC1468	/* (WO) Shader core power off bitmap, high word */
#define TILER_PWROFF_LO         GB02MAC1469	/* (WO) Tiler core power off bitmap, low word */
#define TILER_PWROFF_HI         GB02MAC1469	/* (WO) Tiler core power off bitmap, high word */
#define STACK_PWROFF_LO         GB02MAC1473   /* (RO) Core stack power off bitmap, low word */
#define STACK_PRWOFF_HI         GB02MAC1473   /* (RO) Core stack power off bitmap, high word */

#define SHADER_PWRTRANS_LO      GB02MAC1475 	/* (RO) Shader core power transition bitmap, low word */
#define SHADER_PWRTRANS_HI      GB02MAC1476	/* (RO) Shader core power transition bitmap, high word */
#define TILER_PWRTRANS_LO       GB02MAC1477	/* (RO) Tiler core power transition bitmap, low word */
#define TILER_PWRTRANS_HI       GB02MAC1478	/* (RO) Tiler core power transition bitmap, high word */
#define STACK_PWRTRANS_LO       GB02MAC1481   /* (RO) Core stack power transition bitmap, low word */
#define STACK_PRWTRANS_HI       GB02MAC1482   /* (RO) Core stack power transition bitmap, high word */

#define SHADER_PWRACTIVE_LO     GB02MAC1483	/* (RO) Shader core active bitmap, low word */
#define SHADER_PWRACTIVE_HI     GB02MAC1484	/* (RO) Shader core active bitmap, high word */
#define TILER_PWRACTIVE_LO      GB02MAC1485	/* (RO) Tiler core active bitmap, low word */
#define TILER_PWRACTIVE_HI      GB02MAC1486	/* (RO) Tiler core active bitmap, high word */

#define GB02MAC1597               GB02MAC1490   /* (RW) Stage Manager configuration register (Implementation specific register) */
#define GB02MAC1598           GB02MAC1491	/* (RW) Shader core configuration settings (Implementation specific register) */
#define GB02MAC1599            GB02MAC1492   /* (RW) Tiler core configuration settings (Implementation specific register) */
#define GB02MAC1600           GB02MAC1493	/* (RW) Configuration of the L2 cache and MMU (Implementation specific register) */

/*
 * GB02MAC1540 register values. Values are valid also for
   GB02MAC1542, GB02MAC1544, GB02MAC1546 registers.
 */

#define GB02MAC1603   16

/* Macros returning a bitmask to retrieve page fault or bus error flags from
 * MMU registers */
#define GB02MAC1605(n)      (1UL << (n))
#define GB02MAC1607(n)       (1UL << ((n) + GB02MAC1603))

/*
 * Begin LPAE MMU TRANSTAB register values
 */
#define GB02MAC1608   			0xfffff000
#define GB02MAC1609  			(0u << 0)
#define GB02MAC1610  			(1u << 1)
#define GB02MAC1611     			(3u << 0)

#define GB02MAC1613        			(1u << 2)
#define GB02MAC1615       			(1u << 4)

#define GB02MAC1616      			0x00000003
/*
 * Begin AARCH64 MMU TRANSTAB CFG register values
 */
#define GB02MAC1220 40
#define GB02MAC1221 					((1ULL << GB02MAC1220) - (1ULL << 4))


/*
 * Begin MMU STATUS register values
 */
#define GB02MAC1620 0x01

#define GB02MAC1622                    	(0x7<<3)
#define GB02MAC1624       	(0x0<<3)
#define GB02MAC1626        	(0x1<<3)
#define GB02MAC1628      	(0x2<<3)
#define GB02MAC1630             	(0x3<<3)
#define GB02MAC1632      	(0x4<<3)
#define GB02MAC1634 	(0x5<<3)

#define GB02MAC1636	(3u << 8)
#define GB02MAC1638	(0u << 8)
#define GB02MAC1640	(1u << 8)
#define GB02MAC1641	(2u << 8)
#define GB02MAC1642	(3u << 8)
#define GB02MAC1644				GB02MAC1636 
#define GB02MAC1646			GB02MAC1638
#define GB02MAC1648 				GB02MAC1640
#define GB02MAC1650				GB02MAC1641
#define GB02MAC1652			GB02MAC1642

#if 0
#define GB02MAC1210				(0u << 0)
#define GB02MAC1212			(1u << 0)
#define GB02MAC1214			(2u << 0)
#define GB02MAC1216			(6u << 0)
#define GB02MAC1218			(8u << 0)

#define GB02MAC1661				(0xF << 0)
#define GB02MAC1196			(3u << 24)
#define GB02MAC1197 	(1u << 24)
#define GB02MAC1198 		(2u << 24)
#endif

#define GB02MAC1671 				(3u << 28)
#define GB02MAC1674 					(2u << 28)
#define GB02MAC1677 					(3u << 28)
/*
 * Begin Command Values
 */

/* GB02MAC1514 register commands */
#define GB02MAC1681         					0x00	/* NOP Operation. Writing this value is ignored */
#define GB02MAC1684       					0x01	/* Start processing a stage chain. Writing this value is ignored */
#define GB02MAC1687   					0x02	/* Gently stop processing a stage chain */
#define GB02MAC1690   					0x03	/* Rudely stop processing a stage chain */
#define GB02MAC1693 					0x04	/* Execute SOFT_STOP if STAGE_CHAIN_FLAG is 0 */
#define GB02MAC1696 					0x05	/* Execute HARD_STOP if STAGE_CHAIN_FLAG is 0 */
#define GB02MAC1699 					0x06	/* Execute SOFT_STOP if STAGE_CHAIN_FLAG is 1 */
#define GB02MAC1701 					0x07	/* Execute HARD_STOP if STAGE_CHAIN_FLAG is 1 */

#define GB02MAC1705        					0x07    /* Mask of bits currently in use by the HW */

/* GB02MAC1580 register commands */
#define GB02MAC1708         					0x00	/* NOP Operation */
#define GB02MAC1711      					0x01	/* Broadcasts the values in AS_TRANSTAB and ASn_MEMATTR to all MMUs */
#define GB02MAC1714        					0x02	/* Issue a lock region command to all MMUs */
#define GB02MAC1717      					0x03	/* Issue a flush region command to all MMUs */
#define GB02MAC1720       					0x04	/* Flush all L2 caches then issue a flush region command to all MMUs
					 				(deprecated - only for use with T60x) */
#define GB02MAC1724    					0x04	/* Flush all L2 caches then issue a flush region command to all MMUs */
#define GB02MAC2821   					0x05	/* Wait for memory accesses to complete, flush all the L1s cache then
				 					flush all L2 caches then issue a flush region command to all MMUs */

/* Possible values of GB02MAC1512 and GB02MAC1520 registers */
#define GB02MAC1727        			(0u << 0)
#define GB02MAC1729            			(1u << 8)
#define GB02MAC1731 			(3u << 8)
#define GB02MAC1732                    			(1u << 10)
#define GB02MAC1734             			(1u << 11)
#define GB02MAC1736          			GB02MAC1727
#define GB02MAC1738              			(1u << 12)
#define GB02MAC1740   			(3u << 12)
#define GB02MAC1742       			(1u << 14)
#define GB02MAC1744     			(1u << 15)
#define GB02MAC1746(n)                			((n) << 16)

/* GB02MAC1513 register values */
#define GB02MAC1748 				(1u << 0)
#define GB02MAC1750     				(1u << 8)
#define GB02MAC1751     				(1u << 16)

/* GB02MAC1515 register values */

/* NOTE: Please keep this values in sync with enum gb_sd_event_code in btsgpu_kernel.h.
 * The values are separated to avoid dependency of userspace and kernel code.
 */

/* Group of values representing the stage status insead a particular fault */
#define GB02MAC1753   				0x00
#define GB02MAC1755         				(GB02MAC1753 + 0x02)	/* 0x02 means INTERRUPTED */
#define GB02MAC1757             				(GB02MAC1753 + 0x03)	/* 0x03 means STOPPED */
#define GB02MAC1760          				(GB02MAC1753 + 0x04)	/* 0x04 means TERMINATED */

/* General fault values */
#define GB02MAC1762          				0x40
#define GB02MAC1765        				(GB02MAC1762)	/* 0x40 means CONFIG FAULT */
#define GB02MAC1768         				(GB02MAC1762 + 0x01)	/* 0x41 means POWER FAULT */
#define GB02MAC1771          				(GB02MAC1762 + 0x02)	/* 0x42 means READ FAULT */
#define GB02MAC1774        			 	(GB02MAC1762 + 0x03)	/* 0x43 means WRITE FAULT */
#define GB02MAC1777      				(GB02MAC1762 + 0x04)	/* 0x44 means AFFINITY FAULT */
#define GB02MAC1780           				(GB02MAC1762 + 0x08)	/* 0x48 means BUS FAULT */

/* Instruction or data faults */
#define GB02MAC1783  			0x50
#define GB02MAC1784        			(GB02MAC1783)	/* 0x50 means INSTR INVALID PC */
#define GB02MAC1786       			(GB02MAC1783 + 0x01)	/* 0x51 means INSTR INVALID ENC */
#define GB02MAC1788     			(GB02MAC1783 + 0x02)	/* 0x52 means INSTR TYPE MISMATCH */
#define GB02MAC1790     			(GB02MAC1783 + 0x03)	/* 0x53 means INSTR OPERAND FAULT */
#define GB02MAC1792         			(GB02MAC1783 + 0x04)	/* 0x54 means INSTR TLS FAULT */
#define GB02MAC1794     			(GB02MAC1783 + 0x05)	/* 0x55 means INSTR BARRIER FAULT */
#define GB02MAC1796       			(GB02MAC1783 + 0x06)	/* 0x56 means INSTR ALIGN FAULT */
/* NOTE: No fault with 0x57 code defined in spec. */
#define GB02MAC1798      			(GB02MAC1783 + 0x08)	/* 0x58 means DATA INVALID FAULT */
#define GB02MAC1799        			(GB02MAC1783 + 0x09)	/* 0x59 means TILE RANGE FAULT */
#define GB02MAC1801     			(GB02MAC1783 + 0x0A)	/* 0x5A means ADDRESS RANGE FAULT */

/* Other faults */
#define GB02MAC1804   				0x60
#define GB02MAC1806       				(GB02MAC1804)	/* 0x60 means OUT OF MEMORY */
#define GB02MAC1808             				0x7F	/* 0x7F means UNKNOWN */

/* GB02MAC1403 values */
#define GPU_COMMAND_NOP                				0x00	/* No operation, nothing happens */
#define GPU_COMMAND_SOFT_RESET         				0x01	/* Stop all external bus interfaces, and then reset the entire GPU. */
#define GB02MAC1811         				0x02	/* Immediately reset the entire GPU. */
#define GPU_COMMAND_PRFCNT_CLEAR       				0x03	/* Clear all performance counters, setting them all to zero. */
#define GPU_COMMAND_PRFCNT_SAMPLE      				0x04	/* Sample all performance counters, writing them out to memory */
#define GPU_COMMAND_CYCLE_COUNT_START  				0x05	/* Starts the cycle counter, and system timestamp propagation */
#define GPU_COMMAND_CYCLE_COUNT_STOP   				0x06	/* Stops the cycle counter, and system timestamp propagation */
#define GB02MAC1814       				0x07	/* Clean all caches */
#define GB02MAC1816   				0x08	/* Clean and invalidate all caches */
#define GB02MAC1817 				0x09	/* Places the GPU in protected mode */

/* End Command Values */

/* GB02MAC1404 values */
#define GB02MAC1818		   			(1 << 0)	/* Set if GPU active */
#define GB02MAC1820		   			(1 << 1)	/* Set if Power manager enable */
#define GB02MAC1821           		(1 << 2)	/* Set if the performance counters are active. */
#define GB02MAC1823		   			(1 << 3)     /* Set if Slot enable */
#define GB02MAC1824		   			(1 << 4)	/* Set if Page fault enable */
#define GB02MAC1825		   			(1 << 5)	/* Set if IRQ enable */
#define GB02MAC1826	   		(1 << 6)     /* Set if timer counter enable */
#define GB02MAC1827   		(1 << 7)	/* Set if protected mode is active */
#define GB02MAC1828	   			(1 << 8)     /* Set if Debug mode is active */

/* GB02MAC1412 register values */
#define GB02MAC1829      				0 /* Counter mode position. */
#define GB02MAC1831        				4 /* Address space bitmap position. */
#define GB02MAC1832 				8 /* Set select position. */

#define GB02MAC1833    				0 /* The performance counters are disabled. */
#define PRFCNT_CONFIG_MODE_MANUAL 				1 /* The performance counters are enabled, but are only written out when a PRFCNT_SAMPLE command is issued using the GB02MAC1403 register. */
#define PRFCNT_CONFIG_MODE_TILE   				2 /* The performance counters are enabled, and are written out each time a tile finishes rendering. */
#define GB02MAC1834 		3 /* same as MODE_MANUAL but do not clear the count after sample. */

/* AS<n>_MEMATTR values: */
/* Use GPU implementation-defined caching policy. */
#define GB02MAC1199 			0x88ull
/* The attribute set to force all resources to be cached. */
#define GB02MAC1200    			0x8Full
/* Inner write-alloc cache setup, no outer caching */
#define GB02MAC1201           			0x8Dull

/* Set to implementation defined, outer caching */
#define GB02MAC1202 			0x88ull
/* Set to write back memory, outer caching */
#define GB02MAC1203       			0x8Dull

/* Use GPU implementation-defined  caching policy. */
#define GB02MAC1835 			0x48ull
/* The attribute set to force all resources to be cached. */
#define GB02MAC1836    			0x4Full
/* Inner write-alloc cache setup, no outer caching */
#define GB02MAC1837           			0x4Dull
/* Set to implementation defined, outer caching */
#define GB02MAC1838        			0x88ull
/* Set to write back memory, outer caching */
#define GB02MAC1839              			0x8Dull

/* Symbol for default MEMATTR to use */

/* Default is - HW implementation defined caching */
#define GB02MAC1840               			0
#define GB02MAC1841          		 	3

/* HW implementation defined caching */
#define GB02MAC1204 			0
/* Force cache on */
#define GB02MAC1205    			1
/* Write-alloc */
#define GB02MAC1206           			2
/* Outer coherent, inner implementation defined policy */
#define GB02MAC1207        			3
/* Outer coherent, write alloc inner */
#define GB02MAC1208              			4

/* SS<n>_FEATURES register */

#define GB02MAC1842              			(1u << 1)
#define GB02MAC1843         			(1u << 2)
#define GB02MAC1844       			(1u << 3)
#define GB02MAC1845           			(1u << 4)
#define GB02MAC1846            			(1u << 5)
#define GB02MAC1847          			(1u << 6)
#define GB02MAC1848             			(1u << 7)
#define GB02MAC1849             			(1u << 8)
#define GB02MAC1850          			(1u << 9)
#define GB02MAC1851		   				(1u << 10)

/* End SS<n>_FEATURES register */

/* GB02MAC1600 register */
#define GB02MAC1852        	(24)
#define GB02MAC1853              	(0x0 << GB02MAC1852)
#define GB02MAC1854       	(0x1 << GB02MAC1852)
#define GB02MAC1855      	(0x2 << GB02MAC1852)
#define GB02MAC1856         	(0x3 << GB02MAC1852)

#define GB02MAC1857       	(26)
#define GB02MAC1858            	 	(0x0 << GB02MAC1857)
#define GB02MAC1859      	(0x1 << GB02MAC1857)
#define GB02MAC1860     	(0x2 << GB02MAC1857)
#define GB02MAC1861        	(0x3 << GB02MAC1857)
/* End GB02MAC1600 register */

/* THREAD_* registers */

/* THREAD_FEATURES IMPLEMENTATION_TECHNOLOGY values */
#define GB02MAC1862  				0
#define GB02MAC1863      				1
#define GB02MAC1864         				2
#define GB02MAC1865        				3

/* Default values when registers are not supported by the implemented hardware */
#define GB02MAC1866     					256
#define GB02MAC1867    					256
#define GB02MAC1868    					256
#define GB02MAC1869     					1024
#define GB02MAC1870    					4
#define GB02MAC1871   					10

/* THREAD_FEATURS */
#define GB02MAC1872					0x0A
#define GB02MAC1873				0xFF000000
#define GB02MAC1874							0x04
#define GB02MAC1875						0xFF0000
#define GB02MAC1876							0x6000
#define GB02MAC1877						0xFFFF

/* End THREAD_* registers */

/* GB02MAC1598 register */

#define GB02MAC1878             		(1ul << 3)
#define GB02MAC1879  		(1ul << 4)
#define GB02MAC1880   		(1ul << 6)
#define GB02MAC1881      		(1ul << 16)
#define GB02MAC1882   		(1ul << 16)
#define GB02MAC1883    		(1ul << 18)
#define GB02MAC1884      		(1ul << 25)
/* End GB02MAC1598 register */

/* GB02MAC1599 register */

#define GB02MAC1885      		(1ul << 0)

/* End GB02MAC1599 register */

/* GB02MAC1597 register */

#define GB02MAC1886  			(1ul << 0)
#define GB02MAC1887 			(1ul << 1)
#define GB02MAC1888 		(1ul << 2)
#define GB02MAC1889 		(3)
#define GB02MAC1890 		(0x3F)
#define SM_FORCE_COHERENCY_FEATURES_SHIFT 	(2)
#define GB02MAC1891 		(16)
#define GB02MAC1892 			(0x3F)
#define GB02MAC1893 			(1ul << 31)
/* End GB02MAC1597 register */

/* GB02MAC1410 */
#define GB02MAC1894			24u
#define GB02MAC1895			(0 << GB02MAC1894)
#define GB02MAC1896			(1 << GB02MAC1894)
#define GB02MAC1897			(2 << GB02MAC1894)
#define GB02MAC1898			(3 << GB02MAC1894)

#define GB02MAC1899				23u

#define GB02MAC1900			16u
#define GB02MAC1901			(0 << GB02MAC1900)
#define GB02MAC1902			(1 << GB02MAC1900)
#define GB02MAC1903			(2 << GB02MAC1900)
#define GB02MAC1904			(3 << GB02MAC1900)

#define GB02MAC1905		14u
#define GB02MAC1906		(0 << GB02MAC1905)
#define GB02MAC1907		(1 << GB02MAC1905)
#define GB02MAC1908		(2 << GB02MAC1905)
#define GB02MAC1909		(3 << GB02MAC1905)

#define GB02MAC1910		10u
#define GB02MAC1911		(0 << GB02MAC1910)
#define GB02MAC1912		(1 << GB02MAC1910)
#define GB02MAC1913			(2 << GB02MAC1910)
#define GB02MAC1914		(3 << GB02MAC1910)

#define GB02MAC1915		8u
#define GB02MAC1916			(0 << GB02MAC1915)
#define GB02MAC1918		(1 << GB02MAC1915)
#define GB02MAC1920			(2 << GB02MAC1915)
#define GB02MAC1922			(3 << GB02MAC1915)

#define GB02MAC1924			4
#define GB02MAC1925			(0 << GB02MAC1924)
#define GB02MAC1926			(1 << GB02MAC1924)
#define GB02MAC1927			(2 << GB02MAC1924)
#define GB02MAC1928			(3 << GB02MAC1924)

#define GB02MAC1929			2
#define GB02MAC1930			(0 << GB02MAC1929)
#define GB02MAC1931			(1 << GB02MAC1929)
#define GB02MAC1932			(2 << GB02MAC1929)
#define GB02MAC1933			(3 << GB02MAC1929)

#define GB02MAC1934			0
#define GB02MAC1935			(0 << GB02MAC1934)
#define GB02MAC1936			(1 << GB02MAC1934)
#define GB02MAC1937			(2 << GB02MAC1934)
#define GB02MAC1938			(3 << GB02MAC1934)

#endif /* _GENBU_REGMAP_H_ */
