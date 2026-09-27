#ifndef FACTORY_TEST_INTERNAL_H
#define FACTORY_TEST_INTERNAL_H

#include "pico/stdlib.h"
#include "ports.h"

#define FACTORY_TEST_OK 0
#define FACTORY_TEST_FAIL -1

typedef struct {
    uint gpio_out;
    uint gpio_check;
} factory_gpio_pair_t;

void cdc_printf(const char *fmt, ...);

static inline const char *factory_gpio_signal_name(uint gpio) {
    for (size_t i = 0; i < NUM_PINS; i++) {
        if (board_pins[i].gpio_num == (int)gpio) return board_pins[i].name;
    }
    return "";
}

#endif
