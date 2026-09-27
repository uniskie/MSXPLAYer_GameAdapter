#include "slot_bus.h"
#include "slot_bus_internal.h"

#define BUFDIR_PIN 30
#define REV_F_SELECT_MASK ((uint32_t)0x0f << 24)
#define REV_F_RD_WR_MASK (((uint32_t)1 << (37 - 32)) | ((uint32_t)1 << (38 - 32)))

static inline void data_input(void) {
    gpioc_hi_oe_clr(SLOT_DATA_MASK);
    gpio_put(BUFDIR_PIN, 0);
}

static inline void data_output(uint8_t data) {
    gpioc_hi_out_xor((gpioc_hi_out_get() ^ ((uint32_t)data << 8)) & SLOT_DATA_MASK);
    gpioc_hi_oe_set(SLOT_DATA_MASK);
    gpio_put(BUFDIR_PIN, 1);
}

static void init_pins(uint64_t out_mask) {
    gpio_put_masked64(out_mask, 0x81);
    gpio_set_oeover(7, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(24, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(25, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(26, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(27, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(37, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(38, GPIO_OVERRIDE_NORMAL);
    for (uint gpio = 40; gpio <= 47; gpio++) gpio_pull_up(gpio);
    data_input();
}

static void control_write(uint64_t mask, uint64_t value) {
    uint32_t lo_mask = (uint32_t)mask;
    uint32_t lo_value = (uint32_t)value;
    uint32_t hi_mask = (uint32_t)(mask >> 32);
    uint32_t hi_value = (uint32_t)(value >> 32);

    slot_lo_put_hiz(lo_mask & ~REV_F_SELECT_MASK, lo_value);
    gpioc_lo_out_xor((gpioc_lo_out_get() ^ lo_value) & lo_mask & REV_F_SELECT_MASK);
    gpioc_lo_oe_set(lo_mask & REV_F_SELECT_MASK);
    slot_hi_put_hiz(hi_mask & ~REV_F_RD_WR_MASK, hi_value);
    slot_hi_put_push_pull(hi_mask & REV_F_RD_WR_MASK, hi_value);
}

static inline void select_write(uint32_t value) {
    gpioc_lo_out_xor((gpioc_lo_out_get() ^ value) & REV_F_SELECT_MASK);
    gpioc_lo_oe_set(REV_F_SELECT_MASK);
}

static int __not_in_flash_func(m1_read)(uint8_t slot, uint16_t address, uint8_t *data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_M1_MRQ_MASK, 0);
    data_input();
    select_write(slot_read_select_value(slot, address));
    slot_hi_put_push_pull(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(rdWait);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_push_pull(SLOT_RD_MASK, SLOT_RD_MASK);
    select_write(SLOT_SELECT_MASK);
    slot_hi_put_hiz(SLOT_M1_MRQ_MASK, SLOT_M1_MRQ_MASK);
    *data = (uint8_t)((gpio_hi >> 8) & 0xff);
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    busy_wait_at_least_cycles(memWait);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(mem_read)(uint8_t slot, uint16_t address, uint8_t *data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_MRQ_MASK, 0);
    data_input();
    select_write(slot_read_select_value(slot, address));
    slot_hi_put_push_pull(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(rdWait);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_push_pull(SLOT_RD_MASK, SLOT_RD_MASK);
    select_write(SLOT_SELECT_MASK);
    slot_hi_put_hiz(SLOT_MRQ_MASK, SLOT_MRQ_MASK);
    *data = (uint8_t)((gpio_hi >> 8) & 0xff);
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    busy_wait_at_least_cycles(memWait);
    sltAcc = false;
    return SLOT_BUS_OK;
}

static int __not_in_flash_func(mem_write)(uint8_t slot, uint16_t address, uint8_t data) {
    sltAcc = true;
    if (powerCheck() != SLOT_BUS_OK) return SLOT_BUS_FAIL;
    slot_hi_put_push_pull(SLOT_RD_MASK, SLOT_RD_MASK);
    slot_lo_put_hiz(SLOT_ADDRESS_MASK, (uint32_t)address << 8);
    slot_hi_put_hiz(SLOT_MRQ_MASK, 0);
    data_output(data);
    select_write(slot_write_select_value(slot, address));
    busy_wait_at_least_cycles(36);
    slot_hi_put_push_pull(SLOT_WR_MASK, 0);
    busy_wait_at_least_cycles(wrWait);
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_hi_put_push_pull(SLOT_RD_MASK, SLOT_RD_MASK);
    select_write(SLOT_SELECT_MASK);
    slot_hi_put_hiz(SLOT_MRQ_MASK, SLOT_MRQ_MASK);
    data_input();
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
    slot_hi_put_push_pull(SLOT_RD_MASK, 0);
    busy_wait_at_least_cycles(72);
    uint32_t gpio_hi = gpioc_hi_in_get();
    slot_hi_put_push_pull(SLOT_RD_MASK, SLOT_RD_MASK);
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
    slot_hi_put_push_pull(SLOT_WR_MASK, 0);
    busy_wait_at_least_cycles(18);
    slot_hi_put_push_pull(SLOT_WR_MASK, SLOT_WR_MASK);
    slot_hi_put_hiz(SLOT_IO_MASK, SLOT_IO_MASK);
    data_input();
    sltAcc = false;
    return SLOT_BUS_OK;
}

const slot_bus_ops_t slot_bus_rev_f_ops = {
    .m1_read = m1_read,
    .mem_read = mem_read,
    .mem_write = mem_write,
    .io_read = io_read,
    .io_write = io_write,
    .data_input = data_input,
    .init_pins = init_pins,
    .control_write = control_write,
};
