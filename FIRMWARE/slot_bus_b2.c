#include "slot_bus.h"
#include "slot_bus_internal.h"

static inline void data_input(void) {
    gpioc_hi_oe_clr(SLOT_DATA_MASK);
}

static inline void data_output(uint8_t data) {
    gpioc_hi_oe_set(SLOT_DATA_MASK);
    slot_hi_put_hiz(SLOT_DATA_MASK, (uint32_t)data << 8);
}

static void init_pins(uint64_t out_mask) {
    gpio_put_masked64(out_mask, 0x81);
    data_input();
}

static void control_write(uint64_t mask, uint64_t value) {
    slot_lo_put_hiz((uint32_t)mask, (uint32_t)value);
    slot_hi_put_hiz((uint32_t)(mask >> 32), (uint32_t)(value >> 32));
}

static inline void select_assert(uint32_t value) {
    slot_lo_put_hiz(SLOT_SELECT_MASK, value);
}

static inline void select_latch_release(void) {
    gpioc_lo_out_xor((gpioc_lo_out_get() ^ SLOT_SELECT_MASK) & SLOT_SELECT_MASK);
}

static inline void select_oe_release(void) {
    gpioc_lo_oe_xor((gpioc_lo_oe_get() ^ (~SLOT_SELECT_MASK)) & SLOT_SELECT_MASK);
}

static int __not_in_flash_func(m1_read)(uint8_t slot, uint16_t address, uint8_t *data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    gpioc_hi_oe_set(SLOT_WR_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_M1_MRQ_MASK, 0);
    data_input();
    select_assert(slot_read_select_value(slot, address));
    slot_hi_put_hiz(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(rdWait);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_hiz(SLOT_RD_MASK, SLOT_RD_MASK);
    select_latch_release();
    slot_hi_put_hiz(SLOT_M1_MRQ_MASK, SLOT_M1_MRQ_MASK);
    *data = (uint8_t)((gpio_hi >> 8) & 0xff);
    select_oe_release();
    slot_hi_put_hiz(SLOT_WR_MASK, SLOT_WR_MASK);
    busy_wait_at_least_cycles(memWait);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(mem_read)(uint8_t slot, uint16_t address, uint8_t *data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    gpioc_hi_oe_set(SLOT_WR_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_MRQ_MASK, 0);
    data_input();
    select_assert(slot_read_select_value(slot, address));
    slot_hi_put_hiz(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(rdWait);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_hiz(SLOT_RD_MASK, SLOT_RD_MASK);
    select_latch_release();
    slot_hi_put_hiz(SLOT_MRQ_MASK, SLOT_MRQ_MASK);
    *data = (uint8_t)((gpio_hi >> 8) & 0xff);
    select_oe_release();
    slot_hi_put_hiz(SLOT_WR_MASK, SLOT_WR_MASK);
    busy_wait_at_least_cycles(memWait);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(mem_write)(uint8_t slot, uint16_t address, uint8_t data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    gpioc_hi_oe_set(SLOT_RD_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_MRQ_MASK, 0);
    data_output(data);
    select_assert(slot_write_select_value(slot, address));
    busy_wait_at_least_cycles(36);
    slot_hi_put_hiz(SLOT_WR_MASK, 0);
    busy_wait_at_least_cycles(wrWait);
    slot_hi_put_hiz(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_hi_put_hiz(SLOT_RD_MASK, SLOT_RD_MASK);
    slot_lo_put_hiz(SLOT_SELECT_MASK, SLOT_SELECT_MASK);
    slot_hi_put_hiz(SLOT_MRQ_MASK, SLOT_MRQ_MASK);
    data_input();
    slot_hi_put_hiz(SLOT_RD_MASK, SLOT_RD_MASK);
    busy_wait_at_least_cycles(memWait);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(io_read)(uint16_t address, uint8_t *data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_IO_MASK, 0);
    data_input();
    slot_hi_put_hiz(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(72);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_hiz(SLOT_RD_MASK, SLOT_RD_MASK);
    slot_hi_put_hiz(SLOT_IO_MASK, SLOT_IO_MASK);
    *data = (uint8_t)((gpio_hi >> 8) & 0xff);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(io_write)(uint16_t address, uint8_t data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_IO_MASK, 0);
    data_output(data);
    busy_wait_at_least_cycles(36);
    slot_hi_put_hiz(SLOT_WR_MASK, 0);
    busy_wait_at_least_cycles(18);
    slot_hi_put_hiz(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_hi_put_hiz(SLOT_IO_MASK, SLOT_IO_MASK);
    data_input();
    sltAcc = false;
    return SLOT_BUS_OK;
}

const slot_bus_ops_t slot_bus_b2_ops = {
    .m1_read = m1_read,
    .mem_read = mem_read,
    .mem_write = mem_write,
    .io_read = io_read,
    .io_write = io_write,
    .data_input = data_input,
    .init_pins = init_pins,
    .control_write = control_write,
};
