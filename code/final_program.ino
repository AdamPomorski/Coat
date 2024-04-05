#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>


#define RE 6 
#define DE 16    
#define SDAPIN 10
#define SCLPIN 11
#define ONEWIREBUS 9
#define BTNPIN 2     
      
const byte ModReadBuffer[] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x01, 0x60, 0x0a};
const byte ModWriteCalib7Buffer[] = {0x01, 0x06, 0x00, 0x31, 0x7f, 0xff, 0xb8, 0x75};
const byte ModReadCalib7Buffer[] = {0x01, 0x03, 0x00, 0x31, 0x00, 0x01, 0xd5, 0xc5};
const byte ModResponseCalib7Buffer[] = {0x01, 0x03, 0x02, 0x7f, 0xff, 0x50, 0xab};
byte BufferValue[8], CalibBufferValue[8];
SoftwareSerial mod(5, 17); // RX=5 , TX =17
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONEWIREBUS);
DallasTemperature sensors(&oneWire);

unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
float temp = 0 , ph = 0;
bool isrReady = false;
bool regStatus = true;
bool correctMsg = false;

ICACHE_RAM_ATTR void isr() {
  button_time = millis();
  if (button_time - last_button_time > 250)
  {
    //TODO: calibrating ph meter
  isrReady = true;
  last_button_time = button_time;  
  }
}

void setup() {
  // put your setup code here, to run once:
  Wire.begin(SDAPIN, SCLPIN);
  Serial.begin(115200);
  mod.begin(9600);// modbus configuration
  sensors.begin();
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(BTNPIN, INPUT_PULLUP);
  attachInterrupt(BTNPIN, isr, FALLING);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

}

void loop() {
  byte val1;
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  Serial.print(temp);
  Serial.println("ºC");
 
  if(isrReady){
    ModbusCalib();   
    isrReady = false;
  }
  if(!regStatus){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error during");
    lcd.setCursor(0, 1);
    lcd.print("calibration");
    } 
  delay(1000);
  if(correctMsg){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Succesfully");
    lcd.setCursor(0, 1);
    lcd.print("calibrated to 7");
    correctMsg = false;
  }else{ 
  val1 = ModbusData();
  if(ph>0 && ph<14){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ph: ");
    lcd.print(ph);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");  
    lcd.print(temp);
  }
  }
  delay(1000);


}

byte ModbusCalib(){
  byte i;
  //const byte ModWriteCalib7Buffer[] = {0x01, 0x06, 0x00, 0x31, 0xff, 0xff, 0xd9, 0xb5};
  regStatus = true;
  //byte CalibBufferValue[8];
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(ModWriteCalib7Buffer,sizeof(ModWriteCalib7Buffer))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    }
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(ModReadCalib7Buffer,sizeof(ModReadCalib7Buffer))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(i=0;i<8;i++){
    //Serial.print(mod.read(),HEX);
    CalibBufferValue[i] = mod.read();
    }
    Serial.println("Serial Data received On:");
    if(CalibBufferValue[1]==0x06){
      for (i = 0; i < 7; i++) {
        if(CalibBufferValue[i]!=ModWriteCalib7Buffer[i]){
      regStatus = false;
      Serial.print("Wrong msg in: ");
      Serial.println(i);
        }
      }
      if(regStatus){
      correctMsg = true;
      }
    }
    for (i = 0; i < 8; i++) {
      Serial.print("Modbus Buffer[");
      Serial.print(i);
      Serial.print("]=");
      Serial.println(CalibBufferValue[i], HEX);
      Serial.print("Sent Buffer[");
      Serial.print(i);
      Serial.print("]=");
      Serial.println(ModWriteCalib7Buffer[i], HEX);
    }
     
  }
  return CalibBufferValue[8];
}



byte ModbusData(){
  byte i;
  //const byte ModReadBuffer[] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x01, 0x60, 0x0a};
  //byte BufferValue[8];
  regStatus = true;
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(ModReadBuffer,sizeof(ModReadBuffer))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(i=0;i<8;i++){
    //Serial.print(mod.read(),HEX);
    BufferValue[i] = mod.read();
    }
    if(BufferValue[1]==0x06){
      for (i = 0; i < 7; i++) {
      if(BufferValue[i]!=ModWriteCalib7Buffer[i]){
      regStatus = false;
      Serial.print("Wrong msg in: ");
      Serial.println(i);
    }
    }
     if(regStatus){
      correctMsg = true;
    }
    }
    uint16_t fullNumber = (BufferValue[3] << 8) | BufferValue[4];
    ph = (float)fullNumber/100;
    if(BufferValue[0]!=255){
    Serial.println("Serial Data received On:");
    Serial.print("modbus Buffer[0]="); 
    Serial.println(BufferValue[0],HEX); 
    Serial.print("modbus Buffer[1]="); 
    Serial.println(BufferValue[1],HEX);
    Serial.print("modbus Buffer[2]="); 
    Serial.println(BufferValue[2],HEX);
    Serial.print("modbus Buffer[3]="); 
    Serial.println(BufferValue[3],HEX);
    Serial.print("modbus Buffer[4]="); 
    Serial.println(BufferValue[4],HEX);
    Serial.print("modbus Buffer[5]="); 
    Serial.println(BufferValue[5],HEX);
    Serial.print("modbus Buffer[6]="); 
    Serial.println(BufferValue[6],HEX);
    Serial.print("modbus Buffer[7]="); 
    Serial.println(BufferValue[7],HEX);
    Serial.print("Ph: ");
    Serial.println(ph);
    Serial.println("");}
   // }
  }
  return BufferValue[8];
}
