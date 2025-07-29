/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */

#ifndef __VPU_DWL_DEFS_H__
#define __VPU_DWL_DEFS_H__


#define GB02MAC335      31 /* 1 bit  */
#define GB02MAC337        29 /* 2 bits */
#define GB02MAC339       28 /* 1 bit  */
#define GB02MAC341      17 /* 1 bit  */
#define GB02MAC344        16 /* 1 bit  */
#define GB02MAC346      26 /* 2 bits */
#define GB02MAC348       24 /* 2 bits */
#define GB02MAC350 20 /* 1 bits */
#define GB02MAC352       18 /* 2 bits */
#define GB02MAC354        23 /* 1 bit  */
#define GB02MAC356         26 /* 2 bits */
#define GB02MAC357        23 /* 1 bit  */
#define GB02MAC359        24 /* 1 bit  */
#define GB02MAC361       19 /* 1 bit  */
#define GB02MAC363        22 /* 1 bit  */
#define GB02MAC365      16 /* 1 bit  */
#define GB02MAC367      31 /* 1 bit  */
#define GB02MAC369         31 /* 1 bit  */
#define GB02MAC371       26 /* 3 bits */
#define GB02MAC373        29 /* 3 bits */

#define GB02MAC376 31 /* 1 bit */
#define GB02MAC379 30 /* 1 bit */

#define GB02MAC383    0  /* 1 bits */
#define GB02MAC386     1  /* 1 bits */
#define GB02MAC389        2  /* 1 bits */
#define GB02MAC392        17  /* 2 bits */
#define GB02MAC395         3  /* 1 bits */
#define GB02MAC398         28  /* 3 bits */
#define GB02MAC401     8  /* 4 bits */
#define GB02MAC403  12 /* 3 bits */
#define GB02MAC405       16 /* 1 bits */

#define GB02MAC407       1
#define GB02MAC409   (GB02MAC407 * 4)
#define GB02MAC411       2
#define GB02MAC413   (GB02MAC407 * 4)

#define GB02MAC416        60
#define GB02MAC417    (GB02MAC416 * 4)
#define GB02MAC419          50
#define GB02MAC421      (GB02MAC419 * 4)
#define GB02MAC423        54
#define GB02MAC425    (GB02MAC423 * 4)
#define GB02MAC428        56
#define GB02MAC430    (GB02MAC428 * 4)
#define GB02MAC432           23
#define GB02MAC433       (GB02MAC432 * 4)
#define GB02MAC435         260
#define GB02MAC437     (GB02MAC435 * 4)


#define GB02MAC440              0x01
#define GB02MAC441               0x01
#define GB02MAC442          0x20
#define GB02MAC443    0x10
#define GB02MAC444            0x100

/* Legacy from G1 */
#define GB02MAC445          1
#define GB02MAC448      (GB02MAC445 * 4)
#define GB02MAC451           60
#define GB02MAC454       (GB02MAC451 * 4)

#define GB02MAC456           100
#define GB02MAC458       (GB02MAC456 * 4)
#define GB02MAC419          50
#define GB02MAC421      (GB02MAC419 * 4)
#define GB02MAC423        54
#define GB02MAC425    (GB02MAC423 * 4)

/* VC8000D HW build id */
#define GB02MAC460        309
#define GB02MAC461    (GB02MAC460 * 4)

#define GB02MAC462                 0x01
#define GB02MAC463                  0x01
#define GB02MAC464             0x20
#define GB02MAC465       0x10
#define GB02MAC466        0x10
#define GB02MAC469               0x100
#define GB02MAC470                0x100

#endif
