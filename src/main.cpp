#include <Arduino.h>
#include <TimeLib.h>
#include <DHT.h>

#define BLYNK_TEMPLATE_ID "TMPLrF3QZtDg"
#define BLYNK_DEVICE_NAME "NODEMCU"
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#define soil_humidity_pin A0 //v2
#define ambient_temperature_pin D2 //v3
#define DHTTYPE DHT22 

DHT dht(ambient_temperature_pin, DHTTYPE);
#include "BlynkEdgent.h"

BlynkTimer timer;

int switchStart;
int switchEnd;

//Time Zone holder
time_t date = now();
unsigned long long phtime;
unsigned long long startingTime;
String startingTimeString;

//Sensor
int soil_humidity_value;
int ambient_temperature_value;

//timer ID holder
int soil_humidity_ID;
int ambient_temperature_ID;

//mapping
int soil_humidity_map;





//functions

void soil_humidity() //check mapping
{
  //turn off ang sensor kung possible
  soil_humidity_value = analogRead(soil_humidity_pin);
  soil_humidity_map = map(soil_humidity_value, 0, 1023, 0, 100);
  Blynk.virtualWrite(V2, soil_humidity_map);
  Serial.println("Soil Humidity: " + String(soil_humidity_map));
}

void ambient_temperature()
{
  ambient_temperature_value = dht.readTemperature();
  Blynk.virtualWrite(V3,ambient_temperature_value );
  Serial.println("DHT: " + String(ambient_temperature_value));
}



BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  phtime = param.asLong();
  setTime(phtime);
}

BLYNK_WRITE(V0) //turn on switch
{
  switchStart = param.asInt();
  if(switchStart == 1)
  {
    if(startingTime == 0){
      startingTime = phtime;
      startingTimeString = String(day(startingTime)) + "/"+String(month(startingTime)) +"/"+ String(year(startingTime));
      Serial.println("All timer on " + String(year()));
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong

      timer.enable(soil_humidity_ID); //enabling the timer for soil humidity
      timer.enable(ambient_temperature_ID); //enabling the timer for ambient temperature
    }
    digitalWrite(2, HIGH);
    Serial.println("date "+ String(startingTime) + "\n phtime: " + String(now()) );
    Blynk.virtualWrite(V7, 0); // sets end switch to zero
    
  }
  else{
    digitalWrite(2, LOW);
  }
}

BLYNK_WRITE(V7)
{
  switchEnd = param.asInt();
  if(switchEnd == 1){
    if(startingTime != 0)
    {
      Blynk.virtualWrite(V0, 0);
      startingTime = 0;
      startingTimeString ="Not Started";
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong

      timer.disable(soil_humidity_ID); //disabling timer for soil humidty
      timer.disable(ambient_temperature_ID); //disabling ambient_temperature ID
    }
  } 
}

BLYNK_WRITE(V6){
  startingTime = param.asLong();
}

BLYNK_WRITE(V5)
{
 startingTimeString = param.asString();
}
BLYNK_CONNECTED()
{
  //Blynk.syncVirtual(V0);  
 // Blynk.syncVirtual(V7);  
  //Blynk.syncVirtual(V5);
  //Blynk.syncVirtual(V6);
   Blynk.syncAll();
  Blynk.sendInternal("rtc", "sync");
 
}

void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT); // Initialise digital pin 2 as an output pin
  pinMode(soil_humidity_pin, INPUT);

  timer.setTimeout(3600000L, [] () {} ); // dummy/sacrificial Function
  soil_humidity_ID = timer.setInterval(2000L, soil_humidity);
  ambient_temperature_ID = timer.setInterval(2500L, ambient_temperature);
  Serial.begin(115200);
  dht.begin();
  BlynkEdgent.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("Starting time on loop " + String(startingTime));
  timer.run();
  BlynkEdgent.run();
}