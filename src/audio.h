#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif


uint8_t getAudioReg8(uint16_t addr);
void setAudioReg8(uint16_t addr, uint8_t val8);
void guiDrawAudio(void);
void audioInit(void);
void audioDeinit(void);


#ifdef __cplusplus
}
#endif

#endif //__AUDIO_H__
