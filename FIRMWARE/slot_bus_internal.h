#ifndef SLOT_BUS_INTERNAL_H
#define SLOT_BUS_INTERNAL_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/gpio_coproc.h"

#define SLOT_BUS_OK 0
#define SLOT_BUS_FAIL -1

#define SLOT_DATA_MASK ((uint32_t)0xff << 8)
#define SLOT_WR_MASK ((uint32_t)1 << 5)
#define SLOT_RD_MASK ((uint32_t)1 << 6)
#define SLOT_MRQ_MASK ((uint32_t)1 << 4)
#define SLOT_IO_MASK ((uint32_t)1 << 3)
#define SLOT_M1_MRQ_MASK ((uint32_t)5 << 2)
#define SLOT_ADDRESS_MASK ((uint32_t)0xffff << 8)
#define SLOT_SELECT_MASK ((uint32_t)0x3f << 24)

extern bool sltAcc;
extern bool p6_16kbMode;
extern int wrWait;
extern int rdWait;
extern int memWait;

int powerCheck(void);

static inline void slot_hi_put_hiz(uint32_t mask, uint32_t value) {
    gpioc_hi_out_xor((gpioc_hi_out_get() ^ value) & mask);
    gpioc_hi_oe_xor((gpioc_hi_oe_get() ^ (~value)) & mask);
}

static inline void slot_lo_put_hiz(uint32_t mask, uint32_t value) {
    gpioc_lo_out_xor((gpioc_lo_out_get() ^ value) & mask);
    gpioc_lo_oe_xor((gpioc_lo_oe_get() ^ (~value)) & mask);
}

static inline void slot_hi_put_push_pull(uint32_t mask, uint32_t value) {
    gpioc_hi_out_xor((gpioc_hi_out_get() ^ value) & mask);
    gpioc_hi_oe_set(mask);
}

static inline uint32_t slot_read_select_value(uint8_t slot, uint16_t address) {
    uint32_t value = SLOT_SELECT_MASK;
    if (slot == 1) {
        if (p6_16kbMode) {
            value = ((address >= 0x6000) && (address < 0x8000)) ?
                    ((uint32_t)0x37 << 24) : ((uint32_t)0x3e << 24);
        } else if ((address >= 0x4000) && (address < 0x8000)) {
            value = (uint32_t)0x38 << 24;
        } else if ((address >= 0x8000) && (address < 0xc000)) {
            value = (uint32_t)0x34 << 24;
        } else {
            value = (uint32_t)0x3e << 24;
        }
    }
    return value;
}

static inline uint32_t slot_write_select_value(uint8_t slot, uint16_t address) {
    uint32_t value = SLOT_SELECT_MASK;
    if (slot == 1) {
        if (p6_16kbMode) {
            value = ((address >= 0x6000) && (address < 0x8000)) ?
                    ((uint32_t)0x37 << 24) : ((uint32_t)0x3e << 24);
        } else {
            value = (uint32_t)0x3e << 24;
        }
    }
    return value;
}

#endif
