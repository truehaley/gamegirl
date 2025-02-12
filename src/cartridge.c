#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "utils.h"
#include "cpu.h"
#include "cartridge.h"

const OldLicenseeDecoder oldLicensees[] = {
    {0x00,    "None"},
    {0x01,    "Nintendo"},
    {0x08,    "Capcom"},
    {0x09,    "HOT-B"},
    {0x0A,    "Jaleco"},
    {0x0B,    "Coconuts Japan"},
    {0x0C,    "Elite Systems"},
    {0x13,    "EA (Electronic Arts)"},
    {0x18,    "Hudson Soft"},
    {0x19,    "ITC Entertainment"},
    {0x1A,    "Yanoman"},
    {0x1D,    "Japan Clary"},
    {0x1F,    "Virgin Games Ltd.3"},
    {0x24,    "PCM Complete"},
    {0x25,    "San-X"},
    {0x28,    "Kemco"},
    {0x29,    "SETA Corporation"},
    {0x30,    "Infogrames5"},
    {0x31,    "Nintendo"},
    {0x32,    "Bandai"},
    {0x33,    "NewLicensee"},
    {0x34,    "Konami"},
    {0x35,    "HectorSoft"},
    {0x38,    "Capcom"},
    {0x39,    "Banpresto"},
    {0x3C,    "Entertainment Interactive (stub)"},
    {0x3E,    "Gremlin"},
    {0x41,    "Ubi Soft1"},
    {0x42,    "Atlus"},
    {0x44,    "Malibu Interactive"},
    {0x46,    "Angel"},
    {0x47,    "Spectrum HoloByte"},
    {0x49,    "Irem"},
    {0x4A,    "Virgin Games Ltd.3"},
    {0x4D,    "Malibu Interactive"},
    {0x4F,    "U.S. Gold"},
    {0x50,    "Absolute"},
    {0x51,    "Acclaim Entertainment"},
    {0x52,    "Activision"},
    {0x53,    "Sammy USA Corporation"},
    {0x54,    "GameTek"},
    {0x55,    "Park Place13"},
    {0x56,    "LJN"},
    {0x57,    "Matchbox"},
    {0x59,    "Milton Bradley Company"},
    {0x5A,    "Mindscape"},
    {0x5B,    "Romstar"},
    {0x5C,    "Naxat Soft14"},
    {0x5D,    "Tradewest"},
    {0x60,    "Titus Interactive"},
    {0x61,    "Virgin Games Ltd.3"},
    {0x67,    "Ocean Software"},
    {0x69,    "EA (Electronic Arts)"},
    {0x6E,    "Elite Systems"},
    {0x6F,    "Electro Brain"},
    {0x70,    "Infogrames5"},
    {0x71,    "Interplay Entertainment"},
    {0x72,    "Broderbund"},
    {0x73,    "Sculptured Software6"},
    {0x75,    "The Sales Curve Limited7"},
    {0x78,    "THQ"},
    {0x79,    "Accolade15"},
    {0x7A,    "Triffix Entertainment"},
    {0x7C,    "MicroProse"},
    {0x7F,    "Kemco"},
    {0x80,    "Misawa Entertainment"},
    {0x83,    "LOZC G."},
    {0x86,    "Tokuma Shoten"},
    {0x8B,    "Bullet-Proof Software2"},
    {0x8C,    "Vic Tokai Corp.16"},
    {0x8E,    "Ape Inc.17"},
    {0x8F,    "I’Max18"},
    {0x91,    "Chunsoft Co.8"},
    {0x92,    "Video System"},
    {0x93,    "Tsubaraya Productions"},
    {0x95,    "Varie"},
    {0x96,    "Yonezawa19/S’Pal"},
    {0x97,    "Kemco"},
    {0x99,    "Arc"},
    {0x9A,    "Nihon Bussan"},
    {0x9B,    "Tecmo"},
    {0x9C,    "Imagineer"},
    {0x9D,    "Banpresto"},
    {0x9F,    "Nova"},
    {0xA1,    "Hori Electric"},
    {0xA2,    "Bandai"},
    {0xA4,    "Konami"},
    {0xA6,    "Kawada"},
    {0xA7,    "Takara"},
    {0xA9,    "Technos Japan"},
    {0xAA,    "Broderbund"},
    {0xAC,    "Toei Animation"},
    {0xAD,    "Toho"},
    {0xAF,    "Namco"},
    {0xB0,    "Acclaim Entertainment"},
    {0xB1,    "ASCII Corporation or Nexsoft"},
    {0xB2,    "Bandai"},
    {0xB4,    "Square Enix"},
    {0xB6,    "HAL Laboratory"},
    {0xB7,    "SNK"},
    {0xB9,    "Pony Canyon"},
    {0xBA,    "Culture Brain"},
    {0xBB,    "Sunsoft"},
    {0xBD,    "Sony Imagesoft"},
    {0xBF,    "Sammy Corporation"},
    {0xC0,    "Taito"},
    {0xC2,    "Kemco"},
    {0xC3,    "Square"},
    {0xC4,    "Tokuma Shoten"},
    {0xC5,    "Data East"},
    {0xC6,    "Tonkin House"},
    {0xC8,    "Koei"},
    {0xC9,    "UFL"},
    {0xCA,    "Ultra Games"},
    {0xCB,    "VAP, Inc."},
    {0xCC,    "Use Corporation"},
    {0xCD,    "Meldac"},
    {0xCE,    "Pony Canyon"},
    {0xCF,    "Angel"},
    {0xD0,    "Taito"},
    {0xD1,    "SOFEL (Software Engineering Lab)"},
    {0xD2,    "Quest"},
    {0xD3,    "Sigma Enterprises"},
    {0xD4,    "ASK Kodansha Co."},
    {0xD6,    "Naxat Soft14"},
    {0xD7,    "Copya System"},
    {0xD9,    "Banpresto"},
    {0xDA,    "Tomy"},
    {0xDB,    "LJN"},
    {0xDD,    "Nippon Computer Systems"},
    {0xDE,    "Human Ent."},
    {0xDF,    "Altron"},
    {0xE0,    "Jaleco"},
    {0xE1,    "Towa Chiki"},
    {0xE2,    "Yutaka # Needs more info"},
    {0xE3,    "Varie"},
    {0xE5,    "Epoch"},
    {0xE7,    "Athena"},
    {0xE8,    "Asmik Ace Entertainment"},
    {0xE9,    "Natsume"},
    {0xEA,    "King Records"},
    {0xEB,    "Atlus"},
    {0xEC,    "Epic/Sony Records"},
    {0xEE,    "IGS"},
    {0xF0,    "A Wave"},
    {0xF3,    "Extreme Entertainment"},
    {0xFF,    "LJN"},
};

const NewLicenseeDecoder newLicensees[] = {
    { .code.ascii = {'0', '0'},    .name = "None" },
    { .code.ascii = {'0', '1'},    .name = "Nintendo Research & Development 1" },
    { .code.ascii = {'0', '8'},    .name = "Capcom" },
    { .code.ascii = {'1', '3'},    .name = "EA (Electronic Arts)" },
    { .code.ascii = {'1', '8'},    .name = "Hudson Soft" },
    { .code.ascii = {'1', '9'},    .name = "B-AI" },
    { .code.ascii = {'2', '0'},    .name = "KSS" },
    { .code.ascii = {'2', '2'},    .name = "Planning Office WADA" },
    { .code.ascii = {'2', '4'},    .name = "PCM Complete" },
    { .code.ascii = {'2', '5'},    .name = "San-X" },
    { .code.ascii = {'2', '8'},    .name = "Kemco" },
    { .code.ascii = {'2', '9'},    .name = "SETA Corporation" },
    { .code.ascii = {'3', '0'},    .name = "Viacom" },
    { .code.ascii = {'3', '1'},    .name = "Nintendo" },
    { .code.ascii = {'3', '2'},    .name = "Bandai" },
    { .code.ascii = {'3', '3'},    .name = "Ocean Software/Acclaim Entertainment" },
    { .code.ascii = {'3', '4'},    .name = "Konami" },
    { .code.ascii = {'3', '5'},    .name = "HectorSoft" },
    { .code.ascii = {'3', '7'},    .name = "Taito" },
    { .code.ascii = {'3', '8'},    .name = "Hudson Soft" },
    { .code.ascii = {'3', '9'},    .name = "Banpresto" },
    { .code.ascii = {'4', '1'},    .name = "Ubi Soft1" },
    { .code.ascii = {'4', '2'},    .name = "Atlus" },
    { .code.ascii = {'4', '4'},    .name = "Malibu Interactive" },
    { .code.ascii = {'4', '6'},    .name = "Angel" },
    { .code.ascii = {'4', '7'},    .name = "Bullet-Proof Software2" },
    { .code.ascii = {'4', '9'},    .name = "Irem" },
    { .code.ascii = {'5', '0'},    .name = "Absolute" },
    { .code.ascii = {'5', '1'},    .name = "Acclaim Entertainment" },
    { .code.ascii = {'5', '2'},    .name = "Activision" },
    { .code.ascii = {'5', '3'},    .name = "Sammy USA Corporation" },
    { .code.ascii = {'5', '4'},    .name = "Konami" },
    { .code.ascii = {'5', '5'},    .name = "Hi Tech Expressions" },
    { .code.ascii = {'5', '6'},    .name = "LJN" },
    { .code.ascii = {'5', '7'},    .name = "Matchbox" },
    { .code.ascii = {'5', '8'},    .name = "Mattel" },
    { .code.ascii = {'5', '9'},    .name = "Milton Bradley Company" },
    { .code.ascii = {'6', '0'},    .name = "Titus Interactive" },
    { .code.ascii = {'6', '1'},    .name = "Virgin Games Ltd.3" },
    { .code.ascii = {'6', '4'},    .name = "Lucasfilm Games4" },
    { .code.ascii = {'6', '7'},    .name = "Ocean Software" },
    { .code.ascii = {'6', '9'},    .name = "EA (Electronic Arts)" },
    { .code.ascii = {'7', '0'},    .name = "Infogrames5" },
    { .code.ascii = {'7', '1'},    .name = "Interplay Entertainment" },
    { .code.ascii = {'7', '2'},    .name = "Broderbund" },
    { .code.ascii = {'7', '3'},    .name = "Sculptured Software6" },
    { .code.ascii = {'7', '5'},    .name = "The Sales Curve Limited7" },
    { .code.ascii = {'7', '8'},    .name = "THQ" },
    { .code.ascii = {'7', '9'},    .name = "Accolade" },
    { .code.ascii = {'8', '0'},    .name = "Misawa Entertainment" },
    { .code.ascii = {'8', '3'},    .name = "lozc" },
    { .code.ascii = {'8', '6'},    .name = "Tokuma Shoten" },
    { .code.ascii = {'8', '7'},    .name = "Tsukuda Original" },
    { .code.ascii = {'9', '1'},    .name = "Chunsoft Co.8" },
    { .code.ascii = {'9', '2'},    .name = "Video System" },
    { .code.ascii = {'9', '3'},    .name = "Ocean Software/Acclaim Entertainment" },
    { .code.ascii = {'9', '5'},    .name = "Varie" },
    { .code.ascii = {'9', '6'},    .name = "Yonezawa/s’pal" },
    { .code.ascii = {'9', '7'},    .name = "Kaneko" },
    { .code.ascii = {'9', '9'},    .name = "Pack-In-Video" },
    { .code.ascii = {'9', 'H'},    .name = "Bottom Up" },
    { .code.ascii = {'A', '4'},    .name = "Konami (Yu-Gi-Oh!)" },
    { .code.ascii = {'B', 'L'},    .name = "MTO" },
    { .code.ascii = {'D', 'K'},    .name = "Kodansha " },
};



Status loadCartridge(Cartridge * const cart, const char * const filename)
{
    RomImage *rom;
    uint8_t checksum = 0;

    memset(cart, 0, sizeof(Cartridge));
    rom = &(cart->rom);

    printf("Loading Cartridge ROM \'%s\'...\n", filename);
    if( SUCCESS != loadRom(rom, filename, CARTRIDGE_ENTRY) ) {
        goto failure;
    }

    cart->header = (CartridgeHeader *) &(rom->contents[CART_HEADER_OFFSET]);

    printf("    checksum...");
    for (uint16_t address = CART_HEADER_CHECK_START; address <= CART_HEADER_CHECK_END; address++) {
        checksum = checksum - rom->contents[address] - 1;
    }
    if( checksum != cart->header->headerChecksum ) {
        printf("FAIL\n");
        goto failure;
    }
    printf("PASS\n");

    printf("    title...\"");
    if(CART_CGB_SUPPORTS_COLOR_MASK == (cart->header->title.v2.cgbMode & CART_CGB_SUPPORTS_COLOR_MASK)) {
        cart->cgbMode = cart->header->title.v2.cgbMode;
        printf("%.15s\"\n", cart->header->title.v2.title);
        if( CART_CGB_ONLY_COLOR == cart->cgbMode ) {
            printf("    COLOR ONLY\n");
        } else {
            printf("    COLOR SUPPORTED\n");
        }
    } else {
        cart->cgbMode = 0;
        printf("%.16s\"\n    MONO ONLY\n", cart->header->title.v1.title);
    }

    printf("    licensee...");
    if( CART_LICENSEE_NEW == cart->header->oldLicensee ) {
        for(int index=0; index < NUM_ELEMENTS(newLicensees); index++) {
            if( newLicensees[index].code.val == cart->header->newLicensee ) {
                cart->licensee = newLicensees[index].name;
                break;
            }
        }
    } else {
        for(int index=0; index < NUM_ELEMENTS(oldLicensees); index++) {
            if( oldLicensees[index].code == cart->header->oldLicensee ) {
                cart->licensee = oldLicensees[index].name;
                break;
            }
        }
    }
    if( NULL == cart->licensee ) {
        printf("NOT FOUND\n");
    } else {
        printf("%s\n", cart->licensee);
    }

    printf("    rom size...");
    if( CART_ROM_512K >= cart->header->romSize ) {
        printf("%dK (%d banks)\n", (1<<cart->header->romSize)*32, (2<<cart->header->romSize));
        cart->romSize = (1<<cart->header->romSize)*32;
    } else if( CART_ROM_8M >= cart->header->romSize) {
        printf("%dM (%d banks)\n", (1<<cart->header->romSize), (2<<cart->header->romSize));
        cart->romSize = (1<<cart->header->romSize)*32;
    } else {
        printf("UNKNOWN\n");
    }

    printf("    ram size...");
    switch(cart->header->ramSize) {
        case CART_RAM_2K:
            cart->ramSize = 0;
            break;
        case CART_RAM_8K:
            cart->ramSize = 8192;
            break;
        case CART_RAM_32K:
            cart->ramSize = 32768;
            break;
        case CART_RAM_64K:
            cart->ramSize = 65536;
            break;
        case CART_RAM_128K:
            cart->ramSize = 131072;
            break;
        case CART_RAM_NONE:
        default:
            cart->ramSize = 0;
            break;
    }
    printf("%dK\n", cart->ramSize/1024);

    printf("    mapper...");
    if(0 != cart->header->cartridgeType) {
        printf("UNKNOWN\n");
    } else {
        printf("NONE\n");
    }

    preprocessRom(rom, CARTRIDGE_ENTRY);

    printf("...Success\n");
    return SUCCESS;
failure:
    printf("...Failure\n");
    return FAILURE;
}

void unloadCartridge(Cartridge * const cart)
{
    unloadRom(&cart->rom);
}
