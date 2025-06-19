/* Copyright (C) Xi'an Sietium Electronics Co.Ltd */
/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __XT_H__
#define __XT_H__

#define GB02MAC1040                       0x400000
#define GB02MAC1041                       0x4000000

#define GB02MAC1042               0x0000000   //0
#define GB02MAC1043                  0x0400000   //1
#define GB02MAC1046                  0x0403000
#define GB02MAC1048                  0x0800000   //2
#define GB02MAC1050                    0x0C00000   //3
#define GB02MAC1052                    0x1000000   //4
#define GB02MAC1053               0x1400000   //5
#define GB02MAC1055               0x1800000   //6
#define GB02MAC1057                0x1C00000   //7
#define GB02MAC1058            0x2000000   //8
#define GB02MAC1059               0x2400000   //9
#define GB02MAC1060              0x2800000   //10
#define GB02MAC1061              0x400000
#define GB02MAC1062(i) \
	(GB02MAC1060 + i * GB02MAC1061)
#define GB02MAC131          0x0000000   //16
#define GB02MAC132         0x2000000   //16
#define GB02MAC133         0x3000000   //16
#define GB02MAC134          0x4000000   //17
#define GB02MAC135         0x6000000   //17
#define GB02MAC136         0x7000000   //17
#define GB02MAC137          0x8000000   //18
#define GB02MAC138         0xA000000   //18
#define GB02MAC139         0xB000000   //18
#define GB02MAC140          0xC000000   //19
#define GB02MAC141         0xE000000   //19
#define GB02MAC142         0xF000000   //19

#endif  /* __XTH__ */
