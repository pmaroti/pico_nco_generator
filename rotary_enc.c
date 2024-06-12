#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "rotary_enc.h"

static int rotation;
static bool rotary_changed;
static bool sw_changed;
static uint8_t rotary_encoder_A;
static uint8_t rotary_encoder_B;
static uint8_t rotary_switch;


static void hmi_callback(uint gpio, uint32_t events) {
  rotary_changed = true;
  if (gpio == rotary_encoder_A) {
    rotary_changed = true;
    rotation += gpio_get(rotary_encoder_B) ? 1 : -1;
  } else if (gpio == rotary_switch) {
    sw_changed = true;
  }
}

bool rotary_init(uint rotary_encoder_A_pin, uint sw_pin) {

    rotary_encoder_A = rotary_encoder_A_pin;
    rotary_encoder_B = rotary_encoder_A + 1;
    rotary_switch = sw_pin;

    // configure the used pins as input with pull down
    gpio_init(rotary_encoder_A);
    gpio_set_dir(rotary_encoder_A, GPIO_IN);
    gpio_pull_down(sw_pin);
    gpio_set_irq_enabled(rotary_encoder_A,  GPIO_IRQ_EDGE_RISE, true);

    gpio_init(rotary_encoder_B);
    gpio_set_dir(rotary_encoder_B, GPIO_IN);
    gpio_pull_down(rotary_encoder_B);


    gpio_init(sw_pin);
    gpio_set_dir(sw_pin, GPIO_IN);
    gpio_pull_down(sw_pin);
    gpio_set_irq_enabled(sw_pin, GPIO_IRQ_EDGE_RISE, true);    

    gpio_set_irq_enabled_with_callback(rotary_encoder_A, GPIO_IRQ_EDGE_RISE, true, hmi_callback);

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