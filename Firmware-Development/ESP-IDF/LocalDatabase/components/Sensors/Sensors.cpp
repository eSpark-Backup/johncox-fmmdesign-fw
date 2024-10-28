#include "Sensors.hpp"

#include <stdint.h>
#include "driver/i2c.h"


esp_err_t Sensors::sensorsInit()
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0,
        // .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

float Sensors::getTemp(ENGINE_e engine, PROBE_e probe)
{
    float volts;
    float temp;
    uint8_t ads_channel = 0;

    switch(engine)
    {
        case ENGINE_1:
            if(PROBE_1 == probe)        {   ads_channel = CONFIG_MUX_AIN0;  }
            else if(PROBE_2 == probe)   {   ads_channel = CONFIG_MUX_AIN1;  }
            else                        {   return 0;   }
            break;
        case ENGINE_2:
            if(PROBE_1 == probe)        {   ads_channel = CONFIG_MUX_AIN2;  }
            else if(PROBE_2 == probe)   {   ads_channel = CONFIG_MUX_AIN3;  }
            else                        {   return 0;   }
            break;
        default:
            return 0;
    }

    volts = ads_read(ADS1115_TEMP_SENSOR_ADDR, ads_channel, TEMP_SENSOR_ALERT_GPIO);
    temp = 100 * (volts - 0.5);
    return temp;
}

float Sensors::getFuel(ENGINE_e engine, PROBE_e probe)
{
    float volts;
    float fuel;
    uint8_t ads_channel = 0;

    switch(engine)
    {
        case ENGINE_1:
            if(PROBE_1 == probe)        {   ads_channel = CONFIG_MUX_AIN0;  }
            else if(PROBE_2 == probe)   {   ads_channel = CONFIG_MUX_AIN1;  }
            else                        {   return 0;   }
            break;
        case ENGINE_2:
            if(PROBE_1 == probe)        {   ads_channel = CONFIG_MUX_AIN2;  }
            else if(PROBE_2 == probe)   {   ads_channel = CONFIG_MUX_AIN3;  }
            else                        {   return 0;   }
            break;
        default:
            return 0;
    }

    volts = ads_read(ADS1115_TANK_SENSOR_ADDR, ads_channel, TANK_SENSOR_ALERT_GPIO);
    fuel = ((2.5-volts)/(2.5-0.34)) * 100;          //  (vMAX-v)/(vMAX-vMIN)
    return fuel;
}

float Sensors::ads_read(uint8_t dev_addr, uint8_t ads_channl, gpio_num_t alert_pin)
{
    uint8_t data[2];
    ESP_ERROR_CHECK(ads1115_register_write_byte(dev_addr, CONFIG_REGISTER, CONFIG_REGISTER_VAL(CONFIG_OS,ads_channl,CONFIG_PGA_4096,CONFIG_MODE,CONFIG_DR_250SPS,CONFIG_COMP_MODE,CONFIG_COMP_POL,CONFIG_COMP_LAT,CONFIG_COMP_QUE)));
    // vTaskDelay(10/portTICK_RATE_MS);
    ESP_ERROR_CHECK(ads1115_register_read(dev_addr, CONFIG_REGISTER, data, 2));

    // ESP_LOGI(TAG, "CONFIG_REGISTER = %X %X", data[0], data[1]);

    do
    {
        vTaskDelay(1/portTICK_RATE_MS);
    }while(gpio_get_level((gpio_num_t)alert_pin));

    ESP_ERROR_CHECK(ads1115_register_read(dev_addr, CONVERSION_REGISTER, data, 2));
    // volts = (data[1] | (data[0] << 8)) * 0.000125;
    return ((data[1] | (data[0] << 8)) * 0.000125);
}

esp_err_t Sensors::ads1115_register_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

esp_err_t Sensors::ads1115_register_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint16_t data)
{
    int ret;
    uint8_t write_buf[3] = {reg_addr, (uint8_t)(data >> 8), (uint8_t)data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);

    return ret;
}