/* N2KStreamESP32.h
 * Copyright (c) 2015-2024 Timo Lappalainen, Kave Oy, www.kave.fi
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

/**************************************************************************//**
 * \file  N2KStreamESP32.h
 * \brief This File contains an I/O stream used in the NMEA2000 libraries.
 * 
 * 
 * Arduino users get away with using the standard Stream class and its
 * subclasses. Forward declare the Stream class here and include Arduino.h in
 * the application.  (\#define ARDUINO)
 *  
 * Non Arduino platforms need to implement this themselves if they want to use
 * functions which operate on streams.
 * 
 */

#ifndef _tN2kStreamESP32_H_
#define _tN2kStreamESP32_H_

#include "N2kDef.h"
#include <stdint.h>
#include <stddef.h>
#include <cstring>
#include "driver/uart.h"
#include "driver/gpio.h"

#include "N2kStream.h"

// #define UART_PORT UART_NUM_0 // Choose the UART port
#define TX_PIN GPIO_NUM_1            // Set TX pin
#define RX_PIN GPIO_NUM_3            // Set RX pin
#define BUF_SIZE 1024        // Buffer size

class N2KStreamESP32 : public N2kStream {
public:
    N2KStreamESP32(int uart_port): _uart_port(uart_port) {
        // Configure UART parameters
        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        };
        uart_param_config(_uart_port, &uart_config);
        uart_set_pin(_uart_port, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(_uart_port, BUF_SIZE, 0, 0, NULL, 0);
    }

    virtual int read() override {
        uint8_t data;
        int len = uart_read_bytes(_uart_port, &data, 1, 20 / portTICK_RATE_MS);
        return (len > 0) ? data : -1;
    }

    virtual int peek() override {
        uint8_t data;
        int len = uart_read_bytes(_uart_port, &data, 1, 1);  // Peek without advancing the buffer
        if (len > 0) {
            uart_flush_input(_uart_port);  // Reset buffer to maintain peek
            uart_write_bytes(_uart_port, (const char*)&data, 1); // Reinsert the data
        }
        return (len > 0) ? data : -1;
    }

    virtual size_t write(const uint8_t* data, size_t size) override {
        return uart_write_bytes(_uart_port, (const char*)data, size);
    }

    size_t print(const char* str) {
        return write(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }

    size_t println(const char* str) {
        size_t n = print(str);
        n += print("\r\n");
        return n;
    }

    size_t print(int val, uint8_t radix = 10) {
        char buf[32];
        itoa(val, buf, radix);
        return print(buf);
    }

    size_t println(int val, uint8_t radix = 10) {
        size_t n = print(val, radix);
        n += print("\r\n");
        return n;
    }
private:
    int _uart_port;
};

#endif
