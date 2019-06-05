/* Arduino script for use with ESP8266 ESP-01 and BME-280 weather station. 
   This script collects data from BME-280 sensor and send it to MQTT Server and to your telegram channel/user.
   */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
#include <Wire.h>
#include <config.h>
#include <Ticker.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <PubSubClient.h>
uint64_t sleepTimeS =  11400000000; // wait 3 hours (in µSeconds)
uint64_t sleepTime = 300000000; // wait 5 minutes (in µSeconds)
  
#define mqtt_server "<ip>"
#define mqtt_user "<user>"
#define mqtt_password "<password>"
#define humidity_topic "weatherstation/humidity"
#define temperature_celsius_topic "weatherstation/temperature"
#define pressure_topic "weatherstation/pressure"
#define volt_topic "weatherstation/voltage"


//MQTT Service msg 
long lastMsg = 0;
char msg[50];
int value = 0;
WiFiClient espClient;
PubSubClient client(espClient);

extern "C" {
#include "user_interface.h" //os_timer
}

// Telegram Settings
// First create a bot with telegram's botfather and write down the bot settings. 
// Findout your own telegramID (this is the adminID to communicate with the bot).
// If you create a channel, findout the channelID (chatID).

#define botName "<botName>"  // for Bot use
#define botUserName "<botUserName>" // for Bot use
#define botToken "<like 123456789:AABBccDDeeFfgGHHiIjjKklLMmnNooPPqQrR" // for Bot use
//#define adminID "like 12345678" // your ID, use this ID if you want do talk directly to the bot
#define chatID "like -123456789" // channelID, use this if you want to talk to the bot via channel, (leading "-" needed)

// Wifi settings 
static char ssid[] = "<your-SSID";
static char password[] = "your-password";
static char hostname[] = "<your-hostname-for-ESP-01";
int state = 0;


// ---------------------------------------------------------------------------------------------------------------

ADC_MODE(ADC_VCC);

char vcc[10];
char v_str[10];
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

#define NB_TRYWIFI    10

Adafruit_BME280 bme; // I2C
unsigned long delayTime;
//////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(9600);
  while(!Serial) {} // Wait
  Wire.pins(0, 2);           
  Wire.begin();
  if (! bme.begin(&Wire)) {
  if (! bme.begin(&Wire)) Serial.println("Could not find BME280 sensor!");
    while (1);
  }
  delayTime = 5000;
  client.setServer(mqtt_server, 1883); 
  rst_info *rinfo = ESP.getResetInfoPtr();
  Serial.println(String("\nResetInfo.reason = ") + (*rinfo).reason + ": " + ESP.getResetReason()); 
  wifi_station_set_hostname(hostname);
  WiFi.begin(SSID, password);
  Serial.print("\n  Connecting to " + String(SSID) + " ");
  int _try = 0;
  // Wait until it is connected to the network
  while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
        }
  delay(300);
  _try++;
  if ( _try >= NB_TRYWIFI ) {
         Serial.println("Can't connect to wifi accesspoint, going into deepsleep!");
         delay(500);
         system_deep_sleep(sleepTime);
         delay(300);
         }
  Serial.println();       
  Serial.println("wifi connected.");  
  delay(300);
  Serial.println("IP-Adresse: " + WiFi.localIP().toString());
  Serial.println("Hostname:  " + String(hostname));
  delay(300);
  bot.begin();          // initialize telegram bot
  delay(300);
  //bot.sendMessage(chatID, "Weatherstation online.", ""); // Bot <-> Channel
  Serial.println("Weatherstation online.");
  Serial.println("- - - - - - - - - - - - - - - - - - -\n");
  delay(300);
  }
    String macToStr(const uint8_t* mac)
    {
    String result;
    for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
    result += ':';
    }
    return result;
    } 
     
            //MQTT Connection
            void reconnect() {
            // Loop until we're reconnected
            while (!client.connected()) 
            {
            delay (300);
            Serial.println("Trying MQTT Connection...");
            // Generate client name based on MAC address and last 8 bits of microsecond counter
            String clientName;  
            clientName += " Weatherstation1 ";
            uint8_t mac[6];
            WiFi.macAddress(mac);
            clientName += macToStr(mac);
            Serial.print("Connecting mit ");
            Serial.print(mqtt_server);
            Serial.print(" as ");
            Serial.print(clientName);
            Serial.println();
            
            // Attempt to connect
            // If you do not want to use a username and password, change next line to
            if (client.connect((char*) clientName.c_str())) {
           // if (client.connect((char*) clientName.c_str()), mqtt_user, mqtt_password) {
            Serial.println("Verbunden.");
            //bot.sendMessage(chatID, "Verbunden mit MQTT Server"); // Bot <-> Channel
            } else {
                    Serial.print("Failed to establish Connection, rc=");
                    Serial.print(client.state());
                    Serial.println(" retry in 3 Minutes");
                    // Wait 3 minutes before retrying
                    delay(500);
                    system_deep_sleep(sleepTime);
                    delay(300);
                    }
           Serial.println("- - - - - - - - - - - - - - - - - - -\n");  
           }
 }


 void get_temperature() {

 { // I2C scanner beginn
 byte error, address;
 int nDevices;
 Serial.println("Scanning...");
 nDevices = 0;
 for(address = 1; address < 127; address++ )
 {
   // The i2c_scanner uses the return value of
   // the Write.endTransmisstion to see if
   // a device did acknowledge to the address.
   Wire.beginTransmission(address);
   error = Wire.endTransmission();
   if (error == 0)
   {
     Serial.print("I2C device found at address 0x");
     if (address<16)
       Serial.print("0");
     Serial.print(address,HEX);
     Serial.println("  !");
     nDevices++;
   }
   else if (error==4)
   {
     Serial.print("Unknow error at address 0x");
     if (address<16)
       Serial.print("0");
     Serial.println(address,HEX);
   }   
 }
 if (nDevices == 0)
   Serial.println("No I2C devices found\n");
 else
   Serial.println("done\n");
 delay(5000);           // wait 5 seconds for next scan
}  // I2C scanner end

  //delay (60000);
 {

   bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
                    delayTime = 61000;
   
   vccRead();
   delay(50);
   
           {
             bme.takeForcedMeasurement();
             Serial.print("Temperature:     ");
             Serial.print(bme.readTemperature());
             Serial.println("°C ");
             Serial.print("Humidity:    ");
             Serial.print(bme.readHumidity());
             Serial.println(" %");
             Serial.print("Pressure:           ");
             Serial.print(bme.readPressure() / 100.0F);
             Serial.println(" hPa");
             Serial.print("Batterystatus:    "); Serial.print(v_str); Serial.println(" V ");
             bot.sendMessage(chatID, "T " + String(bme.readTemperature()) + " °C, " + "Hum " + String(bme.readHumidity()) + " %, " + "Pres " + String(bme.readPressure() / 100.0F) + " hPa, " + "B " + String(v_str) + " V, ", "");          
             Serial.println();
             delay(200);
             }
            Serial.print( "Gehe in den DeepSleep. \n");
            system_deep_sleep(sleepTimeS);
            yield();
        }
 }

 void vccRead() {
  // most exact output
  uint16_t v = ESP.getVcc();
  float_t v_cal = ((float)v/1024.0f);
  dtostrf(v_cal, 5, 1, v_str);
  delay(20);
  }
    
//////////////////////////////////////////////////////////////////
void loop()
{
  reconnect();
  get_temperature(); 
  yield();
}

//////////////////////////////////////////////////////////////////

// 28.3.2019 works perfekt with new sensor, 25µA in deepSleep
