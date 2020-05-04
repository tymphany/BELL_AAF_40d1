/* Copyright (c) 2017 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file 
 * LED configure private header file.
 */

#ifndef LED_CFG_PRIVATE_H
#define LED_CFG_PRIVATE_H

#include "hydra_log/hydra_log.h"
#include "led_cfg/led_cfg.h"
#include "mmu/mmu.h"

#include <led.h>
#include "trap_api/trap_api.h"
#include "ipc/ipc.h"
#include "hal/halauxio.h"


#define LED_MAX_DUTY_CYCLE_VALUE 0xFFF
#define LED_MAX_PERIOD_VALUE 0xF
#define LED_MAX_PWM_HIGH (0xFFFF >> 1)
#define LED_MAX_PWM_LOW (0xFFFF >> 1)
#define LED_MAX_PWM_PERIOD (LED_MAX_PWM_HIGH + LED_MAX_PWM_LOW)
#define LED_MAX_FLASH_RATE 0xF
#define MAX_LOGARITHMIC_MODE_OFFSET 0xF
#define NUM_LED_CTRL_INST (NUM_OF_LED_CTRLS)

typedef struct client_led_config {
    uint32 groups;
    uint16 duty_cycle;
    uint16 low_duty_cycle;
    led_config driver_config;
    uint8 period;
    unsigned int invert:1;
    unsigned int flashing:1;
} client_led_config;

/**
 * @brief Check if the LED controller is clocked. If not then we can't access
 * the registers. The clock is turned on by the Curator when a PIO is muxed to
 * the LED controller and turned off in deep sleep if there is no PIO muxed to
 * the LED controller.
 * @return TRUE if LED controller is clocked, FALSE otherwise.
 */
bool led_cfg_is_clocked(void);

/**
 * @brief Debugging function to dump LED driver configuration.
 * @param led LED index.
 * @param led_conf Pointer to configuration buffer.
 */
void led_cfg_log_config(led_id led, client_led_config *led_conf);

#endif /* LED_CFG_PRIVATE_H */
