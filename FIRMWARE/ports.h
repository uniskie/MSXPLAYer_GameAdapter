#ifndef PORTS_H
#define PORTS_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DIR_INPUT,
    DIR_OUTPUT
} PinDirection;

typedef struct {
    int  gpio_num;
    PinDirection dir;
    const char* name;
} PinDef;

extern PinDef board_pins[];   // 実体は ports.c に定義
extern const size_t NUM_PINS; // 実体は ports.c に定義

#endif // PORTS_H