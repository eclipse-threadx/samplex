#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpio_setpinmux(uint8_t io_pad, uint8_t mux);
uint8_t gpio_getpinmux(uint8_t io_pad);
void gpio_set(unsigned long mask);
void gpio_clear(unsigned long mask);
void gpio_toggle(unsigned long mask);
void gpio_pin_set_dir(uint8_t pin, uint8_t mode);
uint32_t gpio_pin_read_status(uint8_t pin);

#endif
