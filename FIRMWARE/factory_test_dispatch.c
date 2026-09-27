#include "factory_test.h"

extern int factory_gpio_test_b2(void);
extern int factory_gpio_test_rev_f(void);

static int (*factory_gpio_test_impl)(void) = factory_gpio_test_b2;

void factory_test_select(bool rev_f) {
    factory_gpio_test_impl = rev_f ? factory_gpio_test_rev_f : factory_gpio_test_b2;
}

int factory_gpio_test(void) {
    return factory_gpio_test_impl();
}
