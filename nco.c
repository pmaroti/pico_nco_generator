
#define I2C0_SDA 12
#define I2C0_SCL 13

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include <ctype.h>
#include <stdio.h>

#include "ssd1306.h"
#include "si5351.h"
#include "rotary_enc.h"



#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0
#define PIN_START 6
#define DELAYUS 10

/* I2C0 pins */


/*

scilab:

round(sin((1:256)/256*6.28)*127+128)'

*/

static unsigned char sintbl[] = { 
  131, 134, 137, 140, 144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174, 177, 179, 
  182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216, 218, 220, 222, 
  224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 
  250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 
  254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 247, 245, 244, 243, 241, 240, 239, 
  237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 
  201, 199, 196, 193, 191, 188, 185, 182, 180, 177, 174, 171, 168, 165, 162, 159, 156, 
  153, 150, 147, 144, 141, 138, 134, 131, 128, 125, 122, 119, 116, 113, 110, 106, 103, 
  100,  97,  94,  91,  88,  85,  83,  80,  77,  74,  71,  68,  66,  63,  60,  58,  55,
   53,  50,  48,  45,  43,  41,  38,  36,  34,  32,  30,  28,  26,  24,  23,  21,  19,
   18,  16,  15,  13,  12,  11,  10,   9,   7,   7,   6,   5,   4,   3,   3,   2,   2,
    2,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   3,   3,   4,   5,   6,   6,
    7,   8,   9,  11,  12,  13,  14,  16,  17,  19,  21,  22,  24,  26,  28,  30,  32,
   34,  36,  38,  40,  42,  45,  47,  50,  52,  55,  57,  60,  62,  65,  68,  71,  73,
   76,  79,  82,  85,  88,  91,  94,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124, 128 };

static volatile int idx = 0;

/*

https://en.wikipedia.org/wiki/Numerically_controlled_oscillator

Fclk = 1 / DELAYUS     # 100kHz
freq = Fclk / (2^N / fcw)    # 100000 / (65536/328) -> 500 Hz

fcw = f*2^N / Fclk    # 500*65536/100000 -> 327.68

fcw = f*2^N / Fclk    # 8200*65536/100000 -> 5373.952

*/

static volatile uint16_t frequency;

static volatile uint16_t fcw = 328;

static volatile uint16_t accumulator = 0x0;


ssd1306_t disp;

static void alarm_irq(void) {
    unsigned char amplitude;
    accumulator = accumulator + fcw ;

    uint16_t idx = accumulator >> 8 ;
    
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    amplitude = sintbl[idx] >> 2;
    pwm_set_gpio_level(PIN_START,128 + amplitude);
    pwm_set_gpio_level(PIN_START+1,128 - amplitude);

    timer_hw->alarm[ALARM_NUM]= timer_hw->timerawl + DELAYUS;
}

static void alarm_in_us() {
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);
    // Enable interrupt in block and at processor

    // Alarm is only 32 bits so if trying to delay more
    // than that need to be careful and keep track of the upper
    // bits
    timer_hw->alarm[ALARM_NUM]= timer_hw->timerawl + DELAYUS;
}


void init_pwm() {
  gpio_set_function(PIN_START, GPIO_FUNC_PWM);
  gpio_set_drive_strength(PIN_START, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_function(PIN_START+1, GPIO_FUNC_PWM);
  gpio_set_drive_strength(PIN_START+1, GPIO_DRIVE_STRENGTH_12MA);  

  const uint16_t pwm_max = 255; // 8 bit pwm
  const int magnitude_pwm_slice = pwm_gpio_to_slice_num(PIN_START); // GPIO1
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 2.f); // 125MHz
  pwm_config_set_wrap(&config, pwm_max);
  pwm_config_set_output_polarity(&config, false, false);
  pwm_init(magnitude_pwm_slice, &config, true);
}


void setup_gpios(void) {
  i2c_init(i2c0, 400000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  gpio_pull_up(4);
  gpio_pull_up(5);  
}


void init_display() {
  disp.external_vcc=false;
  ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
  sleep_ms(1);
  ssd1306_clear(&disp);

}

void display_freq() {
  char frequency_string[15];
  ssd1306_clear(&disp);
  sprintf(frequency_string, "%d", frequency);
  ssd1306_draw_string(&disp, 8, 24, 1, frequency_string);
  ssd1306_show(&disp);
}


void setup_si5351() {
  si5351_init(0x60, SI5351_CRYSTAL_LOAD_8PF, 26000000, 0, i2c1); // I am using a 26 MHz TCXO
  si5351_set_clock_pwr(SI5351_CLK0, 0); // safety first

  si5351_drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351_drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);

  si5351_set_freq(14074000ULL * 100ULL, SI5351_CLK1);
  si5351_output_enable(SI5351_CLK0, 0); // TX off
  si5351_output_enable(SI5351_CLK1, 1); // RX on
  si5351_output_enable(SI5351_CLK2, 1); // RX IF on  
}


void setup_rotary_encoder() {
  rotary_init(1, 0);
}


int main() {
  stdio_init_all();

  setup_gpios();

  setup_si5351();
  setup_rotary_encoder();

  init_pwm();
  init_display();
  display_freq();
  alarm_in_us();
  while (true) {
    int command = getchar_timeout_us(0);
    if (command != PICO_ERROR_TIMEOUT) {
      printf("command: %c\r\n", command);
      switch (command) {
      case 'f':
        frequency = 0;
        while(1)
        {
          char c = getchar();
          if(!isdigit(c)) break;
          frequency *= 10;
          frequency += (c - '0');
        }
        printf("terminate: %c\r\n");
        printf("frequency: %u Hz\r\n", frequency);
        fcw =  ((float)frequency) * 65536.0 / 100000.0;
        display_freq();
        break;
      default:
        printf("f [frequency]\r\n");
        break;
      }
    }
    if (poll_rotary()) {
      frequency += get_rotation();
      set_rotation(0);
      fcw =  ((float)frequency) * 65536.0 / 100000.0;
      display_freq();
    }
    sleep_us(1000);
  }
}