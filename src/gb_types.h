#ifndef __GB_TYPES_H__
#define __GB_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SUCCESS,
    FAILURE,

} Status;


typedef struct {
    int size;
    uint8_t *contents;
    uint8_t *contentFlags;
    int entrypoint;
} RomImage;

typedef struct {
    int size;
    uint8_t *contents;
} RamImage;


#ifdef __cplusplus
}
#endif
#endif // __GB_TYPES_H__
