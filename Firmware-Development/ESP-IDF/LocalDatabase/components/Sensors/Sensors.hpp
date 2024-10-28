#ifndef SENSORS
#define SENSORS

#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "hal/gpio_types.h"


#define I2C_MASTER_SCL_IO           GPIO_NUM_22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_21      /*!< GPIO number used for I2C master data  */

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define ADS1115_TANK_SENSOR_ADDR                 0x49        /*!< Slave address of the ADS1115 sensor */
#define TANK_SENSOR_ALERT_GPIO           GPIO_NUM_33
#define ADS1115_TEMP_SENSOR_ADDR                 0x48        /*!< Slave address of the ADS1115 sensor */
#define TEMP_SENSOR_ALERT_GPIO           GPIO_NUM_32

#define CONVERSION_REGISTER                 0X00        /*!< Register holding ADC conversion code */
#define CONFIG_REGISTER                     0X01        /*!< Configuration Register */
#define LOW_THRESHOLD_REGISTER              0X02        /*!< Register holding ADC conversion code */
#define HIGH_THRESHOLD_REGISTER             0X03        /*!< Register holding ADC conversion code */

#define CONFIG_OS                   (0x0)
#define CONFIG_MUX_AIN0             (0X4)
#define CONFIG_MUX_AIN1             (0X5)
#define CONFIG_MUX_AIN2             (0X6)
#define CONFIG_MUX_AIN3             (0X7)
#define CONFIG_PGA_6144             (0X0)
#define CONFIG_PGA_4096             (0X1)
#define CONFIG_PGA_2048             (0X2)
#define CONFIG_MODE                 (0X0)       /*!< 0 FOR CONTINUOUS MODE, 1 FOR SINGLE-SHOT */

#define CONFIG_DR_8SPS              (0X0)
#define CONFIG_DR_16SPS             (0X1)
#define CONFIG_DR_32SPS             (0X2)
#define CONFIG_DR_64SPS             (0X3)
#define CONFIG_DR_128SPS            (0X4)
#define CONFIG_DR_250SPS            (0X5)
#define CONFIG_DR_475SPS            (0X6)
#define CONFIG_DR_860SPS            (0X7)
#define CONFIG_COMP_MODE            (0X0)          /*!< 0 FOR TRADITIONAL COMPARATOR (DEFAULT), 1 FOR WINDOW COMPARATOR*/
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
}PROBE_e;
typedef enum
{
    ENGINE_1 = 1,
    ENGINE_2,
}ENGINE_e;

class Sensors
{
    public:
        static esp_err_t sensorsInit(void);
        static float     getTemp(ENGINE_e engine, PROBE_e probe);
        static float     getFuel(ENGINE_e engine, PROBE_e probe);

    private:
        static float     ads_read(uint8_t dev_addr, uint8_t ads_channl, gpio_num_t alert_pin);

        static esp_err_t ads1115_register_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);
        static esp_err_t ads1115_register_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint16_t data);
};

#endif