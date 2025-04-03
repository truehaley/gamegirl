// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __TIMER_H__
#define __TIMER_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t getTimerReg8(uint16_t addr);
void setTimerReg8(uint16_t addr, uint8_t val8);

void timerTick(void);
void timerInit(void);

#ifdef __cplusplus
}
#endif

#endif //__TIMER_H__
