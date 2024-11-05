#ifndef SENSORS
#define SENSORS

#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "driver/pcnt.h"
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

/*
*   P80 flow meter resolution = 1.5 mL
*       flow rate (min) = 0.05 L/min    (or)    0.833  mL/s
*       flow rate (max) =   16 L/min    (or)    266.66 mL/s
*
*       pulse/s (min) = 0.8333/1.5 = 0.555
*       pulse/s (max) = 266.66/1.5 = 177.77
*/
#define FLOWSENSOR_READING_PERIOD   4000             // take reading every 4 secs
#define FLOWSENSOR_LEAST_COUNT      1.5f
#define PCNT_L_LIM_VAL              0
#define PCNT_H_LIM_VAL              INT16_MAX
#define pulseToLiter                (FLOWSENSOR_LEAST_COUNT / 1000.0)
// #define STACK_DEPTH                 (60/(FLOWSENSOR_READING_PERIOD/1000))

#define ENGINE1_SUPPLY_FLOW_UNIT    PCNT_UNIT_0
#define ENGINE1_SUPPLY_FLOW_GPIO    GPIO_NUM_25
#define ENGINE1_SUPPLY_TEMP_MUX     CONFIG_MUX_AIN0
#define ENGINE1_RETURN_FLOW_UNIT    PCNT_UNIT_1
#define ENGINE1_RETURN_FLOW_GPIO    GPIO_NUM_26
#define ENGINE1_RETURN_TEMP_MUX     CONFIG_MUX_AIN1
#define ENGINE2_SUPPLY_FLOW_UNIT    PCNT_UNIT_2
#define ENGINE2_SUPPLY_FLOW_GPIO    GPIO_NUM_27
#define ENGINE2_SUPPLY_TEMP_MUX     CONFIG_MUX_AIN2
#define ENGINE2_RETURN_FLOW_UNIT    PCNT_UNIT_3
#define ENGINE2_RETURN_FLOW_GPIO    GPIO_NUM_18
#define ENGINE2_RETURN_TEMP_MUX     CONFIG_MUX_AIN3

#define I2C_MASTER_SCL_IO           GPIO_NUM_22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_21      /*!< GPIO number used for I2C master data  */

#define I2C_MASTER_NUM              0                /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000           /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1

#define ADS1115_TANK_SENSOR_ADDR    0x49             /*!< Slave address of the ADS1115 sensor */
#define TANK_SENSOR_ALERT_GPIO      GPIO_NUM_33
#define ADS1115_TEMP_SENSOR_ADDR    0x48             /*!< Slave address of the ADS1115 sensor */
#define TEMP_SENSOR_ALERT_GPIO      GPIO_NUM_32

#define CONVERSION_REGISTER         0X00             /*!< Register holding ADC conversion code */
#define CONFIG_REGISTER             0X01             /*!< Configuration Register */
#define LOW_THRESHOLD_REGISTER      0X02             /*!< Register holding ADC conversion code */
#define HIGH_THRESHOLD_REGISTER     0X03             /*!< Register holding ADC conversion code */

#define CONFIG_OS                   (0x0)
#define CONFIG_MUX_AIN0             (0X4)
#define CONFIG_MUX_AIN1             (0X5)
#define CONFIG_MUX_AIN2             (0X6)
#define CONFIG_MUX_AIN3             (0X7)
#define CONFIG_PGA_6144             (0X0)
#define CONFIG_PGA_4096             (0X1)
#define CONFIG_PGA_2048             (0X2)
#define CONFIG_MODE                 (0X0)            /*!< 0 FOR CONTINUOUS MODE, 1 FOR SINGLE-SHOT */

#define CONFIG_DR_8SPS              (0X0)
#define CONFIG_DR_16SPS             (0X1)
#define CONFIG_DR_32SPS             (0X2)
#define CONFIG_DR_64SPS             (0X3)
#define CONFIG_DR_128SPS            (0X4)
#define CONFIG_DR_250SPS            (0X5)
#define CONFIG_DR_475SPS            (0X6)
#define CONFIG_DR_860SPS            (0X7)
#define CONFIG_COMP_MODE            (0X0)            /*!< 0 FOR TRADITIONAL COMPARATOR (DEFAULT), 1 FOR WINDOW COMPARATOR*/
#define CONFIG_COMP_POL             (0X0)
#define CONFIG_COMP_LAT             (0X1)
#define CONFIG_COMP_QUE             (0X2)

#define CONFIG_REGISTER_VAL(OS,MUX,PGA,MODE,DR,COMP_MODE,COMP_POL,COMP_LAT,COMP_QUE)  \
                           (0XFFFF & ((OS << 15) | (MUX << 12) | (PGA << 9) | (MODE << 8) |  \
                           (DR << 5) | (COMP_MODE << 4) | (COMP_POL << 3) | (COMP_LAT << 2) | (COMP_QUE)))

typedef enum
{
    PROBE_1,
    PROBE_2,
    SUPPLY_FLOW,
    RETURN_FLOW,
}PROBE_e;
typedef enum
{
    ENGINE_1,
    ENGINE_2,
    ENGINE_MAX,
}ENGINE_e;
typedef enum
{
    TANK_1,
    TANK_2,
    TANK_3,
    TANK_4,
}TANK_e;
typedef struct
{
    pcnt_unit_t unit;
    gpio_num_t gpio;
}PCNT_CHANNEL_s;

// struct SensorStackStruct
// {
//     SensorStackStruct(void)
//     {
//         head = 0;
//         count = 0;
//         sum = 0;
//         for (int i = 0; i < STACK_DEPTH; i++)
//         {
//             values[i] = 0;
//         }
//     }

//     int16_t values[STACK_DEPTH];  // Array to hold sensor values
//     int head;                 // Points to the current insert position
//     int count;                // Number of items currently in the stack
//     int sum;                  // Running sum of all items for quick average calculation
// };
// typedef SensorStackStruct SensorStack;

class Sensors
{
    public:
        static esp_err_t sensorsInit(void);
        static float     getTemp(ENGINE_e engine, PROBE_e probe);
        static float     getFuel(TANK_e tank);
        static float     getFlow(ENGINE_e engine, PROBE_e flow);

        static bool      isTempSensorConnected(void);
        static bool      isFuelSensorConnected(void);
        static bool      isNMEA200BUSAvailable(void);

        static void      startFlowRateTask(void);
        static void      suspendFlowRateTask(void);
    private:
        static float     adsRead(uint8_t dev_addr, uint8_t ads_channl, gpio_num_t alert_pin);

        static esp_err_t ads1115RegisterRead(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);
        static esp_err_t ads1115RegisterWriteByte(uint8_t dev_addr, uint8_t reg_addr, uint16_t data);

        static void      pcntInit(pcnt_unit_t unit, gpio_num_t sigGPIO);
        static void      flowRateTask(TimerHandle_t handle);
        // void  initStack(SensorStack *stack);
        // static void  pushValue(SensorStack *stack, int16_t new_value);
        // static float calculateAverage(SensorStack *stack);

        static esp_err_t _fuel_sensor_state, _temp_sensor_state;
        static esp_err_t _is_NMEA200_available;
        static TimerHandle_t _xHandle;

        // static SensorStack   *_sensorStacks[ENGINE_MAX*2];
        static float     _sensorStacks[ENGINE_MAX*2];
        static PCNT_CHANNEL_s _flow_sensors[ENGINE_MAX*2];
};

#endif