#include "factory_test_internal.h"
#include "slot_bus.h"

static const factory_gpio_pair_t gpio_pairs[] = {
    {7, 31}, {24, 32}, {25, 33}, {26, 34}, {27, 35}, {37, 8}, {38, 9},
    {18, 21}, {19, 22}, {20, 36}, {23, 39}
};

static const factory_gpio_pair_t buffer_pairs[] = {
    {40, 10}, {41, 11}, {42, 12}, {43, 13},
    {44, 14}, {45, 15}, {46, 16}, {47, 17}
};

static inline bool output_only(uint gpio) {
    return gpio == 7 || (gpio >= 24 && gpio <= 27) || gpio == 37 || gpio == 38;
}

int factory_gpio_test_rev_f(void) {
    const int pair_count = sizeof(gpio_pairs) / sizeof(gpio_pairs[0]);
    const int buffer_count = sizeof(buffer_pairs) / sizeof(buffer_pairs[0]);
    cdc_printf("REV_F GPIO Pair List\n");
    for (int i = 0; i < pair_count; i++) {
        cdc_printf("GPIO%d(%s)-GPIO%d(%s)\n", gpio_pairs[i].gpio_out,
                   factory_gpio_signal_name(gpio_pairs[i].gpio_out), gpio_pairs[i].gpio_check,
                   factory_gpio_signal_name(gpio_pairs[i].gpio_check));
    }
    for (int i = 0; i < buffer_count; i++) {
        cdc_printf("GPIO%d(%s)-GPIO%d(%s) [BUFDIR]\n", buffer_pairs[i].gpio_out,
                   factory_gpio_signal_name(buffer_pairs[i].gpio_out), buffer_pairs[i].gpio_check,
                   factory_gpio_signal_name(buffer_pairs[i].gpio_check));
    }
    cdc_printf("\nSwitch GPIO7-47 to test mode...\n");

    for (uint pin = 7; pin <= 47; pin++) {
        if (pin >= 28 && pin <= 30) continue;
        gpio_init(pin);
        if (output_only(pin)) {
            gpio_put(pin, 1);
            gpio_set_dir(pin, GPIO_OUT);
        } else {
            gpio_pull_up(pin);
            gpio_set_dir(pin, GPIO_IN);
        }
    }
    gpio_put(30, 0);
    cdc_printf("Complete\n\n");
    busy_wait_ms(10);
    cdc_printf("Verify all GPIO7-47 are high (1)...\n");
    for (uint pin = 7; pin <= 47; pin++) {
        if (pin >= 28 && pin <= 30) continue;
        if (!gpio_get(pin)) {
            cdc_printf("Error: GPIO%d = 0 (Expected: 1)\n", pin);
            cdc_printf("Initial GPIO Check Failed\n\n");
            return FACTORY_TEST_FAIL;
        }
    }
    cdc_printf("Complete: All GPIO are high\n\n");
    for (int i = 0; i < pair_count; i++) {
        uint pin_out = gpio_pairs[i].gpio_out;
        uint pin_check = gpio_pairs[i].gpio_check;
        cdc_printf("========================================\n");
        cdc_printf("Test %d/%d: GPIO%d(%s)-GPIO%d(%s)\n", i + 1, pair_count,
                   pin_out, factory_gpio_signal_name(pin_out), pin_check,
                   factory_gpio_signal_name(pin_check));
        cdc_printf("========================================\n");
        cdc_printf("Switch GPIO%d to output mode and set to 0...\n", pin_out);
        gpio_put(pin_out, 0);
        gpio_set_dir(pin_out, GPIO_OUT);
        busy_wait_ms(5);
        if (gpio_get(pin_out) != 0) {
            cdc_printf("Error: GPIO%d output value is not 0\n", pin_out);
            cdc_printf("Test %d Failed: Step 3\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Set Complete\n", pin_out);
        cdc_printf("Verify GPIO%d is low (0)...\n", pin_check);
        if (gpio_get(pin_check) != 0) {
            cdc_printf("Error: GPIO%d Expected: 0 (Actual: 1)\n", pin_check);
            cdc_printf("Test %d Failed: Step 4-1\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Verified\n", pin_check);
        cdc_printf("Verify GPIO%d, GPIO%d and others are high (1)...\n", pin_out, pin_check);
        for (uint pin = 7; pin <= 47; pin++) {
            if ((pin >= 28 && pin <= 30) || pin == pin_out || pin == pin_check) continue;
            if (!gpio_get(pin)) {
                cdc_printf("[STEP 4] Error: GPIO%d = 0 (Expected: 1)\n", pin);
                cdc_printf("Test %d Failed: Step 4-2\n\n", i + 1);
                return FACTORY_TEST_FAIL;
            }
        }
        cdc_printf("GPIO%d, GPIO%d and others all high verified\n", pin_out, pin_check);
        gpio_put(pin_out, 1);
        if (output_only(pin_out)) {
            cdc_printf("Restore GPIO%d to high output...\n", pin_out);
        } else {
            cdc_printf("Switch GPIO%d to input mode...\n", pin_out);
            gpio_set_dir(pin_out, GPIO_IN);
        }
        cdc_printf("Complete\nTest %d: Success\n\n", i + 1);
    }

    cdc_printf("========================================\n");
    cdc_printf("BUFDIR Check 1: GPIO40-47 input\n");
    cdc_printf("========================================\n");
    cdc_printf("Set BUFDIR to input direction...\n");
    gpio_put(30, 0);
    cdc_printf("Complete\n\n");
    for (int i = 0; i < buffer_count; i++) {
        uint pin_buf = buffer_pairs[i].gpio_out;
        uint pin_pair = buffer_pairs[i].gpio_check;
        cdc_printf("Test %d/%d: GPIO%d(%s) -> GPIO%d(%s)\n", i + 1, buffer_count,
                   pin_pair, factory_gpio_signal_name(pin_pair), pin_buf,
                   factory_gpio_signal_name(pin_buf));
        cdc_printf("Verify GPIO%d is high (1)...\n", pin_buf);
        if (!gpio_get(pin_buf)) {
            cdc_printf("Error: GPIO%d Expected: 1 (Actual: 0)\n", pin_buf);
            cdc_printf("BUFDIR Check 1 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("Switch GPIO%d to output mode and set to 0...\n", pin_pair);
        gpio_put(pin_pair, 0);
        gpio_set_dir(pin_pair, GPIO_OUT);
        busy_wait_ms(5);
        if (gpio_get(pin_buf) != 0) {
            cdc_printf("Error: GPIO%d Expected: 0 (Actual: 1)\n", pin_buf);
            cdc_printf("BUFDIR Check 1 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Verified\n", pin_buf);
        cdc_printf("Restore GPIO%d to input mode...\n", pin_pair);
        gpio_put(pin_pair, 1);
        gpio_set_dir(pin_pair, GPIO_IN);
        busy_wait_ms(5);
        if (!gpio_get(pin_buf)) {
            cdc_printf("Error: GPIO%d Expected: 1 (Actual: 0)\n", pin_buf);
            cdc_printf("BUFDIR Check 1 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 1 Verified\n", pin_buf);
        cdc_printf("Test %d: Success\n\n", i + 1);
    }
    cdc_printf("BUFDIR Check 1: Success\n");

    cdc_printf("========================================\n");
    cdc_printf("BUFDIR Check 2: GPIO40-47 output\n");
    cdc_printf("========================================\n");
    cdc_printf("Switch GPIO40-47 to output mode and set to 1...\n");
    for (uint pin = 40; pin <= 47; pin++) {
        gpio_disable_pulls(pin);
        gpio_put(pin, 1);
        gpio_set_dir(pin, GPIO_OUT);
    }
    cdc_printf("Set BUFDIR to output direction...\n");
    gpio_put(30, 1);
    cdc_printf("Complete\n\n");
    for (int i = 0; i < buffer_count; i++) {
        uint pin_buf = buffer_pairs[i].gpio_out;
        uint pin_pair = buffer_pairs[i].gpio_check;
        cdc_printf("Test %d/%d: GPIO%d(%s) -> GPIO%d(%s)\n", i + 1, buffer_count,
                   pin_buf, factory_gpio_signal_name(pin_buf), pin_pair,
                   factory_gpio_signal_name(pin_pair));
        cdc_printf("Verify GPIO%d is high (1)...\n", pin_pair);
        if (!gpio_get(pin_pair)) {
            cdc_printf("Error: GPIO%d Expected: 1 (Actual: 0)\n", pin_pair);
            cdc_printf("BUFDIR Check 2 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("Set GPIO%d to 0...\n", pin_buf);
        gpio_put(pin_buf, 0);
        busy_wait_ms(5);
        if (gpio_get(pin_pair) != 0) {
            cdc_printf("Error: GPIO%d Expected: 0 (Actual: 1)\n", pin_pair);
            cdc_printf("BUFDIR Check 2 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 0 Verified\n", pin_pair);
        cdc_printf("Set GPIO%d to 1...\n", pin_buf);
        gpio_put(pin_buf, 1);
        busy_wait_ms(5);
        if (!gpio_get(pin_pair)) {
            cdc_printf("Error: GPIO%d Expected: 1 (Actual: 0)\n", pin_pair);
            cdc_printf("BUFDIR Check 2 Failed: Test %d\n\n", i + 1);
            return FACTORY_TEST_FAIL;
        }
        cdc_printf("GPIO%d = 1 Verified\n", pin_pair);
        cdc_printf("Test %d: Success\n\n", i + 1);
    }
    slot_bus_data_input();
    cdc_printf("BUFDIR Check 2: Success\n\n");
    cdc_printf("========================================\n");
    cdc_printf("REV_F GPIO Test Finished\n");
    cdc_printf("========================================\n\n");
    return FACTORY_TEST_OK;
}
