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
