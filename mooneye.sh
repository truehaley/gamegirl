#!/bin/bash
TEST_ROOT="tmp/mts"
currentTest=0

if [ $# -eq 1 ]; then
    echo "Starting with test number $1"
    startingTest=$1
else
    echo "Running all tests"
    startingTest=0
fi

function run_test() {
    ((currentTest++))
    if [ $startingTest -gt $currentTest ]; then
        # skip tests up until the specified one
        return
    fi
    echo "running test $currentTest: $1"
    output=$(bin/Debug/gamegirl "$TEST_ROOT/$1" --mooneye --run --fastboot --exitbreak 2>&1)
    if [ $? -ne 42 ]; then
        echo "FAILED!"
        echo "Output from command:"
        echo "==========================================================="
        echo "$output"
        echo "==========================================================="
        echo "Execute this command to re-run:"
        echo "    bin/Debug/gamegirl $TEST_ROOT/$1"
        echo "Execute this command to continue:"
        ((nextTest=currentTest+1))
        echo "    $0 $nextTest"
        exit
    fi
}

function run_acceptance() {
    echo "acceptance tests"
    #run_test acceptance/add_sp_e_timing.gb         # FIXME
    #run_test acceptance/boot_div-S.gb                      # SGB
    #run_test acceptance/boot_div-dmg0.gb                   # DMG0
    #run_test acceptance/boot_div-dmgABCmgb.gb      # FIXME
    #run_test acceptance/boot_div2-S.gb                     # SGB
    #run_test acceptance/boot_hwio-S.gb                     # SGB
    #run_test acceptance/boot_hwio-dmg0.gb                  # DMG0
    #run_test acceptance/boot_hwio-dmgABCmgb.gb     # FIXME
    #run_test acceptance/boot_regs-dmg0.gb                  # DMG0
    run_test acceptance/boot_regs-dmgABC.gb
    #run_test acceptance/boot_regs-mgb.gb                   # MGB
    #run_test acceptance/boot_regs-sgb.gb                   # SGB
    #run_test acceptance/boot_regs-sgb2.gb                  # SGB
    run_test acceptance/call_cc_timing.gb           # FIXME
    run_test acceptance/call_cc_timing2.gb          # FIXME
    run_test acceptance/call_timing.gb              # FIXME
    run_test acceptance/call_timing2.gb             # FIXME
    run_test acceptance/di_timing-GS.gb
    run_test acceptance/div_timing.gb
    run_test acceptance/ei_sequence.gb
    run_test acceptance/ei_timing.gb
    run_test acceptance/halt_ime0_ei.gb
    run_test acceptance/halt_ime0_nointr_timing.gb
    run_test acceptance/halt_ime1_timing.gb
    run_test acceptance/halt_ime1_timing2-GS.gb
    run_test acceptance/if_ie_registers.gb
    run_test acceptance/intr_timing.gb
    run_test acceptance/jp_cc_timing.gb             # FIXME
    run_test acceptance/jp_timing.gb                # FIXME
    run_test acceptance/ld_hl_sp_e_timing.gb        # FIXME
    run_test acceptance/pop_timing.gb
    run_test acceptance/push_timing.gb              # FIXME
    run_test acceptance/rapid_di_ei.gb
    run_test acceptance/ret_cc_timing.gb            # FIXME
    run_test acceptance/ret_timing.gb               # FIXME
    run_test acceptance/reti_intr_timing.gb
    run_test acceptance/reti_timing.gb              # FIXME
    run_test acceptance/rst_timing.gb               # FIXME

    #tmp/mts/acceptance/bits:
    run_test acceptance/bits/mem_oam.gb
    run_test acceptance/bits/reg_f.gb
    run_test acceptance/bits/unused_hwio-GS.gb      # FIXME

    #tmp/mts/acceptance/instr:
    run_test acceptance/instr/daa.gb

    #tmp/mts/acceptance/interrupts:
    run_test acceptance/interrupts/ie_push.gb       # FIXME

    #tmp/mts/acceptance/serial:
    run_test /acceptance/serial/boot_sclk_align-dmgABCmgb.gb    # FIXME

}

function run_acceptance_timer() {
    echo "timer tests"
    run_test acceptance/timer/div_write.gb
    run_test acceptance/timer/rapid_toggle.gb
    run_test acceptance/timer/tim00.gb
    run_test acceptance/timer/tim00_div_trigger.gb
    run_test acceptance/timer/tim01.gb
    run_test acceptance/timer/tim01_div_trigger.gb
    run_test acceptance/timer/tim10.gb
    run_test acceptance/timer/tim10_div_trigger.gb
    run_test acceptance/timer/tim11.gb
    run_test acceptance/timer/tim11_div_trigger.gb
    run_test acceptance/timer/tima_reload.gb
    run_test acceptance/timer/tima_write_reloading.gb
    run_test acceptance/timer/tma_write_reloading.gb
}

function run_acceptance_ppu() {
    echo "ppu tests"
    run_test acceptance/ppu/hblank_ly_scx_timing-GS.gb      # FIXME
    run_test acceptance/ppu/intr_1_2_timing-GS.gb
    run_test acceptance/ppu/intr_2_0_timing.gb
    run_test acceptance/ppu/intr_2_mode0_timing.gb          # FIXME
    run_test acceptance/ppu/intr_2_mode0_timing_sprites.gb  # FIXME
    run_test acceptance/ppu/intr_2_mode3_timing.gb          # FIXME
    run_test acceptance/ppu/intr_2_oam_ok_timing.gb         # FIXME
    run_test acceptance/ppu/lcdon_timing-GS.gb              # FIXME
    run_test acceptance/ppu/lcdon_write_timing-GS.gb        # FIXME
    run_test acceptance/ppu/stat_irq_blocking.gb
    run_test acceptance/ppu/stat_lyc_onoff.gb               # FIXME
    run_test acceptance/ppu/vblank_stat_intr-GS.gb          # FIXME

    #tmp/mts/acceptance/oam_dma:
    run_test acceptance/oam_dma/basic.gb
    run_test acceptance/oam_dma/reg_read.gb
    #run_test acceptance/oam_dma/sources-GS.gb      # FIXME

    run_test acceptance/oam_dma_restart.gb          # FIXME
    run_test acceptance/oam_dma_start.gb            # FIXME
    run_test acceptance/oam_dma_timing.gb           # FIXME

}

function run_mbc1() {
    echo "mbc1 tests"
    run_test emulator-only/mbc1/bits_bank1.gb
    run_test emulator-only/mbc1/bits_bank2.gb
    run_test emulator-only/mbc1/bits_mode.gb
    run_test emulator-only/mbc1/bits_ramg.gb
    run_test emulator-only/mbc1/multicart_rom_8Mb.gb
    run_test emulator-only/mbc1/ram_256kb.gb
    run_test emulator-only/mbc1/ram_64kb.gb
    run_test emulator-only/mbc1/rom_16Mb.gb
    run_test emulator-only/mbc1/rom_1Mb.gb
    run_test emulator-only/mbc1/rom_2Mb.gb
    run_test emulator-only/mbc1/rom_4Mb.gb
    run_test emulator-only/mbc1/rom_512kb.gb
    run_test emulator-only/mbc1/rom_8Mb.gb
}

function run_mbc2() {
    echo "mbc2 tests"
    run_test emulator-only/mbc2/bits_ramg.gb
    run_test emulator-only/mbc2/bits_romb.gb
    run_test emulator-only/mbc2/bits_unused.gb
    run_test emulator-only/mbc2/ram.gb
    run_test emulator-only/mbc2/rom_1Mb.gb
    run_test emulator-only/mbc2/rom_2Mb.gb
    run_test emulator-only/mbc2/rom_512kb.gb
}

function run_mbc5() {
    echo "mbc5 tests"
    run_test emulator-only/mbc5/rom_16Mb.gb
    run_test emulator-only/mbc5/rom_1Mb.gb
    run_test emulator-only/mbc5/rom_2Mb.gb
    run_test emulator-only/mbc5/rom_32Mb.gb
    run_test emulator-only/mbc5/rom_4Mb.gb
    run_test emulator-only/mbc5/rom_512kb.gb
    run_test emulator-only/mbc5/rom_64Mb.gb
    run_test emulator-only/mbc5/rom_8Mb.gb
}

function run_madness() {
    echo "madness tests"
    run_test madness/mgb_oam_dma_halt_sprites.gb    # FIXME
}

function run_manual() {
    echo "manual tests"
    run_test manual-only/sprite_priority.gb
}

function run_misc() {
    echo "misc tests"
    #run_test misc/boot_div-A.gb        # AGB
    #run_test misc/boot_div-cgb0.gb     # CGB
    #run_test misc/boot_div-cgbABCDE.gb # CGB
    #run_test misc/boot_hwio-C.gb       # CGB
    #run_test misc/boot_regs-A.gb       # AGB
    #run_test misc/boot_regs-cgb.gb     # CGB

    #run_test misc/bits/unused_hwio-C.gb        # CGB

    #run_test misc/ppu/vblank_stat_intr-C.gb    #CGB
}

function run_utils() {
    echo "utils are not tests"
    #run_test utils/bootrom_dumper.gb
    #run_test utils/dump_boot_hwio.gb
}



run_acceptance
run_acceptance_timer
run_acceptance_ppu
run_mbc1
# run_mbc2      # not yet
# run_mbc5      # not yet
run_madness
# run_manual    # manual only
# run_misc       # none apply (CGB and AGB only)
# run_utils      # Not Tests

echo "*********** Tests Complete *************"
