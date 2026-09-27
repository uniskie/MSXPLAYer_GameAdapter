#ifndef SLOT_BUS_H
#define SLOT_BUS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int (*m1_read)(uint8_t slot, uint16_t address, uint8_t *data);
    int (*mem_read)(uint8_t slot, uint16_t address, uint8_t *data);
    int (*mem_write)(uint8_t slot, uint16_t address, uint8_t data);
    int (*io_read)(uint16_t address, uint8_t *data);
    int (*io_write)(uint16_t address, uint8_t data);
    void (*data_input)(void);
    void (*init_pins)(uint64_t out_mask);
    void (*control_write)(uint64_t mask, uint64_t value);
} slot_bus_ops_t;

extern const slot_bus_ops_t slot_bus_b2_ops;
extern const slot_bus_ops_t slot_bus_rev_f_ops;

void slot_bus_select(bool rev_f);
void slot_bus_data_input(void);
void slot_bus_init_pins(uint64_t out_mask);
void slot_bus_control_write(uint64_t mask, uint64_t value);

int slotM1ReadData(uint8_t slot, uint16_t address, uint8_t *data);
int slotReadData(uint8_t slot, uint16_t address, uint8_t *data);
int slotWriteData(uint8_t slot, uint16_t address, uint8_t data);
int slotReadIO(uint16_t address, uint8_t *data);
int slotWriteIO(uint16_t address, uint8_t data);

#endif
