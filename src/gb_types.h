// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __GB_TYPES_H__
#define __GB_TYPES_H__
// IWYU pragma: always_keep

#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

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

typedef struct {
    const int count;
    struct {
        const char *label;
        const int width;
    } list[8];  // list in msb to lsb order
} RegViewFields;

typedef struct {
    const uint8_t * const value;
    const char *name;
    const char *offset;
    RegViewFields fields;
} RegView;

typedef struct {
    int regCount;
    Vector2 (* guiDrawCustomRegLine)(const Vector2 viewAnchor, int index);
    float lineHeight;
    RegView regs[];
} RegViewList;


#ifdef __cplusplus
}
#endif
#endif // __GB_TYPES_H__
