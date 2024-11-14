#include "Sensors.hpp"

#include <stdint.h>
#include "driver/i2c.h"

// #define STACK_SIZE (10 * 1024)

static const char* TAG = "Sensors";

esp_err_t Sensors::_fuel_sensor_state = ESP_FAIL;
esp_err_t Sensors::_temp_sensor_state = ESP_FAIL;
esp_err_t Sensors::_is_NMEA200_available = ESP_FAIL;

TimerHandle_t Sensors::_xHandle = NULL;
float  Sensors::_sensorStacks[ENGINE_MAX*2];
PCNT_CHANNEL_s Sensors::_flow_sensors[ENGINE_MAX*2] = {
    { .unit = ENGINE1_SUPPLY_FLOW_UNIT, .gpio = ENGINE1_SUPPLY_FLOW_GPIO },
    { .unit = ENGINE1_RETURN_FLOW_UNIT, .gpio = ENGINE1_RETURN_FLOW_GPIO },
    { .unit = ENGINE2_SUPPLY_FLOW_UNIT, .gpio = ENGINE2_SUPPLY_FLOW_GPIO },
    { .unit = ENGINE2_RETURN_FLOW_UNIT, .gpio = ENGINE2_RETURN_FLOW_GPIO },
};

esp_err_t Sensors::init()
{
    ESP_LOGI(TAG, "Initializing Sensors.");

    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0,
    };
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);

    esp_err_t status = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    
    uint8_t temp;
    _fuel_sensor_state = ads1115RegisterRead(ADS1115_TANK_SENSOR_ADDR, CONVERSION_REGISTER, &temp, 1);
    _temp_sensor_state = ads1115RegisterRead(ADS1115_TEMP_SENSOR_ADDR, CONVERSION_REGISTER, &temp, 1);

    for(int i=0; i<(ENGINE_MAX*2); i++)
    {
        pcntInit(_flow_sensors[i].unit, _flow_sensors[i].gpio);
        _sensorStacks[i] = 0;
        // _sensorStacks[i] = new SensorStack();
    }

    ESP_LOGI(TAG, "Starting Flow Rate Task.");
    _xHandle = xTimerCreate("acquireFuelFlow", (FLOWSENSOR_READING_PERIOD / portTICK_PERIOD_MS),
                                pdTRUE, NULL, Sensors::flowRateTask);

    return status;
}

float Sensors::getTemp(ENGINE_e engine, PROBE_e probe)
{
    float volts;
    float temp;
    uint8_t ads_channel = 0;

    switch(engine)
    {
        case ENGINE_1:
            if(PROBE_1 == probe)        {   ads_channel = ENGINE1_SUPPLY_TEMP_MUX;  }
            else if(PROBE_2 == probe)   {   ads_channel = ENGINE1_RETURN_TEMP_MUX;  }
            else                        {   return 0;   }
            break;
        case ENGINE_2:
            if(PROBE_1 == probe)        {   ads_channel = ENGINE2_SUPPLY_TEMP_MUX;  }
            else if(PROBE_2 == probe)   {   ads_channel = ENGINE2_RETURN_TEMP_MUX;  }
            else                        {   return 0;   }
            break;
        default:
            return 0;
    }

    volts = adsRead(ADS1115_TEMP_SENSOR_ADDR, ads_channel, TEMP_SENSOR_ALERT_GPIO);
    temp = 100 * (volts - 0.5);
    // ESP_LOGI(TAG, "Volts is %f Temp is %f for engine %d mux %x", volts, temp, engine, ads_channel);
    vTaskDelay(1/portTICK_PERIOD_MS);
    return temp;
}

float Sensors::getFuel(TANK_e tank)
{
    float volts;
    float fuel;
    uint8_t ads_channel = 0;

    switch(tank)
    {
        case TANK_1:
            ads_channel = CONFIG_MUX_AIN0;
            break;
        case TANK_2:
            ads_channel = CONFIG_MUX_AIN1;
            break;
        case TANK_3:
            ads_channel = CONFIG_MUX_AIN2;
            break;
        case TANK_4:
            ads_channel = CONFIG_MUX_AIN3;
            break;
        default:
            return 0;
    }

    volts = adsRead(ADS1115_TANK_SENSOR_ADDR, ads_channel, TANK_SENSOR_ALERT_GPIO);
    fuel = ((2.5-volts)/(2.5-0.34)) * 100;          //  (vMAX-v)/(vMAX-vMIN)
    // ESP_LOGI(TAG, "Volts is %f fuel is %f for tank %d mux %x", volts, fuel, tank, ads_channel);
    return fuel;
}

float Sensors::getFlow(ENGINE_e engine, PROBE_e flow)
{
    switch(engine)
    {
        case ENGINE_1:
            if(SUPPLY_FLOW == flow)        {   return _sensorStacks[0];}//Sensors::calculateAverage(_sensorStacks[0]);  }
            else if(RETURN_FLOW == flow)   {   return _sensorStacks[1];}//Sensors::calculateAverage(_sensorStacks[1]);  }
            else                        {   return 0;   }
            break;
        case ENGINE_2:
            if(SUPPLY_FLOW == flow)        {   return _sensorStacks[2];}//Sensors::calculateAverage(_sensorStacks[2]);  }
            else if(RETURN_FLOW == flow)   {   return _sensorStacks[3];}//Sensors::calculateAverage(_sensorStacks[3]);  }
            else                        {   return 0;   }
            break;
        default:
            return 0;
    }
}

bool Sensors::isTempSensorConnected(void)
{
    if(_temp_sensor_state == ESP_OK)
        return true;

    return false;
}

bool Sensors::isFuelSensorConnected(void)
{
    if(_fuel_sensor_state == ESP_OK)
        return true;

    return false;
}

bool Sensors::isNMEA200BUSAvailable(void)
{
    if(_is_NMEA200_available == ESP_OK)
        return true;

    return false;
}

void Sensors::startFlowRateTask(void)
{
    if (_xHandle)
    {
        for(int i=0; i<(ENGINE_MAX*2); i++)
        {
            pcnt_counter_resume(_flow_sensors[i].unit);
            pcnt_counter_clear(_flow_sensors[i].unit);
        }
        xTimerStart(_xHandle, 0);
        ESP_LOGI(TAG, "Flow Rate Task started.");
    }
}

void Sensors::suspendFlowRateTask(void)
{
    if (_xHandle)
    {
        xTimerStop(_xHandle, 0);
        ESP_LOGI(TAG, "Flow Rate Task suspended.");
    }
}

float Sensors::adsRead(uint8_t dev_addr, uint8_t ads_channl, gpio_num_t alert_pin)
{
    uint8_t data[2];
    ads1115RegisterWriteByte(dev_addr, CONFIG_REGISTER, CONFIG_REGISTER_VAL(CONFIG_OS,ads_channl,CONFIG_PGA_4096,CONFIG_MODE,CONFIG_DR_250SPS,CONFIG_COMP_MODE,CONFIG_COMP_POL,CONFIG_COMP_LAT,CONFIG_COMP_QUE));
    // vTaskDelay(10/portTICK_RATE_MS);
    ads1115RegisterRead(dev_addr, CONFIG_REGISTER, data, 2);

    // ESP_LOGE(TAG, "dev_addr = %X", dev_addr);
    // ESP_LOGE(TAG, "CONFIG_REGISTER = %X", CONFIG_REGISTER_VAL(CONFIG_OS,ads_channl,CONFIG_PGA_4096,CONFIG_MODE,CONFIG_DR_250SPS,CONFIG_COMP_MODE,CONFIG_COMP_POL,CONFIG_COMP_LAT,CONFIG_COMP_QUE));
    // ESP_LOGE(TAG, "CONFIG_REGISTER = %X %X", data[0], data[1]);

    do
    {
        vTaskDelay(1/portTICK_RATE_MS);
    }while(gpio_get_level((gpio_num_t)alert_pin));

    ads1115RegisterRead(dev_addr, CONVERSION_REGISTER, data, 2);
    // volts = (data[1] | (data[0] << 8)) * 0.000125;
    return ((data[1] | (data[0] << 8)) * 0.000125);
}

esp_err_t Sensors::ads1115RegisterRead(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

esp_err_t Sensors::ads1115RegisterWriteByte(uint8_t dev_addr, uint8_t reg_addr, uint16_t data)
{
    int ret;
    uint8_t write_buf[3] = {reg_addr, (uint8_t)(data >> 8), (uint8_t)data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);

    return ret;
}

void Sensors::pcntInit(pcnt_unit_t unit, gpio_num_t sigGPIO)
{
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = sigGPIO,
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,
        .lctrl_mode = (pcnt_ctrl_mode_t)PCNT_MODE_KEEP,
        .hctrl_mode = (pcnt_ctrl_mode_t)PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_DIS,
        .neg_mode = PCNT_COUNT_INC,
        .counter_h_lim = PCNT_H_LIM_VAL,
        .counter_l_lim = PCNT_L_LIM_VAL,
        .unit = unit,
        .channel = PCNT_CHANNEL_0,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(unit, 100);
    pcnt_filter_enable(unit);

    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
}

void Sensors::flowRateTask(TimerHandle_t handle)
{
    int16_t counts = 0;
    // ESP_LOGW(TAG, "Running flowRateTask()");
    for(int i=0; i<(ENGINE_MAX*2); i++)
    {
        pcnt_get_counter_value(_flow_sensors[i].unit, &counts);
        pcnt_counter_clear(_flow_sensors[i].unit);
        // pushValue(_sensorStacks[i], counts);
        float pulses_per_second = (float)counts / (FLOWSENSOR_READING_PERIOD / 1000.0);
        _sensorStacks[i] = pulses_per_second * 60.0 * 60.0 * pulseToLiter;              // store value in (litres per hour) unit
        // ESP_LOGW(TAG, "Placed value %d in _sensorStacks[%d]", counts, i);
    }
}

// void Sensors::initStack(SensorStack *stack)
// {
//     stack->head = 0;
//     stack->count = 0;
//     stack->sum = 0;
//     for (int i = 0; i < STACK_DEPTH; i++)
//     {
//         stack->values[i] = 0;
//     }
// }

// void Sensors::pushValue(SensorStack *stack, int16_t new_value)
// {
//     if (stack->count < STACK_DEPTH)
//     {
//         stack->count++;
//     }
//     else
//     {
//         stack->sum -= stack->values[stack->head];
//     }
    
//     stack->values[stack->head] = new_value;
//     stack->sum += new_value;
    
//     stack->head = (stack->head + 1) % STACK_DEPTH;
// }

// float Sensors::calculateAverage(SensorStack *stack)
// {
//     if (stack->count == 0) {
//         return 0; // Return 0 if no values in the stack
//     }
//     return (float)stack->sum / stack->count;
// }
