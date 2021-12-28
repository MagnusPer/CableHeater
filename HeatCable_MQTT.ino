
/*       
 *   Heat Cable control system using external temperature sensor (DB18B20) and a predefined temperature treshold (SetTemp) 
 *   when relay to heatcable shall be activated.   
 * 
 *   LCD display (Omilex sheild-LCD16x2) for visualize external(OutTemp) and treshold(SetTemp) temperature with relay status    
 *   
 *   Pressed button(1) incresses the treshold and button(2) decreasess   
 * 
 */

#include <PubSubClient.h> 
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <LCD16x2.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// Define Wemos D1 Pin and EEPROM 
#define One_Wire_Bus D5                          // The pin location of the Thermometer sensor
#define SDA_PIN D1
#define SCL_PIN D2
#define LCD_RST_PIN D3
#define RELAY_PIN D7
#define EEPROM_SIZE 2


//Constants
const char *wifi_ssid                     = "xxx";
const char *wifi_pwd                      = "xxx";
const char *wifi_hostname                 = "HeatCable";
const char* mqtt_server                   = "xxx";      // MQTT Boker IP, your home MQTT server eg Mosquitto on RPi, or some public MQTT
const int mqtt_port                       = 1883;                 // MQTT Broker PORT, default is 1883 but can be anything.
const char *mqtt_user                     = "xxx";       // MQTT Broker User Name
const char *mqtt_pwd                      = "xxx";          // MQTT Broker Password 
String clientId                           = "HeatCable : " + String(ESP.getChipId(), HEX);
const char *ota_hostname                  = "HeatCable";
const char *ota_pwd                       = "xxx";


// Global variables 
float HeatCable_version                   = 1.0;          // Heat Cable program version
float OutTemp                             = 0;            // Temperature sensor
float previousOutTemp                     = 0;     
signed char SetTemp                       = 0;            // Treshold level temperature to activate the RELAY_PIN (Heat Cable)  
signed char previousSetTemp               = 0; 

int EEPROMaddr                            = 0;
bool Backlight                            = false;        // Turn on and off the LCD backlight  
bool debug                                = true;        // If true activate debug values to write to serial port
bool relayStatus                          = false;        // Status on relay 
unsigned long setBacklightMillis          = 0;            // Time when button was presed
const unsigned long backlightTime         = 10000;        // Time in milliseconds the backlight is on after pressed button (10 sec)

unsigned long readTempPrevMillis          = 0;            // Store previous millis
const unsigned long readTempTime          = 10000;        // Time in milliseconds the temperature sensor update value (30 sec)

unsigned long ReportTimerStatusPrevMillis = 0;            // Store the previous millis
const unsigned long ReportTimerStatus     = 60000;        // Time in milliseconds (1 min) to report MQTT alive status

unsigned long ReportTimerLongPrevMillis   = 0;            // Store the previous millis
const unsigned long ReportTimerLong       = 600000;       // Timer in milliseconds (5 min) to report MQTT status            


// MQTT Constants
const char *Status_topic                  = "HeatCable/Status";
const char *RelayStatus_topic             = "HeatCable/RelayStatus";
const char *OutTemp_topic                 = "HeatCable/OutTemp";
const char *SetTemp_topic                 = "HeatCable/SetTemp";



OneWire oneWire(One_Wire_Bus);                            // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);                      // Pass the oneWire reference to Dallas Temperature sensor 

WiFiClient espClient;                                     // Setup WiFi client definition WiFi
PubSubClient client(espClient);                           // Setup MQTT client

LCD16x2 lcd;                                              // Setup a LCD16x2 instance to communicate to display


/**************************************************************************/
/* Setup                                                                  */
/**************************************************************************/

void setup(){
  setup_wifi();
  setup_ota(); 
  if (debug) { Serial.begin(115200); Serial.println("HeatCable"); }
 
  EEPROM.begin(EEPROM_SIZE);        
  sensors.begin();                                   // Start up the Dallas Temperature library
  Wire.begin(SDA_PIN, SCL_PIN);                      // join i2c bus 

  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LCD_RST_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LCD_RST_PIN, HIGH);

  sensors.requestTemperatures();                
  OutTemp = sensors.getTempCByIndex(0);
  
  initLCD();

}

/**************************************************************************/
/* Setup WiFi connection                                                  */
/**************************************************************************/

void setup_wifi() {

    /*  WiFi status return values and meaning 
        WL_IDLE_STATUS      = 0,
        WL_NO_SSID_AVAIL    = 1,
        WL_SCAN_COMPLETED   = 2,
        WL_CONNECTED        = 3,
        WL_CONNECT_FAILED   = 4,
        WL_CONNECTION_LOST  = 5,
        WL_DISCONNECTED     = 6 */
  
    if (debug){ Serial.print("WiFi.status(): "); Serial.println(WiFi.status()); }
    
    int WiFi_retry_counter = 0;
   
    WiFi.hostname(wifi_hostname);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pwd);
    
    // Loop until reconnected or max retry then restart
    while (WiFi.status() != WL_CONNECTED){
        WiFi_retry_counter ++;
        if (WiFi_retry_counter == 30) {ESP.restart();}  
        if (debug){ Serial.print("WiFi retry: "); Serial.println(WiFi_retry_counter); } 
        delay(1000);
    }
    
    if (debug){ Serial.print("WiFi connected: ");Serial.println(WiFi.localIP()); }

}

/**************************************************************************/
/* Setup OTA connection                                                   */
/**************************************************************************/

void setup_ota() {

    ArduinoOTA.setHostname(ota_hostname); 
    ArduinoOTA.setPassword(ota_pwd);
    ArduinoOTA.onStart([]() {});
    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
    ArduinoOTA.onError([](ota_error_t error) {
          // (void)error;
          // ESP.restart();
          });
    ArduinoOTA.begin();

}

/**************************************************************************/
/* Setup MQTT connection                                                   */
/**************************************************************************/

void reconnect() {

    int MQTT_retry_counter = 0;
    
    // Loop until reconnected or max retry then leave
    while (!client.connected() && MQTT_retry_counter < 30) {
       client.setServer(mqtt_server, mqtt_port);
       if (debug){ Serial.print("Connecting to MQTT server "); Serial.println(MQTT_retry_counter); }
       client.setServer(mqtt_server, mqtt_port);
       client.connect(clientId.c_str(), mqtt_user, mqtt_pwd);
       MQTT_retry_counter ++;
       delay (1000);
    }
    
    if (debug && client.connected()){ Serial.println(" MQTT connected"); }
    
}


/**************************************************************************/
/* Publish status to MQTT Broker                                          */
/**************************************************************************/

void publishStatusMQTT(){

  // Report status to MQTT broker every reportInterval 
  
  if(millis() - ReportTimerStatusPrevMillis > ReportTimerStatus) {
      
     ReportTimerStatusPrevMillis = millis();
     client.publish(Status_topic, String("online").c_str());
  }

  
  if((millis() - ReportTimerLongPrevMillis) > ReportTimerLong ) {
     
     ReportTimerLongPrevMillis = millis();
     client.publish(RelayStatus_topic, String(relayStatus).c_str());
     client.publish(OutTemp_topic, String(OutTemp).c_str());
     client.publish(SetTemp_topic, String(SetTemp).c_str());
  }
  
}


/**************************************************************************/
/* Main loop                                                              */
/**************************************************************************/

void loop(){
    
    ArduinoOTA.handle();
    client.loop();
    if (WiFi.status() != WL_CONNECTED){ setup_wifi(); }             // Check WiFi connnection reconnect otherwise 
    if (!client.connected()) { reconnect(); }                       // Check MQTT connnection reconnect otherwise 

   
    publishStatusMQTT();                                            // Publish MQTT status
    getOutTemp();                                                   // Read temperature sensor
    writeOutTemp();                                                 // Write temperature sensor value on LCD 
    writeSetTemp();                                                 // Write stored temperature treshold value  
    setRelay();                                                     // Set relay according to threshoild values 
    setBacklight();                                                 // Enlight LCD backlight if any buttons is pressed during a defined time   
    checkButtons();                                                 // Check if any buttons is pressed and 

}

/**************************************************************************/
/* Init LCD                                                               */
/**************************************************************************/

void initLCD(){

    Wire.begin(SDA_PIN, SCL_PIN);                                 // join i2c bus 
    
    delay(1000);            
    lcd.lcdSetBlacklight(1000);
    lcd.lcdClear();
    lcd.lcdGoToXY(1,1);
    lcd.lcdWrite("Welcome to");
    lcd.lcdGoToXY(1,2);
    lcd.lcdWrite("HeatCable");
    lcd.lcdGoToXY(11,2);
    lcd.lcdWrite(HeatCable_version,1);
  
    delay(2000);
          
    lcd.lcdClear();
    lcd.lcdGoToXY(1,1);
    lcd.lcdWrite("Out: ");
    lcd.lcdGoToXY(1,2);
    lcd.lcdWrite("Set: ");

}

/**************************************************************************/
/* Read temperature sensor data                                           */
/**************************************************************************/

void getOutTemp(){

    if(millis() - readTempPrevMillis  > readTempTime ) {
      
      readTempPrevMillis = millis();
      sensors.requestTemperatures();                
      OutTemp = sensors.getTempCByIndex(0);
  }

}


void writeOutTemp(){

  if (previousOutTemp != OutTemp){       // don't update LCD if same value 
    
      lcd.lcdGoToXY(6,1);
      lcd.lcdWrite("     ");
      lcd.lcdWrite(OutTemp, 1);
      lcd.lcdGoToXY(11,1);
      lcd.lcdWrite("C");
    }
    
   previousOutTemp = OutTemp;  
}

/**************************************************************************/
/* Write LCD treshold tempoerature, when relay should activate/deactivate */
/**************************************************************************/

void writeSetTemp(){
    
    SetTemp = EEPROM.read(EEPROMaddr);
    
    if (previousSetTemp != SetTemp){    // don't update LCD if same value 
      
      lcd.lcdGoToXY(6,2);
      lcd.lcdWrite("     ");
      lcd.lcdWrite(SetTemp, 1);
      lcd.lcdGoToXY(11,2);
      lcd.lcdWrite("C");
   }
   
   previousSetTemp = SetTemp;
}


/**************************************************************************/
/* Update LCD and activate/deactivate relay                               */
/**************************************************************************/

void setRelay(){

  if (OutTemp < SetTemp) {            // Activate Heat cable
    relayStatus = true;
    digitalWrite(RELAY_PIN, HIGH); 
    lcd.lcdGoToXY(14,1);
    lcd.lcdWrite("ON ");
  }

  if (OutTemp > SetTemp) {            // Deactivate Heat cable
    relayStatus = false;
    digitalWrite(RELAY_PIN, LOW);
    lcd.lcdGoToXY(14,1);
    lcd.lcdWrite("OFF");
  }
}

/**************************************************************************/
/* Turn on and off LCD backlight                                          */
/**************************************************************************/

void setBacklight(){
 
   unsigned long currentMillis = millis();
        
  if (Backlight == true) { 
    lcd.lcdSetBlacklight(1000);
    if(currentMillis - setBacklightMillis > backlightTime) {
        Backlight = false;
      }
    }
  else {
    lcd.lcdSetBlacklight(0);
  } 
}

/**************************************************************************/
/* Check if any buttons on LCD display sheild is pressed                  */
/**************************************************************************/

void checkButtons(){
 
  int buttons;
  buttons = lcd.readButtons();
  
  if (buttons == 14) {                // Button 1
     SetTemp++;
     EEPROM.write(EEPROMaddr, SetTemp); 
     EEPROM.commit();
     Backlight = true;
     setBacklightMillis = millis();
    }
    
  if (buttons == 13) {              // Button 2
       SetTemp--;
       EEPROM.write(EEPROMaddr, SetTemp);  
       EEPROM.commit();
       Backlight = true;
       setBacklightMillis = millis();
     }
  
  if (buttons == 11){               // Button 3
      Backlight = true;
      setBacklightMillis = millis();
    }
  
  if (buttons == 7) {               // Button 4 
      Backlight = true;
      setBacklightMillis = millis();
    }

  if (buttons == 0) {               // Can not read status from LCD => trigg a restart via LCD reset PIN 
      if (debug) { Serial.println(" RESTART "); }
      digitalWrite(LCD_RST_PIN, LOW);           
      delay(3000);
      digitalWrite(LCD_RST_PIN, HIGH);
      delay(1000);
      ESP.restart();
    }
}
