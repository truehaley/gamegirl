#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a,b)    ((a<b)?a:b)
#define MAX(a,b)    ((a<b)?b:a)
#define NUM_ELEMENTS(array)     (sizeof(array) / sizeof(array[0]))

typedef enum {
    SUCCESS,
    FAILURE,

} Status;


#ifdef __cplusplus
}
#endif

#endif //__UTILS_H__
