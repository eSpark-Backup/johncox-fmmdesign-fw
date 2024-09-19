#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>

static const char *TAG = "app_main";

N2KStreamESP32 myStream(UART_NUM_0);

#ifdef __cplusplus
extern "C" {
#endif

const unsigned long TransmitMessages[] PROGMEM={130310L,130311L,130312L,0};

tN2kSyncScheduler TemperatureScheduler(false,2000,500);
tN2kSyncScheduler EnvironmentalScheduler(false,500,510);
tN2kSyncScheduler OutsideEnvironmentalScheduler(false,500,520);

void OnN2kOpen() {
  // Start schedulers now.
  TemperatureScheduler.UpdateNextTime();
  EnvironmentalScheduler.UpdateNextTime();
  OutsideEnvironmentalScheduler.UpdateNextTime();
}

// *****************************************************************************
double ReadCabinTemp() {
  return CToKelvin(33.5); // Read here the true temperature e.g. from analog input
//   return (5.0 * analogRead(12)*100/1024);
}

// *****************************************************************************
double ReadWaterTemp() {
  return CToKelvin(15.5); // Read here the true temperature e.g. from analog input
//   return (5.0 * analogRead(13)*100/1024);
}

// *****************************************************************************
void SendN2kTemperature() {
  tN2kMsg N2kMsg;

  if ( TemperatureScheduler.IsTime() ) {
    TemperatureScheduler.UpdateNextTime();
    SetN2kTemperature(N2kMsg, 1, 1, N2kts_MainCabinTemperature, ReadCabinTemp());
    NMEA2000.SendMsg(N2kMsg);
  }
  
  if ( EnvironmentalScheduler.IsTime() ) {
    EnvironmentalScheduler.UpdateNextTime();
    SetN2kEnvironmentalParameters(N2kMsg, 1, N2kts_MainCabinTemperature, ReadCabinTemp());
    NMEA2000.SendMsg(N2kMsg);
  }
  
  if ( OutsideEnvironmentalScheduler.IsTime() ) {
    OutsideEnvironmentalScheduler.UpdateNextTime();
    SetN2kOutsideEnvironmentalParameters(N2kMsg, 1, ReadWaterTemp());
    NMEA2000.SendMsg(N2kMsg);
  }
}

void app_main(void)
{
    // Set Product information
  NMEA2000.SetProductInformation("00000001", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "Simple temp monitor",  // Manufacturer's Model ID
                                 "1.2.0.21 (2022-09-30)",  // Manufacturer's Software version code
                                 "1.1.0.0 (2022-09-30)" // Manufacturer's Model version
                                 );
  // Set device information
  NMEA2000.SetDeviceInformation(112233, // Unique number. Use e.g. Serial number.
                                130, // Device function=Temperature. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                75, // Device class=Sensor Communication Interface. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                273//2040 // Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );
  // Uncomment 2 rows below to see, what device will send to bus. Use e.g. OpenSkipper or Actisense NMEA Reader                           
  NMEA2000.SetForwardStreamESP32(&myStream);

  // If you want to use simple ascii monitor like Arduino Serial Monitor, uncomment next line
  //NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode,22);
  //NMEA2000.SetDebugMode(tNMEA2000::dm_Actisense); // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
  // NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
  // Here we tell library, which PGNs we transmit
  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
  NMEA2000.SetOnOpen(OnN2kOpen);
  NMEA2000.Open();

  while(1)
  {
   SendN2kTemperature();
   NMEA2000.ParseMessages();

   vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

#ifdef __cplusplus
}
#endif