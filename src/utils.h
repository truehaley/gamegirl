// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __UTILS_H__
#define __UTILS_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a,b)    ((a<b)?a:b)
#define MAX(a,b)    ((a<b)?b:a)
#define NUM_ELEMENTS(array)     (sizeof(array) / sizeof(array[0]))

#define MSB(val16)  ((uint8_t)(((val16) & 0xFF00) >> 8))
#define LSB(val16)  ((uint8_t)(((val16) & 0x00FF)))

#define BIT(val, bit)   ( ((val)&(1<<(bit)))>>(bit) )

#ifdef __cplusplus
}
#endif

#endif //__UTILS_H__
