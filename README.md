# Cable Heater

Heat Cable control system using external temperature sensor (DB18B20) and a confirable temperature treshold (SetTemp) when relay to heatcable shall be activated. LCD display (Omilex sheild-LCD16x2) to visualize external(OutTemp) and treshold(SetTemp) temperature with relay status.    

Two versions available **basic** without any WiFi connection and one using **MQTT** topics to publish status. 


## Installation
Needed Arduino Libraries to be included in [IDE](https://www.arduino.cc/en/Main/Software). Install them either from GitHub repository directly or within the IDE application itself **Sketch > Import Library** 

| Library                            | Link to GitHub                                      |  Basic  |  MQTT  | 
| ---------------------------------- | --------------------------------------------------- |---------|--------|
| PubSubClient                       |  https://github.com/knolleary/pubsubclient          |         |   X    |     
| OneWire                            |                                                     |   X     |   X    |
| Dallas Temperature                 |                                                     |   X     |   X    |   
| Wire                               |                                                     |   X     |   X    |  
| LCD16x2                            | https://www.olimex.com/Products/Duino/Shields/SHIELD-LCD16x2/open-source-hardware |   X     |   X    |  
| Dallas Temperature                 |                                                     |   X     |   X    |  


## MQTT Topics
MQTT Topics to be published. 

| Topic                              | Description                                         |
| ---------------------------------- | --------------------------------------------------- |
| HeatCable/Status                   |  set topic - Status "online/offline"                |
| HeatCable/RelayStatus              |  set topic - Relay status "activated/deactivated"   |
| HeatCable/OutTemp                  |  set topic - Outdoor temperature                    |
| HeatCable/SetTemp                  |  set topic - Configured temperature                 |


## Wiring
<img src="https://github.com/MagnusPer/WeatherStation/blob/master/images/WeatherStation.jpg" width="800">



## BOM List
| Part                               | Comment/Link                                        |
| ---------------------------------- | --------------------------------------------------- |
|  Wemos D1 mini                     | https://www.wemos.cc/en/latest/                     |   
|  Omilex sheild-LCD16x2             | https://www.olimex.com/Products/Duino/Shields/SHIELD-LCD16x2/open-source-hardware |
|  Tempsensor DB18B20                |                                                     |  
|  Power Supply HLK-PM03             | http://www.hlktech.net/product_detail.php?ProId=59  |  


