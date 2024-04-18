#include <Arduino.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <MQUnifiedsensor.h>

#define placa "ESP32-WROOM-32E"
#define Voltage_Resolution 5
#define pin 32 //GPIO32 or A0 at sensor
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 12 // For ESP32-WROOM-32E
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm

unsigned long previousMillis = 0;
const long interval = 3000;
char convertedPPM[8];
float PPM;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
WiFiManager wm;

void setup() {
    // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);
    //wm.resetSettings();

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("Willhalm_esp32","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
        wm.startWebPortal();
        lcd.init();
        lcd.backlight();
        lcd.setCursor(0,0);
        lcd.print(WiFi.localIP());
    }

    //Set math model to calculate the PPM concentration and the value of constants
    MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
    MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate CO2 concentration
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("0");

    /*****************************  MQ Init ********************************************/ 
    //Remarks: Configure the pin of arduino as input.
    //pinMode(pin, INPUT);
    /************************************************************************************/ 
    MQ135.init();
    lcd.print("5");
    /* 
      //If the RL value is different from 10K please assign your RL value with the following method:
      MQ135.setRL(10);
    */

  /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
  // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");
  lcd.print("6");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  lcd.print("7");
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  lcd.print("8");
  /*****************************  MQ CAlibration ********************************************/
    MQ135.serialDebug(true);
    lcd.print("9");
}
void loop() {
    lcd.print("1");
    wm.process();
    lcd.print("2");
        if (millis() - previousMillis >= interval)
        {


            previousMillis = millis();
            MQ135.update(); // Update data, the arduino will read the voltage from the analog pin

            MQ135.setA(605.18); MQ135.setB(-3.937); // Configure the equation to calculate CO concentration value
            float CO = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
            lcd.print("B");
            MQ135.setA(77.255); MQ135.setB(-3.18); //Configure the equation to calculate Alcohol concentration value
            float Alcohol = MQ135.readSensor(); // SSensor will read PPM concentration using the model, a and b values set previously or from the setup
            
            MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to calculate CO2 concentration value
            float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
            lcd.print("C");
            MQ135.setA(44.947); MQ135.setB(-3.445); // Configure the equation to calculate Toluen concentration value
            float Toluen = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
            
            MQ135.setA(102.2 ); MQ135.setB(-2.473); // Configure the equation to calculate NH4 concentration value
            float NH4 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
            lcd.print("D");
            MQ135.setA(34.668); MQ135.setB(-3.369); // Configure the equation to calculate Aceton concentration value
            float Aceton = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
            Serial.print("|   "); Serial.print(CO); 
            Serial.print("   |   "); Serial.print(Alcohol);
            // Note: 400 Offset for CO2 source: https://github.com/miguel5612/MQSensorsLib/issues/29
            /*
            Motivation:
            We have added 400 PPM because when the library is calibrated it assumes the current state of the
            air as 0 PPM, and it is considered today that the CO2 present in the atmosphere is around 400 PPM.(2024 already 425PPM)
            https://www.lavanguardia.com/natural/20190514/462242832581/concentracion-dioxido-cabono-co2-atmosfera-bate-record-historia-humanidad.html
            */
            Serial.print("   |   "); Serial.print(CO2 + 425);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("ppm: ");
            lcd.print(CO2 + 425);
            Serial.print("   |   "); Serial.print(Toluen); 
            Serial.print("   |   "); Serial.print(NH4); 
            Serial.print("   |   "); Serial.print(Aceton);
            Serial.println("   |");
            /*
              Exponential regression:
            GAS      | a      | b
            CO       | 605.18 | -3.937  
            Alcohol  | 77.255 | -3.18 
            CO2      | 110.47 | -2.862
            Toluen  | 44.947 | -3.445
            NH4      | 102.2  | -2.473
            Aceton  | 34.668 | -3.369
            */

        }
}