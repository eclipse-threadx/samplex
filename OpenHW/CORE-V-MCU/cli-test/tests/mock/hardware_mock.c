#include "gpio.h"

void gpio_setpinmux(uint8_t io_pad, uint8_t mux)
{
    (void)io_pad;
    (void)mux;
}

uint8_t gpio_getpinmux(uint8_t io_pad)
{
    (void)io_pad;
    return 0U;
}

void gpio_set(unsigned long mask)
{
    (void)mask;
}

void gpio_clear(unsigned long mask)
{
    (void)mask;
}

void gpio_toggle(unsigned long mask)
{
    (void)mask;
}

void gpio_pin_set_dir(uint8_t pin, uint8_t mode)
{
    (void)pin;
    (void)mode;
}

uint32_t gpio_pin_read_status(uint8_t pin)
{
    (void)pin;
    return 0U;
}
