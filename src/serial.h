// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t getSerialReg8(uint16_t addr);
void setSerialReg8(uint16_t addr, uint8_t val8);
void serialInit(void);

#ifdef __cplusplus
}
#endif

#endif //__SERIAL_H__
