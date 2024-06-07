#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"

#include "pio_rotary_encoder.pio.h"


/*! \brief Set specified GPIO to be pulled up.
 *  \ingroup rotary_lib
 *
 * \param pio pio device
 * \param sm statemachine number
 * \param rotPinNr rotaryA number (B is the consecutive one)
 * \param swPinNr switch number (B is the consecutive one)
 */
bool rotary_init(PIO, uint8_t, uint, uint);
bool poll_rotary(void);
int get_rotation(void);
void set_rotation(int);

