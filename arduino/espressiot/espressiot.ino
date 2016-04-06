//
// ESPressIoT Controller for Espresso Machines
// 2016 by Roman Schmitz
//
// Uses PID library
//

#include <PID_v1.h>
#define SIMULATION_MODE

//
// STANDARD reset values based on Gaggia CC
//
#define S_P 115.0
#define S_I 4.0
#define S_D 850.0
#define S_aP 100.0
#define S_aI 0.0
#define S_aD 0.0
#define S_TSET 94.0
#define S_TBAND 1.5

//
// Intervals for I/O
//
#define HEATER_INTERVAL 1000
#define DISPLAY_INTERVAL 1000
#define PID_INTERVAL 200

//
// global variables
//
double gTargetTemp=S_TSET;
double gOvershoot=S_TBAND;
double gInputTemp=20.0;
double gOutputPwr=0.0;
double gP = S_P, gI = S_I, gD = S_D;
double gaP = S_aP, gaI = S_aI, gaD = S_aD;

unsigned long time_now=0;
unsigned long time_last=0;

boolean tuning = false;
boolean osmode = false;

//
// parameters for tuning loop
//
double aTuneStep=100.0, aTuneNoise=0.2;

//
// gloabl classes
//
PID ESPPID(&gInputTemp, &gOutputPwr, &gTargetTemp, gP, gI, gD, DIRECT);

void setup()
{
  gOutputPwr=0;

  Serial.begin(115200);
  Serial.println("Settin up PID...");

  // setup components
  setupHeater();
  setupSensor();

  // start PID
  ESPPID.SetTunings(gP, gI, gD);
  ESPPID.SetSampleTime(PID_INTERVAL);
  ESPPID.SetOutputLimits(0, 1000);
  ESPPID.SetMode(AUTOMATIC);
 
  time_now=millis();
  time_last=time_now;
    
}

void serialStatus() {
  Serial.print(gInputTemp, 2); Serial.print(" ");
  Serial.print(gTargetTemp, 2); Serial.print(" ");
  Serial.print(gOutputPwr, 2); Serial.print(" ");
  Serial.print(gP, 2); Serial.print(" ");
  Serial.print(gI, 2); Serial.print(" ");
  Serial.print(gD, 2); Serial.print(" ");
  Serial.print(ESPPID.GetKp(), 2); Serial.print(" ");
  Serial.print(ESPPID.GetKi(), 2); Serial.print(" ");
  Serial.print(ESPPID.GetKd(), 2);
  Serial.println("");
}

void loop() {
  time_now=millis();

  updateTempSensor(); 
  gInputTemp=getTemp();

  if(abs(time_now-time_last)>=PID_INTERVAL or time_last > time_now) {
    if(tuning==true)
    {
      ESPPID.SetMode(MANUAL);
      if(gInputTemp<(gTargetTemp-aTuneNoise)) setHeatPowerPercentage(aTuneStep);
      else if(gInputTemp>(gTargetTemp+aTuneNoise)) setHeatPowerPercentage(0);      
    }
    else  {
      if( !osmode && abs(gTargetTemp-gInputTemp)>=gOvershoot ) {        
        ESPPID.SetTunings(gaP, gaI, gaD);
        osmode=true;
      }
      else if( osmode && abs(gTargetTemp-gInputTemp)<gOvershoot ) {
        ESPPID.SetTunings(gP,gI,gD);
        osmode=false;
      }
      if(ESPPID.Compute()==true) {   
        setHeatPowerPercentage(gOutputPwr);
        serialStatus();
      }
    }        
  }

  updateHeater();
}
