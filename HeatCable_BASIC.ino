
/*       
 *   Heat Cable control system using external temperature sensor (DB18B20) and a predefined temperature treshold (SetTemp) 
 *   when relay to heatcable shall be activated.   
 * 
 *   LCD display (Omilex sheild-LCD16x2) for visialisation external (OutTemp)and treshold (SetTemp)temperature also the realay status    
 *   
 *   Pressed button(1) incresses the treshold and button(2) decreasess   
 * 
 */


#include <ESP8266WiFi.h>
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

// Global variables 
float HeatCable_version           = 1.0;          // Heat Cable program version
float OutTemp                     = 0;            // Temperature sensor
float previousOutTemp             = 0;     
signed char SetTemp               = 0;            // Treshold level temperature to activate the RELAY_PIN (Heat Cable)  
signed char previousSetTemp       = 0; 

int EEPROMaddr                    = 0;
bool Backlight                    = false;        // Turn on and off the LCD backlight  
unsigned long setBacklightMillis  = 0;            // Time when button was presed
unsigned long backlightTime       = 10000;        // Time in milliseconds the backlight is on after pressed button (10 sec)

unsigned long readTempPrevMillis  = 0;            // Store previous millis
unsigned long readTempTime        = 10000;        // Time in milliseconds the temperature sensor update value (30 sec)


OneWire oneWire(One_Wire_Bus);                     // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);               // Pass the oneWire reference to Dallas Temperature sensor 

LCD16x2 lcd;                                       // Setup a LCD16x2 instance to communicate to display


void setup(){
  
  WiFi.mode( WIFI_OFF );                            // turn off WiFi 
  WiFi.forceSleepBegin();
  
  EEPROM.begin(EEPROM_SIZE);        
  sensors.begin();                                   // Start up the Dallas Temperature library
  Wire.begin(SDA_PIN, SCL_PIN);                      // join i2c bus 
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LCD_RST_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LCD_RST_PIN, HIGH);

  sensors.requestTemperatures();                
  OutTemp = sensors.getTempCByIndex(0);
  
  initLCD();

}

void loop(){
  
    getOutTemp();             // Read temperature sensor
    writeOutTemp();           // Write temperature sensor value on LCD 
    writeSetTemp();           // Write stored temperature treshold value  
    setRelay();               // Set relay according to threshoild values 
    setBacklight();           // Enlight LCD backlight if any buttons is pressed during a defined time   
    checkButtons();           // Check if any buttons is pressed and 

}


void initLCD(){

    Wire.begin(SDA_PIN, SCL_PIN);                      // join i2c bus 
    
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

void setRelay(){

  if (OutTemp < SetTemp) {            // Activate Heat cable
    digitalWrite(RELAY_PIN, HIGH); 
    lcd.lcdGoToXY(14,1);
    lcd.lcdWrite("ON ");
  }

  if (OutTemp > SetTemp) {            // Deactivate Heat cable
    digitalWrite(RELAY_PIN, LOW);
    lcd.lcdGoToXY(14,1);
    lcd.lcdWrite("OFF");
  }
}

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

  if (buttons == 0) {               // Can not read status from LCD trigg a restart via LCD reset PIN 
      Serial.println(" RESTART "); 
      digitalWrite(LCD_RST_PIN, LOW);           
      delay(3000);
      digitalWrite(LCD_RST_PIN, HIGH);
      delay(1000);
      ESP.restart();
    }
}
