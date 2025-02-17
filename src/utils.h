#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a,b)    ((a<b)?a:b)
#define MAX(a,b)    ((a<b)?b:a)
#define NUM_ELEMENTS(array)     (sizeof(array) / sizeof(array[0]))

#define MSB(val16)  ((uint8_t)(((val16) & 0xFF00) >> 8))
#define LSB(val16)  ((uint8_t)(((val16) & 0x00FF)))


#ifdef __cplusplus
}
#endif

#endif //__UTILS_H__
