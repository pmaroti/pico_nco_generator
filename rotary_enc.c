#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"

#include "pio_rotary_encoder.pio.h"
#include "rotary_enc.h"

PIO rotary_pio;
uint rotary_sm;
static int rotation;
static bool rotary_changed;
static bool sw_changed;


static void pio_irq_handler() {
    // test if irq 0 was raised
    if (pio0_hw->irq & 1)
    {
        rotation = rotation - 1;
    }
    // test if irq 1 was raised
    if (pio0_hw->irq & 2)
    {
        rotation = rotation + 1;
    }

    rotary_changed = true;

    // clear both interrupts
    pio0_hw->irq = 3;
}

void sw_callback(uint gpio, uint32_t events) {
    sw_changed = true;
}



bool rotary_init(PIO pio, uint8_t sm, uint rotary_encoder_A, uint sw_pin) {

    uint8_t rotary_encoder_B = rotary_encoder_A + 1;
    rotary_pio = pio;
    rotary_sm = sm;

    // configure the used pins as input with pull down
    pio_gpio_init(rotary_pio, rotary_encoder_A);
    gpio_set_pulls(rotary_encoder_A, false, true);
    pio_gpio_init(rotary_pio, rotary_encoder_B);
    gpio_set_pulls(rotary_encoder_B, false, true);

    // load the pio program into the pio memory
    uint offset = pio_add_program(rotary_pio, &pio_rotary_encoder_program);

    // make a sm config
    pio_sm_config c = pio_rotary_encoder_program_get_default_config(offset);

    // set the 'in' pins
    sm_config_set_in_pins(&c, rotary_encoder_A);
    // set shift to left: bits shifted by 'in' enter at the least
    // significant bit (LSB), no autopush
    sm_config_set_in_shift(&c, false, false, 0);

    // set the IRQ handler
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq_handler);
    // enable the IRQ
    irq_set_enabled(PIO0_IRQ_0, true);
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS | PIO_IRQ0_INTE_SM1_BITS;
    // init the sm.
    // Note: the program starts after the jump table -> initial_pc = 16
    pio_sm_init(rotary_pio, rotary_sm, 16, &c);
    // enable the sm
    pio_sm_set_enabled(rotary_pio, rotary_sm, true);
    rotary_changed = false;

    //handling rotary switch
    gpio_init(sw_pin);
    gpio_set_dir(sw_pin, GPIO_IN);
    gpio_pull_down(sw_pin);
    gpio_set_irq_enabled_with_callback(sw_pin, GPIO_IRQ_EDGE_RISE, true, &sw_callback);

}

// set the current rotation to a specific value
void set_rotation(int _rotation) {
    rotation = _rotation;
}

// get the current rotation
int get_rotation(void) {
    return rotation;
}

bool poll_rotary(void) {
    if (rotary_changed) {
        rotary_changed = false;
        return true;
    }
    return false;
}

bool poll_sw(void) {
    if (sw_changed) {
        sw_changed = false;
        return true;
    }
    return false;
}