#include "factory_test_internal.h"

static const factory_gpio_pair_t gpio_pairs[] = {
    {7, 9}, {8, 10}, {11, 13}, {12, 14}, {15, 17}, {16, 18}, {19, 21},
    {20, 22}, {23, 25}, {24, 26}, {27, 32}, {31, 33}, {34, 36}, {35, 37},
    {38, 40}, {39, 41}, {42, 45}, {43, 46}, {44, 47}
};

int factory_gpio_test_b2(void) {
    const int pair_count = sizeof(gpio_pairs) / sizeof(gpio_pairs[0]);
    cdc_printf("REV_B2 GPIO Pair List\n");
    for (int i = 0; i < pair_count; i++) {
        cdc_printf("GPIO%d(%s)-GPIO%d(%s)\n", gpio_pairs[i].gpio_out,
                   factory_gpio_signal_name(gpio_pairs[i].gpio_out), gpio_pairs[i].gpio_check,
                   factory_gpio_signal_name(gpio_pairs[i].gpio_check));
    }
    cdc_printf("\nSwitch GPIO7-47 to input mode...\n");
    for (uint pin = 7; pin <= 47; pin++) {
        if (pin >= 28 && pin <= 30) continue;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
    }
    cdc_printf("Complete\n\n");
    busy_wait_ms(10);

    for (int i = 0; i < pair_count; i++) {
        uint pin_out = gpio_pairs[i].gpio_out;
        uint pin_check = gpio_pairs[i].gpio_check;
        cdc_printf("========================================\n");
        cdc_printf("Test %d/%d: GPIO%d(%s)-GPIO%d(%s)\n", i + 1, pair_count,
                   pin_out, factory_gpio_signal_name(pin_out), pin_check,
                   factory_gpio_signal_name(pin_check));
        cdc_printf("========================================\n");
        cdc_printf("Verify all GPIO7-47 are high (1)...\n");
        for (uint pin = 7; pin <= 47; pin++) {
            if (pin >= 28 && pin <= 30) continue;
            if (!gpio_get(pin)) {
                cdc_printf("Error: GPIO%d = 0 (Expected: 1)\n", pin);
                cdc_printf("Test %d Failed: Step 2\n\n", i + 1);
                return FACTORY_TEST_FAIL;
            }
        }
        cdc_printf("Complete: All GPIO are high\n");
        busy_wait_ms(50);
        cdc_printf("Switch GPIO%d to output mode and set to 0...\n", pin_out);
        gpio_init(pin_out);
        gpio_set_dir(pin_out, GPIO_OUT);
        gpio_put(pin_out, 0);
        busy_wait_ms(5);
        if (gpio_get(pin_out) != 0) {
            cdc_printf("Error: GPIO%d output value is not 0\n", pin_out);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Set Complete\n", pin_out);
        cdc_printf("Verify GPIO%d is low (0)...\n", pin_check);
        if (gpio_get(pin_check) != 0) {
            cdc_printf("Error: GPIO%d Expected: 0 (Actual: 1)\n", pin_check);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Verified\n", pin_check);
        for (uint pin = 7; pin <= 47; pin++) {
            if ((pin >= 28 && pin <= 30) || pin == pin_out || pin == pin_check) continue;
            if (!gpio_get(pin)) {
                cdc_printf("[STEP 4] Error: GPIO%d = 0 (Expected: 1)\n", pin);
                return FACTORY_TEST_FAIL;
            }
        }
        cdc_printf("GPIO%d, GPIO%d and others all high verified\n", pin_out, pin_check);
        cdc_printf("Switch GPIO%d to input mode...\n", pin_out);
        gpio_init(pin_out);
        gpio_set_dir(pin_out, GPIO_IN);
        cdc_printf("Complete\nTest %d: Success\n\n", i + 1);
    }
    return FACTORY_TEST_OK;
}
