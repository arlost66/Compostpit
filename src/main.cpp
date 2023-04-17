#include <Arduino.h>
#include <TimeLib.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include <ESP8266WiFi.h>


#define BLYNK_TEMPLATE_ID "TMPLrF3QZtDg"
#define BLYNK_DEVICE_NAME "NODEMCU"
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#define USE_NODE_MCU_BOARD

#define soil_humidity_pin A0 //v2
#define ambient_temperature_pin D2 //v3
#define soil_temperature_pin D1 //v1
#define gas_sensor_pin D5
#define water_pump_pin D7
#define servo_motor_pin D4
#define dc_motor_pin D6

#define switch_pin 10 //SD3

int  dc_state;
//led pin is relay pin of servo motor
int switchNew;
int switchOld; //put on timer

#define DHTTYPE DHT22

OneWire oneWire(soil_temperature_pin);
DallasTemperature sensors(&oneWire); //soil temperature

DHT dht(ambient_temperature_pin, DHTTYPE);
#include "BlynkEdgent.h"

Servo myservo;

BlynkTimer timer;

//int switch_dc_motor;

const int loadAddress = 30;
int load = 0;

int switchStart;
int switchEnd;

//Time Zone holder
time_t date = now();
unsigned long long phtime = (unsigned long long) date;
unsigned long long startingTime;
unsigned long long temp;
String startingTimeString;
String endTimeString;

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
int soil_moisture_automation_ID;
int shredder_ID;
int reenable_ID;
int reconnect_ID;

//mapping
int soil_humidity_map;

//turning mode
int turn_mode;

//alerts flag
int alert_sent_turning[6] = {0,0,0,0,0,0};
int soil_temperature_flag[7] = {0,0,0,0,0,0,0};
int soil_humidity_flag[6] = {0,0,0,0,0,0};
int gas_flag[2] = {0,0};


BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  phtime = param.asLong();
  setTime(phtime);
}

BLYNK_WRITE(V0) //turn on switch
{
  switchStart = param.asInt();
  if(switchStart == 1)
  {
    if(startingTime == 0)
    {
      startingTime = phtime;
      Serial.println("Starting TIme unix: " + String(phtime));
      Serial.println("Starting time: " + String(startingTime));


      startingTimeString = String(day(startingTime)) + "/"+String(month(startingTime)) +"/"+ String(year(startingTime));
      endTimeString = String(day(startingTime + (86400 * 36))) + "/" + String(month(startingTime + (86400 * 36))) + "/" + String(year(startingTime + (86400*36)));
      
      if(Blynk.connected() != 0)
          {
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong
      Blynk.virtualWrite(V9, endTimeString);
          }

      Serial.println("LOG SENT");
     // Blynk.logEvent("turning_time_event", String("V1 notification log"));

      timer.restartTimer(soil_humidity_ID);
      timer.restartTimer(ambient_temperature_ID);
      timer.restartTimer(soil_temperature_ID);
      timer.restartTimer(gas_sensor_ID);
    
      timer.enable(soil_humidity_ID); //enabling the timer for soil humidity
      timer.enable(ambient_temperature_ID); //enabling the timer for ambient temperature
      timer.enable(soil_temperature_ID);
      timer.enable(gas_sensor_ID);
      timer.enable(turning_automation_ID);
      timer.enable(soil_temperature_automation_ID);
      timer.enable(soil_moisture_automation_ID);
      timer.disable(reenable_ID);
    }
     else if(startingTime != 0 )
    {
       Serial.println("TURN ON");

      timer.restartTimer(soil_humidity_ID);
      timer.restartTimer(ambient_temperature_ID);
      timer.restartTimer(soil_temperature_ID);
      timer.restartTimer(gas_sensor_ID);

      timer.enable(soil_temperature_automation_ID);
      timer.enable(soil_moisture_automation_ID);
      timer.enable(turning_automation_ID);
       
       timer.disable(reenable_ID);
    }
   // digitalWrite(2, HIGH);
    if(Blynk.connected() != 0)
    {
      Blynk.virtualWrite(V7, 0); // sets end switch to zero
    }
    
    Serial.println("All timer on " + String(timer.isEnabled(soil_moisture_automation_ID)));
  } 
   // Serial.println("date "+ String(startingTime) + "\n phtime: " + String(now()) );
  else if(switchStart == 0)
  {
     if(startingTime != 0)
     {
      timer.restartTimer(soil_humidity_ID);
      timer.restartTimer(ambient_temperature_ID);
      timer.restartTimer(soil_temperature_ID);
      timer.restartTimer(gas_sensor_ID);

      digitalWrite(water_pump_pin, LOW);
      digitalWrite(dc_motor_pin, LOW);
      timer.disable(soil_temperature_automation_ID);
      timer.disable(soil_moisture_automation_ID);
      timer.disable(turning_automation_ID);
      timer.enable(reenable_ID);
      Serial.println(" Switch Automation is disabled but monitoring is still ongoing" + String(timer.isEnabled(soil_moisture_automation_ID)) );

      //digitalWrite(2, LOW);
     }
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
      endTimeString =" ";
      if(Blynk.connected() != 0)
          {
      Blynk.virtualWrite(V5,startingTimeString); // sends string in V5
      Blynk.virtualWrite(V6, startingTime); //UNIX value in longlong
      Blynk.virtualWrite(V9, endTimeString);
          }

      timer.disable(soil_humidity_ID); //disabling timer for soil humidty
      timer.disable(ambient_temperature_ID); //disabling ambient_temperature ID
      timer.disable(soil_temperature_ID);// disabling soil temperature ID
      timer.disable(gas_sensor_ID);
      timer.disable(turning_automation_ID);
      timer.disable(soil_temperature_automation_ID);
      timer.disable(soil_moisture_automation_ID);

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
      soil_temperature_flag[6] = 0;

      soil_humidity_flag[0] = 0;
      soil_humidity_flag[1] = 0;
      soil_humidity_flag[2] = 0;

      gas_flag[0] = 0;
      gas_flag[1] = 0;

    }
    if(Blynk.connected() != 0)
          {
    Blynk.virtualWrite(V0, 0);
          }
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
    gas_sensor_value = param.asInt();
}
BLYNK_WRITE(V6){
  startingTime = param.asLong();
}

BLYNK_WRITE(V5)
{
 startingTimeString = param.asString();
}

BLYNK_WRITE(V8)
{
  load = param.asInt();
  digitalWrite(dc_motor_pin, load);
}

//functions

void soil_humidity() //check mapping
{
  //turn off ang sensor kung possible
  soil_humidity_value = analogRead(soil_humidity_pin);
  soil_humidity_map = map(soil_humidity_value, 0, 1023, 100, 0);
  if(Blynk.connected() != 0)
          {
           Blynk.virtualWrite(V2, soil_humidity_map);
          }
  //Serial.println("Soil Humidity: " + String(soil_humidity_map));
  Serial.println("S Automation is disabled but monitoring is still ongoing" + String(timer.isEnabled(soil_moisture_automation_ID)) + String(timer.isEnabled(soil_temperature_automation_ID)) + String(timer.isEnabled(turning_automation_ID)) );
}

void ambient_temperature()
{
  ambient_temperature_value = dht.readTemperature();
  
  //2147483647 if no sensor is detected;
  if(isnan(ambient_temperature_value))
  {
    Serial.println("Failed to read from DHT sensor");
  }
  else if(ambient_temperature_value == 2147483647)
  {
    Serial.println("Failed to read from DHT sensor");
  }
  else{
    if(Blynk.connected() != 0)
          {
    Blynk.virtualWrite(V3,ambient_temperature_value );
          }
  Serial.println("DHT: " + String(ambient_temperature_value));
  }
}

void soil_temperature()
{
  sensors.requestTemperatures();
  soil_temperature_value = sensors.getTempCByIndex(0);
  if(Blynk.connected() != 0)
          {
            Blynk.virtualWrite(V1, soil_temperature_value);
          }
  
  if (soil_temperature_value != DEVICE_DISCONNECTED_C)
  {
   Serial.println("Soil Temperature: " + String(soil_temperature_value));
  }
  else {
    Serial.println("Error could not read temperature data DS18B20");
  }
  //if soil temperature is -127 the sensor is not functioning properly
}


void gas_sensor()
{
  gas_sensor_value = digitalRead(gas_sensor_pin);
  if(gas_sensor_value == 1)
  {
    
    if(gas_flag[0] == 0)
    {
      
      Serial.println("Methane gas not detected");
      gas_flag[0] = 1;
      gas_flag[1] = 0;
    }
    
  }
  else if(gas_sensor_value == 0)
  {
    if(gas_flag[1] == 0)
    {
      if(Blynk.connected() != 0)
          {
      Serial.println("Methane gas detected");
      Blynk.logEvent("info", String("Methane is detected. Mix your compost asap."));
      gas_flag[0] = 0;
      gas_flag[1] = 1;
          }
    }
  }

  if (gas_sensor_value == 1)
  {
    if(Blynk.connected() != 0)
          {
             Blynk.virtualWrite(V4, 0);
          }
   
  }
  else if(gas_sensor_value == 0)
  {
    if(Blynk.connected() != 0)
          {
    Blynk.virtualWrite(V4, 1);
          }
  }
  

  
}

void water_pump(String temp1)
{
 if(temp1 == "on")
 {
    digitalWrite(water_pump_pin, HIGH);
    Serial.println("Water is on");
 }
 else if(temp1 == "off")
 {
    digitalWrite(water_pump_pin, LOW);
    Serial.println("Water is off");
 }
}

void servo_motor(String temp1)
{
  if(temp1 == "open")
  {
    myservo.write(180);
    Serial.println("Servo is open");
  }
  else if(temp1 == "close")
  {
    myservo.write(0);
    Serial.println("Servo is close");
  }
}

void turning_automation()//should not be activated within the first day
{
  //soil  moisture == 40 or 60%
 // Serial.println("Automation() in ");
   //setting turning Logic
 // Serial.println("Starting Time: " + String(startingTime) + "\n PHTIME: " + String((long long int)now()));
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
      //Serial.println("Activate water pump before setting turn mode into 1;");
      turn_mode = 1;
    }
  }
  Serial.println("Turn_mode = " + String(turn_mode));
  Serial.println("Alert sent turning: " + String(alert_sent_turning[0]) + String(alert_sent_turning[1]) + String(alert_sent_turning[2]) + String(alert_sent_turning[3]) + String(alert_sent_turning[4]) + String(alert_sent_turning[5]));
  if(turn_mode == 1)
  {//|| startingTime + (86400 * 6) == phtime || startingTime + (86400 * 9) == phtime || startingTime + (86400 * 12) == phtime

          Serial.println("Starting Time: " + String(startingTime));
          Serial.println("Starting Time: " + String((long long int)now()));
          Serial.println("Starting Time: " + String(startingTime + (86400 * 3)));
    if(startingTime + (86400 * 3) >= (long long int)now() )
        {
          
          if(alert_sent_turning[0] == 0)
          {
            if(Blynk.connected() != 0)
            {
               Blynk.logEvent("info", String("Turn the compost. In the next 9 days in 3 days interval. 1/4 notification"));
               alert_sent_turning[0] = 1;
            }
            
          }
          Serial.println("Day3");
          //notify for turning
        }
    else if(startingTime + (86400 * 6) >= (long long int)now())
       {
        if(alert_sent_turning[1] == 0)
        {
          if(Blynk.connected() != 0)
          {
            Blynk.logEvent("info", String("Turn the compost. In the next 6 days in 3 days interval. 2/4 notification"));
            alert_sent_turning[1] = 1;
          }
          
        
        Serial.println("Day 6");
       }
    else if(startingTime + (86400 * 9) >= (long long int)now())
    {
      if(alert_sent_turning[2] == 0)
      {
        if(Blynk.connected() != 0)
          {
       Blynk.logEvent("info", String("Turn the compost. In the next 3 days in 3 days interval. 3/4 notification"));
       alert_sent_turning[2] = 1;
          }
      }
      Serial.println("Day 9");
    } 
    else if(startingTime + (86400 * 12) >= (long long int)now())
    {
      if(alert_sent_turning[3] == 0)
      {
        if(Blynk.connected() != 0)
          {
        Blynk.logEvent("info", String("Turn the compost. The last day of turning. 4/4 notification"));
        alert_sent_turning[3] = 1;
          }
      }
      Serial.println("Day 12");
    }
    else
    {
      Serial.println("done");
    }
  }
  else if(turn_mode == 2 )
  {//|| startingTime + (86400 * 4) == phtime || startingTime + (86400 * 6) == phtime || startingTime + (86400 * 8) == phtime || startingTime + (86400 * 10) == phtime || startingTime + (86400 * 12) == phtime
     if(startingTime + (86400 * 2) >= (long long int)now()  )
      {
        if(alert_sent_turning[0] == 0)
        {
          if(Blynk.connected() != 0)
          {
          Blynk.logEvent("info", String("Turn the compost. In the next 10 days in 2 days interval. 1/6 notification"));
          alert_sent_turning[0] = 1;
          }
        }
        Serial.println("Day 2");
        //notify for turning
      }   
      else if(startingTime + (86400 * 4) >= (long long int)now() )
      {
        if(alert_sent_turning[1] == 0)
        {
          if(Blynk.connected() != 0)
          {
           Blynk.logEvent("info", String("Turn the compost. In the next 8 days in 2 days interval. 2/6 notification"));
           alert_sent_turning[1] = 1;
          }
        }
       Serial.println("Day  4");
      }
      else if(startingTime + (86400 * 6) >= (long long int)now()  )
      {
        if(alert_sent_turning[2] == 0)
        {
          if(Blynk.connected() != 0)
          {
          Blynk.logEvent("info", String("Turn the compost. In the next 6 days in 2 days interval. 3/6 notification"));
          alert_sent_turning[2] = 1;
          }
        }
        Serial.println("Day 6");
      }
      else if (startingTime + (86400 * 8) >= (long long int)now() )
      {
        if (alert_sent_turning[3] == 0)
        {
          if(Blynk.connected() != 0)
          {
          Blynk.logEvent("info", String("Turn the compost. In the next 4 days in 2 days interval. 4/6 notification"));
          alert_sent_turning[3] = 1;
          }
          Serial.println("Day 8");
        }
      }
      else if (startingTime + (86400 * 10) >= (long long int)now() )
      {
        if(alert_sent_turning[4] == 0)
        {
          if(Blynk.connected() != 0)
          {
          Blynk.logEvent("info", String("Turn the compost. In the next 2 days in 2 days interval. 5/6 notification"));
          alert_sent_turning[4] = 1;
          }
        }
       Serial.println("Day 10");
      }
      else if (startingTime + (86400 * 12) >= (long long int)now() )
      {
        if (alert_sent_turning[5] == 0)
        {
          if(Blynk.connected() != 0)
          {
           Blynk.logEvent("info", String("Turn the compost. The last day of turning. 6/6 notification"));
          alert_sent_turning[5] = 1;
          }
        }
        Serial.println("Day 12");
      }  
      else{
        Serial.println("TURN 2 else");
      }
  }
}
}

void soil_temperature_automation()
{
  //Serial.println("Soil Temperature automation");
  if(startingTime + (86400 * 2) >= (long long int)now()) //three days
  {
    Serial.println("startingTime + (86400 * 2) >= (long long int)now()");
    //temperature
    if(soil_temperature_value < 32  )
    {
      if(soil_temperature_flag[0] == 0)
      {
        if(Blynk.connected() != 0)
          {
          Blynk.logEvent("soil_temperature", String ("The temperature is normally low in the few days of composting."));
          Serial.println("After three days LOG SENT");
          soil_temperature_flag[0] = 1;
          soil_temperature_flag[1] = 0;
          soil_temperature_flag[2] = 0;
          soil_temperature_flag[3] = 0;
          soil_temperature_flag[4] = 0;
          soil_temperature_flag[5] = 0;
          soil_temperature_flag[6] = 0;
          }
      }
      //close vent
    }
    else if(soil_temperature_value >= 32 )
    {
      if(soil_temperature_flag[5] == 0)
      {
        if(Blynk.connected() != 0)
          {
        Blynk.logEvent("soil_temperature", String("Compost is in good temperature")); 
        soil_temperature_flag[5] = 1;
        soil_temperature_flag[0] = 0;
        soil_temperature_flag[1] = 0;
        soil_temperature_flag[2] = 0;
        soil_temperature_flag[3] = 0;
        soil_temperature_flag[4] = 0;
        soil_temperature_flag[6] = 0;
          }
      }
      
      Serial.println("Temperature is back to optimal temperature");
     
    }
  }

  else if(startingTime+(86400 * 25) >= (long long int) now())
  {

    Serial.println("startingTime+(86400 * 25) >= (long long int) now() && startingTime  + (86400 * 3");
    if(soil_temperature_value >= 32 && soil_temperature_value < 60  )
    {
      if(soil_temperature_flag[1] == 0)
      {
        if(Blynk.connected() != 0)
          {
          Blynk.logEvent("soil_temperature", String("Compost is in Optimal Temperature"));
          Serial.println("After three days LOG SENT optimal");
           soil_temperature_flag[1] = 1;
           soil_temperature_flag[2] = 0;
           soil_temperature_flag[3] = 0;
           soil_temperature_flag[4] = 0;
           soil_temperature_flag[0] = 0;
           soil_temperature_flag[5] = 0;
           soil_temperature_flag[6] = 0;
          }
      }
      //close vents
    }
    else if(soil_temperature_value < 32 )
    {
      if(soil_temperature_flag[2] == 0)
      {
        if(Blynk.connected() != 0)
          {
           if(ambient_temperature_value < soil_temperature_value)
        {
           // Blynk.logEvent("soil_temperature", String("Compost is below Optimal temperature. Add Green Materials or Nitrogen rich materials"));
       }
        else if(soil_temperature_value > ambient_temperature_value)
       {
            Blynk.logEvent("soil_temperature", String("Compost is below Optimal temperature. Add Green Materials or Nitrogen rich materials"));
        }
        soil_temperature_flag[2] = 1;
        soil_temperature_flag[1] = 0;
        soil_temperature_flag[3] = 0;
        soil_temperature_flag[4] = 0;
        soil_temperature_flag[5] = 0;
        soil_temperature_flag[0] = 0;
        soil_temperature_flag[6] = 0;
      }
      }  
    }
    else if(soil_temperature_value > 59 )
    {
     
      if(Blynk.connected() != 0)
          {
       if(soil_temperature_flag[3] == 0)
      {
        if(Blynk.connected() != 0)
          {
        Blynk.logEvent("soil_temperature", String("Compost is in excessive temperature. Turn the compost"));
        soil_temperature_flag[3] = 1;
        soil_temperature_flag[2] = 0;
        soil_temperature_flag[1] = 0;
        soil_temperature_flag[4] = 0;
        soil_temperature_flag[5] = 0;
        soil_temperature_flag[0] = 0;
         soil_temperature_flag[6] = 0;
         }
      }
    }
      Serial.println("Overheating");
    }
    
  }

  else if(startingTime + (86400 * 36) >= (long long int) now())//compost is in curing stage
  {
     Serial.println("startingTime + (86400 * 36) >= (long long int) now() && startingTime + (86400 * 26) <= (long long int) now()");
    if(soil_temperature_flag[6] == 0)
    {
      if(Blynk.connected() != 0)
          {
         Blynk.logEvent("soil_temperature", String("The Compost is now in curing stage."));
          soil_temperature_flag[6] = 1;
          soil_temperature_flag[4] = 0;
          soil_temperature_flag[3] = 0;
          soil_temperature_flag[2] = 0;
          soil_temperature_flag[1] = 0;
          soil_temperature_flag[5] = 0;
          soil_temperature_flag[0] = 0;
     // water_pump("off");
          }
    }
  }

  if(soil_temperature_value >= 40)
  {
    servo_motor("open");

  }
  else
  {
    servo_motor("close");
  }
}

//int soil_humidity_flag[6]
void soil_moisture_automation()
{
  if(soil_humidity_map < 40 )
  {
    if(soil_humidity_flag[0] == 0)
    {
      Blynk.logEvent("soil_moisture", String("The moisture level is low. Adding water. You may want to add green Materials"));
       soil_humidity_flag[0] = 1;
       soil_humidity_flag[1] = 0;
       soil_humidity_flag[2] = 0;
    }
    //add water and close vent
    Serial.println("Adding water < 40 soil moisture");
   
    water_pump("on");
  }
  else if(soil_humidity_map > 60)
  {
    //Turn your compost and add brown materials.
    //open vents
    if(soil_humidity_flag[1] == 0)
    {
      Blynk.logEvent("soil_moisture", String("The moisture level is high. opening vents and turn compost. You may add brown or Carbon-rich materials"));
       soil_humidity_flag[1] = 1;
       soil_humidity_flag[0] = 0;
       soil_humidity_flag[2] = 0; 
    }
    //open vents
    Serial.println("Humidity level " + String(soil_humidity_map) );
    water_pump("off");
  }
  else if(soil_humidity_map <= 60 && soil_humidity_map >= 40 )
  {
    if(soil_humidity_flag[2] == 0)
    {
      Blynk.logEvent("soil_moisture", String("The moisture is in optimal level."));
    soil_humidity_flag[2] = 1;
    soil_humidity_flag[1] = 0;
    soil_humidity_flag[0] = 0;
    }
    Serial.println("Optima Moisture " + String(soil_humidity_map) );
    water_pump("off");
    
  }
}

void reenable()
{
  int enable = timer.isEnabled(soil_temperature_automation_ID);
    if(enable != 1)
    {
      timer.enable(soil_temperature_automation_ID);
      timer.enable(soil_moisture_automation_ID);
      timer.enable(turning_automation_ID);
      timer.disable(reenable_ID);
      if(Blynk.connected() != 0)
          {
            Blynk.virtualWrite(V0, 1);
            Serial.println("Reenable Automation is disabled but monitoring is still ongoing");
          }
    }
}

int flagShredder[2] = {0,0};
void shredder()
{
  // int switchState = digitalRead(switch_pin);

  // if(switchState == LOW)
  // {
  //   digitalWrite(dc_motor_pin, HIGH);
  //   load = 1;
    
  //   if(flagShredder[0] == 0)
  //   {
  //     Blynk.virtualWrite(V9, load);
  //     EEPROM.write(loadAddress, load);
  //     flagShredder[0] = 1;
  //     flagShredder[1] = 0;
  //   }
  // }
  // else 
  // {
  //   digitalWrite(dc_motor_pin, LOW);
  //   load = 0;
  //   if(flagShredder[1] == 0)
  //   {
  //     Blynk.virtualWrite(V9, load);
  //     EEPROM.write(loadAddress, load);
  //     flagShredder[1] = 1;
  //     flagShredder[0] = 0;
  //   }
  // }
  BlynkEdgent.run();
  switchNew = digitalRead(switch_pin);
  if(switchOld == 0 && switchNew == 1)
  {
    if(dc_state == 0)
    {
      digitalWrite(dc_motor_pin, HIGH);
      dc_state = 1;
      if(flagShredder[0] == 0)
     {
        //Blynk.virtualWrite(V9, load);
       EEPROM.write(loadAddress, load);
       flagShredder[0] = 1;
       flagShredder[1] = 0;
     }
      Serial.println("Shredder ON");
    }
    else
    {
      digitalWrite(dc_motor_pin, LOW);
      dc_state = 0;
       if(flagShredder[1] == 0)
      {
        //Blynk.virtualWrite(V9, load);
        EEPROM.write(loadAddress, load);
       flagShredder[1] = 1;
        flagShredder[0] = 0;
       }
      Serial.println("Shredder OFF");
    }
  }
  switchOld = switchNew;
  
}

BLYNK_CONNECTED()
{
  Blynk.syncAll();
  Blynk.sendInternal("rtc", "sync");
}
String temp32;
void recon()
{
  if (Blynk.connected() == 0 )
  {
    BlynkEdgent.run();
  }
  else if(Blynk.connected() == 1)
  {
   // timer.disable(reconnect_ID);
  }


    
}

void setup() {
  // put your setup code here, to run once:

  pinMode(2, OUTPUT); // Initialise digital pin 2 as an output pin
  pinMode(soil_humidity_pin, INPUT);
  pinMode(soil_temperature_pin, INPUT);
  pinMode(ambient_temperature_pin, INPUT);
  pinMode(gas_sensor_pin, INPUT);
  pinMode(switch_pin, INPUT_PULLUP);

  load = EEPROM.read(loadAddress);
  digitalWrite(dc_motor_pin, load);

  pinMode(water_pump_pin, OUTPUT);
  myservo.attach(servo_motor_pin);
  pinMode(dc_motor_pin, OUTPUT);

  digitalWrite(water_pump_pin, LOW);
  digitalWrite(dc_motor_pin, LOW);

  //restart timer if dugay na on

  timer.setTimeout(3600000L, [] () {} ); // dummy/sacrificial Function
  soil_humidity_ID = timer.setInterval(2000L, soil_humidity);
  ambient_temperature_ID = timer.setInterval(2500L, ambient_temperature);
  soil_temperature_ID = timer.setInterval(2200L, soil_temperature);
  gas_sensor_ID = timer.setInterval(4000L, gas_sensor);
  turning_automation_ID = timer.setInterval(1600L, turning_automation);
  soil_temperature_automation_ID = timer.setInterval(2300L, soil_temperature_automation);
  soil_moisture_automation_ID = timer.setInterval(2000L, soil_moisture_automation );
  reenable_ID = timer.setInterval(3600000L, reenable); //3600000 1 hour
  reconnect_ID = timer.setInterval(3000L, recon);
  //timer.setInterval(300L, switchDC);

  timer.setInterval(300L, shredder);
  Serial.begin(9600);

  dht.begin();//ambient temperature
  sensors.begin();//soil temperature
  BlynkEdgent.begin();
  BlynkEdgent.run();
}


void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("Starting time on loop " + String(startingTime));
//   timer.run();

 timer.run();
}