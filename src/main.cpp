#include <WiFi.h>
#include <AsyncTCP.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Logging.h>

// #include <ModbusBridgeWiFi.h>
#include <ModbusClientRTU.h>
#include "config.h"
#include "pages.h"

#include "ModbusServer.h"

#include <M5_Ethernet.h>
#include <SPI.h>
#include "ModbusBridgeEthernet.h"
#include "ModbusServerEthernet.h"


ModbusServerEthernet myServer;

// EthernetServer webServer(80);
 AsyncWebServer webServer(80);

Config config;
Preferences prefs;
ModbusClientRTU *MBclient;

//  ModbusBridgeWiFi MBbridge;
ModbusBridgeEthernet MBbridge;

WiFiManager wm;

#define RESET_P 26

// // SPI pins
#define ETH_SPI_SCK     23
#define ETH_SPI_MISO    19
#define ETH_SPI_MOSI    18
// Reset W5500 module
void WizReset() {
  Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  delay(250);
  digitalWrite(RESET_P, LOW);
  delay(50);
  digitalWrite(RESET_P, HIGH);
  delay(350);
  Serial.print("Done.\n");
}

void setup() {
  debugSerial.begin(115200);
  dbgln();
  dbgln("[config] load")
  prefs.begin("modbusRtuGw");
  config.begin(&prefs);
  debugSerial.end();
  debugSerial.begin(config.getSerialBaudRate(), config.getSerialConfig());
  dbgln("[wifi] start");
  WiFi.mode(WIFI_STA);
  wm.setClass("invert");
  auto reboot = false;
  wm.setAPCallback([&reboot](WiFiManager *wifiManager){reboot = true;});
  wm.autoConnect();
  if (reboot){
    ESP.restart();
  }
  dbgln("[wifi] finished");
  dbgln("[modbus] start");

  MBUlogLvl = LOG_LEVEL_WARNING;
  RTUutils::prepareHardwareSerial(modbusSerial);
#if defined(RX_PIN) && defined(TX_PIN)
  // use rx and tx-pins if defined in platformio.ini
  modbusSerial.begin(config.getModbusBaudRate(), config.getModbusConfig(), RX_PIN, TX_PIN );
  dbgln("Use user defined RX/TX pins");
#else
  // otherwise use default pins for hardware-serial2
  modbusSerial.begin(config.getModbusBaudRate(), config.getModbusConfig(),32,33);    //刘永相
#endif

SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);


  // // Fire up Ethernet
  Ethernet.init(22);
  IPAddress lIP;
  byte mac[6]={0xAA,0xAB,0xAC,0xAD,0xAE,0xAF}; // Fill in appropriate values here!

  // // prime the WIZ5500 module
  WizReset();

  // Try to get an IP via DHCP
  if (Ethernet.begin(mac) == 0) {
    // No. DHCP did not work.
    Serial.print("Failed to configure Ethernet using DHCP\n");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.print("Ethernet shield was not found.  Sorry, can't run without hardware. :(\n");
    } else {
      if (Ethernet.linkStatus() == LinkOFF) {
        Serial.print("Ethernet cable is not connected.\n");
      }
    }
    while (1) {}  // Do nothing any more
  }

  // Serial.printf("Link Status:%s",Ethernet.linkStatus);


  // We seem to have a connection to the router
  lIP = Ethernet.localIP();
  Serial.printf("My IP address: %u.%u.%u.%u\n", lIP[0], lIP[1], lIP[2], lIP[3]);


  MBclient = new ModbusClientRTU(config.getModbusRtsPin());
  MBclient->setTimeout(1000);
  MBclient->begin(modbusSerial, 1);


  

  for (uint8_t i = 1; i < 248; i++)
  {
    MBbridge.attachServer(i, i, ANY_FUNCTION_CODE, MBclient);
  }  
  MBbridge.start(config.getTcpPort(), 10, config.getTcpTimeout());
  dbgln("[modbus] finished");

  setupPages1(&webServer, MBclient, &MBbridge, &config, &wm);    //无法使用web页面

  webServer.begin();
  dbgln("[setup] finished");
}

void loop() {
  // put your main code here, to run repeatedly:
}