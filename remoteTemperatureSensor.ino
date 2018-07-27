// Temperature sensor using bmp085, esp8266, and fona808. Adapted from the geotagging example.
// Libraries
//#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

#define LED_PIN             13
#define LED_PIN2            14

#define FONA_RX              1   // FONA serial RX pin (pin 2 for shield).
#define FONA_TX              3  // FONA serial TX pin (pin 3 for shield).
#define FONA_RST             12   // FONA reset pin (pin 4 for shield)


// FONA GPRS configuration
#define FONA_APN             ""  // APN used by cell data service (leave blank if unused)
#define FONA_USERNAME        ""  // Username used by cell data service (leave blank if unused).
#define FONA_PASSWORD        ""  // Password used by cell data service (leave blank if unused).

// Adafruit IO configuration
#define AIO_SERVER           "io.adafruit.com"  // Adafruit IO server name.
#define AIO_SERVERPORT       1883  // Adafruit IO port.
#define AIO_USERNAME         "redacted"  // Adafruit IO username (see http://accounts.adafruit.com/).
#define AIO_KEY             "redacted"  // Adafruit IO key (see settings page at: https://io.adafruit.com/settings).
#define CLIENTID             __TIME__ AIO_USERNAME
// Feeds
#define LOCATION_FEED_NAME       "location"  // Name of the AIO feed to log regular location updates.
#define MAX_TX_FAILURES      10  // Maximum number of publish failures in a row before resetting the whole sketch.

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);     // FONA software serial connection.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);                 // FONA library connection.

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, CLIENTID, AIO_USERNAME, AIO_KEY);

uint8_t txFailures = 0;                                       // Count of how many publish failures have occured in a row.

// Feeds configuration
Adafruit_MQTT_Publish location_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "redacted");

Adafruit_MQTT_Publish battery_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/redacted");

Adafruit_MQTT_Publish temperature_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/redacted");

void setup() {

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(LED_PIN2, OUTPUT);
  digitalWrite(LED_PIN2, LOW);

  if(!bmp.begin()){
    halt();
  }

  flash_lights();

  
  
  // Initialize serial output.
  // cant use serial with fona on the esp8266 (tx/rx are tied up with the usb). 
  // So commented out all of the serial.prints
//  Serial.begin(115200);

  // Initialize the FONA module
//  Serial.println("Initializing FONA....(may take 10 seconds)");
  fonaSS.begin(4800);
  if (!fona.begin(fonaSS)) {
    halt();
  }
  fonaSS.println("AT+CMEE=2");
//  Serial.println("FONA is OK");


  // Wait for FONA to connect to cell network (up to 8 seconds, then watchdog reset).
//  Serial.println("Checking for network...");
  int time0 = millis();
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  while (fona.getNetworkStatus() != 1) {
   delay(500);
   if (millis()-time0 > 30*1000){
    halt();
   }
  }

  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN2, LOW);
  // Enable GPS.
  fona.enableGPS(true);

  // Start the GPRS data connection.

//  fona.setGPRSNetworkSettings(F(FONA_APN));
  //fona.setGPRSNetworkSettings(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD));
  delay(2000);

//  Serial.println(F("Disabling GPRS"));
  fona.enableGPRS(false);
  delay(2000);
//  Serial.println("Enabling GPRS");
  if (!fona.enableGPRS(true)) {
    halt();
  }
//  Serial.println("Connected to Cellular!");

  // Wait a little bit to stabilize the connection.
  delay(3000);

  // Now make the MQTT connection.
  digitalWrite(LED_PIN, HIGH);
  int8_t ret = 1;
  int attempts = 0;

  while (ret != 0 && attempts < 10) {
    ret = mqtt.connect();
    attempts++;
    delay(500);
  }
  digitalWrite(LED_PIN, LOW);
  if (ret != 0) {
    halt();
  }
//  Serial.println("MQTT Connected!");


}

void loop() {

  // Reset everything if disconnected or too many transmit failures occured in a row.
  if ( (txFailures >= MAX_TX_FAILURES)) {
    halt();
  }

  // Grab a GPS reading.
  float latitude, longitude, speed_kph, heading, altitude;
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  // Grab battery reading
  uint16_t vbat;
  fona.getBattPercent(&vbat);

  // get the temperature
  float temperature;
  bmp.getTemperature(&temperature);
  temperature = temperature*9.0/5.0 + 32;
  logTemperature(temperature, temperature_feed);

  // Log the current location to the path feed, then reset the counter.
  logLocation(latitude, longitude, altitude, location_feed);
  logBatteryPercent(vbat, battery_feed);
  
  // Wait 5 seconds
  delay(5000);

}







// Log battery
void logBatteryPercent(uint32_t indicator, Adafruit_MQTT_Publish& publishFeed) {

  // Publish
//  Serial.print(F("Publishing battery percentage: "));
//  Serial.println(indicator);
  if (!publishFeed.publish(indicator)) {
//    Serial.println(F("Publish failed!"));
    txFailures++;
    txFailure_flash();
  }
  else {
//    Serial.println(F("Publish succeeded!"));
    txFailures = 0;
    success_flash();
  }
  
}

// Log temperature
void logTemperature(float data, Adafruit_MQTT_Publish& publishFeed) {

  // Publish
//  Serial.print(F("Publishing battery percentage: "));
//  Serial.println(data);
  if (!publishFeed.publish(data)) {
//    Serial.println(F("Publish failed!"));
    txFailures++;
    txFailure_flash();
  }
  else {
//    Serial.println(F("Publish succeeded!"));
    txFailures = 0;
    success_flash();
  }
  
}

// Serialize the lat, long, altitude to a CSV string that can be published to the specified feed.
void logLocation(float latitude, float longitude, float altitude, Adafruit_MQTT_Publish& publishFeed) {
  // Initialize a string buffer to hold the data that will be published.
  char sendBuffer[120];
  memset(sendBuffer, 0, sizeof(sendBuffer));
  int index = 0;


  // Now set latitude, longitude, altitude separated by commas.
  dtostrf(latitude, 2, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(longitude, 3, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(altitude, 2, 6, &sendBuffer[index]);
  //index += strlen(&sendBuffer[index]);
  //sendBuffer[index++] = '"';

  // Finally publish the string to the feed.
//  Serial.print(F("Publishing location: "));
//  Serial.println(sendBuffer);
  if (!publishFeed.publish(sendBuffer)) {
//    Serial.println(F("Publish failed!"));
    txFailures++;
    txFailure_flash();
  }
  else {
//    Serial.println(F("Publish succeeded!"));
    txFailures = 0;
    success_flash();
  }
}


// Halt function called when an error occurs.  Will print an error and stop execution while
// doing a fast blink of the LED.  If the watchdog is enabled it will reset after 8 seconds.
void halt() {
  // Flash the LED a few times
  for (int i=0; i<5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  
  ESP.restart();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

//  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
//    Serial.println(mqtt.connectErrorString(ret));
//    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
//  Serial.println("MQTT Connected!");
}


void flash_lights() {
  for (int i=0; i<10; i++){
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN2, LOW);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED_PIN2, HIGH);
    delay(100);
    digitalWrite(LED_PIN2, LOW);
  }
}

void success_flash() {
  for (int i=0; i<10; i++){
    digitalWrite(LED_PIN2, HIGH);
    delay(100);
    digitalWrite(LED_PIN2, LOW);
    delay(100);
  }
}

void txFailure_flash() {
  for (int i=0; i<txFailures; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
}

