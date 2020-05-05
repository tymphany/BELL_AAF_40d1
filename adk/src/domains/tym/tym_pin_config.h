#ifndef __SINK_TYM_PIN_CONFIG__
#define __SINK_TYM_PIN_CONFIG__

#define POWER_PIN                0
/* No configuration RST_PIN */
#define RST_PIN                  1
/* configuration at fw_cfg_filesystem subsys3_config1.htf
 * hex format 2 3
 * DigMic0PioConfig = [clock data]
 * DigMic0PioConfig = [2 3]
 */
#define MIC_CLK_PIN              2
/* configuration at fw_cfg_filesystem subsys3_config1.htf
 * hex format 2 3
 * DigMic0PioConfig = [clock data]
 * DigMic0PioConfig = [2 3]
 */
#define MIC_DATA_PIN             3
/* function : GPIO output.H:enable mic power and I2C_3V3 ,L:disable mic power and I2C_3V3*/
#define ENABLE_PIN               4
/* function: GPIO ,L:commu, H:USB , set:L =>commu*/
#define MODE_SWITCH_PIN          5
/* function : TRBI200  */
#define TBR_MOSI                 6
#define TBR_MISO                 7
#define TBR_CLK                  8

/* function : THERMAL_SENSOR  Power Driver Output*/
/* setting ADK Configuration Tool configuration set-> Battery->Battery Temperature->Thermistor ADC configuration->Driver Thermistor Output PIO -> PIO15 */
#define THERMAL_SENSOR_PIN      15
/* setting ADK Configuration Tool configuration set-> Battery->Battery Temperature->Thermistor ADC configuration->Vthm ADC souce -> LED(5)*/
/* THERMAL NTC LED(5) */
/* function : I2C SCL */
#define I2C_SCL_PIN             16
/* function : I2C SDA */
#define I2C_SDA_PIN             17
/* function : GPIO input */
#define IQS_RDY_PIN             18

/* function : GPIO output*/
#define STATUS_PIN              20
/* function : GPIO input */
#define IR_PAUSE_PIN            21

/*function : GPIO Output */
#define TOUCH_POWER             19
/*function : GPIO Input */
#define BUD_DETECT_PIN          66 /*AIO[0]*/
/*function : GPIO INPUT */
#define IQS_HOLD_PIN            70 /*aio[4]*/

#endif
