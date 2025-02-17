#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CART_HEADER_OFFSET      (0x0100)
#define CART_HEADER_CHECK_START (0x0134)
#define CART_HEADER_CHECK_END   (0x014c)

typedef struct __attribute__((packed)) {
    // 0x0100
    uint8_t entrypoint[4];
    // 0x0104 - 0x0133
    uint8_t nintendoLogo[48];
    // 0x0134 - 0x143
    union {
        struct {
            const char title[16];
        } v1;
        struct {
            const char title[15];
            uint8_t cgbMode;
        } v2;
        struct {
            const char title[11];
            uint8_t mfgCode[4];
            uint8_t cgbMode;
        } v3;
    } title;
    // 0x144
    uint16_t newLicensee;
    // 0x146
    uint8_t sgbMode;
    // 0x147
    uint8_t cartridgeType;
    // 0x148
    uint8_t romSize;
    // 0x149
    uint8_t ramSize;
    // 0x14a
    uint8_t destCode;
    // 0x14b
    uint8_t oldLicensee;
    // 0x14c
    uint8_t version;
    // 0x14d
    uint8_t headerChecksum;
    // 0x14e - 0x14f
    uint16_t globalChecksum;  // big endian
} CartridgeHeader;

// Color Game Boy settings
#define CART_CGB_SUPPORTS_COLOR_MASK    (0x80)
#define CART_CGB_ONLY_COLOR             (0xC0)

// Super Game Boy settings
#define CART_SGB_UNSUPPORTED    (0x00)  // technically any value other than 0x03 means unsupported
#define CART_SGB_ENHANCED       (0x03)

typedef enum {
    ROM_ONLY,
    MBC1,
    MBC2,
    MMM01,
    MBC3,
    MBC5,
    MBC6,
    MBC7,
    CAMERA,
    TAMA5,
    HUC3,
    HUC1,
} MapperType;

typedef struct {
    uint8_t type;
    MapperType mapper;
    uint8_t mapperFlags;
} CartridgeTypeDecoder;

#define CART_TYPE_RAM       (0x01)
#define CART_TYPE_BATTERY   (0x02)
#define CART_TYPE_TIMER     (0x04)
#define CART_TYPE_RUMBLE    (0x08)
#define CART_TYPE_SENSOR    (0x10)

/*
$00	ROM ONLY
$01	MBC1
$02	MBC1+RAM
$03	MBC1+RAM+BATTERY
$05	MBC2
$06	MBC2+BATTERY
$08	ROM+RAM 9
$09	ROM+RAM+BATTERY 9
$0B	MMM01
$0C	MMM01+RAM
$0D	MMM01+RAM+BATTERY
$0F	MBC3+TIMER+BATTERY
$10	MBC3+TIMER+RAM+BATTERY 10
$11	MBC3
$12	MBC3+RAM 10
$13	MBC3+RAM+BATTERY 10
$19	MBC5
$1A	MBC5+RAM
$1B	MBC5+RAM+BATTERY
$1C	MBC5+RUMBLE
$1D	MBC5+RUMBLE+RAM
$1E	MBC5+RUMBLE+RAM+BATTERY
$20	MBC6
$22	MBC7+SENSOR+RUMBLE+RAM+BATTERY
$FC	POCKET CAMERA
$FD	BANDAI TAMA5
$FE	HuC3
$FF	HuC1+RAM+BATTERY
*/

// ROM sizing
#define CART_ROM_32K        (0x00)  // No banking
#define CART_ROM_64K        (0x01)  // 4 banks
#define CART_ROM_128K       (0x02)  // 8 banks
#define CART_ROM_256K       (0x03)  // 16 banks
#define CART_ROM_512K       (0x04)  // 32 banks
#define CART_ROM_1M         (0x05)  // 64 banks
#define CART_ROM_2M         (0x06)  // 128 banks
#define CART_ROM_4M         (0x07)  // 256 banks
#define CART_ROM_8M         (0x08)  // 512 banks

// RAM sizing
#define CART_RAM_NONE       (0x00)
#define CART_RAM_2K         (0x01)  // unoffical value, may be error in some homebrew roms
#define CART_RAM_8K         (0x02)  // 1 bank of 8k
#define CART_RAM_32K        (0x03)  // 4 banks of 8k
#define CART_RAM_128K       (0x04)  // 16 banks of 8k
#define CART_RAM_64K        (0x05)  // 8 banks of 8k

// Destination Codes
#define CART_DEST_JAPAN_AND_OVERSEA (0x00)
#define CART_DEST_OVERSEA_ONLY      (0x01)

typedef struct {
    const uint8_t code;
    const char * const name;
} OldLicenseeDecoder;

#define CART_LICENSEE_NEW    (0x33)

typedef struct {
    union {
        const uint8_t ascii[2];
        const uint16_t val;
    } code;
    const char * const name;
} NewLicenseeDecoder;

typedef uint8_t (mapperGetRom8)(uint16_t addr);

typedef struct {
    RomImage rom;
    CartridgeHeader const *header;
    char const *licensee;
    uint8_t cgbMode;
    uint32_t romSize;
    uint32_t ramSize;
    mapperGetRom8 *getCartMappedRom8;
} Cartridge;


Status loadCartridge(Cartridge * const cart, const char * const filename);
void unloadCartridge(Cartridge * const cart);



#ifdef __cplusplus
}
#endif
#endif // __CARTRIDGE_H__
