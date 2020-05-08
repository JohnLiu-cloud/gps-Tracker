#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

// LEDs definitions for PCA10040
#define LEDS_NUMBER    0

#define LED_START      0
#define LED_1          0
#define LED_2          0
#define LED_3          0
#define LED_4          0
#define LED_STOP       0

#define LEDS_ACTIVE_STATE 0

#define LEDS_INV_MASK  LEDS_MASK

#define LEDS_LIST { LED_1, LED_2, LED_3, LED_4 }

#define BSP_LED_0      LED_1
#define BSP_LED_1      LED_2
#define BSP_LED_2      LED_3
#define BSP_LED_3      LED_4

#define BUTTONS_NUMBER 3

#define BUTTON_START   13
#define BUTTON_1       13
#define BUTTON_2       14
#define BUTTON_3       15
#define BUTTON_STOP    15
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1, BUTTON_2, BUTTON_3 }

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2
#define BSP_BUTTON_2   BUTTON_3

#define RX_PIN_NUMBER  17
#define TX_PIN_NUMBER  16
#define CTS_PIN_NUMBER 19
#define RTS_PIN_NUMBER 18
#define HWFC           true

#define MODULE_POWER_ON 5

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_BOARD_H
