/*
 * NMEA2000_CAN.h
 *
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

/***********************************************************************//**
  \file   NMEA2000_CAN.h
  \brief  Automatic library selection according to the selected board

  Just include this <NMEA2000_CAN.h> to your project and it will
  automatically select suitable CAN library according to board
  selected on Arduino development environment. You can still force
  library by adding one of next defines before including NMEA2000_CAN.h:
 
  \code{cpp}
  #define USE_N2K_CAN 1  // for use with SPI and MCP2515 can bus controller
  #define USE_N2K_CAN 2  // for use with due based CAN
  #define USE_N2K_CAN 3  // for use with Teensy 3.1/3.2/3.5/3.6 boards
  #define USE_N2K_CAN 4  // for use with avr boards
  #define USE_N2K_CAN 5  // for use with socketCAN (linux, etc) systems
  #define USE_N2K_CAN 6  // for use with MBED (ARM) systems
  #define USE_N2K_CAN 7  // for use with ESP32
  #define USE_N2K_CAN 8  // for use with Teensy 3.1/3.2/3.5/3.6/4.0/4.1 boards
  #define USE_N2K_CAN 9  // for use with Arduino CAN API (e.g. UNO R4 or Portenta C33)
  \endcode

  <b>Depending of your board you will need to also install "driver" libraries:</b>  

  see also \ref secHWlib

  Arduino CAN shield (mcp_can) with MCP2515 chip:
    - https://github.com/ttlappalainen/CAN_BUS_Shield
    - https://github.com/ttlappalainen/NMEA2000_mcp
    
  Arduino DUE internal CAN:
    - https://github.com/ttlappalainen/due_can
    - https://github.com/ttlappalainen/NMEA2000_due
    
  Teensy 3.2-3.6 internal CAN:
    - https://github.com/ttlappalainen/FlexCAN_Library
    - https://github.com/ttlappalainen/NMEA2000_teensy
    
  Teensy 4.X internal CAN:
    - https://github.com/ttlappalainen/NMEA2000_Teensyx
    
  ESP32 internal CAN:
    - https://github.com/ttlappalainen/NMEA2000_esp32
  
  AVR:
    - https://github.com/thomasonw/avr_can
    - https://github.com/thomasonw/NMEA2000_avr
  
  MBED (note that there may be problem with fastpackets on MBED):
    - https://github.com/thomasonw/NMEA2000_mbed
    
  RPi socket CAN:
    -  https://github.com/thomasonw/NMEA2000_socketCAN
  

  <b>There are also library specific defines:</b>
  
  mcp_can:

    \code{cpp}
    #define N2k_SPI_CS_PIN 53  // Pin for SPI Can Select
    #define N2k_CAN_INT_PIN 21 // Use interrupt  and it is connected to pin 21
    #define USE_MCP_CAN_CLOCK_SET 8  // possible values 8 for 8Mhz and 16 for 16 Mhz clock
    \endcode

  esp32:

    \code{cpp}
    #define ESP32_CAN_TX_PIN GPIO_NUM_16
    #define ESP32_CAN_RX_PIN GPIO_NUM_4
    \endcode
    
  Teensy:
	\code{cpp}
    #define NMEA2000_TEENSY_CAN_BUS 1 // Select second CAN bus for Teensy 3.5 or 3.6
    \endcode

  Teensyx:

    \code{cpp}
    #define NMEA2000_TEENSYX_CAN_BUS tNMEA2000_Teensyx::CAN2 // select CAN bus 2
    #define NMEA2000_TEENSYX_TX_PIN tNMEA2000_Teensyx::pinAlternate // Use selected CAN alternate Tx pin
    #define NMEA2000_TEENSYX_RX_PIN tNMEA2000_Teensyx::pinAlternate // Use selected CAN alternate Rx pin
    #define USE_NMEA2000_TEENSYX_FOR_TEENSY_3X // Force NMEA2000_TEENSYX also for Teensy 3.x boards
    \endcode

  Arduino DUE:
    
    \code{cpp}
    #define NMEA2000_ARDUINO_DUE_CAN_BUS tNMEA2000_due::CANDevice1  // Use CAN bus 1 instead of 0 for Arduino DUE
    \endcode

*/


#ifndef _NMEA2000_CAN_H_
#define _NMEA2000_CAN_H_

#include "N2kMsg.h"
#include "NMEA2000.h"

#include "NMEA2000_esp32.h"       // https://github.com/ttlappalainen/NMEA2000_esp32
tNMEA2000 &NMEA2000=*(new tNMEA2000_esp32());

#endif

