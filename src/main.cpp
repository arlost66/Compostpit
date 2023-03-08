#include <Arduino.h>
#include <TimeLib.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BLYNK_TEMPLATE_ID "TMPLrF3QZtDg"
#define BLYNK_DEVICE_NAME "NODEMCU"
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#define soil_humidity_pin A0 //v2
#define ambient_temperature_pin D2 //v3
#define soil_temperature_pin D1 //v1
#define gas_sensor_pin D5

#define DHTTYPE DHT22

OneWire oneWire(soil_temperature_pin);
DallasTemperature sensors(&oneWire); //soil temperature

DHT dht(ambient_temperature_pin, DHTTYPE);
#include "BlynkEdgent.h"

BlynkTimer timer;

int switchStart;
int switchEnd;

//Time Zone holder
time_t date = now();
unsigned long long phtime = (unsigned long long) date;
unsigned long long startingTime;
String startingTimeString;

//Sensor
int soil_humidity_value;
int ambient_temperature_value;
int soil_temperature_value;
int gas_sensor_value;

//timer ID holder
int soil_humidity_ID;
int ambient_temperature_ID;
int soil_temperature_ID;
int gas_sensor_ID;
int turning_automation_ID;
int soil_temperature_automation_ID;

//mapping
int soil_humidity_map;

//turning mode
int turn_mode;

//alerts flag
int alert_sent_turning[6] = {0,0,0,0,0,0};
int soil_temperature_flag[6] = {0,0,0,0,0,0};





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
      Serial.println("Starting TIme unix: " + String(phtime));
      Serial.println("Starting time: " + String(startingTime));


      startingTimeString = String(day(startingTime)) + "/"+String(month(startingTime)) +"/"+ String(year(startingTime));
      Serial.println("All timer on " + String(year()));
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong

       Serial.println("LOG SENT");
      Blynk.logEvent("turning_time_event", String("V1 notification log"));

      timer.enable(soil_humidity_ID); //enabling the timer for soil humidity
      timer.enable(ambient_temperature_ID); //enabling the timer for ambient temperature
      timer.enable(soil_temperature_ID);
      timer.enable(gas_sensor_ID);
      timer.enable(turning_automation_ID);
      timer.enable(soil_temperature_automation_ID);
    
    }
    digitalWrite(2, HIGH);
   // Serial.println("date "+ String(startingTime) + "\n phtime: " + String(now()) );
    Blynk.virtualWrite(V7, 0); // sets end switch to zero
    
  }
  else{
    digitalWrite(2, LOW);
  }
}

BLYNK_WRITE(V7)//turn off switch
{
  switchEnd = param.asInt();
  if(switchEnd == 1){
    if(startingTime != 0)
    {
      //Blynk.virtualWrite(V0, 0);
      startingTime = 0;
      startingTimeString ="Not Started";
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong

      timer.disable(soil_humidity_ID); //disabling timer for soil humidty
      timer.disable(ambient_temperature_ID); //disabling ambient_temperature ID
      timer.disable(soil_temperature_ID);// disabling soil temperature ID
      timer.disable(gas_sensor_ID);
      timer.disable(turning_automation_ID);
      timer.disable(soil_temperature_automation_ID);

      turn_mode = 0; // for the first 12 days of turning
      alert_sent_turning[0] = 0;
      alert_sent_turning[1] = 0;
      alert_sent_turning[2] = 0;
      alert_sent_turning[3] = 0;
      alert_sent_turning[4] = 0;
      alert_sent_turning[5] = 0;

      soil_temperature_flag[0] = 0;
      soil_temperature_flag[1] = 0;
      soil_temperature_flag[2] = 0;
      soil_temperature_flag[3] = 0;
      soil_temperature_flag[4] = 0;
      soil_temperature_flag[5] = 0;
      
    }
    Blynk.virtualWrite(V0, 0);
  } 
}

BLYNK_WRITE(V1) // soil tempearure sync
{
  soil_temperature_value = param.asInt();
}
BLYNK_WRITE(V2)// soil humidity sync
{
  soil_humidity_value = param.asInt();
}
BLYNK_WRITE(V3)
{
  ambient_temperature_value = param.asInt();
}

BLYNK_WRITE(V4)
{

}
BLYNK_WRITE(V6){
  startingTime = param.asLong();
}

BLYNK_WRITE(V5)
{
 startingTimeString = param.asString();
}

//functions

void soil_humidity() //check mapping
{
  //turn off ang sensor kung possible
  soil_humidity_value = analogRead(soil_humidity_pin);
  soil_humidity_map = map(soil_humidity_value, 0, 1023, 0, 100);
  Blynk.virtualWrite(V2, soil_humidity_map);
 // Serial.println("Soil Humidity: " + String(soil_humidity_map));
}

void ambient_temperature()
{
  ambient_temperature_value = dht.readTemperature();
  
  //2147483647 if no sensor is detected;
  if(isnan(ambient_temperature_value))
  {
    //Serial.println("Failed to read from DHT sensor");
  }
  else if(ambient_temperature_value == 2147483647)
  {
    //Serial.println("Failed to read from DHT sensor");
  }
  else{
    Blynk.virtualWrite(V3,ambient_temperature_value );
  Serial.println("DHT: " + String(ambient_temperature_value));
  }
}

void soil_temperature()
{
  sensors.requestTemperatures();
  soil_temperature_value = sensors.getTempCByIndex(0);
  Blynk.virtualWrite(V1, soil_temperature_value);
  if (soil_temperature_value != DEVICE_DISCONNECTED_C)
  {
   // /* code */Serial.println("Soil Temperature: " + String(soil_temperature_value));
  }
  else{
    Serial.println("Error could not read temperature data DS18B20");
  }
  //if soil temperature is -127 the sensor is not functioning properly
}

void gas_sensor()
{
  gas_sensor_value = digitalRead(gas_sensor_pin);
  Blynk.virtualWrite(V4, gas_sensor_value);
  if(gas_sensor_value == 0)
  {
    //Serial.println("Methane gas is not detected");
  }
  else
  {
   // Serial.println("Methane gas detected");
  }
}


void turning_automation()//should not be activated within the first day
{
  //soil  moisture == 40 or 60%
  Serial.println("Automation() in ");
   //setting turning Logic
  Serial.println("Starting Time: " + String(startingTime) + "\n PHTIME: " + String((long long int)now()));
  if(startingTime + (86400 * 1) >= (long long int)now() && turn_mode == 0)
  {
    if(soil_humidity_value >= 40 || soil_humidity_value < 60  )
    { //86400 == 1 day seconds
      turn_mode = 1;
      Serial.println("Turn_mode = 1");
    }
    else if(soil_humidity_value >= 60 || soil_humidity_value < 70)
    {
      turn_mode = 2;
      Serial.println("Turn_mode = 2");
    }
    else if( soil_humidity_value < 40)
    {
      //activate water pump
      Serial.println("Activate water pump before setting turn mode into 1;");
      turn_mode = 1;
    }
  }

  if(turn_mode == 1)
  {//|| startingTime + (86400 * 6) == phtime || startingTime + (86400 * 9) == phtime || startingTime + (86400 * 12) == phtime
    if(startingTime + (86400 * 3) <= (long long int)now() && alert_sent_turning[0] == 0 )
        {
          Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 9 days in 3 days interval. 1/4 notification"));
          alert_sent_turning[0] = 1;
          Serial.println("Sent status True: ");
          //notify for turning
        }
    else if(startingTime + (86400 * 6) <= (long long int)now() && alert_sent_turning[1] == 0 )
       {
         Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 6 days in 3 days interval. 2/4 notification"));
         alert_sent_turning[1] = 1;
       }
    else if(startingTime + (86400 * 9) <= (long long int)now() && alert_sent_turning[2] == 0)
    {
      Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 3 days in 3 days interval. 3/4 notification"));
      alert_sent_turning[2] = 1;
    }
    else if(startingTime + (86400 * 12) <= (long long int)now() && alert_sent_turning[3] == 0)
    {
      Blynk.logEvent("turning_time_event", String("Turn the compost. The last day of turning. 4/4 notification"));
      alert_sent_turning[3] = 1;
    }


  }
  else if(turn_mode == 2 )
  {//|| startingTime + (86400 * 4) == phtime || startingTime + (86400 * 6) == phtime || startingTime + (86400 * 8) == phtime || startingTime + (86400 * 10) == phtime || startingTime + (86400 * 12) == phtime
     if(startingTime + (86400 * 2) <= (long long int)now() && alert_sent_turning[0] == 0 )
      {
        Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 10 days in 2 days interval. 1/6 notification"));
          alert_sent_turning[0] = 1;
        //notify for turning
      }   
      else if(startingTime + (86400 * 4) <= (long long int)now() && alert_sent_turning[1] == 0 )
      {
        Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 8 days in 2 days interval. 2/6 notification"));
          alert_sent_turning[1] = 1;
      }
      else if(startingTime + (86400 * 6) <= (long long int)now() && alert_sent_turning[2] == 0 )
      {
        Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 6 days in 2 days interval. 3/6 notification"));
          alert_sent_turning[2] = 1;
      }
      else if (startingTime + (86400 * 8) <= (long long int)now() && alert_sent_turning[3] == 0)
      {
       Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 4 days in 2 days interval. 4/6 notification"));
          alert_sent_turning[3] = 1;
      }
      else if (startingTime + (86400 * 10) <= (long long int)now() && alert_sent_turning[4] == 0)
      {
        Blynk.logEvent("turning_time_event", String("Turn the compost. In the next 2 days in 2 days interval. 5/6 notification"));
          alert_sent_turning[4] = 1;
      }
      else if (startingTime + (86400 * 12) <= (long long int)now() && alert_sent_turning[5] == 0)
      {
        Blynk.logEvent("turning_time_event", String("Turn the compost. The last day of turning. 6/6 notification"));
          alert_sent_turning[5] = 1;
      }  

    
  }
}


void soil_temperature_automation()
{
  Serial.println("Soil Temperature automation");
  // if(startingTime  <= (long long int)now()) //three days
  // {
  //   if(soil_temperature_value < 32 && soil_temperature_flag[0] == 0)
  //   {
  //     Blynk.logEvent("soil_temperature", String("Add green or Nitrogen rich material as the soil temperature is Low."));
  //     Serial.println("After three days LOG SENT");
  //     //close vent
  //     soil_temperature_flag[0] = 1;
  //   }
  //   else if(soil_temperature_value >= 32 && soil_temperature_flag[0] == 1)
  //   {
  //     Blynk.logEvent("soil_temperature", String("Compost is in good temperature"));
  //     Serial.println("Temperature is back to optimal temperature");
  //     soil_temperature_flag[0] == 0;
  //   }
  //                //   //
  // }
  //else if(startingTime  >= (long long int)now() && startingTime  + (86400 * 4)<= (long long int)now())
 // else if(startingTime+(86400 * 20) >= (long long int) now() && startingTime  + (86400 * 4)<= (long long int)now())

Serial.println("temp" + String(soil_temperature_value));
  if(startingTime <= (long long int) now())
  {
    if(soil_temperature_value >= 32 && soil_temperature_value < 60 && soil_temperature_flag[1] == 0 )
    {
      Blynk.logEvent("soil_temperature", String("Compost is in Optimal Temperature"));
      Serial.println("After three days LOG SENT optimal");
      soil_temperature_flag[1] = 1;
      soil_temperature_flag[2] = 0;
      soil_temperature_flag[3] = 0;
      soil_temperature_flag[4] = 0;
      //close vents
    }

    else if(soil_temperature_value < 32 && soil_temperature_flag[2] == 0)
    {
      Blynk.logEvent("soil_temperature", String("Compost is below Optimal temperature please add green or nitrogen rich material"));
      Serial.println("less than 25 days and 4th day");
      soil_temperature_flag[2] = 1;
      soil_temperature_flag[1] = 0;
      soil_temperature_flag[3] = 0;
      soil_temperature_flag[4] = 0;
      //close vents
    }
    else if(soil_temperature_value > 59 && soil_temperature_value < 71 && soil_temperature_flag[3] == 0)
    {
      Blynk.logEvent("soil_temperature", String("Compost is in excessive temperature."));
      soil_temperature_flag[3] = 1;
      soil_temperature_flag[2] = 0;
      soil_temperature_flag[1] = 0;
      soil_temperature_flag[4] = 0;
      Serial.println("Overheating");
      //open vents;
    }
    
    else if(soil_temperature_value >= 72 && soil_humidity_value < 79 && soil_temperature_flag[4] == 0)
    {
      Blynk.logEvent("soil_temperature", String("Compost is in excessive temperature. System will Add water"));
      soil_temperature_flag[4] = 1;
      soil_temperature_flag[3] = 0;
      soil_temperature_flag[2] = 0;
      soil_temperature_flag[1] = 0;
      Serial.println("Compost is in excessive temperature. System will Add water");
      //add water;
    }
  }
  
  
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
  pinMode(soil_temperature_pin, INPUT);
  pinMode(ambient_temperature_pin, INPUT);
  pinMode(gas_sensor_pin, INPUT);

  timer.setTimeout(3600000L, [] () {} ); // dummy/sacrificial Function
  soil_humidity_ID = timer.setInterval(2000L, soil_humidity);
  ambient_temperature_ID = timer.setInterval(2500L, ambient_temperature);
  soil_temperature_ID = timer.setInterval(2000L, soil_temperature);
  gas_sensor_ID = timer.setInterval(4000L, gas_sensor);
  turning_automation_ID = timer.setInterval(30000L, turning_automation);
  soil_temperature_automation_ID = timer.setInterval(2300L, soil_temperature_automation);

  Serial.begin(115200);

  dht.begin();//ambient temperature
  sensors.begin();//soil temperature
  BlynkEdgent.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("Starting time on loop " + String(startingTime));
  
      
  timer.run();
  BlynkEdgent.run();
}