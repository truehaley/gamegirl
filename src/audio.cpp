#include "gb.h"
#include "gui.h"

struct {
    union {
        uint8_t val;
        struct {
            uint8_t rightVol:3;
            uint8_t vinRightPan:1;
            uint8_t leftVol:3;
            uint8_t vinLeftPan:1;
        };
    } VOLUME;   // NR50 - FF24

    union {
        uint8_t val;
        struct {
            uint8_t ch1RightPan:1;
            uint8_t ch2RightPan:1;
            uint8_t ch3RightPan:1;
            uint8_t ch4RightPan:1;
            uint8_t ch1LeftPan:1;
            uint8_t ch2LeftPan:1;
            uint8_t ch3LeftPan:1;
            uint8_t ch4LeftPan:1;
        };
    } PAN;      // NR51 - FF25

    union {
        uint8_t val;
        struct {
            uint8_t ch1On:1;    // read only
            uint8_t ch2On:1;    // read only
            uint8_t ch3On:1;    // read only
            uint8_t ch4On:1;    // read only
            uint8_t reserved:3;
            uint8_t enable:1;
        };
    } CTRL;     // NR52 - FF26
} regs;

class PulseChannel {
public:
    struct {
        union {
            uint8_t val;
            struct {
                uint8_t step:3;
                uint8_t direction:1;
                uint8_t pace:3;
                uint8_t reserved:1;
            };
        } SWEEP;    // NRx0 - FF10, xxxx

        union {
            uint8_t val;
            struct {
                uint8_t length:6;   // Write only?
                uint8_t duty:2;     // 0- 12.5%, 1- 25%, 2- 50%, 3- 75%
            };
        } TIMER;    // NRx1 - FF11, FF16

        union {
            uint8_t val;
            struct {
                uint8_t pace:3;
                uint8_t direction:1;
                uint8_t startVol:4;
            };
        } ENVLP;    // NRx2 - FF12, FF17

        union {
            uint8_t val;
            uint8_t periodLow;  // Write only
        } LPER;     // NRx3 - FF13, FF18

        union {
            uint8_t val;
            struct {
                uint8_t periodHigh:3;   // Write-only
                uint8_t reserved:3;
                uint8_t lengthEn:1;
                uint8_t trigger:1;      // Write-only
            };
        } CTRL;     // NRx4 - FF14, FF19
    } regs;

    bool hasSweep;

public:
    PulseChannel(bool hasSweep) : hasSweep{hasSweep} {
        memset(&regs, 0, sizeof(regs));
    };

    uint8_t getReg8(uint8_t regOffset) {
        if( 0 == regOffset && hasSweep ) {
            return regs.SWEEP.val;

        } else if( 1 == regOffset ) {
            return regs.TIMER.val;

        } else if( 2 == regOffset ) {
            return regs.ENVLP.val;

        } else if( 3 == regOffset ) {
            return regs.LPER.val;

        } else if( 4 == regOffset ) {
            return regs.CTRL.val;

        }
        return UNMAPPED_REG_VAL;
    }

    void setReg8(uint8_t regOffset, uint8_t val8) {
        if( 0 == regOffset && hasSweep ) {
            regs.SWEEP.val = val8;

        } else if( 1 == regOffset ) {
            regs.TIMER.val = val8;

        } else if( 2 == regOffset ) {
            regs.ENVLP.val = val8;

        } else if( 3 == regOffset ) {
            regs.LPER.val = val8;

        } else if( 4 == regOffset ) {
            regs.CTRL.val = val8;

        }
    }
};

class WaveChannel {
public:
    struct {
        union {
            uint8_t val;
            struct {
                uint8_t reserved:7;
                uint8_t enable:1;
            };
        } DAC;      // NR30 - FF1A

        union {
            uint8_t val;
            uint8_t length;
        } TIMER;    // NR31 - FF1B

        union {
            uint8_t val;
            struct {
                uint8_t reserved:5;
                uint8_t level:2;    // 0- mute, 1- 100%, 2- 50%, 3- 25%
                uint8_t reserved2:1;
            };
        } ENVLP;    // NR32 - FF1C

        union {
            uint8_t val;
            uint8_t periodLow;  // Write only
        } LPER;     // NR33 - FF1D

        union {
            uint8_t val;
            struct {
                uint8_t periodHigh:3;   // Write-only
                uint8_t reserved:3;
                uint8_t lengthEn:1;
                uint8_t trigger:1;      // Write-only
            };
        } CTRL;     // NR34 - FF1E

    } regs;

    union{
        uint8_t val;
        struct {
            uint8_t nib2:4;
            uint8_t nib1:4;
        };
    } waveRam[16];    // FF30 - FF3F

public:
    WaveChannel() {
        memset(&regs, 0, sizeof(regs));
        memset(waveRam, 0, sizeof(waveRam));
    };

    uint8_t getReg8(uint8_t regOffset) {
        if( 0 == regOffset ) {
            return regs.DAC.val;

        } else if( 1 == regOffset ) {
            return regs.TIMER.val;

        } else if( 2 == regOffset ) {
            return regs.ENVLP.val;

        } else if( 3 == regOffset ) {
            return regs.LPER.val;

        } else if( 4 == regOffset ) {
            return regs.CTRL.val;

        }
        return UNMAPPED_REG_VAL;
    }

    void setReg8(uint8_t regOffset, uint8_t val8) {
        if( 0 == regOffset ) {
            regs.DAC.val = val8;

        } else if( 1 == regOffset ) {
            regs.TIMER.val = val8;

        } else if( 2 == regOffset ) {
            regs.ENVLP.val = val8;

        } else if( 3 == regOffset ) {
            regs.LPER.val = val8;

        } else if( 4 == regOffset ) {
            regs.CTRL.val = val8;

        }
    }

    uint8_t getWave8(uint8_t offset) {
        return waveRam[offset & 0x0F].val;
    }

    void setWave8(uint8_t offset, uint8_t val8) {
        waveRam[offset & 0x0F].val = val8;
    }
};

class NoiseChannel {
public:
    struct {
        // NR40 - FF1F does not exist

        union {
            uint8_t val;
            struct {
                uint8_t length:6;   // Write-only
                uint8_t reserved:2;
            };
        } TIMER;    // NR41 - FF20

        union {
            uint8_t val;
            struct {
                uint8_t pace:3;
                uint8_t direction:1;
                uint8_t startVol:4;
            };
        } ENVLP;    // NR42 - FF21

        union {
            uint8_t val;
            struct {
                uint8_t divider:3;
                uint8_t width:1;
                uint8_t shift:4;
            };
        } LFSR;     // NR43 - FF22

        union {
            uint8_t val;
            struct {
                uint8_t reserved:6;
                uint8_t lengthEn:1;
                uint8_t trigger:1;      // Write-only
            };
        } CTRL;     // NR44 - FF23
    } regs;

public:
    NoiseChannel() {
        memset(&regs, 0, sizeof(regs));
    };

    uint8_t getReg8(uint8_t regOffset) {
        if( 1 == regOffset ) {
            return regs.TIMER.val;

        } else if( 2 == regOffset ) {
            return regs.ENVLP.val;

        } else if( 3 == regOffset ) {
            return regs.LFSR.val;

        } else if( 4 == regOffset ) {
            return regs.CTRL.val;

        }
        return UNMAPPED_REG_VAL;
    }

    void setReg8(uint8_t regOffset, uint8_t val8) {
        if( 1 == regOffset ) {
            regs.TIMER.val = val8;

        } else if( 2 == regOffset ) {
            regs.ENVLP.val = val8;

        } else if( 3 == regOffset ) {
            regs.LFSR.val = val8;

        } else if( 4 == regOffset ) {
            regs.CTRL.val = val8;

        }
    }
};

static PulseChannel ch1(true);
static PulseChannel ch2(false);
static WaveChannel  ch3;
static NoiseChannel ch4;

const RegViewList audioRegView = {
    43,
    NULL,
    REGVIEW_DEFAULT_LINEHEIGHT,
    {
        { NULL, "MASTER REGS", NULL, {0, {}} },
        { &regs.VOLUME.val,    "VOL",   "FF24", {4, {{"LPAN",1},{"LVOL",3},{"RPAN",1},{"RVOL",3},}}},
        { &regs.PAN.val,       "PAN",   "FF25", {8, {{"CH4L",1},{"CH3L",1},{"CH2L",1},{"CH1L",1},{"CH4R",1},{"CH3R",1},{"CH2R",1},{"CH1R",1},}}},
        { &regs.CTRL.val,      "CTRL",  "FF26", {8, {{"EN",1},{"RSVD",3},{"CH4ON",1},{"CH3ON",1},{"CH2ON",1},{"CH1ON",1},}}},

        { NULL, "CHANNEL 1 - PULSE ", NULL, {0, {}} },
        { &ch1.regs.SWEEP.val, "SWEEP", "FF10", {4, {{"RSVD",1},{"PACE",3},{"DIR",1},{"STEP",3},}}},
        { &ch1.regs.TIMER.val, "TIMER", "FF11", {2, {{"DUTY",2},{"LEN",6},}}},
        { &ch1.regs.ENVLP.val, "ENVLP", "FF12", {3, {{"IVOL",4},{"DIR",1},{"PACE",3},}}},
        { &ch1.regs.LPER.val,  "LPER",  "FF13", {1, {{"PERLO",8},}}},
        { &ch1.regs.CTRL.val,  "CTRL",  "FF14", {4, {{"TRIG",1},{"LENEN",1},{"RSVD",3},{"PERHI",3},}}},

        { NULL, "CHANNEL 2 - PULSE", NULL, {0, {}} },
        { &ch2.regs.TIMER.val, "TIMER", "FF16", {2, {{"DUTY",2},{"LEN",6},}}},
        { &ch2.regs.ENVLP.val, "ENVLP", "FF17", {3, {{"IVOL",4},{"DIR",1},{"PACE",3},}}},
        { &ch1.regs.LPER.val,  "LPER",  "FF18", {1, {{"PERLOW",8},}}},
        { &ch1.regs.CTRL.val,  "CTRL",  "FF19", {4, {{"TRIG",1},{"LENEN",1},{"RSVD",3},{"PERHI",3},}}},

        { NULL, "CHANNEL 3 - WAVE", NULL, {0, {}} },
        { &ch3.regs.DAC.val,   "DAC",   "FF1A", {2, {{"EN",1},{"RSVD",7},}}},
        { &ch3.regs.TIMER.val, "TIMER", "FF1B", {1, {{"LEN",8},}}},
        { &ch3.regs.ENVLP.val, "ENVLP", "FF1C", {3, {{"RSVD",1},{"LEVEL",2},{"RSVD",5},}}},
        { &ch3.regs.LPER.val,  "LPER",  "FF1D", {1, {{"PERLO",8},}}},
        { &ch3.regs.CTRL.val,  "CTRL",  "FF1E", {4, {{"TRIG",1},{"LENEN",1},{"RSVD",3},{"PERHI",3},}}},

        { NULL, "CHANNEL 4 - NOISE", NULL, {0, {}} },
        { &ch4.regs.TIMER.val, "TIMER", "FF20", {2, {{"RSVD",2},{"LEN",6},}}},
        { &ch4.regs.ENVLP.val, "ENVLP", "FF21", {3, {{"IVOL",4},{"DIR",1},{"PACE",3},}}},
        { &ch4.regs.LFSR.val,  "LFSR",  "FF22", {3, {{"SHIFT",4},{"WIDTH",1},{"DIV",3},}}},
        { &ch4.regs.CTRL.val,  "CTRL",  "FF23", {3, {{"TRIG",1},{"LENEN",1},{"RSVD",6},}}},

        { NULL, "WAVE TABLE", NULL, {0, {}} },
        { &ch3.waveRam[0].val, "WAV0",  "FF30", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[1].val, "WAV1",  "FF31", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[2].val, "WAV2",  "FF32", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[3].val, "WAV3",  "FF33", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[4].val, "WAV4",  "FF34", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[5].val, "WAV5",  "FF35", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[6].val, "WAV6",  "FF36", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[7].val, "WAV7",  "FF37", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[8].val, "WAV8",  "FF38", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[9].val, "WAV9",  "FF39", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[10].val,"WAV10", "FF3A", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[11].val,"WAV11", "FF3B", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[12].val,"WAV12", "FF3C", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[13].val,"WAV13", "FF3D", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[14].val,"WAV14", "FF3E", {2, {{"NIB0",4},{"NIB1",4},}}},
        { &ch3.waveRam[15].val,"WAV15", "FF3F", {2, {{"NIB0",4},{"NIB1",4},}}},
    }
};

uint8_t getAudioReg8(uint16_t addr)
{
    if( (REG_AUD_CH1_SWEEP <= addr) && (REG_AUD_CH1_CTRL >= addr) ) {
        return ch1.getReg8(addr - REG_AUD_CH1_SWEEP);

    } else if( (REG_AUD_CH2_TIMER <= addr) && (REG_AUD_CH2_CTRL >= addr) ) {
        return ch2.getReg8(addr - 0xFF15);

    } else if( (REG_AUD_CH3_DAC <= addr) && (REG_AUD_CH3_CTRL >= addr) ) {
        return ch3.getReg8(addr - REG_AUD_CH3_DAC);

    } else if( (REG_AUD_CH4_TIMER <= addr) && (REG_AUD_CH4_CTRL >= addr) ) {
        return ch4.getReg8(addr - 0xFF1F);

    } else if( REG_AUD_MAST_VOL == addr ) {   // VOLUME - NR50
        return regs.VOLUME.val;

    } else if( REG_AUD_MAST_PAN == addr ) {   // PAN - NR51
        return regs.PAN.val;

    } else if( REG_AUD_MAST_CTRL == addr ) {   // CONTROL - NR52
        return regs.CTRL.val;

    } else if(  (REG_AUD_CH3_WAV0 <= addr) && (REG_AUD_CH3_WAVF >= addr) ) {
        return ch3.getWave8(addr - REG_AUD_CH3_WAV0);

    } else {
        return UNMAPPED_REG_VAL;
    }
}

void setAudioReg8(uint16_t addr, uint8_t val8)
{
    if( (REG_AUD_CH1_SWEEP <= addr) && (REG_AUD_CH1_CTRL >= addr) ) {
        ch1.setReg8(addr - REG_AUD_CH1_SWEEP, val8);

    } else if( (REG_AUD_CH2_TIMER <= addr) && (REG_AUD_CH2_CTRL >= addr) ) {
        ch2.setReg8(addr - 0xFF15, val8);

    } else if( (REG_AUD_CH3_DAC <= addr) && (REG_AUD_CH3_CTRL >= addr) ) {
        ch3.setReg8(addr - REG_AUD_CH3_DAC, val8);

    } else if( (REG_AUD_CH4_TIMER <= addr) && (REG_AUD_CH4_CTRL >= addr) ) {
        ch4.setReg8(addr - 0xFF1F, val8);

    } else if( REG_AUD_MAST_VOL == addr ) {   // VOLUME - NR50
        regs.VOLUME.val = val8;

    } else if( REG_AUD_MAST_PAN == addr ) {   // PAN - NR51
        regs.PAN.val = val8;

    } else if( REG_AUD_MAST_CTRL == addr ) {   // CONTROL - NR52
        regs.CTRL.val = val8;

    } else if(  (REG_AUD_CH3_WAV0 <= addr) && (REG_AUD_CH3_WAVF >= addr) ) {
        ch3.setWave8(addr - REG_AUD_CH3_WAV0, val8);

    }
}

void guiDrawAudio(void)
{

}

void audioInit(void)
{
    addRegView(&audioRegView, "AUDIO");
}

void audioDeinit(void)
{

}
